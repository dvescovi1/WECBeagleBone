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
;  File:  vfp.s

        INCLUDE kxarm.h

        EXPORT OALVFPGetFPSID
        EXPORT OALVFPEnable
        
        TEXTAREA

; Note: The VFP specific instructions cannot be assembled using the CE6 arm assembler.
;ASSEMBLER_ARM_ARCH7_SUPPORT EQU 0

;-------------------------------------------------------------------------------
;
;  Function:  OALVFPEnable
;
;  This function enables the Vector Floating Point system
;
        LEAF_ENTRY OALVFPEnable

        ; Enable access to co-processor cp10 and cp11
        ; for both kernel and user access
        mrc     p15, 0, r0, c1, c0, 2

        orr     r0, r0, #(3 << 22)          ; Kernel+user access to cp11
        orr     r0, r0, #(3 << 20)          ; Kernel+user access to cp10

        mcr     p15, 0, r0, c1, c0, 2       ; Set co-processor access bits


        ; Enable VFP co-processor by setting EN bit in FPEXC register
        mov     r0, #0
        orr     r0, r0, #(1 << 30)          ; EN (1=enable VFPLite and NEON coprocessor)

        mcr     p10, 7, r0, c8, c0, 0       ; Set FPEXC register

        mov     pc, lr

        ENTRY_END OALVFPEnable


;-------------------------------------------------------------------------------
;
;  Function:  OALVFPGetFPSID
;
;  This function returns the Vector Floating Point FPSID register contents
;
        LEAF_ENTRY OALVFPGetFPSID

        ; Read the FPSID register
        mrc     p10, 7, r0, c0, c0, 0
        mov     pc, lr

        ENTRY_END OALVFPGetFPSID


;-------------------------------------------------------------------------------
;
;  Function:  OALGetCACR
;
;  This function returns the Non-secure Access Control register contents
;
        LEAF_ENTRY OALGetCACR

        ; Read the CACR register
        mrc     p15, 0, r0, c1, c0, 2
        mov     pc, lr

        ENTRY_END OALGetCACR

;-------------------------------------------------------------------------------
;
;  Function:  OALGetNACR
;
;  This function returns the Non-secure Access Control register contents
;
        LEAF_ENTRY OALGetNACR

        ; Read the NACR register
        mrc     p15, 0, r0, c1, c1, 2
        mov     pc, lr

        ENTRY_END OALGetNACR

VFP_ENABLE	EQU     0x40000000
 
;-------------------------------------------------------------------------------
;
;  Function:  OALSaveNeonRegisters(DWORD * pSaveArea)
;
;  This function saves off all the Neon registers for a thread context switch
;
        LEAF_ENTRY OALSaveNeonRegisters

        ; save current VFP/Neon enable state
 IF ASSEMBLER_ARM_ARCH7_SUPPORT = 1
        fmrx        r2, fpexc
 ELSE
        DCD         0xeef82a10  ; fmrx        r2, fpexc 
 ENDIF
        
        ; enable VFP/Neon
        mov         r1, #VFP_ENABLE

 IF ASSEMBLER_ARM_ARCH7_SUPPORT = 1

        fmxr        fpexc, r1

        ; VFP/Neon registers not managed by CE6 kernel in every thread switch 
        ; save all 32 Neon registers + FPSCR
        vstmia.64   r0!, {d0-d15}
        vstmia.64   r0!,  {d16-d31}
        fmrx        r3, fpscr
        str         r3, [r0]

        ; restore VFP/Neon enable state
        fmxr        fpexc, r2
        
 ELSE
        ; use hex values for NEON instructions
        DCD         0xeee81a10  ; fmxr        fpexc, r1
        ; VFP/Neon registers not managed by CE6 kernel in every thread switch 
        ; save all 32 Neon registers + FPSCR
        DCD         0xeca00b20  ; vstmia.64   r0!, {d0-d15} 
        DCD         0xece00b20  ; vstmia.64   r0!,  {d16-d31} 
        DCD         0xeef13a10  ; fmrx        r3, fpscr
        str         r3, [r0]

        ; restore VFP/Neon enable state
        DCD         0xeee82a10  ; fmxr        fpexc, r2 

 ENDIF  

        mov         pc, lr
        
        ENTRY_END OALSaveNeonRegisters

