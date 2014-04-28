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
;   File:  startup.s
;
        INCLUDE kxarm.h
        INCLUDE bsp.inc
       
        IMPORT  KernelStart

        TEXTAREA
        
;-------------------------------------------------------------------------------
;
;  Function:  StartUp
;
;  This function is entry point to Windows CE OS. 
;
;
        LEAF_ENTRY StartUp

        ;---------------------------------------------------------------
        ; Invalidate all caches
        ;---------------------------------------------------------------

        ; Invalidate TLB and I cache
        mov     r0, #0                          ; setup up for MCR
        mcr     p15, 0, r0, c8, c7, 0           ; invalidate TLB's
        mcr     p15, 0, r0, c7, c5, 0           ; invalidate icache


        ; Flush D cache
        mrc     p15, 1, r0, c0, c0, 1           ; read clidr
        ands    r3, r0, #0x7000000  
        mov     r3, r3, lsr #23                 ; cache level value
        beq     doneb               

        mov     r10, #0                         ; start clean at cache level 0
loop1b  add     r2, r10, r10, lsr #1            ; work out 3x current cache level
        mov     r1, r0, lsr r2                  ; extract cache type bits from clidr
        and     r1, r1, #7                      ; mask of the bits for current cache only
        cmp     r1, #2                          ; see what cache we have at this level
        blt     skipb                           ; skip if no cache, or just i-cache

        mcr     p15, 2, r10, c0, c0, 0          ; select current cache level in cssr
        mov     r1, #0
        mcr     p15, 0, r1, c7, c5, 4           ; prefetch flush to sync the change to the cachesize id reg
        mrc     p15, 1, r1, c0, c0, 0           ; read the new csidr
        and     r2, r1, #7                      ; extract the length of the cache lines
        add     r2, r2, #4                      ; add 4 (line length offset)
        ldr     r4, =0x3ff
        ands    r4, r4, r1, lsr #3              ; r4 is maximum number on the way size
        clz     r5, r4                          ; r5 find bit position of way size increment
        ldr     r7, =0x7fff
        ands    r7, r7, r1, lsr #13             ; r7 extract max number of the index size

loop2b  mov     r9, r4                          ; r9 is working copy of max way size
loop3b  orr     r11, r10, r9, lsl r5            ; factor way and cache number into r11
        orr     r11, r11, r7, lsl r2            ; factor index number into r11

        mcr     p15, 0, r11, c7, c14, 2         ; clean and invalidate by set/way

        subs    r9, r9, #1                      ; decrement the way
        bge     loop3b

        subs    r7, r7, #1                      ; decrement the index
        bge     loop2b

skipb   add     r10, r10, #2                    ; increment cache number
        cmp     r3, r10
        bgt     loop1b

doneb   mov     r10, #0                         ; switch back to cache level 0
        mcr     p15, 2, r10, c0, c0, 0          ; select current cache level in cssr


        ;---------------------------------------------------------------
        ; Jump to WinCE KernelStart
        ;---------------------------------------------------------------
        ; Compute the OEMAddressTable's physical address and 
        ; load it into r0. KernelStart expects r0 to contain
        ; the physical address of this table. The MMU isn't 
        ; turned on until well into KernelStart.  


        add     r0, pc, #g_oalAddressTable - (. + 8)
        bl      KernelStart
        b       .

        ; Include memory configuration file with g_oalAddressTable
        INCLUDE addrtab_cfg.inc

        ENTRY_END 

;-------------------------------------------------------------------------------
;
;  Function:  GetCp15ControlRegister
;
;  Returns value of CP15 control register, used in OEMInit()
;
;

        EXPORT GetCp15ControlRegister
        LEAF_ENTRY GetCp15ControlRegister

        mrc     p15, 0, R0, c1, c0, 0 ; Read Control Register
        mov     pc, lr

        ENTRY_END 

;-------------------------------------------------------------------------------
;
;  Function:  GetCp15AuxiliaryControlRegister
;
;  Returns value of CP15 control register, used in OEMInit()
;
;

        EXPORT GetCp15AuxiliaryControlRegister
        LEAF_ENTRY GetCp15AuxiliaryControlRegister

        mrc p15, 0, r0, c1, c0, 1 ; Read Auxiliary Control Register
        mov     pc, lr

        ENTRY_END 

;-------------------------------------------------------------------------------
;
;  Function:  EnableUnalignedAccess
;
;  disables strict alignment checking
;
;

        EXPORT EnableUnalignedAccess
        LEAF_ENTRY EnableUnalignedAccess

        mrc     p15, 0, r1, c1, c0, 0 ; Read Control Register
        bic     r1, r1, #0x2            ; disable strict alignment check.
        mcr     p15, 0, r1, c1, c0, 0   ; MMU ON:  All memory accesses are now virtual.
        mov     pc, lr

        ENTRY_END 
 
        END
