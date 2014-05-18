;
;   File:  startup.s
;

        INCLUDE kxarm.h
        INCLUDE image_cfg.inc
        
        IMPORT  XLDRMain

OEM_HIGH_SECURITY_HS    EQU     1
OEM_HIGH_SECURITY_GP    EQU     2

;-------------------------------------------------------------------------------
;
;  Function:  StartUp
;
;  This function is entry point to X-Loader for Windows CE
;
;
        STARTUPTEXT

        LEAF_ENTRY StartUp

        ;---------------------------------------------------------------
        ; Set SVC mode & disable IRQ/FIQ
        ;---------------------------------------------------------------
        
        mrs     r0, cpsr                        ; Get current mode bits.
        bic     r0, r0, #0x1F                   ; Clear mode bits.
        orr     r0, r0, #0xD3                   ; Disable IRQs/FIQs, SVC mode
        msr     cpsr_c, r0                      ; Enter supervisor mode

        ;---------------------------------------------------------------
        ; Flush all caches
        ;---------------------------------------------------------------

        ; Invalidate TLB and I cache
        mov     r0, #0                          ; setup up for MCR
        mcr     p15, 0, r0, c8, c7, 0           ; invalidate TLB's
        mcr     p15, 0, r0, c7, c5, 0           ; invalidate icache

        ;---------------------------------------------------------------
        ; Initialize CP15 control register
        ;---------------------------------------------------------------

        ; Set CP15 control bits register
        mrc     p15, 0, r0, c1, c0, 0
        bic     r0, r0, #(1 :SHL: 13)           ; Exception vector location (V bit) (0=normal)
        bic     r0, r0, #(1 :SHL: 12)           ; I-cache (disabled)
        orr     r0, r0, #(1 :SHL: 11)           ; Branch prediction (enabled)
        bic     r0, r0, #(1 :SHL: 2)            ; D-cache (disabled - enabled within WinCE kernel startup)
        orr     r0, r0, #(1 :SHL: 1)            ; alignment fault (enabled)
        bic     r0, r0, #(1 :SHL: 0)            ; MMU (disabled - enabled within WinCE kernel startup)
        mcr     p15, 0, r0, c1, c0, 0

        mov     r0, #OEM_HIGH_SECURITY_GP
        
        ;---------------------------------------------------------------
        ; Jump to XLDRMain
        ;---------------------------------------------------------------

        ldr     sp, =(IMAGE_XLDR_STACK_PA + IMAGE_XLDR_STACK_SIZE)
        b       XLDRMain

        ENTRY_END StartUp


;-------------------------------------------------------------------------------
;
;  Function:  EnableCache_GP
;
        LEAF_ENTRY EnableCache_GP

        ; Enable ICache
        mrc     p15, 0, r0, c1, c0, 0
        orr     r0, r0, #(1 :SHL: 12)           ; I-cache (enabled)
        mcr     p15, 0, r0, c1, c0, 0


        ; Invalidate L2 cache for GP devices
        mov     r12, #0x1                       ; invalidate L2 cache
        dcd     0xE1600070                      ; GP-only need to use ROM svc.

        
        ; Set L2 Cache Auxiliary register
        mrc     p15, 1, r0, c9, c0, 2
        orr     r0, r0, #(1 :SHL: 22)           ; Write-allocate in L2 disabled

        mov     r12, #0x2                       ; Set L2 Cache Auxiliary Control register
        dcd     0xE1600070                      ; GP-only need to use ROM svc.


        ; Set Auxiliary Control register bits
        mrc     p15, 0, r0, c1, c0, 1
        orr     r0, r0, #(1 :SHL: 16)           ; CP14/CP15 pipeline flush (on)
        bic     r0, r0, #(1 :SHL: 9)            ; PLDNOP Executes PLD instrs as NOPs (off)
        orr     r0, r0, #(1 :SHL: 7)            ; Prevent BTB branch size mispredicts (on)
        orr     r0, r0, #(1 :SHL: 6)            ; IBE Invalidate BTB enable (on)
        orr     r0, r0, #(1 :SHL: 5)            ; L1NEON enable caching of NEON data within L1 (on)
        bic     r0, r0, #(1 :SHL: 4)            ; Speculative access on AXI (off)
        bic     r0, r0, #(1 :SHL: 3)            ; L1 cache parity detection (off)
        orr     r0, r0, #(1 :SHL: 1)            ; L2 cache (on)
        orr     r0, r0, #(1 :SHL: 0)            ; L1 dcache alias support (off)

        mcr     p15, 0, r0, c1, c0, 1           ; Set Auxiliary Control register (unsecure bank)

        mov     r12, #0x3                       ; Set Auxiliary Control register (secure bank)
        dcd     0xE1600070                      ; GP-only need to use ROM svc.

        bx      lr

        ENTRY_END EnableCache_GP

;-------------------------------------------------------------------------------

        END

