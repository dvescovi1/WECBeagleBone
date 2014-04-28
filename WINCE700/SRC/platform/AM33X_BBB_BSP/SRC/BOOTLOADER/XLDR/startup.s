;
;   File:  startup.s
;

        INCLUDE kxarm.h
        INCLUDE image_cfg.inc
        
        IMPORT  XLDRMain

;-------------------------------------------------------------------------------
;
;  Function:  StartUp
;
;  This function is entry point to X-Loader for Windows CE
;
;
        STARTUPTEXT

        LEAF_ENTRY StartUp

        mov     r9, #OEM_HIGH_SECURITY_GP       ; Set flag indicating GP device

 IF 0   ;; assume no support for HS devices on EVM
        ;---------------------------------------------------------------
        ; Check for running location to determine GP vs HS device
        ;---------------------------------------------------------------

        mov     r1, pc                          ; Get the PC
        ldr     r0, =IMAGE_XLDR_CODE_PA         ; Get XLDR start addr
        sub     r1, r1, r0                      ; Compute any offset; offset indicates XLDR is signed for HS
        sub     r1, r1, #8                      ; PC is +8 from instruction
        cmp     r1, #0                          
        beq     Done                            ; No offset, jump past reloc code
        
        mov     r8, r1                          ; Copy of offset
        mov     r9, #OEM_HIGH_SECURITY_HS       ; Set flag indicating HS device
        
        
        ;   Use CopyCode routine to copy itself to a safe, temporary execution area
        ;   b/c we are going to overwrite this section of code with the relocation         
      
Copy1   ldr     r0, =IMAGE_XLDR_DATA_PA         ; Move CopyCode routine to DATA area of XLDR
        ldr     r1, =CopyCode                   ; Source of CopyCode
        add     r1, r1, r8                      ; + offset
        mov     r2, #0x30                       ; Fixed size of CopyCode routine
        ldr     r3, =Copy2                      ; Jump to Copy2 when done
        add     r3, r3, r8                      ; + offset
        b       CopyCode

        ;   Setup for copy of XLDR to correct start address in SRAM using safe CopyCode
        ;   At end of copy of XLDR code, jump to XLDR start address
        ;   The offset check will jump past all the Copy1/Copy2 code
        
Copy2   ldr     r0, =IMAGE_XLDR_CODE_PA         ; Destination of XLDR
        ldr     r1, =IMAGE_XLDR_CODE_PA         ; Source of XLDR
        add     r1, r1, r8                      ; + offset
        ldr     r2, =IMAGE_XLDR_CODE_SIZE       ; Size of XLDR
        ldr     r3, =IMAGE_XLDR_CODE_PA         ; Jump to XLDR in new location when done
        ldr     pc, =IMAGE_XLDR_DATA_PA         ; Run CopyCode that was moved to DATA section of XLDR
 ENDIF  ;; assume no support for HS devices on EVM

                        
        ;---------------------------------------------------------------
        ; Set SVC mode & disable IRQ/FIQ
        ;---------------------------------------------------------------
        
Done    mrs     r0, cpsr                        ; Get current mode bits.
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

        ;---------------------------------------------------------------
        ; Check r9 for HS flag
        ;---------------------------------------------------------------
        
        cmp     r9, #OEM_HIGH_SECURITY_HS
        moveq   r0, #OEM_HIGH_SECURITY_HS
        movne   r0, #OEM_HIGH_SECURITY_GP
        
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
;
;  Function:  EnableCache_HS
;
        LEAF_ENTRY EnableCache_HS

        stmfd   sp!, {r4-r12, lr}               ; store off registers to stack

        ; Enable ICache
        mrc     p15, 0, r0, c1, c0, 0
        orr     r0, r0, #(1 :SHL: 12)           ; I-cache (enabled)
        mcr     p15, 0, r0, c1, c0, 0


        ; Invalidate L2 cache for EMU/HS devices
        mov     r1, #0x00                       ; Process ID
        mov     r2, #0x07                       ; PPA call flags
        ldr     r3, =g_L2InvalidateParams       ; Call arguments
        mov     r6, #0xFF                       ; New PPA task
        mov     r0, #0x28                       ; PPA L2 Cache Invalidate service ID
        mov     r12, r0                      

        mcr     p15, 0, r0, c7, c10, 4          ; data sync barrier
        mcr     p15, 0, r0, c7, c10, 5          ; data memory barrier
        dcd     0xE1600071                      ; Invoke PPA service
        

        ; Set L2 Cache Auxiliary register
        mov     r1, #0x00                       ; Process ID
        mov     r2, #0x07                       ; PPA call flags
        ldr     r3, =g_WriteL2CacheAuxControlParams
        mov     r6, #0xFF                       ; New PPA task
        mov     r12, #0x29                      ; PPA L2 Cache Aux Control Reg Write service ID

        mcr     p15, 0, r0, c7, c10, 4          ; data sync barrier
        mcr     p15, 0, r0, c7, c10, 5          ; data memory barrier
        dcd     0xE1600071                      ; Invoke PPA service


        ; Set CP15 aux control bits register
        mov     r1, #0x00                       ; Process ID
        mov     r2, #0x07                       ; PPA call flags
        ldr     r3, =g_WriteAuxControlParams    ; Call arguments
        mov     r6, #0xFF                       ; New PPA task
        mov     r12, #0x2A                      ; PPA Aux Control Reg Write service ID

        mcr     p15, 0, r0, c7, c10, 4          ; data sync barrier
        mcr     p15, 0, r0, c7, c10, 5          ; data memory barrier
        dcd     0xE1600071                      ; Invoke PPA service


        ; Set Auxiliary Control register bits
        mrc     p15, 0, r0, c1, c0, 1
        orr     r0, r0, #(1 :SHL: 1)            ; L2 cache (on)
        mcr     p15, 0, r0, c1, c0, 1           ; Set Auxiliary Control register (unsecure bank)


        ldmfd   sp!, {r4-r12, lr}               ; restore registers
        bx      lr

        ENTRY_END EnableCache_HS

;;-------------------------------------------------------------------------------
;;
;;  Function:  OALStall
;;
;;  This function implements busy stall loop. On entry r0 contains number
;;  of microseconds to stall. We assume that system is already on final CPU
;;  frequency.
;;
;        LEAF_ENTRY OALStall
;
;10      ldr     r2, =BSP_STALL_DELAY
;20      subs    r2, r2, #1
;        bne     %B20
;        subs    r0, r0, #1
;        bne     %B10
;        bx      lr
;
;
;       ENTRY_END OALStall
;      

;-------------------------------------------------------------------------------
;
;  Function:  CopyCode
;
;   r0 -    Dest
;   r1 -    Src
;   r2 -    Len
;   r3 -    Jump address
;
        LEAF_ENTRY CopyCode

10      ldr     r5, [r1]
        str     r5, [r0]    
        add     r0, r0, #4
        add     r1, r1, #4
        sub     r2, r2, #4
        cmp     r2, #0
        bne     %B10
        
        mov     pc, r3
        nop
        
        ENTRY_END CopyCode

;-------------------------------------------------------------------------------
;
;  Global data
;

OEM_HIGH_SECURITY_HS    EQU     1
OEM_HIGH_SECURITY_GP    EQU     2

g_L2InvalidateParams

        dcd     0x00, 0x00, 0x00, 0x00

g_WriteAuxControlParams

        dcd     0xE3, 0x10, 0x00, 0x00  

g_WriteL2CacheAuxControlParams

        dcd     0x00, 0x00, 0x40, 0x00

        END

;-------------------------------------------------------------------------------
