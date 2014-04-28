; All rights reserved ADENEO EMBEDDED 2010
;
;==============================================================================
;             Texas Instruments OMAP(TM) Platform Software
; (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
;
; Use of this software is controlled by the terms and conditions found
; in the license agreement under which this software has been supplied.
;
;==============================================================================
;
;
;  File: am3xx_const.inc
;

;------------------------------------------------------------------------------
; useful ARM masks
IRQ_MASK                        EQU 0x80    ; IRQ mask value
FIQ_MASK                        EQU 0x40    ; FIQ mask value
MODE_MASK                       EQU 0x1F    ; Processor Mode Mask
TBIT_MASK                       EQU 0x20    ; Thumb mode bit mask 

;------------------------------------------------------------------------------
; ARM modes
USR_MODE                        EQU 0X10    ; user mode
FIQ_MODE                        EQU 0x11    ; Fast Interrupt Mode (FIQ)
IRQ_MODE                        EQU 0x12    ; Interrupt Mode (IRQ)
SUP_MODE                        EQU 0x13    ; Supervisor Mode
ABORT_MODE                      EQU 0x17    ; Abort Mode
UNDEF_MODE                      EQU 0x1B    ; undef Mode
SYS_MODE                        EQU 0x1F    ; sys Mode

;------------------------------------------------------------------------------
; Cache related constants
ICACHE_MASK                     EQU 0x1000
DCACHE_MASK                     EQU 0x0004
MAX_ASSOCIATIVITY               EQU 0x03FF  ; max val of associativity (10-bit)
MAX_SETNUMBER                   EQU 0x7FFF  ; max val for set number (15-bit)

;------------------------------------------------------------------------------
; MMU related constants
MMU_MASK                        EQU 0x0001
DESC_MASK                       EQU 0x0003
PTL1_SECTION                    EQU 0x0002 
PTL1_KRW                        EQU 0x0400  ; bits 10, 11
TTBRBIT_MASK                    EQU 0xFFFFC000
MB_BOUNDARY                     EQU 0xFFF00000
WINCE_FIRSTPT                   EQU 0xFFFD0000 ; from armhigh.inc


WFI                             EQU 0xE320F003

;------------------------------------------------------------------------------
        END
