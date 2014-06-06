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
;------------------------------------------------------------------------------
;
;  File:  image_cfg.inc
;

;------------------------------------------------------------------------------
;
;  Define:  IMAGE_XLDR_xxx
;
;  These constants are used to define the location of the XLDR code, RAM, and
;  stack. The locations defined below are for the XLDR running in SRAM.
;  If these addresses are modified, the corresponding xldrtoc.raw file
;  would need to be updated as well. 
;

IMAGE_XLDR_CODE_PA      EQU 0x402F1000
IMAGE_XLDR_CODE_SIZE    EQU 0x00015000

IMAGE_XLDR_DATA_PA      EQU 0x40306000
IMAGE_XLDR_DATA_SIZE    EQU 0x00004000

IMAGE_XLDR_STACK_PA     EQU 0x4030A000
IMAGE_XLDR_STACK_SIZE   EQU 0x00001000

;------------------------------------------------------------------------------
;
;  Define: IMAGE_EBOOT_xxx
;
;  Following constants define EBOOT image layout. 
;

IMAGE_EBOOT_CODE_PA     EQU 0x8FE00000
IMAGE_EBOOT_CODE_SIZE   EQU 0x00040000

IMAGE_EBOOT_DATA_PA     EQU 0x8FE80000
IMAGE_EBOOT_DATA_SIZE   EQU 0x00050000

                        END
