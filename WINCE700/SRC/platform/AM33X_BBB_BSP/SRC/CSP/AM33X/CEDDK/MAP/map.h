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
//  File:  map.h
//
//  This file contains the data structure used to define to map the hardware
//  register regions onto to memory.


#ifndef _MAP_H_
#define _MAP_H_


//------------------------------------------------------------------------------
// This data structure is used to define a memory layout entry for a platform.
//
typedef struct {
    DWORD       dwStart;
    DWORD       dwSize;
    void       *pv;
} MEMORY_REGISTER_ENTRY;

#endif _MAP_H_