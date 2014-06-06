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
//  File:  args.c
//
//  This file implements the OEMArgsQuery handler.
//

#include "omap.h"

#define BLOCK_SIZE          4   //granularity of the allocation

typedef DWORD T_BLOCK_COUNT_TYPE;
#define BLOCK_COUNT_SIZE    sizeof(T_BLOCK_COUNT_TYPE)

UCHAR* pool;
UCHAR* map;
DWORD  poolSize;
DWORD  nbBlock;
static CRITICAL_SECTION        Pool_cs;
static BOOL _PostInit = FALSE;

#define IS_BLOCK_USED(x) (map[(x)/(8*sizeof(*map))] & (1<<((x) & ((8*sizeof(*map))-1))))
#define MARK_BLOCK_USED(x) map[(x)/(8*sizeof(*map))] |= (1<<((x) & ((8*sizeof(*map))-1)))
#define MARK_BLOCK_UNUSED(x) map[(x)/(8*sizeof(*map))] &= ~(1<<((x) & ((8*sizeof(*map))-1)))

void OALLocalAllocInit(UCHAR* buffer,DWORD size)
{
    DWORD mapSize;

    DEBUGMSG(0,(TEXT("OALLocalAllocInit (0x%x,0x%x)\r\n"),buffer,size));
    
    nbBlock = (size+BLOCK_SIZE- 1)/ BLOCK_SIZE;
    mapSize = (nbBlock+(8*sizeof(*map))-1) / (8*sizeof(*map));
    poolSize = size - mapSize*sizeof(*map);
    nbBlock = (poolSize+BLOCK_SIZE- 1)/ BLOCK_SIZE;    
    map = buffer;
    pool = map + mapSize*sizeof(*map);
    memset(buffer,0,size);

    DEBUGMSG(0,(TEXT("nbBlock 0x%x\r\n"),nbBlock));
}

BOOL OALAllocPostInit(void)
{
    InitializeCriticalSection(&Pool_cs);
    _PostInit = TRUE;
    return TRUE;
}

/* Critical section is initialized in OALAllocPostInit, before it is initialized critical section is not used */
HLOCAL 
OALLocalAlloc(
    UINT flags, 
    UINT size
    )
{
    DWORD i;
    DWORD startBlock = (DWORD) -1;
    DWORD contiguous = 0;
    T_BLOCK_COUNT_TYPE nbRequestBlock = (T_BLOCK_COUNT_TYPE) (((size+BLOCK_COUNT_SIZE) + BLOCK_SIZE- 1)/ BLOCK_SIZE);
    HLOCAL lhandle=NULL;

    UNREFERENCED_PARAMETER(flags);
    DEBUGMSG(0,(TEXT("OALLocalAlloc request for %d bytes => %d block\r\n"),size,nbRequestBlock));
	
    if(_PostInit )
        EnterCriticalSection(&Pool_cs);

    for (i=0;i<nbBlock;i++)
    {
        if (IS_BLOCK_USED(i))
        {
            DEBUGMSG(0,(TEXT("block %d is used\r\n"),i));
            contiguous = 0;
            startBlock = (DWORD) -1;
        }
        else
        {
            if (contiguous == 0)
            {
                DEBUGMSG(0,(TEXT("block %d is unused\r\n"),i));
                startBlock = i;
            }
            contiguous++;      
            if (nbRequestBlock == contiguous)
            {
                DEBUGMSG(0,(TEXT("found %d free blocks (%d,%d)\r\n"),nbRequestBlock,startBlock,i));
                break;
            }
        }
    }
    if ((startBlock == (DWORD) -1) || (nbRequestBlock != contiguous))
    {
        RETAILMSG(1, (TEXT("OALLocalAlloc returned NULL !\r\n")));
    }
    else
    {        
        T_BLOCK_COUNT_TYPE* p = (T_BLOCK_COUNT_TYPE*) (pool + (startBlock * BLOCK_SIZE));
        *p = nbRequestBlock;
        for (i=0;i<nbRequestBlock;i++)
        {
            DEBUGMSG(0,(TEXT("marking block %d used\r\n"),i+startBlock));
            MARK_BLOCK_USED(i+startBlock);
        }
        DEBUGMSG(0,(TEXT("returned value is 0x%x\r\n"),(HLOCAL) (pool + (startBlock * BLOCK_SIZE) + BLOCK_COUNT_SIZE)));
        lhandle = (HLOCAL) (pool + (startBlock * BLOCK_SIZE) + BLOCK_COUNT_SIZE);
    }

    if(_PostInit ) LeaveCriticalSection(&Pool_cs);
    return lhandle;	
	
}

HLOCAL 
OALLocalFree(
    HLOCAL hMemory 
    )
{
    T_BLOCK_COUNT_TYPE index,start;
    T_BLOCK_COUNT_TYPE count = *((T_BLOCK_COUNT_TYPE*) ((UCHAR*) hMemory - BLOCK_COUNT_SIZE));
    DEBUGMSG(0,(TEXT("OALLocalFree freeing 0x%x (%d blocks) \r\n"),hMemory,count));
	
    if(_PostInit ) EnterCriticalSection(&Pool_cs);

    start = ((UCHAR*)hMemory - BLOCK_COUNT_SIZE - pool) / BLOCK_SIZE;
    for (index=start;index<start+count;index++)
    {
        DEBUGMSG(0,(TEXT("marking block %d unused\r\n"),index));
        MARK_BLOCK_UNUSED(index);
    }
    if(_PostInit ) LeaveCriticalSection(&Pool_cs);
	
    return NULL;
}
