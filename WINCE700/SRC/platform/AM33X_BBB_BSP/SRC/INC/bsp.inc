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
;==============================================================================
;             Texas Instruments OMAP(TM) Platform Software
; (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
;
; Use of this software is controlled by the terms and conditions found
; in the license agreement under which this software has been supplied.
;
;==============================================================================
;
;------------------------------------------------------------------------------
;
;  File:  bsp_cfg.inc
;

;------------------------------------------------------------------------------
;
;  Define:  BSP_STALL_DELAY
;
;  This constant is used to generate stall in assembler code. We assume
;  worst case when CPU clock is already at 330 MHz which give us
;  165 instruction cycles (divide by 2) per microseconds.
;
BSP_STALL_DELAY         EQU     240

;------------------------------------------------------------------------------
;
;  Define:  BSP_PTES_PA
;
;  This is the area of PTEs used by the MMU in EBOOT
;

; HAS_MMU
BSP_PTES_PA             EQU     (0x8FEE0000)
IMAGE_EBOOT_STACK_CA    EQU     (0x8FEE4000)
IMAGE_EBOOT_STACK_SIZE  EQU     (0x0001C000)


;------------------------------------------------------------------------------
;
;  Define:  IMAGE_EBOOT_STACK_xxxxx
;
;  This defines the stack size and loction for EBOOT (virtual address)
;

; NO_MMU
;IMAGE_EBOOT_STACK_CA    EQU     (0x8FEE0000) ; PA-0x8FEE0000
;IMAGE_EBOOT_STACK_SIZE  EQU     (0x00020000)

        END
