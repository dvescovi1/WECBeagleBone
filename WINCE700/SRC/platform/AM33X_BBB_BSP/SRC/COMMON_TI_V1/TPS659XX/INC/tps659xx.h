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
//  Header:  tps659xx.h
//
#ifndef __TPS659XX_H
#define __TPS659XX_H

#ifdef __cplusplus
extern "C" {
#endif

#include "triton.h"
  
#define TPS659XX_MAX_GPIO_COUNT          (18)

BOOL 
SendPBMessage(
    void   *pDevice,
    UCHAR power_res_id,UCHAR res_state
    );

BOOL BSPSetT2MSECURE(BOOL fSet);


#ifdef __cplusplus
}
#endif

#endif // __TPS659XX_H

