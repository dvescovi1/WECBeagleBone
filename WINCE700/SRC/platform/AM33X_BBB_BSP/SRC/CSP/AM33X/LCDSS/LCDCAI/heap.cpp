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
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this sample source code is subject to the terms of the Microsoft
// license agreement under which you licensed this sample source code. If
// you did not accept the terms of the license agreement, you are not
// authorized to use this sample source code. For the terms of the license,
// please see the license agreement between you and Microsoft or, if applicable,
// see the LICENSE.RTF on your install media or the root of your tools installation.
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES.
//
#include "omap.h"
#include "heap.h"
//
//  Defines
//
#define ZONE_HEAP   0


//
//  Structs
//



//
//  Globals
//


//------------------------------------------------------------------------------
Heap::Heap( DWORD size, DWORD base, Heap *pNext, Heap *pPrev )
{
    DEBUGMSG(ZONE_HEAP,(TEXT("Heap::Heap size %d address 0x%08x pPrev %08x pNext %08x this %08x\r\n"),
        size, base, pPrev, pNext, this ));

    if( (m_pNext = pNext) != NULL )
        m_pNext->m_pPrev = this;

    if( (m_pPrev = pPrev) != NULL )
        m_pPrev->m_pNext = this;

    m_pStart = base;
    m_nSize = size;
    m_fUsed = 0;
}

//------------------------------------------------------------------------------
Heap::~Heap()
{
    DEBUGMSG(ZONE_HEAP,(TEXT("Heap::~Heap 0x%08x (size=%d)\r\n"),this,m_nSize));

    if( !m_pPrev )
    {
        DEBUGMSG(ZONE_HEAP,(TEXT("Deleting heap!\r\n")));

        // if this is the base, then we are deleting the heap,
        Heap *pLast = this;
        
        while( pLast->m_pNext )
            pLast = pLast->m_pNext;
        
        while( pLast != this )
        {
            pLast = pLast->m_pPrev;
            pLast->m_pNext->m_pPrev = (Heap *)NULL; // prevent Heap::~Heap from doing anything
            delete pLast->m_pNext;
            pLast->m_pNext = (Heap *)NULL;
        }
    }
    else
    {
        DEBUGMSG(ZONE_HEAP,(TEXT("Deleting node only\r\n")));

        // otherwise, we are just freeing this node
        m_pPrev->m_nSize += m_nSize;
        m_pPrev->m_pNext = m_pNext;
        if( m_pNext )
            m_pNext->m_pPrev = m_pPrev;
    }
}

//------------------------------------------------------------------------------
DWORD Heap::TotalUsed()
{
    Heap *pNode;
    DWORD size = 0;

    for( pNode=this; pNode; pNode = pNode->m_pNext )
        if( pNode->m_fUsed )
            size += pNode->m_nSize;

    return size;
}

//------------------------------------------------------------------------------
DWORD Heap::TotalFree()
{
    Heap *pNode;
    DWORD size = 0;

    for( pNode = this; pNode; pNode = pNode->m_pNext )
        if( !pNode->m_fUsed )
            size += pNode->m_nSize;

    return size;
}


//------------------------------------------------------------------------------
Heap *Heap::Allocate( DWORD size )
{
    Heap *pNode = this;

    DEBUGMSG(ZONE_HEAP,(TEXT("Heap::Allocate(%d)\r\n"),size));

    while( pNode && ( ( pNode->m_fUsed ) || ( pNode->m_nSize < size ) ) )
        pNode = pNode->m_pNext;

    if( pNode && ( pNode->m_nSize > size ) )
    {
        DEBUGMSG(ZONE_HEAP,(TEXT("Splitting %d byte node at 0x%08x\r\n"), pNode->m_nSize, pNode ));

        // Ok, have to split into used & unused section
        Heap *pFreeNode = new Heap(
                                pNode->m_nSize - size,      // size
                                pNode->m_pStart + size,     // start
                                pNode->m_pNext,             // next
                                pNode                       // prev
                                );

        if( !pFreeNode )
        {
            pNode = (Heap *)NULL;   // out of memory for new node
            DEBUGMSG(ZONE_HEAP,(TEXT("Failed to allocate new node\r\n")));
        }
        else
        {
            pNode->m_nSize = size;
        }
    }

    if( pNode )
    {
        pNode->m_fUsed = 1;
        DEBUGMSG(ZONE_HEAP,(TEXT("Marking node at 0x%08x as used (offset = 0x08x)\r\n"),pNode, pNode->Address()));
    }

    return pNode;
}

//------------------------------------------------------------------------------
void Heap::Free()
{
    Heap *pMerge;
    Heap *pNode = this;

    DEBUGMSG(ZONE_HEAP,(TEXT("Heap::Free 0x%08x (size=%d)\r\n"),pNode,pNode->m_nSize));

    pNode->m_fUsed = 0;
    pMerge = pNode->m_pPrev;

    if( pMerge && !pMerge->m_fUsed )
    {
        DEBUGMSG(ZONE_HEAP,(TEXT("Heap::Free - merging node %08x with prior node (%08x)\r\n"),
            pNode, pMerge ));
        delete pNode;           // Merge pNode with prior node
        pNode = pMerge;
    }
    pMerge = pNode->m_pNext;

    if( pMerge && !pMerge->m_fUsed )
    {
        DEBUGMSG(ZONE_HEAP,(TEXT("Heap::Free - merging %08x with subsequent node (%08x)\r\n"),
            pNode, pMerge ));
        delete pMerge;          // Merge pNode with subsequent node
    }
}

