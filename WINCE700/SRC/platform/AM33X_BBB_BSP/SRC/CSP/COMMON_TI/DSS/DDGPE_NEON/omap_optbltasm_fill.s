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
;   File:  omap_optbltasm_fill.s
;
;   Revision:  March 31, 2009
;

	AREA	omap_optbltasm,CODE,READONLY

	EXPORT	BlockFill8
	EXPORT	BlockFill16
	EXPORT	BlockFill24
	EXPORT	BlockFill32

	INCLUDE	omap_optbltasm.inc

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; Predefined registers (for reference)
;	
; r13 a.k.a. sp
; r14 a.k.a. lr
; r15 a.k.a. pc

;----------------------------------------------------------------------------
; ASSUMPTIONS
;----------------------------------------------------------------------------
; All destination memory has write combining enabled.  This mitigates the
;  need to align writes.
;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Common symbols
;
; It is convenient to use meaningful names for registers in the code below.
;  But some assemblers don't like redefining the aliases within the same
;  source file.  So a compromise is to use a set of standard assignments for
;  the entire file.  These are common across the routines below.  Not all are
;  used, but this consistency simplifies things.
;
dstptr	rn	r4
dststep	rn	r5
dstwidth	rn	r10
dstheight	rn	r11
color	rn	r12
pels	rn	lr

;----------------------------------------------------------------------------
; void BlockFill8(void*  dstptr,
;                 int32  dststride,
;                 uint32 dstleft,
;                 uint32 dsttop,
;                 uint32 dstwidth,
;                 uint32 dstheight,
;                 uint8  color)
;
; Performs a solid fill with an 8-bit constant color of the specified
;  rectangle.
;
BlockFill8
	stmfd	sp!, {r4-r12,lr}
	ONESURFINIT	8, 10
	ldr	color, [sp, #((10 + 2) * 4)]
	vdup.8	q0, color
	vdup.8	q1, color
BF8_lineloop
	subs	pels, dstwidth, #32
	blt	BF8_post
BF8_grouploop
	vst1.8	{d0,d1,d2,d3}, [dstptr]!
	subs	pels, pels, #32
	bge	BF8_grouploop
BF8_post
	adds	pels, pels, #32
	beq	BF8_linedone
BF8_postloop
	strb	color, [dstptr], #1
	subs	pels, pels, #1
	bgt	BF8_postloop
BF8_linedone
	add	dstptr, dstptr, dststep
	subs	dstheight, dstheight, #1
	bne	BF8_lineloop
	ldmfd	sp!, {r4-r12,pc}

;----------------------------------------------------------------------------
; void BlockFill16(void*  dstptr,
;                  int32  dststride,
;                  uint32 dstleft,
;                  uint32 dsttop,
;                  uint32 dstwidth,
;                  uint32 dstheight,
;                  uint8  color)
;
; Performs a solid fill with a 16-bit constant color of the specified
;  rectangle.
;
BlockFill16
	stmfd	sp!, {r4-r12,lr}
	ONESURFINIT	16, 10
	ldr	color, [sp, #((10 + 2) * 4)]
	vdup.16	q0, color
	vdup.16	q1, color
BF16_lineloop
	subs	pels, dstwidth, #16
	blt	BF16_post
BF16_grouploop
	vst1.16	{d0,d1,d2,d3}, [dstptr]!
	subs	pels, pels, #16
	bge	BF16_grouploop
BF16_post
	adds	pels, pels, #16
	beq	BF16_linedone
BF16_postloop
	strh	color, [dstptr], #2
	subs	pels, pels, #1
	bgt	BF16_postloop
BF16_linedone
	add	dstptr, dstptr, dststep
	subs	dstheight, dstheight, #1
	bne	BF16_lineloop
	ldmfd	sp!, {r4-r12,pc}

;----------------------------------------------------------------------------
; void BlockFill24(void*  dstptr,
;                  int32  dststride,
;                  uint32 dstleft,
;                  uint32 dsttop,
;                  uint32 dstwidth,
;                  uint32 dstheight,
;                  uint8  color)
;
; Performs a solid fill with a 24-bit constant color of the specified
;  rectangle.
;
BlockFill24
	stmfd	sp!, {r4-r12,lr}
	ONESURFINIT	24, 10
	ldr	color, [sp, #((10 + 2) * 4)]
	vdup.8	d0, color
	mov	color, color, lsr #8
	vdup.8	d1, color
	mov	color, color, lsr #8
	vdup.8	d2, color
BF24_lineloop
	subs	pels, dstwidth, #8
	blt	BF24_post
BF24_grouploop
	vst3.8	{d0,d1,d2}, [dstptr]!
	subs	pels, pels, #8
	bge	BF24_grouploop
BF24_post
	adds	pels, pels, #8
	beq	BF24_linedone
BF24_postloop
	vst3.8	{d0[0],d1[0],d2[0]}, [dstptr]!
	subs	pels, pels, #1
	bgt	BF24_postloop
BF24_linedone
	add	dstptr, dstptr, dststep
	subs	dstheight, dstheight, #1
	bne	BF24_lineloop
	ldmfd	sp!, {r4-r12,pc}
	
;----------------------------------------------------------------------------
; void BlockFill32(void*  dstptr,
;                  int32  dststride,
;                  uint32 dstleft,
;                  uint32 dsttop,
;                  uint32 dstwidth,
;                  uint32 dstheight,
;                  uint8  color)
;
; Performs a solid fill with a 32-bit constant color of the specified
;  rectangle.
;
BlockFill32
	stmfd	sp!, {r4-r12,lr}
	ONESURFINIT	32, 10
	ldr	color, [sp, #((10 + 2) * 4)]
	vdup.32	q0, color
	vdup.32	q1, color
BF32_lineloop
	subs	pels, dstwidth, #8
	blt	BF32_post
BF32_grouploop
	vst1.32	{d0,d1,d2,d3}, [dstptr]!
	subs	pels, pels, #8
	bge	BF32_grouploop
BF32_post
	adds	pels, pels, #8
	beq	BF32_linedone
BF32_postloop
	str	color, [dstptr], #4
	subs	pels, pels, #1
	bgt	BF32_postloop
BF32_linedone
	add	dstptr, dstptr, dststep
	subs	dstheight, dstheight, #1
	bne	BF32_lineloop
	ldmfd	sp!, {r4-r12,pc}

	END

