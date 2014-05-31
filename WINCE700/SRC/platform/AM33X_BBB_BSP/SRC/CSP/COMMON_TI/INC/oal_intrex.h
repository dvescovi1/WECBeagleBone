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
//  File:  oal_intrex.h
//

#ifndef __OAL_INTREX_H
#define __OAL_INTREX_H

//------------------------------------------------------------------------------
//
//  Function: OALIntrSetDataIrqs
//
void
OALIntrSetDataIrqs(
    UINT32 count,
    const UINT32 *pIrqs, 
    LPVOID pvData, 
    DWORD cbData
    );

//-----------------------------------------------------------------------------
//
//  Function: OALSWIntrSetDataIrq
//
void
OALSWIntrSetDataIrq(
    UINT32 irq, 
    LPVOID pvData, 
    DWORD cbData
    );

//-----------------------------------------------------------------------------
//
//  Function: OALSWIntrEnableIrq
//
BOOL
OALSWIntrEnableIrq(
    UINT32 irq
    );

//-----------------------------------------------------------------------------
//
//  Function: OALSWIntrDisableIrq
//
VOID
OALSWIntrDisableIrq(
    UINT32 irq
    );

//-----------------------------------------------------------------------------
//
//  Function: OALSWIntrDoneIrq
//
VOID
OALSWIntrDoneIrq(
    UINT32 irq
    );

#endif //__OAL_INTREX_H
//-----------------------------------------------------------------------------