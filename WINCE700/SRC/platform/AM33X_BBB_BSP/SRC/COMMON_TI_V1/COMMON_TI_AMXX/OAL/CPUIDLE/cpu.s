;/*
;===============================================================================
;*             Texas Instruments OMAP(TM) Platform Software
;* (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
;*
;* Use of this software is controlled by the terms and conditions found
;* in the license agreement under which this software has been supplied.
;*
;===============================================================================
;*/
;
;  File:  cpu.s

        INCLUDE kxarm.h
        INCLUDE armmacros.s
        INCLUDE am3xx_const.inc

        EXPORT INTERRUPTS_STATUS

        TEXTAREA

BEGIN_REGION

;-------------------------------------------------------------------------------
;
;  Function:  INTERRUPTS_STATUS
;
;  returns current arm interrupts status.
;
 LEAF_ENTRY INTERRUPTS_STATUS

        mrs     r0, cpsr                    ; (r0) = current status
        ands    r0, r0, #0x80               ; was interrupt enabled?
        moveq   r0, #1                      ; yes, return 1
        movne   r0, #0                      ; no, return 0

 ENTRY_END INTERRUPTS_STATUS

;-------------------------------------------------------------------------------
;
;  Function:  OALCPUIdle
;
;  This function puts the mpu in suspend.
;  r0 = addr CPUIDLE_INFO
;
 LEAF_ENTRY OALCPUIdle
                                         ;--------------------------------------
                                         ; store register values into stack    
                                         ;
        stmdb      sp!, {r3 - r12, lr} 

                                         ;--------------------------------------
                                         ; memory barrier    
                                         ;
        mov        r2, #0x0
        mcr        p15, 0, r2, c7, c10, 4        
        mcr        p15, 0, r2, c7, c10, 5    
        nop                       
        dcd        WFI
        

        ldmia      sp!, {r3 - r12, lr} 
        mov        pc, lr               
 ENTRY_END OALCPUIdle
;-------------------------------------------------------------------------------


END_REGION
;-------------------------------------------------------------------------------

        END

