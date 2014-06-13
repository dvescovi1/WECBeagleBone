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
//  File:  flush.c
//
//  This file implements OEMCacheRangeFlush function suitable for ARM CPU
//  with separate data and instruction caches.
//
#include "omap.h"
#include <oal.h>
#include <oalex.h>


//
//  Defines
//

#define KERNEL_GLOBAL_ADDRESS   0x70000000          // Everything above this address is a kernel global address


//
//  Functions
//

DWORD   OALGetFCSE();
DWORD   OALGetContextID();
VOID    OALInvalidateDCacheLines(VOID* pAddress, UINT32 size);
VOID    OALClearITLBAsid(DWORD dwASID);
VOID    OALClearDTLBAsid(DWORD dwASID);
VOID    OALClearTLBAsid(DWORD dwASID);

VOID    OALFlushL2Cache();
VOID    OALCleanL2Cache();
VOID    OALCleanL2CacheLines(VOID* pAddress, UINT32 size);

//
//  Externs
//

extern  DWORD   g_dwRangeLimit;


//------------------------------------------------------------------------------
//
//  Function:  OEMCacheRangeFlush
//
//  Implements entry point for all cache and TLB flush operations. It takes
//  following parameters:
//    
//      pAddress - starting VA on which the cache operation is to be performed
//      length - length of flush
//      flags  - specifies the cache operation to be performed:
//      
//          CACHE_SYNC_WRITEBACK: write back DATA cache
//          CACHE_SYNC_DISCARD: write back and discard DATA cache
//          CACHE_SYNC_INSTRUCTIONS: discard I-Cache
//          CACHE_SYNC_FLUSH_I_TLB: flush instruction TLB
//          CACHE_SYNC_FLUSH_D_TLB: flush data TLB
//          CACHE_SYNC_FLUSH_TLB: flush both I/D TLB
//          CACHE_SYNC_L2_WRITEBACK: write back L2 cache
//          CACHE_SYNC_L2_DISCARD: write back and discard L2 cache
//          CACHE_SYNC_ALL: perform all the above operations.
//            
//  If both pAddress and length are 0, the entire cache (or TLB, as directed by
//  flags) will be flushed. Only the kernel can set the TLB flush flags when
//  it calls this routine, and when a TLB flush is performed with 
//  length == PAGE_SIZE, pAddress is guaranteed to be on a page boundary.
//                


