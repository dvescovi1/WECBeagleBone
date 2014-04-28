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
;  File:  jumpto.s

        INCLUDE kxarm.h

        TEXTAREA

;-------------------------------------------------------------------------------
;
;  Function:  JumpTo
;
        LEAF_ENTRY JumpTo

        mov     pc, r0

;------------------------------------------------------------------------------

        END
