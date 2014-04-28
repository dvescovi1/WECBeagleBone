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

#ifndef __AM38XX_DISPLAY_CFG_H__
#define __AM38XX_DISPLAY_CFG_H__

#ifdef __cplusplus
extern "C" {
#endif

// Get frame buffer physical memory address and size
BOOL
Display_GetFrameBufMemory(
    DWORD   *pFrameBufMemLen,
    DWORD   *pFrameBufMemAddr
    );

// Get vpss shared memory (shared memory to communicate with VPSS M3 display processor) address and size
BOOL
Display_GetVPSSSharedMemory(
    DWORD   *pVPSSSharedMemLen,
    DWORD   *pVPSSSharedMemAddr
    );


#ifdef __cplusplus
}
#endif

#endif __AM38XX_DISPLAY_CFG_H__