void OEMCacheRangeFlush(VOID *pAddress, DWORD length, DWORD flags)
{
//    BOOL    bEnabled;

    OALMSG(OAL_CACHE&&OAL_VERBOSE, (
        L"+OEMCacheRangeFlush(0x%08x, %d, 0x%08x)\r\n", pAddress, length, flags
    ));


    //bEnabled = INTERRUPTS_ENABLE(FALSE);

    if ((flags & CACHE_SYNC_DISCARD) != 0) {
        // Write back and invalidate the selected portions of the data cache
        if (length == 0 || pAddress == NULL) {
            OALFlushDCache();
        } else {
            // Normalize address to cache line alignment
            UINT32 mask = g_oalCacheInfo.L1DLineSize - 1;
            UINT32 address = (UINT32)pAddress & ~mask;
            // Adjust size to reflect cache line alignment
            length += (UINT32)pAddress - address;
           // If range is bigger than cache range limit, flush all
            if (length >= g_dwRangeLimit) {
                OALFlushDCache();
            } else {                
                // Flush all the indicated cache entries
                OALFlushDCacheLines((VOID*)address, length);

                // OALFlushDCacheLines will ensure PoC (Point of coherency)
                // Avoid excessive L2 flushing if the specific MVA range has been
                // flushed here
                flags &= (~(CACHE_SYNC_L2_DISCARD | CACHE_SYNC_L2_WRITEBACK));
            }                
        }
        
    } else if ((flags & CACHE_SYNC_WRITEBACK) != 0) {
        // Write back the selected portion of the data cache
        if (length == 0 || pAddress == NULL) {
            OALCleanDCache();
            //OALFlushDCache();
        } else {
            // Normalize address to cache line alignment
            UINT32 mask = g_oalCacheInfo.L1DLineSize - 1;
            UINT32 address = (UINT32)pAddress & ~mask;
            // Adjust size to reflect cache line alignment
            length += (UINT32)pAddress - address;
            // If range is bigger than cache range limit, clean all
            if (length >= g_dwRangeLimit) {
                OALCleanDCache();
                //OALFlushDCache();
            } else {                                
                // Clean all the indicated cache entries to PoC 
                // when CACHE_SYNC_L2_WRITEBACK or CACHE_SYNC_L2_DISCARD is not set
                // since entries will be cleaned (and invalidate) to PoC later by
                // OALCleanL2CacheLines or OALFlushDCacheLines
                if (!(flags & (CACHE_SYNC_L2_WRITEBACK | CACHE_SYNC_L2_DISCARD)))
                {
                    OALCleanDCacheLines((VOID*)address, length);
                }
            }                
        }
    } else if ((flags & TI_CACHE_SYNC_INVALIDATE) != 0) {
        // Invalidate the selected portion of the data cache
        // Invalidate all data cache corrupts the execution 
        // of the operating system.  Only process with given 
        // buffers that are cache line aligned will be invalidated
        // all others will be clean & invalidated
        if ((length != 0) && (pAddress != NULL)) 
        {
            // Check for page alignment
            if( ((DWORD)(pAddress) % g_oalCacheInfo.L1DLineSize) == 0 )
            {
                // Invalidate the indicated cache entries
                OALInvalidateDCacheLines(pAddress, length);
            }
            else
            {
                // Normalize address to cache line alignment
                UINT32 mask = g_oalCacheInfo.L1DLineSize - 1;
                UINT32 address = (UINT32)pAddress & ~mask;
                // Adjust size to reflect cache line alignment
                length += (UINT32)pAddress - address;
                // If range is bigger than cache range limit, flush all
                if (length >= g_dwRangeLimit) {
                    OALFlushDCache();
                } else {                
                    // Flush all the indicated cache entries
                    OALFlushDCacheLines((VOID*)address, length);
                }                
            }
        }                   
    }


    if ((flags & CACHE_SYNC_INSTRUCTIONS) != 0) {
        // WInvalidate the selected portions of the instruction cache
        if (length == 0 || pAddress == NULL) {
            OALFlushICache();
        } else {
            // Normalize address to cache line alignment
            UINT32 mask = g_oalCacheInfo.L1ILineSize - 1;
            UINT32 address = (UINT32)pAddress & ~mask;
            // Adjust size to reflect cache line alignment
            length += (UINT32)pAddress - address;
           // If range is bigger than cache range limit, flush all
            if (length >= g_dwRangeLimit) {
                OALFlushICache();
            } else {                
                // Flush all the indicated cache entries
                OALFlushICacheLines((VOID*)address, length);
            }                
        }
    }
#if (_WINCEOSVER>=600)    
    if ((flags & CACHE_SYNC_L2_DISCARD) != 0) {
        
        // Write back and invalidate the selected portions of the data cache
        if (length == 0 || pAddress == NULL) {
            OALFlushL2Cache();
        } else {
            // Normalize address to cache line alignment
            UINT32 mask = g_oalCacheInfo.L2DLineSize - 1;
            UINT32 address = (UINT32)pAddress & ~mask;
            // Adjust size to reflect cache line alignment
            length += (UINT32)pAddress - address;
           // If range is bigger than cache range limit, flush all
            if (length >= g_oalCacheInfo.L2DSize) {
                OALFlushL2Cache();
            } else {
                // Flush all the indicated cache entries
                OALFlushDCacheLines((VOID*)address, length);
            }                
        }
        
    }
    else if ((flags & CACHE_SYNC_L2_WRITEBACK) != 0) {        
        // Write back and invalidate the selected portions of the data cache
        if (length == 0 || pAddress == NULL) {
            OALCleanL2Cache();
        } else {
            // Normalize address to cache line alignment
            UINT32 mask = g_oalCacheInfo.L2DLineSize - 1;
            UINT32 address = (UINT32)pAddress & ~mask;
            // Adjust size to reflect cache line alignment
            length += (UINT32)pAddress - address;
           // If range is bigger than cache range limit, flush all
            if (length >= g_oalCacheInfo.L2DSize) {
                OALCleanL2Cache();
            } else {
                // Clean all the indicated cache entries
                    OALCleanDCacheLines((VOID*)address, length);
            }                
        }
        
    }


    //
    //  TLB flushing for WinCE 6, 7 and WinMobile 7
    //

    //
    //  Flush instruction TLB
    //
    if ((flags & CACHE_SYNC_FLUSH_I_TLB) != 0) {
        //  If the address is less than the kernel global space, OR with the ASID prior to clearing the TLB
        //  otherwise clear the global TLB entry (no ASID)
        if( length == 0 && pAddress == NULL)
        {
            // flush the whole TLB
            OALClearITLB();
        }
        else if( (DWORD) pAddress < KERNEL_GLOBAL_ADDRESS )
        {
            if( length == (DWORD) PAGE_SIZE )
            {
                // flush process TLB entry (with ASID)
                OALClearITLBEntry((VOID*)((DWORD)pAddress | OALGetContextID()));
            }
            else
            {
                // flush the whole TLB for that ASID
                OALClearITLBAsid(OALGetContextID());
            }
        }
        else
        {
            if( length == (DWORD) PAGE_SIZE )
            {
                // flush global TLB entry (no ASID)
                OALClearITLBEntry(pAddress);
            }
            else
            {
                // flush the whole TLB
                OALClearITLB();
            }
        }
    }

    //
    //  Flush data TLB
    //
    if ((flags & CACHE_SYNC_FLUSH_D_TLB) != 0) {
        //  If the address is less than the kernel global space, OR with the ASID prior to clearing the TLB
        //  otherwise clear the global TLB entry (no ASID)
        if( length == 0 && pAddress == NULL)
        {
            // flush the whole TLB
            OALClearDTLB();
        }
        else if( (DWORD) pAddress < KERNEL_GLOBAL_ADDRESS )
        {
            if( length == (DWORD) PAGE_SIZE )
            {
                // flush process TLB entry (with ASID)
                OALClearDTLBEntry((VOID*)((DWORD)pAddress | OALGetContextID()));
            }
            else
            {
                // flush the whole TLB for that ASID
                OALClearDTLBAsid(OALGetContextID());
            }
        }
        else
        {
            if( length == (DWORD) PAGE_SIZE )
            {
                // flush global TLB entry (no ASID)
                OALClearDTLBEntry(pAddress);
            }
            else
            {
                // flush the whole TLB
                OALClearDTLB();
            }
        }
    }
    
#else

    //
    //  TLB flushing for WinMobile 6.x
    //

    //  Flush for a demand page
//    if ((length == PAGE_SIZE) && (flags & CACHE_SYNC_WRITEBACK)) {
//        //    OALMSG(1, (L"PAGE flush pAddr = %8.8lx  zAddr = %8.8lx\r\n", pAddress, ZeroPtr(pAddress)));
//        OALClearTLB();
//    }

    //
    //  Flush TLB
    //
    if ((flags & CACHE_SYNC_FLUSH_TLB) != 0) {
        if (length == PAGE_SIZE) {
            // flush one TLB entry
            OALClearTLBEntry(pAddress);
        } else {
            // flush the whole TLB
            OALClearTLB();
        }
    }

#endif 

    //INTERRUPTS_ENABLE(bEnabled);

    OALMSG(OAL_CACHE&&OAL_VERBOSE, (L"-OEMCacheRangeFlush\r\n"));
}

//------------------------------------------------------------------------------

