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
;   File:  omap_optbltasm_util.s
;
;   Revision:  March 31, 2009
;

	AREA	omap_optbltasm,CODE,READONLY

	EXPORT	PreloadLine

	INCLUDE	omap_optbltasm.inc

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; Predefined registers (for reference)
;	
; r13 a.k.a. sp
; r14 a.k.a. lr
; r15 a.k.a. pc

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; GLOBALs
;

;
; CACHELINEWIDTH represents the number of bytes in one cache line
;
CACHELINEWIDTH	equ	64

;----------------------------------------------------------------------------
; void PreloadLine(void const*   ptr,
;                  unsigned long bytes)
;
; Uses the PLD instruction to preload the cache with a line worth of data
;  for later processing.  This occurs in the background, while data already
;  in the cache is processed.
;
PreloadLine
	pld	[r0]
	and	r2, r0, #(CACHELINEWIDTH - 1)
	rsb	r2, r2, #CACHELINEWIDTH
	add	r0, r0, r2
	subs	r1, r1, r2
	ble	PL_done
PL_loop
	pld	[r0]
	add	r0, r0, #CACHELINEWIDTH
	subs	r1, r1, #CACHELINEWIDTH
	bgt	PL_loop
PL_done
	bx	lr

	END

