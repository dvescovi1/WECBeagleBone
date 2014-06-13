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
        INCLUDE am33x_const.inc

        EXPORT INTERRUPTS_STATUS        
        EXPORT OALCPUStart
        EXPORT OALCPUIdle
        EXPORT OALCPUEnd
        EXPORT OALGetTTBR
        EXPORT OALConfigEMIFOPP100
        EXPORT OALConfigEMIFOPP50
        

        TEXTAREA

BEGIN_REGION
 
;-------------------------------------------------------------------------------
;
;  Function:  OALCPUStart
;
;  Marker indicating the start of cpu specific assembly. Never should get called
;
 LEAF_ENTRY OALCPUStart
     nop        
 ENTRY_END OALCPUStart
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
;
;  constants
;
max_assoc
       DCD         MAX_ASSOCIATIVITY

max_setnum
       DCD         MAX_SETNUMBER         
;-------------------------------------------------------------------------------


;-------------------------------------------------------------------------------
;
;  Function:  SaveContext
;
;  This function puts the mpu to OFF
;
SaveContext
                                     ;--------------------------------------
                                     ; save sp before modifiying the stack
        mov        r1, sp    
        stmdb      sp!, {r3 - r12, lr} 
 
                                     ;--------------------------------------
                                     ; save content of all registers          
        mrs        r2, SPSR
                                     ;--------------------------------------
                                     ; save the stack pointer stored in r1
        mov        r3, r1                                             
        stmia      r0!, {r2-r3}
                                     ;--------------------------------------
                                     ; save coprocessor access control reg
        mrc        p15, 0, r1, c1, c0, 2

                                     ;--------------------------------------
                                     ; save TTBR0, TTBR1, Trans. tbl base
        mrc        p15, 0, r2, c2, c0, 0
        mrc        p15, 0, r3, c2, c0, 1
        mrc        p15, 0, r4, c2, c0, 2

                                     ;--------------------------------------
                                     ; Data TLB lockdown, instr. TLB lockdown
        mrc        p15, 0, r5, c10, c0, 0
        mrc        p15, 0, r6, c10, c0, 1
        stmia      r0!, {r1-r6}

                                     ;--------------------------------------
                                     ; Primary remap, normal remap regs.
        mrc        p15, 0, r1, c10, c2, 0        
        mrc        p15, 0, r2, c10, c2, 1

                                     ;--------------------------------------
                                     ; secure/non-secure vector base address
                                     ; FCSE PI, Context PID
        mrc        p15, 0, r3, c12, c0, 0
        mrc        p15, 0, r4, c13, c0, 0
        mrc        p15, 0, r5, c13, c0, 1
        stmia      r0!, {r1 - r5}
                                     ;--------------------------------------
                                     ; domain access control reg
                                     ; data status fault, inst. status fault
                                     ; data aux fault status, 
                                     ; intr. aux fault status,
                                     ; data fault addr, instr fault addr
        mrc        p15, 0, r1, c3, c0, 0
        mrc        p15, 0, r2, c5, c0, 0
        mrc        p15, 0, r3, c5, c0, 1
        mrc        p15, 0, r4, c5, c1, 0
        mrc        p15, 0, r5, c5, c1, 1
        mrc        p15, 0, r6, c6, c0, 0
        mrc        p15, 0, r7, c6, c0, 2
        stmia      r0!, {r1 - r7}

                                     ;--------------------------------------
                                     ; user r/w thread & proc id
                                     ; user r/o thread and proc id
                                     ; priv only thread and proc id
                                     ; cache size selction
        mrc        p15, 0, r1, c13, c0, 2
        mrc        p15, 0, r2, c13, c0, 3
        mrc        p15, 0, r3, c13, c0, 4                                        
        mrc        p15, 2, r4, c0, c0, 0
        stmia      r0!, {r1 - r4}


                                     ;--------------------------------------
                                     ; save all modes
        mrs        r3, cpsr
                                     ;--------------------------------------
                                     ; fiq mode
        bic        r1, r3, #MODE_MASK
        orr        r1, r1, #FIQ_MODE
        msr        cpsr, r1
        mrs        r7, spsr
        stmia      r0!, {r7 - r14}
                                     ;--------------------------------------
                                     ; irq mode
        bic        r1, r3, #MODE_MASK
        orr        r1, r1, #IRQ_MODE
        msr        cpsr, r1
        mrs        r7, spsr
        stmia      r0!, {r7, r13, r14}
                                     ;--------------------------------------
                                     ; abort mode
        bic        r1, r3, #MODE_MASK
        orr        r1, r1, #ABORT_MODE
        msr        cpsr, r1
        mrs        r7, spsr
        stmia      r0!, {r7, r13, r14}
                                     ;--------------------------------------
                                     ; undef mode
        bic        r1, r3, #MODE_MASK
        orr        r1, r1, #UNDEF_MODE
        msr        cpsr, r1
        mrs        r7, spsr
        stmia      r0!, {r7, r13, r14}
                                     ;--------------------------------------
                                     ; system/user mode
        bic        r1, r3, #MODE_MASK
        orr        r1, r1, #SYS_MODE
        msr        cpsr, r1
        mrs        r7, spsr
        stmia      r0!, {r7, r13, r14}
                                     ;--------------------------------------
                                     ; original mode
        msr        CPSR, r3
                                     ;--------------------------------------
                                     ; control register
        mrc        p15, 0, r4, c1, c0, 0
        stmia      r0!, {r3, r4}
                                     ;--------------------------------------
                                     ; need to flush all cache, copied
                                     ; from cache code
        mrc     p15, 1, r0, c0, c0, 1   ; read clidr
        ands    r3, r0, #0x7000000  
        mov     r3, r3, lsr #23         ; cache level value
        beq     donea               

        mov     r10, #0                 ; start clean at cache level 0
loop1a  add     r2, r10, r10, lsr #1    ; work out 3x current cache level
        mov     r1, r0, lsr r2          ; extract cache type bits from clidr
        and     r1, r1, #7              ; mask of the bits for current cache only
        cmp     r1, #2                  ; see what cache we have at this level
        blt     skipa                   ; skip if no cache, or just i-cache

        mcr     p15, 2, r10, c0, c0, 0  ; select current cache level in cssr
        mov     r1, #0
        mcr     p15, 0, r1, c7, c5, 4   ; prefetch flush to sync the change to the cachesize id reg
        mrc     p15, 1, r1, c0, c0, 0   ; read the new csidr
        and     r2, r1, #7              ; extract the length of the cache lines
        add     r2, r2, #4              ; add 4 (line length offset)        
        ldr     r4, max_assoc
        ands    r4, r4, r1, lsr #3      ; r4 is maximum number on the way size
        clz     r5, r4                  ; r5 find bit position of way size increment        
        ldr     r7, max_setnum
        ands    r7, r7, r1, lsr #13     ; r7 extract max number of the index size

loop2a  mov     r9, r4                  ; r9 is working copy of max way size
loop3a  orr     r11, r10, r9, lsl r5    ; factor way and cache number into r11
        orr     r11, r11, r7, lsl r2    ; factor index number into r11

        mcr     p15, 0, r11, c7, c14, 2 ; clean and invalidate by set/way

        subs    r9, r9, #1              ; decrement the way
        bge     loop3a

        subs    r7, r7, #1              ; decrement the index
        bge     loop2a

skipa   add     r10, r10, #2            ; increment cache number
        cmp     r3, r10
        bgt     loop1a

donea   mov     r10, #0                 ; switch back to cache level 0
        mcr     p15, 2, r10, c0, c0, 0  ; select current cache level in cssr

        ldmia   sp!, {r3 - r12, lr} 
        mov     pc, lr      
;-------------------------------------------------------------------------------


;-------------------------------------------------------------------------------
;
;  Function:  OALCPURestoreContext
;
;  This function recovers from mpu OFF or mpu open retention
;
 LEAF_ENTRY OALCPURestoreContext
    
romRestoreLocation:
        MSR        cpsr_c, #SYS_MODE
                                                ;--------------------------------------
                                                ; Enable EMIF

;        ldr         r0, =AM33X_OCMC0_PA        
	    mov         r0, #0
        bl          EMIFEnable        

        mov         r1, #0
        mcr         p15, 0, r1, c7, c5, 0       ; invalidate instruction caches
        mcr         p15, 0, r1, c7, c5, 4       ; prefetch flush
        mcr         p15, 0, r1, c7, c5, 6       ; invalidate branch predictor array
        mcr         p15, 0, r1, c7, c5, 7       ; invalidate MVA from branch predictor array (?)
        mcr         p15, 0, r1, c8, c5, 0       ; invalidate instruction TLBs
        mcr         p15, 0, r1, c8, c6, 0       ; invalidate data TLBs
        
                                                ;--------------------------------------
                                                ; get restore location                                                        
        ldr         r1, =AM33X_OCMC0_PA        