;-------------------------------------------------------------------------------
;
;  Function:  OALRestoreNeonRegisters(DWORD *pRestoreArea) 
;
;  This function restores all the Neon registers for a thread context switch
;
        LEAF_ENTRY OALRestoreNeonRegisters

        ; save current VFP/Neon enable state
 IF ASSEMBLER_ARM_ARCH7_SUPPORT = 1
        fmrx        r2, fpexc
 ELSE
        DCD         0xeef82a10  ; fmrx        r2, fpexc 
 ENDIF

        ; enable VFP/Neon
        mov         r1, #VFP_ENABLE

 IF ASSEMBLER_ARM_ARCH7_SUPPORT = 1
        fmxr        fpexc, r1

        ; restore all 32 Neon registers + FPSCR
        vldmia.64   r0!, {d0-d15}
        vldmia.64   r0!,  {d16-d31}        
        ldr         r3, [r0]
        fmxr        fpscr, r3
        
        ; restore VFP/Neon enable state
        fmxr        fpexc, r2
 ELSE
        DCD         0xeee81a10  ; fmxr        fpexc, r1 

        ; restore all 32 Neon registers + FPSCR
        DCD         0xecb00b20  ; vldmia.64   r0!, {d0-d15} 
        DCD         0xecf00b20  ; vldmia.64   r0!,  {d16-d31} 
        ldr         r3, [r0]
        DCD         0xeee13a10  ; fmxr        fpscr, r3

        ; restore VFP/Neon enable state 
        DCD         0xeee82a10  ; fmxr        fpexc, r2 
 ENDIF
         
        mov         pc, lr
        
        ENTRY_END OALRestoreNeonRegisters

;-------------------------------------------------------------------------------
;
;  Function:  OALSaveAllVfpNeonRegisters(DWORD * pSaveArea)
;
;  This function saves off all the Neon registers prior to suspend
;
        LEAF_ENTRY OALSaveAllVfpNeonRegisters
        EXPORT OALSaveAllVfpNeonRegisters

        ; save current VFP/Neon enable state
 IF ASSEMBLER_ARM_ARCH7_SUPPORT = 1
        fmrx        r2, fpexc
 ELSE
        DCD         0xeef82a10  ; fmrx        r2, fpexc 
 ENDIF
        
        ; enable VFP/Neon
        mov         r1, #VFP_ENABLE

 IF ASSEMBLER_ARM_ARCH7_SUPPORT = 1

        fmxr        fpexc, r1

        ; save VFP/Neon data registers
        vstmia.64   r0!, {d0-d15}
        vstmia.64   r0!, {d16-d31}
        ; save VFP/Neon FPSCR register
        fmrx        r3, fpscr
        str         r3, [r0]!

        ; save VFP/Neon enable state (FPEXC register)
        str         r2, [r0]
        
 ELSE
        ; use hex values for NEON instructions
        
        DCD         0xeee81a10  ; fmxr        fpexc, r1

        ; save VFP/Neon data registers
        DCD         0xeca00b20  ; vstmia.64   r0!, {d0-d15} 
        DCD         0xece00b20  ; vstmia.64   r0!, {d16-d31} 
        ; save VFP/Neon FPSCR register
        DCD         0xeef13a10  ; fmrx        r3, fpscr
        str         r3, [r0]!

        ; save VFP/Neon enable state (FPEXC register)
        str         r2, [r0]

 ENDIF  

        mov         pc, lr
        
        ENTRY_END OALSaveAllVfpNeonRegisters

;-------------------------------------------------------------------------------
;
;  Function:  OALRestoreAllVfpNeonRegisters(DWORD *pRestoreArea) 
;
;  This function restores all the Neon registers after resume
;
        LEAF_ENTRY OALRestoreAllVfpNeonRegisters
        EXPORT OALRestoreAllVfpNeonRegisters

        ; enable VFP/Neon
        mov         r1, #VFP_ENABLE

 IF ASSEMBLER_ARM_ARCH7_SUPPORT = 1
        fmxr        fpexc, r1

        ; restore VFP/Neon data registers
        vldmia.64   r0!, {d0-d15}
        vldmia.64   r0!, {d16-d31}        
        ; restore VFP/Neon FPSCR register
        ldr         r3, [r0]!
        fmxr        fpscr, r3
        ; restore VFP/Neon enable state (FPEXC register)
	ldr         r2, [r0]
        fmxr        fpexc, r2
 ELSE
        DCD         0xeee81a10  ; fmxr        fpexc, r1 

        ; restore VFP/Neon data registers
        DCD         0xecb00b20  ; vldmia.64   r0!, {d0-d15} 
        DCD         0xecf00b20  ; vldmia.64   r0!,  {d16-d31} 
        ; restore VFP/Neon FPSCR register
        ldr         r3, [r0]!
        DCD         0xeee13a10  ; fmxr        fpscr, r3
	
        ; restore VFP/Neon enable state (FPEXC register)
	ldr         r2, [r0]
        DCD         0xeee82a10  ; fmxr        fpexc, r2 
 ENDIF
         
        mov         pc, lr
        
        ENTRY_END OALRestoreAllVfpNeonRegisters


;------------------------------------------------------------------------------

        END
