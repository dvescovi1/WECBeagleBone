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
;-------------------------------------------------------------------------------
;
;  File:  tlb.s
;
;
        INCLUDE kxarm.h
        INCLUDE armmacros.s

        TEXTAREA

;-------------------------------------------------------------------------------
;
;  Function:  OALClearITLB
;
;  Flush and invalidate the instruction TLB
;
        LEAF_ENTRY OALClearITLB

        mov     r0, #0
        mcr     p15, 0, r0, c8, c5, 0   ; flush instruction TLB

        nop
        nop

        RETURN

;-------------------------------------------------------------------------------
;
;  Function:  OALClearITLBEntry
;
;  Flush and invalidate an entry in the instruction TLB
;
        LEAF_ENTRY OALClearITLBEntry

        mcr     p15, 0, r0, c8, c5, 1   ; flush instruction TLB entry

        nop
        nop

        RETURN

;-------------------------------------------------------------------------------
;
;  Function:  OALClearITLBAsid
;
;  Flush and invalidate the instruction TLB for a particular process
;
        LEAF_ENTRY OALClearITLBAsid

        mcr     p15, 0, r0, c8, c5, 2   ; flush instruction TLB for ASID

        nop
        nop

        RETURN

;-------------------------------------------------------------------------------
;
;  Function:  OALClearDTLB
;
;  Flush and invalidate the data TLB
;
        LEAF_ENTRY OALClearDTLB

        mov     r0, #0
        mcr     p15, 0, r0, c8, c6, 0   ; flush data TLB

        nop
        nop

        RETURN


;-------------------------------------------------------------------------------
;
;  Function:  OALClearDTLBEntry
;
;  Flush and invalidate an entry in the data TLB
;
        LEAF_ENTRY OALClearDTLBEntry

        mcr     p15, 0, r0, c8, c6, 1   ; flush data TLB entry

        nop
        nop

        RETURN

;-------------------------------------------------------------------------------
;
;  Function:  OALClearDTLBAsid
;
;  Flush and invalidate the data TLB for a particular process
;
        LEAF_ENTRY OALClearDTLBAsid

        mcr     p15, 0, r0, c8, c6, 2   ; flush data TLB for ASID

        nop
        nop

        RETURN

;-------------------------------------------------------------------------------
;
;  Function:  OALClearTLB
;
;  Flush and invalidate the unified TLB
;
        LEAF_ENTRY OALClearTLB

        mov     r0, #0
        mcr     p15, 0, r0, c8, c7, 0   ; flush unified TLB

        mov     r2, #0
        mcr     p15, 0, r2, c7, c10, 4      ; data sync barrier operation
        mcr     p15, 0, r2, c7, c5, 4       ; instr sync barrier operation

        nop
        nop

        RETURN


;-------------------------------------------------------------------------------
;
;  Function:  OALClearTLBEntry
;
;  Flush and invalidate an entry in the unified TLB
;
        LEAF_ENTRY OALClearTLBEntry

        mcr     p15, 0, r0, c8, c7, 1   ; flush unified TLB entry

        mov     r2, #0
        mcr     p15, 0, r2, c7, c10, 4      ; data sync barrier operation
        mcr     p15, 0, r2, c7, c5, 4       ; instr sync barrier operation

        nop
        nop

        RETURN

;-------------------------------------------------------------------------------
;
;  Function:  OALClearTLBAsid
;
;  Flush and invalidate the unified TLB for a particular process
;
        LEAF_ENTRY OALClearTLBAsid

        mcr     p15, 0, r0, c8, c7, 2   ; flush unified TLB for ASID

        mov     r2, #0
        mcr     p15, 0, r2, c7, c10, 4      ; data sync barrier operation
        mcr     p15, 0, r2, c7, c5, 4       ; instr sync barrier operation

        nop
        nop

        RETURN

;-------------------------------------------------------------------------------

        END

        
