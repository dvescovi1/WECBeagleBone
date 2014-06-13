;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; NO_MMU
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;;
;;================================================================================
;;             Texas Instruments OMAP(TM) Platform Software
;; (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
;;
;; Use of this software is controlled by the terms and conditions found
;; in the license agreement under which this software has been supplied.
;;
;;================================================================================
;;
;;   File:  startup.s
;;
;;   Boot startup routine for AM33x boards.
;;
;
;        INCLUDE kxarm.h
;        INCLUDE bsp.inc
;        
;        IMPORT  main
;
;        STARTUPTEXT
;        
;;-------------------------------------------------------------------------------
;;
;;  Function:  StartUp
;;
;;  This function is entry point to Windows CE EBOOT. It should be called
;;  in state with deactivated MMU and disabled caches.
;;
;;  Note that g_oalAddressTable is needed for OEMMapMemAddr because
;;  downloaded image is placed on virtual addresses (but EBOOT runs without
;;  MMU).
;;
;        LEAF_ENTRY StartUp
;
;		;---------------------------------------------------------------
;		; part of XLDR initialization
;		;---------------------------------------------------------------
;        mov     r9, #2   ;OEM_HIGH_SECURITY_GP       ; Set flag indicating GP device
;
; IF 0   ;; assume no support for HS devices on EVM
;        ;---------------------------------------------------------------
;        ; Check for running location to determine GP vs HS device
;        ;---------------------------------------------------------------
;
;        mov     r1, pc                          ; Get the PC
;        ldr     r0, =IMAGE_XLDR_CODE_PA         ; Get XLDR start addr
;        sub     r1, r1, r0                      ; Compute any offset; offset indicates XLDR is signed for HS
;        sub     r1, r1, #8                      ; PC is +8 from instruction
;        cmp     r1, #0                          
;        beq     Done                            ; No offset, jump past reloc code
;        
;        mov     r8, r1                          ; Copy of offset
;        mov     r9, #OEM_HIGH_SECURITY_HS       ; Set flag indicating HS device
;        
;        
;        ;   Use CopyCode routine to copy itself to a safe, temporary execution area
;        ;   b/c we are going to overwrite this section of code with the relocation         
;      
;Copy1   ldr     r0, =IMAGE_XLDR_DATA_PA         ; Move CopyCode routine to DATA area of XLDR
;        ldr     r1, =CopyCode                   ; Source of CopyCode
;        add     r1, r1, r8                      ; + offset
;        mov     r2, #0x30                       ; Fixed size of CopyCode routine
;        ldr     r3, =Copy2                      ; Jump to Copy2 when done
;        add     r3, r3, r8                      ; + offset
;        b       CopyCode
;
;        ;   Setup for copy of XLDR to correct start address in SRAM using safe CopyCode
;        ;   At end of copy of XLDR code, jump to XLDR start address
;        ;   The offset check will jump past all the Copy1/Copy2 code
;        
;Copy2   ldr     r0, =IMAGE_XLDR_CODE_PA         ; Destination of XLDR
;        ldr     r1, =IMAGE_XLDR_CODE_PA         ; Source of XLDR
;        add     r1, r1, r8                      ; + offset
;        ldr     r2, =IMAGE_XLDR_CODE_SIZE       ; Size of XLDR
;        ldr     r3, =IMAGE_XLDR_CODE_PA         ; Jump to XLDR in new location when done
;        ldr     pc, =IMAGE_XLDR_DATA_PA         ; Run CopyCode that was moved to DATA section of XLDR
; ENDIF  ;; assume no support for HS devices on EVM
;
;                        
;        ;---------------------------------------------------------------
;        ; Set SVC mode & disable IRQ/FIQ
;        ;---------------------------------------------------------------
;        
;Done    mrs     r0, cpsr                        ; Get current mode bits.
;        bic     r0, r0, #0x1F                   ; Clear mode bits.
;        orr     r0, r0, #0xD3                   ; Disable IRQs/FIQs, SVC mode
;        msr     cpsr_c, r0                      ; Enter supervisor mode
;
;        ;---------------------------------------------------------------
;        ; Flush all caches
;        ;---------------------------------------------------------------
;
;        ; Invalidate TLB and I cache
;        mov     r0, #0                          ; setup up for MCR
;        mcr     p15, 0, r0, c8, c7, 0           ; invalidate TLB's
;        mcr     p15, 0, r0, c7, c5, 0           ; invalidate icache
;
;        ;---------------------------------------------------------------
;        ; Initialize CP15 control register
;        ;---------------------------------------------------------------
;
;        ; Set CP15 control bits register
;        mrc     p15, 0, r0, c1, c0, 0
;        bic     r0, r0, #(1 :SHL: 13)           ; Exception vector location (V bit) (0=normal)
;        bic     r0, r0, #(1 :SHL: 12)           ; I-cache (disabled)
;        orr     r0, r0, #(1 :SHL: 11)           ; Branch prediction (enabled)
;        bic     r0, r0, #(1 :SHL: 2)            ; D-cache (disabled - enabled within WinCE kernel startup)
;        orr     r0, r0, #(1 :SHL: 1)            ; alignment fault (enabled)
;        bic     r0, r0, #(1 :SHL: 0)            ; MMU (disabled - enabled within WinCE kernel startup)
;        mcr     p15, 0, r0, c1, c0, 0
;
;;       ;---------------------------------------------------------------
;;       ; Check r9 for HS flag
;;       ;---------------------------------------------------------------
;;       
;;        cmp     r9, #OEM_HIGH_SECURITY_HS
;;        moveq   r0, #OEM_HIGH_SECURITY_HS
;;        movne   r0, #OEM_HIGH_SECURITY_GP
;
;
;    ;---------------------------------------------------------------
;    ; Jump to BootMain
;    ;---------------------------------------------------------------
;
;    ldr     sp, =(IMAGE_EBOOT_STACK_CA + IMAGE_EBOOT_STACK_SIZE)
;	
;    ; Jump to the C entrypoint.
;    ;
;    b      main					; Jump to main.c::main(), never to return...
;
;    
;        ENTRY_END 
;
;        ; Include memory configuration file with g_oalAddressTable
;        INCLUDE addrtab_cfg.inc
;
;        END
;
;;------------------------------------------------------------------------------





; HAS_MMU
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
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
;   Boot startup routine for AM33x boards.
;

        INCLUDE kxarm.h
        INCLUDE bsp.inc
        
        IMPORT  main

        STARTUPTEXT
        
;-------------------------------------------------------------------------------
;
;  Function:  StartUp
;
;  This function is entry point to Windows CE EBOOT. It should be called
;  in state with deactivated MMU and disabled caches.
;
;  Note that g_oalEbootAddressTable is needed for OEMMapMemAddr because
;  downloaded image is placed on virtual addresses (but EBOOT runs without
;  MMU).
;
        LEAF_ENTRY StartUp

        ;---------------------------------------------------------------
        ; Jump to BootMain
        ;---------------------------------------------------------------

BUILDTTB

    add     r11, pc, #g_oalEbootAddressTable-(.+8)    ; Pointer to OEMAddressTable.
    
    ; Set the TTB.
    ;
    ldr     r9, =BSP_PTES_PA
    ldr     r0, =0xFFFFC000                   ;
    and     r9, r9, r0                        ; Mask off TTB to be on 16K boundary.
    mcr     p15, 0, r9, c2, c0, 0             ; Set the TTB.

    ; ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    ; ~~~~~~~~~~ MAP CACHED and BUFFERED SECTION DESCRIPTORS ~~~~~~~~~~~~~~~~~~
    ; ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    mov     r0, #0x0A                         ; Section (1MB) descriptor; (C=1 B=0: cached write-through).
    orr     r0, r0, #0x400                    ; Set AP.
20  mov     r1, r11                           ; Pointer to OEMAddressTable.

    ; Start Crunching through the OEMAddressTable[]:
    ;
    ; r2 temporarily holds OEMAddressTable[VA]
    ; r3 temporarily holds OEMAddressTable[PHY]
    ; r4 temporarily holds OEMAddressTable[#MB]
    ;
25  ldr     r2, [r1], #4                       ; Virtual (cached) address to map physical address to.
    ldr     r3, [r1], #4                       ; Physical address to map from.
    ldr     r4, [r1], #4                       ; Number of MB to map.

    cmp     r4, #0                             ; End of table?
    beq     %F29

    ; r2 holds the descriptor address (virtual address)
    ; r0 holds the actual section descriptor
    ;

    ; Create descriptor address.
    ;
    ldr     r6, =0xFFF00000
    and     r2, r2, r6             ; Only VA[31:20] are valid.
    orr     r2, r9, r2, LSR #18    ; Build the descriptor address:  r2 = (TTB[31:14} | VA[31:20] >> 18)

    ; Create the descriptor.
    ;
    ldr     r6, =0xFFF00000
    and     r3, r3, r6             ; Only PA[31:20] are valid for the descriptor and the rest will be static.
    orr     r0, r3, r0             ; Build the descriptor: r0 = (PA[31:20] | the rest of the descriptor)

    ; Store the descriptor at the proper (physical) address
    ;
28  str     r0, [r2], #4
    add     r0, r0, #0x00100000    ; Section descriptor for the next 1MB mapping (just add 1MB).
    sub     r4, r4, #1             ; Decrement number of MB left.
    cmp     r4, #0                 ; Done?
    bne     %B28                   ; No - map next MB.

    bic     r0, r0, #0xF0000000    ; Clear section base address field.
    bic     r0, r0, #0x0FF00000    ; Clear section base address field.
    b       %B25                   ; Get and process the next OEMAddressTable element.

    ; ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    ; ~~~~~~~~~~ MAP UNCACHED and UNBUFFERED SECTION DESCRIPTORS ~~~~~~~~~~~~~~
    ; ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

29  tst     r0, #8                 ; Test for 'C' bit set (means we just used 
                                   ; above loop structure to map cached and buffered space).
    bic     r0, r0, #0x0C          ; Clear cached and buffered bits in the descriptor (clear C&B bits).
    add     r9, r9, #0x0800        ; Pointer to the first PTE for "unmapped uncached space" (0x2000 0000 + V_U_Adx).
    bne     %B20                   ; Repeat the descriptor setup for uncached space (map C=B=0 space).

ACTIVATEMMU
    ; The 1st Level Section Descriptors are setup. Initialize the MMU and turn it on.
    ;
    mov     r1, #1
    mcr     p15, 0, r1, c3, c0, 0   ; Set up access to domain 0.
    mov     r0, #0
    mcr     p15, 0, r0, c8, c7, 0   ; Flush the instruction and data TLBs.
    mcr     p15, 0, r1, c7, c10, 4  ; Drain the write and fill buffers.

	mrc     p15, 0, r1, c1, c0, 0   
    orr     r1, r1, #0x1            ; Enable MMU.
    orr     r1, r1, #0x1000         ; Enable IC.
    orr     r1, r1, #0x4            ; Enable DC.
    ldr     r2, =VirtualStart       ; Get virtual address of 'VirtualStart' label.
    cmp     r2, #0                  ; Make sure no stall on "mov pc,r2" below.

    ; Enable the MMU.
    ;
    mcr     p15, 0, r1, c1, c0, 0   ; MMU ON:  All memory accesses are now virtual.

    ; Jump to the virtual address of the 'VirtualStart' label.
    ;
    mov     pc, r2                  ;
    nop
    nop
    nop

    ; *************************************************************************
    ; *************************************************************************
    ; The MMU and caches are now enabled and we're running in a virtual
    ; address space.
    ;
    
    ALIGN
    
VirtualStart

    ;  Set up a supervisor mode stack.
    ;
    ; NOTE: These values must match the OEMAddressTable and .bib file entries for
    ; the bootloader.
    ;
    ldr     sp, =(IMAGE_EBOOT_STACK_CA + IMAGE_EBOOT_STACK_SIZE)

	
	
    ; Jump to the C entrypoint.
    ;
    bl      main					; Jump to main.c::main(), never to return...

    
        ENTRY_END 

        ; Include memory configuration file with g_oalEbootAddressTable
        INCLUDE addrtab_cfg.inc

        END

;------------------------------------------------------------------------------