;        mov         r1,  r0        
        ldr         r0,  [r1, #MPU_CONTEXT_PA_OFFSET]          
        ldr         r10, [r1, #TLB_INV_FUNC_ADDR_OFFSET]
        
        
                                        ;--------------------------------------
                                        ; restore content of all registers 
        ldmia       r0!, {r2 - r3}
        msr         spsr_cxsf, r2
        mov         sp, r3
                                        ;--------------------------------------
                                        ; restore coprocessor access control reg
        ldmia       r0!, {r1-r6}
        mcr         p15, 0, r1, c1, c0, 2
                                        ;--------------------------------------
                                        ; restore TTBR0, TTBR1, Trans. tbl base
        mcr         p15, 0, r2, c2, c0, 0
        mcr         p15, 0, r3, c2, c0, 1
        mcr         p15, 0, r4, c2, c0, 2        
                                        ;--------------------------------------
                                        ; Data TLB lockdown, instr. TLB lockdown
        mcr         p15, 0, r5, c10, c0, 0
        mcr         p15, 0, r6, c10, c0, 1
                                        ;--------------------------------------
                                        ; Primary remap, normal remap regs.
        ldmia       r0!, {r1 - r5}
        mcr         p15, 0, r1, c10, c2, 0        
        mcr         p15, 0, r2, c10, c2, 1
                                        ;--------------------------------------
                                        ; secure/non-secure vector base address
                                        ; FCSE PI, Context PID
        mcr         p15, 0, r3, c12, c0, 0
        mcr         p15, 0, r4, c13, c0, 0
        mcr         p15, 0, r5, c13, c0, 1        
                                        ;--------------------------------------
                                        ; domain access control reg
                                        ; data status fault, inst. status fault
                                        ; data aux fault status, 
                                        ; intr. aux fault status,
                                        ; data fault addr, instr fault addr
        ldmia       r0!, {r1 - r7}
        mcr         p15, 0, r1, c3, c0, 0
        mcr         p15, 0, r2, c5, c0, 0
        mcr         p15, 0, r3, c5, c0, 1
        mcr         p15, 0, r4, c5, c1, 0
        mcr         p15, 0, r5, c5, c1, 1
        mcr         p15, 0, r6, c6, c0, 0
        mcr         p15, 0, r7, c6, c0, 2
                                        ;--------------------------------------
                                        ; user r/w thread & proc id
                                        ; user r/o thread and proc id
                                        ; priv only thread and proc id
                                        ; cache size selection
        ldmia       r0!, {r1 - r4}
        mcr         p15, 0, r1, c13, c0, 2
        mcr         p15, 0, r2, c13, c0, 3
        mcr         p15, 0, r3, c13, c0, 4                                        
        mcr         p15, 2, r4, c0, c0, 0        
                                        ;--------------------------------------
                                        ; restore all modes
        mrs         r3, cpsr
                                        ;--------------------------------------
                                        ; fiq mode
        bic         r1, r3, #MODE_MASK
        orr         r1, r1, #FIQ_MODE
        msr         CPSR, r1
        ldmia       r0!, {r7 - r14}
        msr         spsr, r7        
                                        ;--------------------------------------
                                        ; irq mode
        bic         r1, r3, #MODE_MASK
        orr         r1, r1, #IRQ_MODE
        msr         CPSR, r1
        ldmia       r0!, {r7, r13, r14}
        msr         spsr, r7
                                        ;--------------------------------------
                                        ; abort mode
        bic         r1, r3, #MODE_MASK
        orr         r1, r1, #ABORT_MODE
        msr         CPSR, r1
        ldmia       r0!, {r7, r13, r14}
        msr         spsr, r7        
                                        ;--------------------------------------
                                        ; undef mode
        bic         r1, r3, #MODE_MASK
        orr         r1, r1, #UNDEF_MODE
        msr         CPSR, r1
        ldmia       r0!, {r7, r13, r14}
        msr         spsr, r7
                                        ;--------------------------------------
                                        ; system/user mode
        bic         r1, r3, #MODE_MASK
        orr         r1, r1, #SYS_MODE
        msr         CPSR, r1
        ldmia       r0!, {r7, r13, r14}
        msr         spsr, r7                                        
                                        ;--------------------------------------
                                        ; system/user mode
        ldmia       r0!, {r1}        
                                        ;--------------------------------------
                                        ; original mode
        msr         CPSR, r1
;        b 	    _test_restore_
        ;------------------------------------------------------------------------------
        ; For ARM it is recommended to make the physical address identical to the 
        ; virtual address, as instruction prefetch can cause problems if not done
        ; so.  Modify the page table entry corresponding to the code location such
        ; that the physical address is identical to the virtual address.

                                        ;--------------------------------------
                                        ; The translation base address could be
                                        ; either in TTBR0 or TTBR1 based on
                                        ; N valud of TTBRC.  If (n>0) and 
                                        ; (31:32-N) of VA is 0 use TTBR0
                                        ; else use TTBR1.                                        
        mrc     p15, 0, r1, c2, c0, 2
        and     r1, r1, #0x7
        cmp     r1, #0x0
        mrceq   p15, 0, r9, c2, c0, 0
        mrcne   p15, 0, r9, c2, c0, 1   
                                        ;--------------------------------------
                                        ; declare masks for TTBR bits
                                        ; and to help set the identity
                                        ; mapping in the MMU 1st PTE
        ldr     r6, =TTBRBIT_MASK
        ldr     r5, =MB_BOUNDARY
                                        ;--------------------------------------
                                        ; get location of ttbr and mask out
                                        ; attribute bits
                                        ; (r9) = base addr of PT
        and     r9, r9, r6                      
                                        ;--------------------------------------
                                        ; get physical address of PTE restore
                                        ; point
                                        ; (r8) = phys addr of PTE restore pt
                                        ; (r6) = mb boundary of PTE restore pt
        ldr     r1, =|$PTE_RESTORE|           
        bic     r2, r1, r5
        and     r6, pc, r5
        orr     r8, r2, r6
                                        ;--------------------------------------
                                        ; (r7) = PTE index
                                        ; clear any description bits
        mov     r7, r8, lsr #18
        bic     r7, r7, #DESC_MASK
                                        ;--------------------------------------
                                        ; (r9) = location of PTE
        add     r9, r9, r7
                                        ;--------------------------------------
                                        ; get identity value based on phys
                                        ; addr of restore pt
                                        ; (r5) = identity value to put in the
                                        ; PTE
        mov     r5, r6                   
        orr     r5, r5, #PTL1_KRW
        orr     r5, r5, #PTL1_SECTION
                                        ;--------------------------------------
                                        ; swap value in PTE to create the 
                                        ; identity map
                                        ; (r6) = orig val in PTE
        ldr     r6, [r9]
        str     r5, [r9]
                                        ;--------------------------------------
                                        ; memory barrier
        mov     r5, #0
        mcr     p15, 0, r5, c7, c10, 4
                                        ;--------------------------------------
                                        ; get original control register value
                                        ; (r9) = original control register value
        ldmia   r0!, {r4}
                                        ;--------------------------------------
                                        ; restore original control register
                                        ; w/ cache and MMU *DISABLED*
        bic     r4, r4, #ICACHE_MASK
        bic     r4, r4, #DCACHE_MASK
        bic     r4, r4, #MMU_MASK
        mcr     p15, r0, r4, c1, c0, 0
                                        ;--------------------------------------
                                        ; drain write buffers            
        mcr     p15, 0, r5, c7, c10, 4
                                        ;--------------------------------------
                                        ; enable MMU 
        orr     r4, r4, #MMU_MASK
        mcr     p15, 0, r4, c1, c0, 0
                                        ;--------------------------------------
                                        ; move pc to restore pt; ie 
                                        ; |%PTE_RESTORE|
        mov     pc, r8

|$PTE_RESTORE|                          ; restore original PTE entry, must use VA
        nop
                                              
        ldr     r9, =WINCE_FIRSTPT      ; non-cached VA for base of first level page tables
        str     r6, [r9, r7]            ; restore saved PTE value
;_test_restore_        
					;--------------------------------------    
	mov     r1, r10
        ldmia   sp!, {r3 - r12, lr} 
                                        
        mov     pc, r1                  ; returns to OALInvalidateTlb in cpu.s

 ENTRY_END OALCPURestoreContext
;-------------------------------------------------------------------------------



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
; decide whether SaveContext is needed or not
        mov        r4, r0
        
        ldr        r1, [r4, #SUSPEND_STATE_OFFSET]
        cmp        r1, #0
        ldrne      r0, [r4, #MPU_CONTEXT_VA_OFFSET]
        blne       SaveContext

;        ldr        r1, [r4, #SUSPEND_STATE_OFFSET]
;        cmp        r1, #0
;        movne      r0, r4
;        blne       EMIFDisable

                                         ;--------------------------------------
                                         ; memory barrier    
                                         ;
        mov        r2, #0x0
        mcr        p15, 0, r2, c7, c10, 4        
        mcr        p15, 0, r2, c7, c10, 5    
        nop                       
        dcd        WFI

;        nop
;        ldr        r1, [r4, #SUSPEND_STATE_OFFSET]
;        cmp        r1, #0
;        movne      r0, r4
;        bne        OALCPURestoreContext    ; this shouldnt return back here
        
        ldmia       sp!, {r3 - r12, lr} 
        mov         pc, lr               
 ENTRY_END OALCPUIdle
;-------------------------------------------------------------------------------



;-------------------------------------------------------------------------------
;
;  Function:  CPUStall
;
;  loops [r0] amount
;  r0 = amount to loop
;
;  uses: r0
;
CPUStall
        cmp         r0, #0x0
        subne       r0, r0, #0x1
        bne         CPUStall
        
        mov         pc, lr
;-------------------------------------------------------------------------------


;-------------------------------------------------------------------------------
;
;  Function:  EMIFDisable
;
;  This function disables access to SDRAM after enabling self-refresh.
;  r0 = ref CPU Info
;
;  return: none
;
;  uses: r0, r1, r2
;
EMIFDisable
        ldr        r1, [r0, #EMIF_REGS_OFFSET]
        ldr        r2, [r1, #EMIF_PWR_MGMT_CTRL_OFFSET]
        bic        r2, r2,  #EMIF_PWR_MGMT_CTRL_REG_SR_TIM_MASK
        orr        r2, r2,  #EMIF_PWR_MGMT_CTRL_REG_SR_TIM_8192_CLKS
        str        r2, [r1, #EMIF_PWR_MGMT_CTRL_OFFSET]
        
        ldr        r2, [r1, #EMIF_PWR_MGMT_CTRL_OFFSET]
        bic        r2, r2,  #EMIF_PWR_MGMT_CTRL_REG_LP_MODE_MASK
        orr        r2, r2,  #EMIF_PWR_MGMT_CTRL_REG_LP_MODE_SELFREFRESH
        str        r2, [r1, #EMIF_PWR_MGMT_CTRL_OFFSET]

        mov        r1, r0
        mov        r0, #1000
        bl         CPUStall

        mov        r0, r1
        ldr        r1, [r0, #CM_PER_REGS_OFFSET]
        mov        r2, #CM_MODULE_MODE_DISABLE
        str        r2, [r1, #CM_PER_EMIF_CLKCTRL_OFFSET]
        str        r2, [r1, #CM_PER_EMIF_FW_CLKCTRL_OFFSET]
 
        bx         lr                   
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
;
;  Function:  EMIFEnable
;
;  This function enables access to SDRAM and disables self-refresh.
;  r0 = ref CPU Info if VA addresses needs to be used in this routine else its 0
;
;  return: none
;
;  uses: r0, r1, r2
;
EMIFEnable

         cmp	    r0, #0
         ldrne      r1, [r0, #CM_PER_REGS_OFFSET]
         ldreq	    r1, =AM33X_CM_PER_PA
         mov        r2, #CM_MODULE_MODE_ENABLE
         str        r2, [r1, #CM_PER_EMIF_CLKCTRL_OFFSET]
         str        r2, [r1, #CM_PER_EMIF_FW_CLKCTRL_OFFSET]
         
_EMIFEnable_chk1         
         ldr        r2, [r1, #CM_PER_EMIF_CLKCTRL_OFFSET]
         cmp        r2, #CM_MODULE_MODE_ENABLE
         bne        _EMIFEnable_chk1
_EMIF_FW_Enable_chk1         
         ldr        r2, [r1, #CM_PER_EMIF_FW_CLKCTRL_OFFSET]
         cmp        r2, #CM_MODULE_MODE_ENABLE
         bne        _EMIF_FW_Enable_chk1
         
	 cmp	    r0, #0         
         ldrne      r1, [r0, #EMIF_REGS_OFFSET]
         ldreq      r1, =AM33X_EMIF_PA
         ldr        r2, [r1, #EMIF_PWR_MGMT_CTRL_OFFSET]
         bic        r2, r2,  #EMIF_PWR_MGMT_CTRL_REG_LP_MODE_MASK
         str        r2, [r1, #EMIF_PWR_MGMT_CTRL_OFFSET]
  
         bx         lr                   
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
;
;  Function:  OALInvalidateTlb
;
;  This function Invalidates the TLBs and enable I and D Cache
;
 LEAF_ENTRY OALInvalidateTlb
        mov        r1, #0
        mrc        p15, 0, r2, c1, c0, 0 ; get control code
        mcr        p15, 0, r1, c8, c7, 0 ; invalidate TLB
        orr        r2, r2, #ICACHE_MASK
        orr        r2, r2, #DCACHE_MASK
        mcr        p15, 0, r2, c1, c0, 0 ; enable i/d cache
        mcr        p15, 0, r1, c7, c10, 4; drain write buffers
        nop                 

        mov        pc, lr                       
    
 ENTRY_END OALInvalidateTlb
;-------------------------------------------------------------------------------

 
;-------------------------------------------------------------------------------
;
;  Function:  configEMIFOPP50
;
;  config EMIF for OPP 50
;  
;
  LEAF_ENTRY OALConfigEMIFOPP50
      
 ;     /*  DDR2 in SR  */
 ;/*     HWREG(SOC_EMIF_0_REGS + EMIF_PWR_MGMT_CTRL) |= 
 ;     ((EMIF_PWR_MGMT_CTRL_REG_LP_MODE_SELFREFRESH <<
 ;         EMIF_PWR_MGMT_CTRL_REG_LP_MODE_SHIFT) & EMIF_PWR_MGMT_CTRL_REG_LP_MODE);*/
     mov        r1, r0
     ldr        r0, [r1, #EMIF_REGS_OFFSET]
     ldr        r2, [r0, #EMIF_PWR_MGMT_CTRL_OFFSET]
     bic        r2, r2,  #EMIF_PWR_MGMT_CTRL_REG_LP_MODE_MASK
     orr        r2, r2,  #EMIF_PWR_MGMT_CTRL_REG_LP_MODE_SELFREFRESH
     str        r2, [r0, #EMIF_PWR_MGMT_CTRL_OFFSET]    
 
 ;/*     for(i_index=0;(i_index < PM_DELAY_COUNT);i_index++) {}  */
 
     ldr         r0, =15104
     bl          CPUStall
     
 ;     /*  PLL Configuration   */
 ;     /*  MN bypass   */
 ;/*     HWREG(SOC_CM_WKUP_REGS + CM_WKUP_CM_CLKMODE_DPLL_DDR) = 
 ;         (HWREG(SOC_CM_WKUP_REGS + CM_WKUP_CM_CLKMODE_DPLL_DDR) & 
 ;             (~CM_WKUP_CM_CLKMODE_DPLL_DDR_DPLL_EN)) | 
 ;                 CM_WKUP_CM_CLKMODE_DPLL_DDR_DPLL_EN_DPLL_MN_BYP_MODE; */
      ldr        r0, [r1, #CM_WKUP_REGS_OFFSET]
      ldr        r2, [r0, #CM_WKUP_CM_CLKMODE_DPLL_DDR_OFFSET]
      bic        r2, r2,  #CM_WKUP_CM_CLKMODE_DPLL_DDR_DPLL_EN_MASK
      orr        r2, r2,  #CM_WKUP_CM_CLKMODE_DPLL_DDR_DPLL_EN_DPLL_MN_BYP_MODE    
      str        r2, [r0, #CM_WKUP_CM_CLKMODE_DPLL_DDR_OFFSET]
      
 ;/*   while(((HWREG(SOC_CM_WKUP_REGS + CM_WKUP_CM_IDLEST_DPLL_DDR)) & 
 ;         CM_WKUP_CM_IDLEST_DPLL_DDR_ST_MN_BYPASS )!= 
 ;             CM_WKUP_CM_IDLEST_DPLL_DDR_ST_MN_BYPASS);*/
_OPP50_OALDPLLBypass_chk1             
     ldr        r2, [r0, #CM_WKUP_CM_IDLEST_DPLL_DDR_OFFSET]
     and        r2, r2,  #CM_WKUP_CM_IDLEST_DPLL_DDR_ST_MN_BYPASS
     cmp        r2, #CM_WKUP_CM_IDLEST_DPLL_DDR_ST_MN_BYPASS
     bne        _OPP50_OALDPLLBypass_chk1
     
 ;     /*  M & N   */
 ;/*     HWREG(SOC_CM_WKUP_REGS + CM_WKUP_CM_CLKSEL_DPLL_DDR) = 
 ;         (PM_OPP50_DDR_M << CM_WKUP_CM_CLKSEL_DPLL_DDR_DPLL_MULT_SHIFT) | 
 ;             (PM_OPP50_DDR_N);*/
 ;     ldr        r2, [r0, #CM_WKUP_CM_CLKSEL_DPLL_DDR_OFFSET]
 ;    bic        r2, r2,  #CM_WKUP_CM_CLKSEL_DPLL_DDR_DPLL_MULT_MASK
 ;     bic        r2, r2,  #CM_WKUP_CM_CLKSEL_DPLL_DDR_DPLL_DIV_MASK
     mov        r2, #0
     orr        r2, r2,  #(PM_OPP50_DDR_M :SHL: CM_WKUP_CM_CLKSEL_DPLL_DDR_DPLL_MULT_SHIFT)
     orr        r2, r2,  #PM_OPP50_DDR_N
     str        r2, [r0, #CM_WKUP_CM_CLKSEL_DPLL_DDR_OFFSET]
     
      
 ;     /*  M2  */  
 ;/*     HWREG(SOC_CM_WKUP_REGS + CM_WKUP_CM_DIV_M2_DPLL_DDR) = 
 ;         (HWREG(SOC_CM_WKUP_REGS + CM_WKUP_CM_DIV_M2_DPLL_DDR) & 
 ;         (~CM_WKUP_CM_DIV_M2_DPLL_DDR_DPLL_CLKOUT_DIV)) | (PM_OPP50_DDR_M2);*/     
      ldr        r2, [r0, #CM_WKUP_CM_DIV_M2_DPLL_DDR_OFFSET]
      bic        r2, r2,  #CM_WKUP_CM_DIV_M2_DPLL_DDR_DPLL_CLKOUT_DIV_MASK
      orr        r2, r2,  #PM_OPP50_DDR_M2
      str        r2, [r0, #CM_WKUP_CM_DIV_M2_DPLL_DDR_OFFSET]
          
      
 ;     /*  PLL Relock  */
 ;/*     HWREG(SOC_CM_WKUP_REGS + CM_WKUP_CM_CLKMODE_DPLL_DDR) = 
 ;     (HWREG(SOC_CM_WKUP_REGS + CM_WKUP_CM_CLKMODE_DPLL_DDR) & 
 ;         (~CM_WKUP_CM_CLKMODE_DPLL_DDR_DPLL_EN)) | 
 ;             CM_WKUP_CM_CLKMODE_DPLL_DDR_DPLL_EN_DPLL_LOCK_MODE; */    
      ldr        r2, [r0, #CM_WKUP_CM_CLKMODE_DPLL_DDR_OFFSET]
      bic        r2, r2,  #CM_WKUP_CM_CLKMODE_DPLL_DDR_DPLL_EN_MASK
      orr        r2, r2,  #CM_WKUP_CM_CLKMODE_DPLL_DDR_DPLL_EN_DPLL_LOCK_MODE    
      str        r2, [r0, #CM_WKUP_CM_CLKMODE_DPLL_DDR_OFFSET]
      
      
 ;/*     while(((HWREG(SOC_CM_WKUP_REGS + CM_WKUP_CM_IDLEST_DPLL_DDR)) & 
 ;         CM_WKUP_CM_IDLEST_DPLL_DDR_ST_DPLL_CLK )!= 
 ;             CM_WKUP_CM_IDLEST_DPLL_DDR_ST_DPLL_CLK_DPLL_LOCKED);*/     
_OPP50_OALDPLLLocked_chk1             
          ldr        r2, [r0, #CM_WKUP_CM_IDLEST_DPLL_DDR_OFFSET]
          and        r2, r2,  #CM_WKUP_CM_IDLEST_DPLL_DDR_ST_DPLL_CLK_DPLL_LOCKED
          cmp        r2, #CM_WKUP_CM_IDLEST_DPLL_DDR_ST_DPLL_CLK_DPLL_LOCKED
          bne        _OPP50_OALDPLLLocked_chk1
      
 ;     /*  EMIF PRCM   */
 ;     /* Enable EMIF4DC Firewall clocks*/
 ;/*     HWREG(SOC_CM_PER_REGS + CM_PER_EMIF_FW_CLKCTRL) = 
 ;      (HWREG(SOC_CM_PER_REGS + CM_PER_EMIF_FW_CLKCTRL) & 
 ;        (~CM_PER_EMIF_FW_CLKCTRL_MODULEMODE)) | CM_PER_EMIF_FW_CLKCTRL_MODULEMODE_ENABLE;*/
     
      
 ;     /* Enable EMIF4DC clocks*/
 ;/*     HWREG(SOC_CM_PER_REGS + CM_PER_EMIF_CLKCTRL) = 
 ;      (HWREG(SOC_CM_PER_REGS + CM_PER_EMIF_CLKCTRL) & (~CM_PER_EMIF_CLKCTRL_MODULEMODE)) |
 ;        CM_PER_EMIF_CLKCTRL_MODULEMODE_ENABLE;*/
      
 ;     /* Poll for module is functional */
 ;/*     while(HWREG(SOC_CM_PER_REGS + CM_PER_EMIF_CLKCTRL) != 
 ;         CM_PER_EMIF_CLKCTRL_MODULEMODE_ENABLE); */
 
          ldr        r0, [r1, #CM_PER_REGS_OFFSET]
          mov        r2, #CM_MODULE_MODE_ENABLE
          str        r2, [r0, #CM_PER_EMIF_CLKCTRL_OFFSET]
          str        r2, [r0, #CM_PER_EMIF_FW_CLKCTRL_OFFSET]
          
_OPP50_EMIFEnable_chk1         
          ldr        r2, [r0, #CM_PER_EMIF_CLKCTRL_OFFSET]
          cmp        r2, #CM_MODULE_MODE_ENABLE
          bne        _OPP50_EMIFEnable_chk1
_OPP50_EMIFFW_Enable_chk1         
          ldr        r2, [r0, #CM_PER_EMIF_FW_CLKCTRL_OFFSET]
          cmp        r2, #CM_MODULE_MODE_ENABLE
          bne        _OPP50_EMIFFW_Enable_chk1             
      
 ; //  /*VTP Enable*/
 ; //  //Write 1 to enable VTP
 ; //  HWREG(SOC_CONTROL_REGS + CONTROL_VTP_CTRL) |= 0x00000040;
 ; //  //Write 0 to CLRZ bit
 ; //  HWREG(SOC_CONTROL_REGS + CONTROL_VTP_CTRL) &= 0xFFFFFFFE;
 ; //  //Write 1 to CLRZ bit
 ; //  HWREG(SOC_CONTROL_REGS + CONTROL_VTP_CTRL) |= 0x00000001;
 ; //  while((HWREG(SOC_CONTROL_REGS + CONTROL_VTP_CTRL) & 0x00000020) != 0x00000020);
 ;     
 ;     /*CMD REG PHY*/
      ldr        r0, [r1, #DDR_PHY_REGS_OFFSET]
 ;/*     HWREG(CMD0_REG_PHY_CTRL_SLAVE_RATIO_0) = DDR2_REG_PHY_CTRL_SLAVE_RATIO;*/
      mov        r2, #DDR2_REG_PHY_CTRL_SLAVE_RATIO
      str        r2, [r0, #CMD0_REG_PHY_CTRL_SLAVE_RATIO_0_OFFSET]
      
 ;/*     HWREG(CMD0_REG_PHY_CTRL_SLAVE_FORCE_0) = 0;*/     
      mov        r2, #0
      str        r2, [r0, #CMD0_REG_PHY_CTRL_SLAVE_FORCE_0_OFFSET]
      
 ;/*     HWREG(CMD0_REG_PHY_CTRL_SLAVE_DELAY_0) = 0;*/
      mov        r2, #0
      str        r2, [r0, #CMD0_REG_PHY_CTRL_SLAVE_DELAY_0_OFFSET]   
      
 ;/*     HWREG(CMD0_REG_PHY_DLL_LOCK_DIFF_0)    = 0;*/     
      mov        r2, #0
      str        r2, [r0, #CMD0_REG_PHY_DLL_LOCK_DIFF_0_OFFSET]   
      
 ;/*     HWREG(CMD0_REG_PHY_INVERT_CLKOUT_0)    = 0;*/
      mov        r2, #0
      str        r2, [r0, #CMD0_REG_PHY_INVERT_CLKOUT_0_OFFSET]   
      
       
 ;/*     HWREG(CMD1_REG_PHY_CTRL_SLAVE_RATIO_0) = DDR2_REG_PHY_CTRL_SLAVE_RATIO;*/
      mov        r2, #DDR2_REG_PHY_CTRL_SLAVE_RATIO
      str        r2, [r0, #CMD1_REG_PHY_CTRL_SLAVE_RATIO_0_OFFSET]   
      
 ;/*     HWREG(CMD1_REG_PHY_CTRL_SLAVE_FORCE_0) = 0;*/
      mov        r2, #0
      str        r2, [r0, #CMD1_REG_PHY_CTRL_SLAVE_FORCE_0_OFFSET]   
      
 ;/*     HWREG(CMD1_REG_PHY_CTRL_SLAVE_DELAY_0) = 0;*/
      mov        r2, #0
      str        r2, [r0, #CMD1_REG_PHY_CTRL_SLAVE_DELAY_0_OFFSET]   
      
 ;/*     HWREG(CMD1_REG_PHY_DLL_LOCK_DIFF_0)    = 0;*/
      mov        r2, #0
      str        r2, [r0, #CMD1_REG_PHY_DLL_LOCK_DIFF_0_OFFSET]   
      
 ;/*     HWREG(CMD1_REG_PHY_INVERT_CLKOUT_0)    = 0;*/
      mov        r2, #0
      str        r2, [r0, #CMD1_REG_PHY_INVERT_CLKOUT_0_OFFSET]   
      
 ;/*     HWREG(CMD2_REG_PHY_CTRL_SLAVE_RATIO_0) = DDR2_REG_PHY_CTRL_SLAVE_RATIO;*/
      mov        r2, #DDR2_REG_PHY_CTRL_SLAVE_RATIO
      str        r2, [r0, #CMD2_REG_PHY_CTRL_SLAVE_RATIO_0_OFFSET]   
      
 ;/*     HWREG(CMD2_REG_PHY_CTRL_SLAVE_FORCE_0) = 0;*/
      mov        r2, #0
      str        r2, [r0, #CMD2_REG_PHY_CTRL_SLAVE_FORCE_0_OFFSET]   
      
 ;/*     HWREG(CMD2_REG_PHY_CTRL_SLAVE_DELAY_0) = 0;*/
      mov        r2, #0
      str        r2, [r0, #CMD2_REG_PHY_CTRL_SLAVE_DELAY_0_OFFSET]   
      
 ;/*     HWREG(CMD2_REG_PHY_DLL_LOCK_DIFF_0)    = 0;*/
      mov        r2, #0
      str        r2, [r0, #CMD2_REG_PHY_DLL_LOCK_DIFF_0_OFFSET]   
      
 ;/*     HWREG(CMD2_REG_PHY_INVERT_CLKOUT_0)    = 0;*/
      mov        r2, #0
      str        r2, [r0, #CMD2_REG_PHY_INVERT_CLKOUT_0_OFFSET]   
      
  
 ;      /*DATA0 and DATA1 PHY config*/
 ;/*     HWREG(DATA0_REG_PHY_RD_DQS_SLAVE_RATIO_0) = (((DDR2_PHY_RD_DQS_SLAVE_RATIO<<30)|(DDR2_PHY_RD_DQS_SLAVE_RATIO<<20)|
 ;                                                 (DDR2_PHY_RD_DQS_SLAVE_RATIO <<10)|(DDR2_PHY_RD_DQS_SLAVE_RATIO<<0)));*/
     ldr        r2, =DATA0_REG_PHY_RD_DQS_SLAVE_RATIO_0_VAL
     str        r2, [r0, #DATA0_REG_PHY_RD_DQS_SLAVE_RATIO_0_OFFSET]   
                                                  
 ;/*     HWREG(DATA0_REG_PHY_RD_DQS_SLAVE_RATIO_1) = DDR2_PHY_RD_DQS_SLAVE_RATIO>>2;*/
      mov        r2, #(DDR2_PHY_RD_DQS_SLAVE_RATIO :SHR: 2)
      str        r2, [r0, #DATA0_REG_PHY_RD_DQS_SLAVE_RATIO_1_OFFSET]   
      
 ;/*     HWREG(DATA0_REG_PHY_WR_DQS_SLAVE_RATIO_0) = 0;*/
      mov        r2, #0
      str        r2, [r0, #DATA0_REG_PHY_WR_DQS_SLAVE_RATIO_0_OFFSET]   
      
 ;/*     HWREG(DATA0_REG_PHY_WR_DQS_SLAVE_RATIO_1) = 0;*/
      mov        r2, #0
      str        r2, [r0, #DATA0_REG_PHY_WR_DQS_SLAVE_RATIO_1_OFFSET]   
      
 ;/*     HWREG(DATA0_REG_PHY_WRLVL_INIT_RATIO_0)   = 0;*/
      mov        r2, #0
      str        r2, [r0, #DATA0_REG_PHY_WRLVL_INIT_RATIO_0_OFFSET]   
      
 ;/*     HWREG(DATA0_REG_PHY_WRLVL_INIT_RATIO_1)   = 0;*/
      mov        r2, #0
      str        r2, [r0, #DATA0_REG_PHY_WRLVL_INIT_RATIO_1_OFFSET]   
      
 ;/*     HWREG(DATA0_REG_PHY_GATELVL_INIT_RATIO_0) = 0;*/
      mov        r2, #0
      str        r2, [r0, #DATA0_REG_PHY_GATELVL_INIT_RATIO_0_OFFSET]   
      
 ;/*     HWREG(DATA0_REG_PHY_GATELVL_INIT_RATIO_1) = 0;*/
      mov        r2, #0
      str        r2, [r0, #DATA0_REG_PHY_GATELVL_INIT_RATIO_1_OFFSET]   
      
 ;/*     HWREG(DATA0_REG_PHY_FIFO_WE_SLAVE_RATIO_0)= (((DDR2_REG_PHY_FIFO_WE_SLAVE_RATIO<<30)|(DDR2_REG_PHY_FIFO_WE_SLAVE_RATIO<<20)|
;                                                  (DDR2_REG_PHY_FIFO_WE_SLAVE_RATIO<<10)|(DDR2_REG_PHY_FIFO_WE_SLAVE_RATIO<<0)));*/
      ldr        r2, =DATA0_REG_PHY_FIFO_WE_SLAVE_RATIO_0_VAL
      str        r2, [r0, #DATA0_REG_PHY_FIFO_WE_SLAVE_RATIO_0_OFFSET]   
                                                  
 ;/*     HWREG(DATA0_REG_PHY_FIFO_WE_SLAVE_RATIO_1)= DDR2_REG_PHY_FIFO_WE_SLAVE_RATIO>>2;*/
      mov        r2, #(DDR2_REG_PHY_FIFO_WE_SLAVE_RATIO :SHR: 2)
      str        r2, [r0, #DATA0_REG_PHY_FIFO_WE_SLAVE_RATIO_1_OFFSET]   
      
 ;/*     HWREG(DATA0_REG_PHY_WR_DATA_SLAVE_RATIO_0)= (((DDR2_REG_PHY_WR_DATA_SLAVE_RATIO<<30)|(DDR2_REG_PHY_WR_DATA_SLAVE_RATIO<<20)|
;                                                  (DDR2_REG_PHY_WR_DATA_SLAVE_RATIO<<10)|(DDR2_REG_PHY_WR_DATA_SLAVE_RATIO<<0)));*/
      ldr        r2, =DATA0_REG_PHY_WR_DATA_SLAVE_RATIO_0_VAL
      str        r2, [r0, #DATA0_REG_PHY_WR_DATA_SLAVE_RATIO_0_OFFSET]   
 
 ;/*     HWREG(DATA0_REG_PHY_WR_DATA_SLAVE_RATIO_1)= DDR2_REG_PHY_WR_DATA_SLAVE_RATIO>>2;*/
      mov        r2, #(DDR2_REG_PHY_WR_DATA_SLAVE_RATIO :SHR: 2)
      str        r2, [r0, #DATA0_REG_PHY_WR_DATA_SLAVE_RATIO_1_OFFSET]   
      
 ;/*     HWREG(DATA0_REG_PHY_DLL_LOCK_DIFF_0)      = 0;*/
      mov        r2, #0
      str        r2, [r0, #DATA0_REG_PHY_DLL_LOCK_DIFF_0_OFFSET]   
      
  
 ;/*     HWREG(DATA1_REG_PHY_RD_DQS_SLAVE_RATIO_0) = (((DDR2_PHY_RD_DQS_SLAVE_RATIO<<30)|(DDR2_PHY_RD_DQS_SLAVE_RATIO<<20)|
 ;                                                 (DDR2_PHY_RD_DQS_SLAVE_RATIO <<10)|(DDR2_PHY_RD_DQS_SLAVE_RATIO<<0)));*/
      ldr        r2, =DATA1_REG_PHY_RD_DQS_SLAVE_RATIO_0_VAL
      str        r2, [r0, #DATA1_REG_PHY_RD_DQS_SLAVE_RATIO_0_OFFSET]   
 
 ;/*     HWREG(DATA1_REG_PHY_RD_DQS_SLAVE_RATIO_1) = DDR2_PHY_RD_DQS_SLAVE_RATIO>>2;*/
      mov        r2, #(DDR2_PHY_RD_DQS_SLAVE_RATIO :SHR: 2)
      str        r2, [r0, #DATA1_REG_PHY_RD_DQS_SLAVE_RATIO_1_OFFSET]   
      
 ;/*     HWREG(DATA1_REG_PHY_WR_DQS_SLAVE_RATIO_0) = 0;*/
      mov        r2, #0
      str        r2, [r0, #DATA1_REG_PHY_WR_DQS_SLAVE_RATIO_0_OFFSET]   
      
 ;/*     HWREG(DATA1_REG_PHY_WR_DQS_SLAVE_RATIO_1) = 0;*/
      mov        r2, #0
      str        r2, [r0, #DATA1_REG_PHY_WR_DQS_SLAVE_RATIO_1_OFFSET]   
      
 ;/*     HWREG(DATA1_REG_PHY_WRLVL_INIT_RATIO_0)   = 0;*/
      mov        r2, #0
      str        r2, [r0, #DATA1_REG_PHY_WRLVL_INIT_RATIO_0_OFFSET]   
      
 ;/*     HWREG(DATA1_REG_PHY_WRLVL_INIT_RATIO_1)   = 0;*/
      mov        r2, #0
      str        r2, [r0, #DATA1_REG_PHY_WRLVL_INIT_RATIO_1_OFFSET]   
      
 ;/*     HWREG(DATA1_REG_PHY_GATELVL_INIT_RATIO_0) = 0;*/
      mov        r2, #0
      str        r2, [r0, #DATA1_REG_PHY_GATELVL_INIT_RATIO_0_OFFSET]   
      
 ;/*     HWREG(DATA1_REG_PHY_GATELVL_INIT_RATIO_1) = 0;*/
      mov        r2, #0
      str        r2, [r0, #DATA1_REG_PHY_GATELVL_INIT_RATIO_1_OFFSET]   
      
 ;/*     HWREG(DATA1_REG_PHY_FIFO_WE_SLAVE_RATIO_0)= (((DDR2_REG_PHY_FIFO_WE_SLAVE_RATIO<<30)|(DDR2_REG_PHY_FIFO_WE_SLAVE_RATIO<<20)|
 ;                                                 (DDR2_REG_PHY_FIFO_WE_SLAVE_RATIO<<10)|(DDR2_REG_PHY_FIFO_WE_SLAVE_RATIO<<0)));*/
      ldr        r2, =DATA1_REG_PHY_FIFO_WE_SLAVE_RATIO_0_VAL
      str        r2, [r0, #DATA1_REG_PHY_FIFO_WE_SLAVE_RATIO_0_OFFSET]   
 
 ;/*     HWREG(DATA1_REG_PHY_FIFO_WE_SLAVE_RATIO_1)= DDR2_REG_PHY_FIFO_WE_SLAVE_RATIO>>2;*/
      mov        r2, #(DDR2_REG_PHY_FIFO_WE_SLAVE_RATIO :SHR: 2)
      str        r2, [r0, #DATA1_REG_PHY_FIFO_WE_SLAVE_RATIO_1_OFFSET]   
 
 ;/*     HWREG(DATA1_REG_PHY_WR_DATA_SLAVE_RATIO_0)= (((DDR2_REG_PHY_WR_DATA_SLAVE_RATIO<<30)|(DDR2_REG_PHY_WR_DATA_SLAVE_RATIO<<20)|
;                                                  (DDR2_REG_PHY_WR_DATA_SLAVE_RATIO<<10)|(DDR2_REG_PHY_WR_DATA_SLAVE_RATIO<<0)));*/
      ldr        r2, =DATA1_REG_PHY_WR_DATA_SLAVE_RATIO_0_VAL
      str        r2, [r0, #DATA1_REG_PHY_WR_DATA_SLAVE_RATIO_0_OFFSET]   
     
 ;/*     HWREG(DATA1_REG_PHY_WR_DATA_SLAVE_RATIO_1)= DDR2_REG_PHY_WR_DATA_SLAVE_RATIO>>2;*/
      mov        r2, #(DDR2_REG_PHY_WR_DATA_SLAVE_RATIO :SHR: 2)
      str        r2, [r0, #DATA1_REG_PHY_WR_DATA_SLAVE_RATIO_1_OFFSET]   
 
 ;/*     HWREG(DATA1_REG_PHY_DLL_LOCK_DIFF_0)      = 0;*/
      mov        r2, #0
      str        r2, [r0, #DATA1_REG_PHY_DLL_LOCK_DIFF_0_OFFSET]   
 
  
 ;     /* IO configuration*/
 ;     //IO to work for mDDR
      ldr        r0, [r1, #SYS_MISC2_REGS_OFFSET]      
      
 ;/*     HWREG(SOC_CONTROL_REGS + CONTROL_DDR_CMD_IOCTRL(0))     |= PM_DDR_IO_CONTROL;*/
      ldr        r2, [r0, #CONTROL_DDR_CMD_IOCTRL_0_OFFSET]    
      orr        r2, r2,  #PM_DDR_IO_CONTROL_LSB
      orr        r2, r2,  #PM_DDR_IO_CONTROL_MSB
      str        r2, [r0, #CONTROL_DDR_CMD_IOCTRL_0_OFFSET]    
      
 ;/*     HWREG(SOC_CONTROL_REGS + CONTROL_DDR_CMD_IOCTRL(1))     |= PM_DDR_IO_CONTROL;*/     
      ldr        r2, [r0, #CONTROL_DDR_CMD_IOCTRL_1_OFFSET]    
      orr        r2, r2,  #PM_DDR_IO_CONTROL_LSB
      orr        r2, r2,  #PM_DDR_IO_CONTROL_MSB
      str        r2, [r0, #CONTROL_DDR_CMD_IOCTRL_1_OFFSET]    
 
 ;/*     HWREG(SOC_CONTROL_REGS + CONTROL_DDR_CMD_IOCTRL(2))     |= PM_DDR_IO_CONTROL;*/     
      ldr        r2, [r0, #CONTROL_DDR_CMD_IOCTRL_2_OFFSET]    
      orr        r2, r2,  #PM_DDR_IO_CONTROL_LSB
      orr        r2, r2,  #PM_DDR_IO_CONTROL_MSB
      str        r2, [r0, #CONTROL_DDR_CMD_IOCTRL_2_OFFSET]    
 
 ;/*     HWREG(SOC_CONTROL_REGS + CONTROL_DDR_DATA_IOCTRL(0))    |= PM_DDR_IO_CONTROL;*/     
      ldr        r2, [r0, #CONTROL_DDR_DATA_IOCTRL_0_OFFSET]    
      orr        r2, r2,  #PM_DDR_IO_CONTROL_LSB
      orr        r2, r2,  #PM_DDR_IO_CONTROL_MSB
      str        r2, [r0, #CONTROL_DDR_DATA_IOCTRL_0_OFFSET]    
 
 ;/*     HWREG(SOC_CONTROL_REGS + CONTROL_DDR_DATA_IOCTRL(1))    |= PM_DDR_IO_CONTROL;*/     
      ldr        r2, [r0, #CONTROL_DDR_DATA_IOCTRL_1_OFFSET]    
      orr        r2, r2,  #PM_DDR_IO_CONTROL_LSB
      orr        r2, r2,  #PM_DDR_IO_CONTROL_MSB
      str        r2, [r0, #CONTROL_DDR_DATA_IOCTRL_1_OFFSET]    
 
 ;     //CKE controlled by EMIF/DDR_PHY
 ;/*     HWREG(SOC_CONTROL_REGS + CONTROL_DDR_CKE_CTRL)      |= 0x00000001;*/     
      ldr        r2, [r0, #CONTROL_DDR_CKE_CTRL_OFFSET]    
      orr        r2, r2,  #1
      str        r2, [r0, #CONTROL_DDR_CKE_CTRL_OFFSET]    
  
 ;     /*EMIF Timings*/     
      ldr        r0, [r1, #EMIF_REGS_OFFSET]
 ;/*     HWREG(SOC_EMIF_0_REGS + EMIF_DDR_PHY_CTRL_1)        =    OPP50_DDR2_READ_LATENCY;*/
      ldr        r2, =OPP50_DDR2_READ_LATENCY
      str        r2, [r0, #EMIF_DDR_PHY_CTRL_1_OFFSET]
      
 ;/*     HWREG(SOC_EMIF_0_REGS + EMIF_DDR_PHY_CTRL_1_SHDW)   =    OPP50_DDR2_READ_LATENCY;*/
      ldr        r2, =OPP50_DDR2_READ_LATENCY
      str        r2, [r0, #EMIF_DDR_PHY_CTRL_1_SHDW_OFFSET]
      
 ;/*     HWREG(SOC_EMIF_0_REGS + EMIF_DDR_PHY_CTRL_2)        =    OPP50_DDR2_READ_LATENCY;*/
      ldr        r2, =OPP50_DDR2_READ_LATENCY
      str        r2, [r0, #EMIF_DDR_PHY_CTRL_2_OFFSET]
      
 ;/*     HWREG(SOC_EMIF_0_REGS + EMIF_SDRAM_TIM_1)           =    OPP50_DDR2_SDRAM_TIMING1;*/
      ldr        r2, =OPP50_DDR2_SDRAM_TIMING1
      str        r2, [r0, #EMIF_SDRAM_TIM_1_OFFSET]
      
 ;/*     HWREG(SOC_EMIF_0_REGS + EMIF_SDRAM_TIM_1_SHDW)      =    OPP50_DDR2_SDRAM_TIMING1;*/
      ldr        r2, =OPP50_DDR2_SDRAM_TIMING1
      str        r2, [r0, #EMIF_SDRAM_TIM_1_SHDW_OFFSET]
      
 ;/*     HWREG(SOC_EMIF_0_REGS + EMIF_SDRAM_TIM_2)           =    OPP50_DDR2_SDRAM_TIMING2;*/
      ldr        r2, =OPP50_DDR2_SDRAM_TIMING2
      str        r2, [r0, #EMIF_SDRAM_TIM_2_OFFSET]
      
 ;/*     HWREG(SOC_EMIF_0_REGS + EMIF_SDRAM_TIM_2_SHDW)      =    OPP50_DDR2_SDRAM_TIMING2;*/
      ldr        r2, =OPP50_DDR2_SDRAM_TIMING2
      str        r2, [r0, #EMIF_SDRAM_TIM_2_SHDW_OFFSET]
      
 ;/*     HWREG(SOC_EMIF_0_REGS + EMIF_SDRAM_TIM_3)           =    OPP50_DDR2_SDRAM_TIMING3;*/
      ldr        r2, =OPP50_DDR2_SDRAM_TIMING3
      str        r2, [r0, #EMIF_SDRAM_TIM_3_OFFSET]
      
 ;/*     HWREG(SOC_EMIF_0_REGS + EMIF_SDRAM_TIM_3_SHDW)      =    OPP50_DDR2_SDRAM_TIMING3;      */
      ldr        r2, =OPP50_DDR2_SDRAM_TIMING3
      str        r2, [r0, #EMIF_SDRAM_TIM_3_SHDW_OFFSET]
      
 ;/*     HWREG(SOC_EMIF_0_REGS + EMIF_SDRAM_REF_CTRL)        =    OPP50_DDR2_REF_CTRL;*/
      ldr        r2, =OPP50_DDR2_REF_CTRL
      str        r2, [r0, #EMIF_SDRAM_REF_CTRL_OFFSET]
      
 ;/*     HWREG(SOC_EMIF_0_REGS + EMIF_SDRAM_REF_CTRL_SHDW)   =    OPP50_DDR2_REF_CTRL;*/
      ldr        r2, =OPP50_DDR2_REF_CTRL
      str        r2, [r0, #EMIF_SDRAM_REF_CTRL_SHDW_OFFSET]
      
 ;/*     HWREG(SOC_EMIF_0_REGS + EMIF_SDRAM_CONFIG)          =    OPP50_DDR2_SDRAM_CONFIG;*/
      ldr        r2, =OPP50_DDR2_SDRAM_CONFIG
      str        r2, [r0, #EMIF_SDRAM_CONFIG_OFFSET]
      
 ;/*     HWREG(SOC_EMIF_0_REGS + EMIF_SDRAM_CONFIG_2)        =    OPP50_DDR2_SDRAM_CONFIG;*/
      ldr        r2, =OPP50_DDR2_SDRAM_CONFIG
      str        r2, [r0, #EMIF_SDRAM_CONFIG_2_OFFSET]
      
  
 ;     /*  DDR out of SR   */
 ;/*     HWREG(SOC_EMIF_0_REGS + EMIF_PWR_MGMT_CTRL) &= 
 ;     ~((EMIF_PWR_MGMT_CTRL_REG_LP_MODE_SELFREFRESH <<
 ;         EMIF_PWR_MGMT_CTRL_REG_LP_MODE_SHIFT) & EMIF_PWR_MGMT_CTRL_REG_LP_MODE);*/
      
      ldr        r0, [r1, #EMIF_REGS_OFFSET]
      ldr        r2, [r0, #EMIF_PWR_MGMT_CTRL_OFFSET]
      bic        r2, r2,  #EMIF_PWR_MGMT_CTRL_REG_LP_MODE_MASK
      str        r2, [r0, #EMIF_PWR_MGMT_CTRL_OFFSET]        
  
 ENTRY_END configEMIFOPP50
;-------------------------------------------------------------------------------
   
;-------------------------------------------------------------------------------
;
;  Function:  configEMIFOPP100
;
;  config EMIF for OPP 100
;  
;
 LEAF_ENTRY OALConfigEMIFOPP100
       
 ;     /*  DDR2 in SR  */
 ;/*     HWREG(SOC_EMIF_0_REGS + EMIF_PWR_MGMT_CTRL) |= 
  ;     ((EMIF_PWR_MGMT_CTRL_REG_LP_MODE_SELFREFRESH <<
  ;         EMIF_PWR_MGMT_CTRL_REG_LP_MODE_SHIFT) & EMIF_PWR_MGMT_CTRL_REG_LP_MODE);*/
      mov        r1, r0
      ldr        r0, [r1, #EMIF_REGS_OFFSET]
      ldr        r2, [r0, #EMIF_PWR_MGMT_CTRL_OFFSET]
      bic        r2, r2,  #EMIF_PWR_MGMT_CTRL_REG_LP_MODE_MASK
      orr        r2, r2,  #EMIF_PWR_MGMT_CTRL_REG_LP_MODE_SELFREFRESH
      str        r2, [r0, #EMIF_PWR_MGMT_CTRL_OFFSET]    
  
  ;/*     for(i_index=0;(i_index < PM_DELAY_COUNT);i_index++) {}  */
  
      ldr         r0, =15104
      bl          CPUStall
      
  ;     /*  PLL Configuration   */
  ;     /*  MN bypass   */
  ;/*     HWREG(SOC_CM_WKUP_REGS + CM_WKUP_CM_CLKMODE_DPLL_DDR) = 
  ;         (HWREG(SOC_CM_WKUP_REGS + CM_WKUP_CM_CLKMODE_DPLL_DDR) & 
  ;             (~CM_WKUP_CM_CLKMODE_DPLL_DDR_DPLL_EN)) | 
  ;                 CM_WKUP_CM_CLKMODE_DPLL_DDR_DPLL_EN_DPLL_MN_BYP_MODE; */
       ldr        r0, [r1, #CM_WKUP_REGS_OFFSET]
       ldr        r2, [r0, #CM_WKUP_CM_CLKMODE_DPLL_DDR_OFFSET]
       bic        r2, r2,  #CM_WKUP_CM_CLKMODE_DPLL_DDR_DPLL_EN_MASK
       orr        r2, r2,  #CM_WKUP_CM_CLKMODE_DPLL_DDR_DPLL_EN_DPLL_MN_BYP_MODE    
       str        r2, [r0, #CM_WKUP_CM_CLKMODE_DPLL_DDR_OFFSET]
       
  ;/*   while(((HWREG(SOC_CM_WKUP_REGS + CM_WKUP_CM_IDLEST_DPLL_DDR)) & 
  ;         CM_WKUP_CM_IDLEST_DPLL_DDR_ST_MN_BYPASS )!= 
  ;             CM_WKUP_CM_IDLEST_DPLL_DDR_ST_MN_BYPASS);*/
_OPP100_OALDPLLBypass_chk1             
      ldr        r2, [r0, #CM_WKUP_CM_IDLEST_DPLL_DDR_OFFSET]
      and        r2, r2,  #CM_WKUP_CM_IDLEST_DPLL_DDR_ST_MN_BYPASS
      cmp        r2, #CM_WKUP_CM_IDLEST_DPLL_DDR_ST_MN_BYPASS
      bne        _OPP100_OALDPLLBypass_chk1
      
  ;     /*  M & N   */
  ;/*     HWREG(SOC_CM_WKUP_REGS + CM_WKUP_CM_CLKSEL_DPLL_DDR) = 
  ;         (PM_OPP100_DDR_M << CM_WKUP_CM_CLKSEL_DPLL_DDR_DPLL_MULT_SHIFT) | 
  ;             (PM_OPP100_DDR_N);*/
  ;     ldr        r2, [r0, #CM_WKUP_CM_CLKSEL_DPLL_DDR_OFFSET]
  ;    bic        r2, r2,  #CM_WKUP_CM_CLKSEL_DPLL_DDR_DPLL_MULT_MASK
  ;     bic        r2, r2,  #CM_WKUP_CM_CLKSEL_DPLL_DDR_DPLL_DIV_MASK
      mov        r2, #0
      orr        r2, r2,  #(PM_OPP100_DDR_M :SHL: CM_WKUP_CM_CLKSEL_DPLL_DDR_DPLL_MULT_SHIFT)
      orr        r2, r2,  #PM_OPP100_DDR_N
      str        r2, [r0, #CM_WKUP_CM_CLKSEL_DPLL_DDR_OFFSET]
      
       
  ;     /*  M2  */  
  ;/*     HWREG(SOC_CM_WKUP_REGS + CM_WKUP_CM_DIV_M2_DPLL_DDR) = 
  ;         (HWREG(SOC_CM_WKUP_REGS + CM_WKUP_CM_DIV_M2_DPLL_DDR) & 
  ;         (~CM_WKUP_CM_DIV_M2_DPLL_DDR_DPLL_CLKOUT_DIV)) | (PM_OPP100_DDR_M2);*/     
       ldr        r2, [r0, #CM_WKUP_CM_DIV_M2_DPLL_DDR_OFFSET]
       bic        r2, r2,  #CM_WKUP_CM_DIV_M2_DPLL_DDR_DPLL_CLKOUT_DIV_MASK
       orr        r2, r2,  #PM_OPP100_DDR_M2
       str        r2, [r0, #CM_WKUP_CM_DIV_M2_DPLL_DDR_OFFSET]
           
       
  ;     /*  PLL Relock  */
  ;/*     HWREG(SOC_CM_WKUP_REGS + CM_WKUP_CM_CLKMODE_DPLL_DDR) = 
  ;     (HWREG(SOC_CM_WKUP_REGS + CM_WKUP_CM_CLKMODE_DPLL_DDR) & 
  ;         (~CM_WKUP_CM_CLKMODE_DPLL_DDR_DPLL_EN)) | 
  ;             CM_WKUP_CM_CLKMODE_DPLL_DDR_DPLL_EN_DPLL_LOCK_MODE; */    
       ldr        r2, [r0, #CM_WKUP_CM_CLKMODE_DPLL_DDR_OFFSET]
       bic        r2, r2,  #CM_WKUP_CM_CLKMODE_DPLL_DDR_DPLL_EN_MASK
       orr        r2, r2,  #CM_WKUP_CM_CLKMODE_DPLL_DDR_DPLL_EN_DPLL_LOCK_MODE    
       str        r2, [r0, #CM_WKUP_CM_CLKMODE_DPLL_DDR_OFFSET]
       
       
  ;/*     while(((HWREG(SOC_CM_WKUP_REGS + CM_WKUP_CM_IDLEST_DPLL_DDR)) & 
  ;         CM_WKUP_CM_IDLEST_DPLL_DDR_ST_DPLL_CLK )!= 
  ;             CM_WKUP_CM_IDLEST_DPLL_DDR_ST_DPLL_CLK_DPLL_LOCKED);*/     
_OPP100_OALDPLLLocked_chk1             
           ldr        r2, [r0, #CM_WKUP_CM_IDLEST_DPLL_DDR_OFFSET]
           and        r2, r2,  #CM_WKUP_CM_IDLEST_DPLL_DDR_ST_DPLL_CLK_DPLL_LOCKED
           cmp        r2, #CM_WKUP_CM_IDLEST_DPLL_DDR_ST_DPLL_CLK_DPLL_LOCKED
           bne        _OPP100_OALDPLLLocked_chk1
       
  ;     /*  EMIF PRCM   */
  ;     /* Enable EMIF4DC Firewall clocks*/
  ;/*     HWREG(SOC_CM_PER_REGS + CM_PER_EMIF_FW_CLKCTRL) = 
  ;      (HWREG(SOC_CM_PER_REGS + CM_PER_EMIF_FW_CLKCTRL) & 
  ;        (~CM_PER_EMIF_FW_CLKCTRL_MODULEMODE)) | CM_PER_EMIF_FW_CLKCTRL_MODULEMODE_ENABLE;*/
      
       
  ;     /* Enable EMIF4DC clocks*/
  ;/*     HWREG(SOC_CM_PER_REGS + CM_PER_EMIF_CLKCTRL) = 
  ;      (HWREG(SOC_CM_PER_REGS + CM_PER_EMIF_CLKCTRL) & (~CM_PER_EMIF_CLKCTRL_MODULEMODE)) |
  ;        CM_PER_EMIF_CLKCTRL_MODULEMODE_ENABLE;*/
       
  ;     /* Poll for module is functional */
  ;/*     while(HWREG(SOC_CM_PER_REGS + CM_PER_EMIF_CLKCTRL) != 
  ;         CM_PER_EMIF_CLKCTRL_MODULEMODE_ENABLE); */
  
           ldr        r0, [r1, #CM_PER_REGS_OFFSET]
           mov        r2, #CM_MODULE_MODE_ENABLE
           str        r2, [r0, #CM_PER_EMIF_CLKCTRL_OFFSET]
           str        r2, [r0, #CM_PER_EMIF_FW_CLKCTRL_OFFSET]
           
_OPP100_EMIFEnable_chk1         
           ldr        r2, [r0, #CM_PER_EMIF_CLKCTRL_OFFSET]
           cmp        r2, #CM_MODULE_MODE_ENABLE
           bne        _OPP100_EMIFEnable_chk1
_OPP100_EMIFFW_Enable_chk1         
           ldr        r2, [r0, #CM_PER_EMIF_FW_CLKCTRL_OFFSET]
           cmp        r2, #CM_MODULE_MODE_ENABLE
           bne        _OPP100_EMIFFW_Enable_chk1
                
               
       
  ; //  /*VTP Enable*/
  ; //  //Write 1 to enable VTP
  ; //  HWREG(SOC_CONTROL_REGS + CONTROL_VTP_CTRL) |= 0x00000040;
  ; //  //Write 0 to CLRZ bit
  ; //  HWREG(SOC_CONTROL_REGS + CONTROL_VTP_CTRL) &= 0xFFFFFFFE;
  ; //  //Write 1 to CLRZ bit
  ; //  HWREG(SOC_CONTROL_REGS + CONTROL_VTP_CTRL) |= 0x00000001;
  ; //  while((HWREG(SOC_CONTROL_REGS + CONTROL_VTP_CTRL) & 0x00000020) != 0x00000020);
  ;     
  ;     /*CMD REG PHY*/
       ldr        r0, [r1, #DDR_PHY_REGS_OFFSET]
  ;/*     HWREG(CMD0_REG_PHY_CTRL_SLAVE_RATIO_0) = DDR2_REG_PHY_CTRL_SLAVE_RATIO;*/
       mov        r2, #DDR2_REG_PHY_CTRL_SLAVE_RATIO
       str        r2, [r0, #CMD0_REG_PHY_CTRL_SLAVE_RATIO_0_OFFSET]
       
  ;/*     HWREG(CMD0_REG_PHY_CTRL_SLAVE_FORCE_0) = 0;*/     
       mov        r2, #0
       str        r2, [r0, #CMD0_REG_PHY_CTRL_SLAVE_FORCE_0_OFFSET]
       
  ;/*     HWREG(CMD0_REG_PHY_CTRL_SLAVE_DELAY_0) = 0;*/
       mov        r2, #0
       str        r2, [r0, #CMD0_REG_PHY_CTRL_SLAVE_DELAY_0_OFFSET]   
       
  ;/*     HWREG(CMD0_REG_PHY_DLL_LOCK_DIFF_0)    = 0;*/     
       mov        r2, #0
       str        r2, [r0, #CMD0_REG_PHY_DLL_LOCK_DIFF_0_OFFSET]   
       
  ;/*     HWREG(CMD0_REG_PHY_INVERT_CLKOUT_0)    = 0;*/
       mov        r2, #0
       str        r2, [r0, #CMD0_REG_PHY_INVERT_CLKOUT_0_OFFSET]   
       
        
  ;/*     HWREG(CMD1_REG_PHY_CTRL_SLAVE_RATIO_0) = DDR2_REG_PHY_CTRL_SLAVE_RATIO;*/
       mov        r2, #DDR2_REG_PHY_CTRL_SLAVE_RATIO
       str        r2, [r0, #CMD1_REG_PHY_CTRL_SLAVE_RATIO_0_OFFSET]   
       
  ;/*     HWREG(CMD1_REG_PHY_CTRL_SLAVE_FORCE_0) = 0;*/
       mov        r2, #0
       str        r2, [r0, #CMD1_REG_PHY_CTRL_SLAVE_FORCE_0_OFFSET]   
       
  ;/*     HWREG(CMD1_REG_PHY_CTRL_SLAVE_DELAY_0) = 0;*/
       mov        r2, #0
       str        r2, [r0, #CMD1_REG_PHY_CTRL_SLAVE_DELAY_0_OFFSET]   
       
  ;/*     HWREG(CMD1_REG_PHY_DLL_LOCK_DIFF_0)    = 0;*/
       mov        r2, #0
       str        r2, [r0, #CMD1_REG_PHY_DLL_LOCK_DIFF_0_OFFSET]   
       
  ;/*     HWREG(CMD1_REG_PHY_INVERT_CLKOUT_0)    = 0;*/
       mov        r2, #0
       str        r2, [r0, #CMD1_REG_PHY_INVERT_CLKOUT_0_OFFSET]   
       
  ;/*     HWREG(CMD2_REG_PHY_CTRL_SLAVE_RATIO_0) = DDR2_REG_PHY_CTRL_SLAVE_RATIO;*/
       mov        r2, #DDR2_REG_PHY_CTRL_SLAVE_RATIO
       str        r2, [r0, #CMD2_REG_PHY_CTRL_SLAVE_RATIO_0_OFFSET]   
       
  ;/*     HWREG(CMD2_REG_PHY_CTRL_SLAVE_FORCE_0) = 0;*/
       mov        r2, #0
       str        r2, [r0, #CMD2_REG_PHY_CTRL_SLAVE_FORCE_0_OFFSET]   
       
  ;/*     HWREG(CMD2_REG_PHY_CTRL_SLAVE_DELAY_0) = 0;*/
       mov        r2, #0
       str        r2, [r0, #CMD2_REG_PHY_CTRL_SLAVE_DELAY_0_OFFSET]   
       
  ;/*     HWREG(CMD2_REG_PHY_DLL_LOCK_DIFF_0)    = 0;*/
       mov        r2, #0
       str        r2, [r0, #CMD2_REG_PHY_DLL_LOCK_DIFF_0_OFFSET]   
       
  ;/*     HWREG(CMD2_REG_PHY_INVERT_CLKOUT_0)    = 0;*/
       mov        r2, #0
       str        r2, [r0, #CMD2_REG_PHY_INVERT_CLKOUT_0_OFFSET]   
       
   
 ;      /*DATA0 and DATA1 PHY config*/
 ;/*     HWREG(DATA0_REG_PHY_RD_DQS_SLAVE_RATIO_0) = (((DDR2_PHY_RD_DQS_SLAVE_RATIO<<30)|(DDR2_PHY_RD_DQS_SLAVE_RATIO<<20)|
 ;                                                 (DDR2_PHY_RD_DQS_SLAVE_RATIO <<10)|(DDR2_PHY_RD_DQS_SLAVE_RATIO<<0)));*/
     ldr        r2, =DATA0_REG_PHY_RD_DQS_SLAVE_RATIO_0_VAL
     str        r2, [r0, #DATA0_REG_PHY_RD_DQS_SLAVE_RATIO_0_OFFSET]   
                                                  
 ;/*     HWREG(DATA0_REG_PHY_RD_DQS_SLAVE_RATIO_1) = DDR2_PHY_RD_DQS_SLAVE_RATIO>>2;*/
      mov        r2, #(DDR2_PHY_RD_DQS_SLAVE_RATIO :SHR: 2)
      str        r2, [r0, #DATA0_REG_PHY_RD_DQS_SLAVE_RATIO_1_OFFSET]   
      
 ;/*     HWREG(DATA0_REG_PHY_WR_DQS_SLAVE_RATIO_0) = 0;*/
      mov        r2, #0
      str        r2, [r0, #DATA0_REG_PHY_WR_DQS_SLAVE_RATIO_0_OFFSET]   
      
 ;/*     HWREG(DATA0_REG_PHY_WR_DQS_SLAVE_RATIO_1) = 0;*/
      mov        r2, #0
      str        r2, [r0, #DATA0_REG_PHY_WR_DQS_SLAVE_RATIO_1_OFFSET]   
      
 ;/*     HWREG(DATA0_REG_PHY_WRLVL_INIT_RATIO_0)   = 0;*/
      mov        r2, #0
      str        r2, [r0, #DATA0_REG_PHY_WRLVL_INIT_RATIO_0_OFFSET]   
      
 ;/*     HWREG(DATA0_REG_PHY_WRLVL_INIT_RATIO_1)   = 0;*/
      mov        r2, #0
      str        r2, [r0, #DATA0_REG_PHY_WRLVL_INIT_RATIO_1_OFFSET]   
      
 ;/*     HWREG(DATA0_REG_PHY_GATELVL_INIT_RATIO_0) = 0;*/
      mov        r2, #0
      str        r2, [r0, #DATA0_REG_PHY_GATELVL_INIT_RATIO_0_OFFSET]   
      
 ;/*     HWREG(DATA0_REG_PHY_GATELVL_INIT_RATIO_1) = 0;*/
      mov        r2, #0
      str        r2, [r0, #DATA0_REG_PHY_GATELVL_INIT_RATIO_1_OFFSET]   
      
 ;/*     HWREG(DATA0_REG_PHY_FIFO_WE_SLAVE_RATIO_0)= (((DDR2_REG_PHY_FIFO_WE_SLAVE_RATIO<<30)|(DDR2_REG_PHY_FIFO_WE_SLAVE_RATIO<<20)|
;                                                  (DDR2_REG_PHY_FIFO_WE_SLAVE_RATIO<<10)|(DDR2_REG_PHY_FIFO_WE_SLAVE_RATIO<<0)));*/
      ldr        r2, =DATA0_REG_PHY_FIFO_WE_SLAVE_RATIO_0_VAL
      str        r2, [r0, #DATA0_REG_PHY_FIFO_WE_SLAVE_RATIO_0_OFFSET]   
                                                  
 ;/*     HWREG(DATA0_REG_PHY_FIFO_WE_SLAVE_RATIO_1)= DDR2_REG_PHY_FIFO_WE_SLAVE_RATIO>>2;*/
      mov        r2, #(DDR2_REG_PHY_FIFO_WE_SLAVE_RATIO :SHR: 2)
      str        r2, [r0, #DATA0_REG_PHY_FIFO_WE_SLAVE_RATIO_1_OFFSET]   
      
 ;/*     HWREG(DATA0_REG_PHY_WR_DATA_SLAVE_RATIO_0)= (((DDR2_REG_PHY_WR_DATA_SLAVE_RATIO<<30)|(DDR2_REG_PHY_WR_DATA_SLAVE_RATIO<<20)|
;                                                  (DDR2_REG_PHY_WR_DATA_SLAVE_RATIO<<10)|(DDR2_REG_PHY_WR_DATA_SLAVE_RATIO<<0)));*/
      ldr        r2, =DATA0_REG_PHY_WR_DATA_SLAVE_RATIO_0_VAL
      str        r2, [r0, #DATA0_REG_PHY_WR_DATA_SLAVE_RATIO_0_OFFSET]   
 
 ;/*     HWREG(DATA0_REG_PHY_WR_DATA_SLAVE_RATIO_1)= DDR2_REG_PHY_WR_DATA_SLAVE_RATIO>>2;*/
      mov        r2, #(DDR2_REG_PHY_WR_DATA_SLAVE_RATIO :SHR: 2)
      str        r2, [r0, #DATA0_REG_PHY_WR_DATA_SLAVE_RATIO_1_OFFSET]   
      
 ;/*     HWREG(DATA0_REG_PHY_DLL_LOCK_DIFF_0)      = 0;*/
      mov        r2, #0
      str        r2, [r0, #DATA0_REG_PHY_DLL_LOCK_DIFF_0_OFFSET]   
      
  
 ;/*     HWREG(DATA1_REG_PHY_RD_DQS_SLAVE_RATIO_0) = (((DDR2_PHY_RD_DQS_SLAVE_RATIO<<30)|(DDR2_PHY_RD_DQS_SLAVE_RATIO<<20)|
 ;                                                 (DDR2_PHY_RD_DQS_SLAVE_RATIO <<10)|(DDR2_PHY_RD_DQS_SLAVE_RATIO<<0)));*/
      ldr        r2, =DATA1_REG_PHY_RD_DQS_SLAVE_RATIO_0_VAL
      str        r2, [r0, #DATA1_REG_PHY_RD_DQS_SLAVE_RATIO_0_OFFSET]   
 
 ;/*     HWREG(DATA1_REG_PHY_RD_DQS_SLAVE_RATIO_1) = DDR2_PHY_RD_DQS_SLAVE_RATIO>>2;*/
      mov        r2, #(DDR2_PHY_RD_DQS_SLAVE_RATIO :SHR: 2)
      str        r2, [r0, #DATA1_REG_PHY_RD_DQS_SLAVE_RATIO_1_OFFSET]   
      
 ;/*     HWREG(DATA1_REG_PHY_WR_DQS_SLAVE_RATIO_0) = 0;*/
      mov        r2, #0
      str        r2, [r0, #DATA1_REG_PHY_WR_DQS_SLAVE_RATIO_0_OFFSET]   
      
 ;/*     HWREG(DATA1_REG_PHY_WR_DQS_SLAVE_RATIO_1) = 0;*/
      mov        r2, #0
      str        r2, [r0, #DATA1_REG_PHY_WR_DQS_SLAVE_RATIO_1_OFFSET]   
      
 ;/*     HWREG(DATA1_REG_PHY_WRLVL_INIT_RATIO_0)   = 0;*/
      mov        r2, #0
      str        r2, [r0, #DATA1_REG_PHY_WRLVL_INIT_RATIO_0_OFFSET]   
      
 ;/*     HWREG(DATA1_REG_PHY_WRLVL_INIT_RATIO_1)   = 0;*/
      mov        r2, #0
      str        r2, [r0, #DATA1_REG_PHY_WRLVL_INIT_RATIO_1_OFFSET]   
      
 ;/*     HWREG(DATA1_REG_PHY_GATELVL_INIT_RATIO_0) = 0;*/
      mov        r2, #0
      str        r2, [r0, #DATA1_REG_PHY_GATELVL_INIT_RATIO_0_OFFSET]   
      
 ;/*     HWREG(DATA1_REG_PHY_GATELVL_INIT_RATIO_1) = 0;*/
      mov        r2, #0
      str        r2, [r0, #DATA1_REG_PHY_GATELVL_INIT_RATIO_1_OFFSET]   
      
 ;/*     HWREG(DATA1_REG_PHY_FIFO_WE_SLAVE_RATIO_0)= (((DDR2_REG_PHY_FIFO_WE_SLAVE_RATIO<<30)|(DDR2_REG_PHY_FIFO_WE_SLAVE_RATIO<<20)|
 ;                                                 (DDR2_REG_PHY_FIFO_WE_SLAVE_RATIO<<10)|(DDR2_REG_PHY_FIFO_WE_SLAVE_RATIO<<0)));*/
      ldr        r2, =DATA1_REG_PHY_FIFO_WE_SLAVE_RATIO_0_VAL
      str        r2, [r0, #DATA1_REG_PHY_FIFO_WE_SLAVE_RATIO_0_OFFSET]   
 
 ;/*     HWREG(DATA1_REG_PHY_FIFO_WE_SLAVE_RATIO_1)= DDR2_REG_PHY_FIFO_WE_SLAVE_RATIO>>2;*/
      mov        r2, #(DDR2_REG_PHY_FIFO_WE_SLAVE_RATIO :SHR: 2)
      str        r2, [r0, #DATA1_REG_PHY_FIFO_WE_SLAVE_RATIO_1_OFFSET]   
 
 ;/*     HWREG(DATA1_REG_PHY_WR_DATA_SLAVE_RATIO_0)= (((DDR2_REG_PHY_WR_DATA_SLAVE_RATIO<<30)|(DDR2_REG_PHY_WR_DATA_SLAVE_RATIO<<20)|
;                                                  (DDR2_REG_PHY_WR_DATA_SLAVE_RATIO<<10)|(DDR2_REG_PHY_WR_DATA_SLAVE_RATIO<<0)));*/
      ldr        r2, =DATA1_REG_PHY_WR_DATA_SLAVE_RATIO_0_VAL
      str        r2, [r0, #DATA1_REG_PHY_WR_DATA_SLAVE_RATIO_0_OFFSET]   
     
 ;/*     HWREG(DATA1_REG_PHY_WR_DATA_SLAVE_RATIO_1)= DDR2_REG_PHY_WR_DATA_SLAVE_RATIO>>2;*/
      mov        r2, #(DDR2_REG_PHY_WR_DATA_SLAVE_RATIO :SHR: 2)
      str        r2, [r0, #DATA1_REG_PHY_WR_DATA_SLAVE_RATIO_1_OFFSET]   
 
 ;/*     HWREG(DATA1_REG_PHY_DLL_LOCK_DIFF_0)      = 0;*/
      mov        r2, #0
      str        r2, [r0, #DATA1_REG_PHY_DLL_LOCK_DIFF_0_OFFSET]         
  
   
  ;     /* IO configuration*/
  ;     //IO to work for mDDR
       ldr        r0, [r1, #SYS_MISC2_REGS_OFFSET]
       
       
  ;/*     HWREG(SOC_CONTROL_REGS + CONTROL_DDR_CMD_IOCTRL(0))     |= PM_DDR_IO_CONTROL;*/
       ldr        r2, [r0, #CONTROL_DDR_CMD_IOCTRL_0_OFFSET]    
       orr        r2, r2,  #PM_DDR_IO_CONTROL_LSB
       orr        r2, r2,  #PM_DDR_IO_CONTROL_MSB
       str        r2, [r0, #CONTROL_DDR_CMD_IOCTRL_0_OFFSET]    
       
  ;/*     HWREG(SOC_CONTROL_REGS + CONTROL_DDR_CMD_IOCTRL(1))     |= PM_DDR_IO_CONTROL;*/     
       ldr        r2, [r0, #CONTROL_DDR_CMD_IOCTRL_1_OFFSET]    
       orr        r2, r2,  #PM_DDR_IO_CONTROL_LSB
       orr        r2, r2,  #PM_DDR_IO_CONTROL_MSB
       str        r2, [r0, #CONTROL_DDR_CMD_IOCTRL_1_OFFSET]    
  
  ;/*     HWREG(SOC_CONTROL_REGS + CONTROL_DDR_CMD_IOCTRL(2))     |= PM_DDR_IO_CONTROL;*/     
       ldr        r2, [r0, #CONTROL_DDR_CMD_IOCTRL_2_OFFSET]    
       orr        r2, r2,  #PM_DDR_IO_CONTROL_LSB
       orr        r2, r2,  #PM_DDR_IO_CONTROL_MSB
       str        r2, [r0, #CONTROL_DDR_CMD_IOCTRL_2_OFFSET]    
  
  ;/*     HWREG(SOC_CONTROL_REGS + CONTROL_DDR_DATA_IOCTRL(0))    |= PM_DDR_IO_CONTROL;*/     
       ldr        r2, [r0, #CONTROL_DDR_DATA_IOCTRL_0_OFFSET]    
       orr        r2, r2,  #PM_DDR_IO_CONTROL_LSB
       orr        r2, r2,  #PM_DDR_IO_CONTROL_MSB
       str        r2, [r0, #CONTROL_DDR_DATA_IOCTRL_0_OFFSET]    
  
  ;/*     HWREG(SOC_CONTROL_REGS + CONTROL_DDR_DATA_IOCTRL(1))    |= PM_DDR_IO_CONTROL;*/     
       ldr        r2, [r0, #CONTROL_DDR_DATA_IOCTRL_0_OFFSET]    
       orr        r2, r2,  #PM_DDR_IO_CONTROL_LSB
       orr        r2, r2,  #PM_DDR_IO_CONTROL_MSB
       str        r2, [r0, #CONTROL_DDR_DATA_IOCTRL_0_OFFSET]    
  
  ;     //CKE controlled by EMIF/DDR_PHY
  ;/*     HWREG(SOC_CONTROL_REGS + CONTROL_DDR_CKE_CTRL)      |= 0x00000001;*/     
       ldr        r2, [r0, #CONTROL_DDR_CKE_CTRL_OFFSET]    
       orr        r2, r2,  #1
       str        r2, [r0, #CONTROL_DDR_CKE_CTRL_OFFSET]    
   
  ;     /*EMIF Timings*/     
       ldr        r0, [r1, #EMIF_REGS_OFFSET]
  ;/*     HWREG(SOC_EMIF_0_REGS + EMIF_DDR_PHY_CTRL_1)        =    OPP100_DDR2_READ_LATENCY;*/
       ldr        r2, =OPP100_DDR2_READ_LATENCY
       str        r2, [r0, #EMIF_DDR_PHY_CTRL_1_OFFSET]
       
  ;/*     HWREG(SOC_EMIF_0_REGS + EMIF_DDR_PHY_CTRL_1_SHDW)   =    OPP100_DDR2_READ_LATENCY;*/
       ldr        r2, =OPP100_DDR2_READ_LATENCY
       str        r2, [r0, #EMIF_DDR_PHY_CTRL_1_SHDW_OFFSET]
       
  ;/*     HWREG(SOC_EMIF_0_REGS + EMIF_DDR_PHY_CTRL_2)        =    OPP100_DDR2_READ_LATENCY;*/
       ldr        r2, =OPP100_DDR2_READ_LATENCY
       str        r2, [r0, #EMIF_DDR_PHY_CTRL_2_OFFSET]
       
  ;/*     HWREG(SOC_EMIF_0_REGS + EMIF_SDRAM_TIM_1)           =    OPP100_DDR2_SDRAM_TIMING1;*/
       ldr        r2, =OPP100_DDR2_SDRAM_TIMING1
       str        r2, [r0, #EMIF_SDRAM_TIM_1_OFFSET]
       
  ;/*     HWREG(SOC_EMIF_0_REGS + EMIF_SDRAM_TIM_1_SHDW)      =    OPP100_DDR2_SDRAM_TIMING1;*/
       ldr        r2, =OPP100_DDR2_SDRAM_TIMING1
       str        r2, [r0, #EMIF_SDRAM_TIM_1_SHDW_OFFSET]
       
  ;/*     HWREG(SOC_EMIF_0_REGS + EMIF_SDRAM_TIM_2)           =    OPP100_DDR2_SDRAM_TIMING2;*/
       ldr        r2, =OPP100_DDR2_SDRAM_TIMING2
       str        r2, [r0, #EMIF_SDRAM_TIM_2_OFFSET]
       
  ;/*     HWREG(SOC_EMIF_0_REGS + EMIF_SDRAM_TIM_2_SHDW)      =    OPP100_DDR2_SDRAM_TIMING2;*/
       ldr        r2, =OPP100_DDR2_SDRAM_TIMING2
       str        r2, [r0, #EMIF_SDRAM_TIM_2_SHDW_OFFSET]
       
  ;/*     HWREG(SOC_EMIF_0_REGS + EMIF_SDRAM_TIM_3)           =    OPP100_DDR2_SDRAM_TIMING3;*/
       ldr        r2, =OPP100_DDR2_SDRAM_TIMING3
       str        r2, [r0, #EMIF_SDRAM_TIM_3_OFFSET]
       
  ;/*     HWREG(SOC_EMIF_0_REGS + EMIF_SDRAM_TIM_3_SHDW)      =    OPP100_DDR2_SDRAM_TIMING3;      */
       ldr        r2, =OPP100_DDR2_SDRAM_TIMING3
       str        r2, [r0, #EMIF_SDRAM_TIM_3_SHDW_OFFSET]
       
  ;/*     HWREG(SOC_EMIF_0_REGS + EMIF_SDRAM_REF_CTRL)        =    OPP100_DDR2_REF_CTRL;*/
       ldr        r2, =OPP100_DDR2_REF_CTRL
       str        r2, [r0, #EMIF_SDRAM_REF_CTRL_OFFSET]
       
  ;/*     HWREG(SOC_EMIF_0_REGS + EMIF_SDRAM_REF_CTRL_SHDW)   =    OPP100_DDR2_REF_CTRL;*/
       ldr        r2, =OPP100_DDR2_REF_CTRL
       str        r2, [r0, #EMIF_SDRAM_REF_CTRL_SHDW_OFFSET]
       
  ;/*     HWREG(SOC_EMIF_0_REGS + EMIF_SDRAM_CONFIG)          =    OPP100_DDR2_SDRAM_CONFIG;*/
       ldr        r2, =OPP100_DDR2_SDRAM_CONFIG
       str        r2, [r0, #EMIF_SDRAM_CONFIG_OFFSET]
       
  ;/*     HWREG(SOC_EMIF_0_REGS + EMIF_SDRAM_CONFIG_2)        =    OPP100_DDR2_SDRAM_CONFIG;*/
       ldr        r2, =OPP100_DDR2_SDRAM_CONFIG
       str        r2, [r0, #EMIF_SDRAM_CONFIG_2_OFFSET]
       
   
  ;     /*  DDR out of SR   */
  ;/*     HWREG(SOC_EMIF_0_REGS + EMIF_PWR_MGMT_CTRL) &= 
  ;     ~((EMIF_PWR_MGMT_CTRL_REG_LP_MODE_SELFREFRESH <<
  ;         EMIF_PWR_MGMT_CTRL_REG_LP_MODE_SHIFT) & EMIF_PWR_MGMT_CTRL_REG_LP_MODE);*/
       
       ldr        r0, [r1, #EMIF_REGS_OFFSET]
       ldr        r2, [r0, #EMIF_PWR_MGMT_CTRL_OFFSET]
       bic        r2, r2,  #EMIF_PWR_MGMT_CTRL_REG_LP_MODE_MASK
       str        r2, [r0, #EMIF_PWR_MGMT_CTRL_OFFSET]        
   
 ENTRY_END configEMIFOPP100
;-------------------------------------------------------------------------------
 
;-------------------------------------------------------------------------------
;
;  Function:  OALCPUEnd
;
;  Marker indicating the end of cpu specific assembly. Never should get called
;
 LEAF_ENTRY OALCPUEnd
        nop        
 ENTRY_END OALCPUEnd
;-------------------------------------------------------------------------------

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
;  Function:  OALGetTTBR
;
;  work-around a new kernel feature which marks all non-cached memory
;  as non-executable.
;
 LEAF_ENTRY OALGetTTBR

     mrc         p15, 0, r1, c2, c0, 2 ; determine if using TTBR0 or 1
     and         r1, r1, #0x7
     cmp         r1, #0x0
     mrceq       p15, 0, r0, c2, c0, 0 ; get TTBR from either TTBR0 or 1
     mrcne       p15, 0, r0, c2, c0, 1
     bic         r0, r0, #0x1F          ; clear control bits
     bx          lr 

 ENTRY_END OALGetTTBR


END_REGION
;-------------------------------------------------------------------------------

        END

