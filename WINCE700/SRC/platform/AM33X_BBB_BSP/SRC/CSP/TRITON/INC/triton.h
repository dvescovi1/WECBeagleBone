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
//  Header:  triton.h
//
#ifndef __TRITON_H
#define __TRITON_H

#include "omap.h"

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------
//
//  Function:  TWLReadIDCode
//
//   Return the ID code of tthe triton
//
UINT8 TWLReadIDCode(void* hTwl);


//------------------------------------------------------------------------------
//  Voltage related functions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
//  Function:  TWLSetVoltage
//
BOOL TWLSetVoltage(UINT voltage,UINT32 mv);
//------------------------------------------------------------------------------
//
//  Function:  TWLGetVoltage
//
BOOL TWLGetVoltage(UINT voltage,UINT32* pmv);
//------------------------------------------------------------------------------
//
//  Function:  TWLGetVoltageStatus
//
BOOL TWLGetVoltageStatus(UINT voltage, UINT32* pStatus);
//------------------------------------------------------------------------------
//
//  Function:  TWLSetSlewRate
//
BOOL TWLSetVoltageSlewRate(UINT voltage,UINT32 rate);
//------------------------------------------------------------------------------
//
//  Function:  TWLGetSlewRate
//
BOOL TWLGetVoltageSlewRate(UINT voltage,UINT32* rate);
//------------------------------------------------------------------------------
//
//  Function:  TWLEnableVoltage
//
BOOL TWLEnableVoltage(UINT voltage,BOOL fOn);





//------------------------------------------------------------------------------
//
//  Function:  TWLOpen
//
//  returns a handle that must be used for subsequent access to the triton
//
HANDLE 
TWLOpen(
    );

//------------------------------------------------------------------------------
//
//  Function:  TWLClose
//
//
VOID
TWLClose(
    HANDLE hContext
    );


#ifdef __cplusplus
}
#endif

#endif // __BSP_TPS659XX_H

