;
; Copyright (c) Microsoft Corporation.  All rights reserved.
;
;
; Use of this sample source code is subject to the terms of the Microsoft
; license agreement under which you licensed this sample source code. If
; you did not accept the terms of the license agreement, you are not
; authorized to use this sample source code. For the terms of the license,
; please see the license agreement between you and Microsoft or, if applicable,
; see the LICENSE.RTF on your install media or the root of your tools installation.
; THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
;
;
;================================================================================
;             Texas Instruments OMAP(TM) Platform Software
; (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
;
; Use of this software is controlled by the terms and conditions found
; in the license agreement under which this software has been supplied.
;
;================================================================================
;
;
;  File:  stall.s

        INCLUDE kxarm.h
        INCLUDE bsp.inc

        TEXTAREA

;-------------------------------------------------------------------------------
;
;  Function:  OALStall
;
;  This function implements busy stall loop. On entry r0 contains number
;  of microseconds to stall. We assume that system is already on final CPU
;  frequency.
;
        LEAF_ENTRY OALStall

10      ldr     r2, =BSP_STALL_DELAY
20      subs    r2, r2, #1
        bne     %B20
        subs    r0, r0, #1
        bne     %B10
        bx      lr
        
        ENTRY_END OALStall
        
;------------------------------------------------------------------------------

        END
