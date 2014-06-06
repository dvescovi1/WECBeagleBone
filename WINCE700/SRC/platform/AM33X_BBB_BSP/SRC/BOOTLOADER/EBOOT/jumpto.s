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
;  File:  jumpto.s

        INCLUDE kxarm.h
        INCLUDE armmacros.s

        IMPORT OALVAtoPA
        IMPORT OALCleanDCache
        
        TEXTAREA

;-------------------------------------------------------------------------------
;
;  Function:  JumpTo
;        
;   This function launches the program at pFunc (pFunc
;   is a physical address).  The MMU is disabled just
;   before jumping to specified address.
;
; Inputs: (r0) - Physical address of program to Launch.
; 
; On return: None - the launched program never returns.
;
; Register used:
;
;-------------------------------------------------------------------------------
    ALIGN
    LEAF_ENTRY JumpTo     
   
    ; r3 now contains the physical launch address.
    ;
    mov     r3, r0

    ; Compute the physical address of the PhysicalStart tag.  We'll jump to this
    ; address once we've turned the MMU and caches off.
    ;
    stmdb   sp!, {r3}
    bl OALCleanDCache
    
    ldr     r0, =PhysicalStart ; we must keep this therwise it doesn't work
    bl      OALVAtoPA
    nop	
    ldmia   sp!, {r3}
    
    ; r0 now contains the physical address of 'PhysicalStart'.
    ; r3 now contains the physical launch address.

    
           
;-------------------------------------------------------------------------------
           
   
    ; Next, we disable the MMU, and I&D caches.
    ;
    mrc     p15, 0, r1, c1, c0, 0
    bic     r1, r1, #0x1            ; Disable MMU.
    bic     r1, r1, #0x1000         ; Disable IC.
    bic     r1, r1, #0x4            ; Disable DC.    
    mcr     p15, 0, r1, c1, c0, 0

    
    ; Jump to 'PhysicalStart'.
    ;
    mov  pc, r0
    nop
    nop
    nop
    nop



PhysicalStart

    ; Flush the I&D TLBs.
    ;
    mcr     p15, 0, r2, c8, c7, 0   ; Flush the I&D TLBs

    ; Jump to the physical launch address.  This should never return...
    ;      
    mov     pc, r3	
    nop
    nop
    nop
    nop
    nop
    nop
;------------------------------------------------------------------------------

        END
