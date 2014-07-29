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
//  File:  oal_alloc.h
//
#ifndef __OAL_ALLOC_H
#define __OAL_ALLOC_H

#ifdef __cplusplus
extern "C" {
#endif

#define LocalAlloc OALLocalAlloc
#define LocalFree  OALLocalFree

void OALLocalAllocInit(UCHAR* buffer,DWORD size);

HLOCAL 
OALLocalFree(
    HLOCAL hMemory 
    );
HLOCAL 
OALLocalAlloc(
    UINT flags, 
    UINT size
    );
BOOL OALAllocPostInit(void);

//------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif

