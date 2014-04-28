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

//
//  File: heap.h
//

#ifndef __HEAP_H
#define __HEAP_H


//------------------------------------------------------------------------------
//
//  Class:  Heap
//
//  Simple heap memory manager.  Adapted from MSFT Heap class for display drivers
//
class Heap
{
    Heap            *m_pNext;
    Heap            *m_pPrev;
    DWORD           m_pStart;
    DWORD           m_nSize;
    DWORD           m_fUsed;

public:
    //------------------------------------------------------------------------------
    //
    //  Method: constructor
    //
    Heap(
        DWORD size,
        DWORD base,
        Heap *pNext = (Heap *)NULL,
        Heap *pPrev = (Heap *)NULL
        );

    //------------------------------------------------------------------------------
    //
    //  Method: destructor
    //
    ~Heap();


    //------------------------------------------------------------------------------
    //
    //  Properties:
    //
    //      NodeSize -      Size of this heap node
    //      Address -       Address this node manages
    //      TotalUsed -     Amount of heap used
    //      TotalFree -     Amount of heap free
    //
    DWORD       NodeSize(){ return m_nSize; }
    DWORD       Address() { return m_pStart; }
    DWORD       TotalUsed();
    DWORD       TotalFree();

    //------------------------------------------------------------------------------
    //
    //  Method: Allocate
    //          Free
    //
    //  Allocates and frees heap memory
    //
    Heap *      Allocate( DWORD size );
    void        Free();
};



#endif //__HEAP_H

