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
;   File:  omap_optbltasm_mask.s
;
;   Revision:  March 31, 2009
;

	AREA	omap_optbltasm,CODE,READONLY

	EXTERN	PreloadLine

	EXPORT	MaskCopy16to16withA1
	EXPORT	MaskCopy32to32withA1

	INCLUDE	omap_optbltasm.inc

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; Predefined registers (for reference)
;	
; r13 a.k.a. sp
; r14 a.k.a. lr
; r15 a.k.a. pc

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
srcptr	rn	r6
srcstep	rn	r7
mskptr	rn	r8
mskstep rn	r9
dstwidth	rn	r10
dstheight	rn	r11
pels	rn	lr

;----------------------------------------------------------------------------
; void MaskCopy32to32withA1(void*       dstptr,
;                           int32       dststride,
;                           uint32      dstleft,
;                           uint32      dsttop,
;                           uint32      dstwidth,
;                           uint32      dstheight,
;                           void const* srcptr,
;                           int32       srcstride,
;                           uint32      srcleft,
;                           uint32      srctop,
;                           void const* mskptr,
;                           int32       mskstride,
;                           uint32      mskleft,
;                           uint32      msktop)
;
; Performs a per-pixel transparent copy between rectangles of 32-bit surfaces
;  using a 1-bit mask.
;
 IF {FALSE}
MaskCopy32to32withA1
	stmfd	sp!, {r4-r12,lr}
	THREESURFINIT	32, 32, 1, 10
	
	ldr	r3, [sp, #((10 + 8) * 4)]	; get mskleft for the start of each line
	and	r3, r3, #7			; get initial left alignment

MC32321_lineloop
	PRELOADLINE32bpp	srcptr, dstwidth
	PRELOADLINE32bpp	dstptr, dstwidth
	PRELOADLINE1bpp		mksptr, dstwidth
	mov	pels, dstwidth
 IF REVERSESUBBYTEPIXELBITS
	mov	r1, #1				; set up...
	mov	r0, r1, lsl r3			; ...mask bit
 ELSE
	mov	r1, #0x80			; set up...
	mov	r0, r1, lsr r3			; ...mask bit
 ENDIF

MC32321_grouploop
	ldrb	r2, [mskptr], #1		; get mask byte
	cmp	r2, #0
	beq	MC32321_transentry
MC32321_singleloop
	tst	r2, r0				; check mask bit
	ldrne	r1, [srcptr]			; load source if mask bit set
	add	srcptr, srcptr, #4		; increment source pointer
	strne	r1, [dstptr]			; copy to dest if mask bit set
	add	dstptr, dstptr, #4		; increment dest pointer
	subs	pels, pels, #1			; decrement pixel counter
	beq	MC32321_linedone		; bail out if done with the line
 IF REVERSESUBBYTEPIXELBITS
	mov	r0, r0, lsl #1			; shift to next mask bit
	cmp	r0, #0x100			; check if done
 ELSE
	movs	r0, r0, lsr #1			; shift to next mask bit
 ENDIF
	bne	MC32321_singleloop		; loop for more pixels
 IF REVERSESUBBYTEPIXELBITS
	mov	r0, #1
 ELSE
	mov	r0, #0x80
 ENDIF
	b	MC32321_grouploop

MC32321_linedone
	subs	dstheight, dstheight, #1
	add	srcptr, srcptr, srcstep
	add	dstptr, dstptr, dststep
	add	mskptr, mskptr, mskstep
	bgt	MC32321_lineloop
	
	ldmfd	sp!, {r4-r12,pc}

MC32321_transloop
	ldrb	r2, [mskptr], #1
	cmp	r2, #0
	bne	MC32321_singleloop
MC32321_transentry
	subs	pels, pels, #8
	blt	MC32321_lastunalignedgroup
	add	srcptr, srcptr, #(8 * 4)
	add	dstptr, dstptr, #(8 * 4)
	bgt	MC32321_transloop
	b	MC32321_linedone
MC32321_lastunalignedgroup
	rsb	pels, pels, #6	; 7 - (pels + 8)
 IF REVERSESUBBYTEPIXELBITS
	mov	r0, #1
	mov	r0, r0, lsl pels
 ELSE
	mov	r0, #0x80
	mov	r0, r0, lsr pels
 ENDIF
	b	MC32321_singleloop

 ELSE	
 IF {FALSE}
 
MaskCopy32to32withA1
	stmfd	sp!, {r4-r12,lr}
	THREESURFINIT	32, 32, 1, 10
	
	ldr	r12, [sp, #((10 + 8) * 4)]	; save mskleft for the start of each line

MC32321_lineloop
	PRELOADLINE32bpp	dstptr, dstwidth
	PRELOADLINE32bpp	srcptr, dstwidth
	PRELOADLINE1bpp		mskptr, dstwidth

	MACRO
	MASKCOPYONEPIXEL	$bit, $tstreg0, $tmpreg1
 IF REVERSESUBBYTEPIXELBITS
	tst	$tstreg0, #(128 >> $bit)
 ELSE
	tst	$tstreg0, #(1 << $bit)
 ENDIF
	ldreq	$tmpreg1, [dstptr]
	ldrne	$tmpreg1, [srcptr]
	str	$tmpreg1, [dstptr], #4
	add	srcptr, srcptr, #4
	MEND

	mov	pels, dstwidth
	
	ands	r3, r12, #7
	beq	MC32321_octetloop
	ldrb	r0, [mskptr], #1
	cmp	r3, #7
	beq	MC32321_pre0
	cmp	r3, #6
	beq	MC32321_pre1
	cmp	r3, #5
	beq	MC32321_pre2
	cmp	r3, #4
	beq	MC32321_pre3
	cmp	r3, #3
	beq	MC32321_pre4
	cmp	r3, #2
	beq	MC32321_pre5
MC32321_pre6
	MASKCOPYONEPIXEL	6, r0, r1
MC32321_pre5
	MASKCOPYONEPIXEL	5, r0, r1
MC32321_pre4
	MASKCOPYONEPIXEL	4, r0, r1
MC32321_pre3
	MASKCOPYONEPIXEL	3, r0, r1
MC32321_pre2
	MASKCOPYONEPIXEL	2, r0, r1
MC32321_pre1
	MASKCOPYONEPIXEL	1, r0, r1
MC32321_pre0
	MASKCOPYONEPIXEL	0, r0, r1
	
	sub	pels, pels, r3
MC32321_octetloop
	subs	pels, pels, #8
	blt	MC32321_octetsdone
	ldrb	r0, [mskptr], #1
	MASKCOPYONEPIXEL	7, r0, r1
	MASKCOPYONEPIXEL	6, r0, r1
	MASKCOPYONEPIXEL	5, r0, r1
	MASKCOPYONEPIXEL	4, r0, r1
	MASKCOPYONEPIXEL	3, r0, r1
	MASKCOPYONEPIXEL	2, r0, r1
	MASKCOPYONEPIXEL	1, r0, r1
	MASKCOPYONEPIXEL	0, r0, r1
	subs	pels, pels, #8
	bge	MC32321_octetloop
MC32321_octetsdone
	adds	pels, pels, #8
	beq	MC32321_linedone

	ldrb	r0, [mskptr], #1
	cmp	pels, #1
	beq	MC32321_post7
	cmp	pels, #2
	beq	MC32321_post6
	cmp	pels, #3
	beq	MC32321_post5
	cmp	pels, #4
	beq	MC32321_post4
	cmp	pels, #5
	beq	MC32321_post3
	cmp	pels, #6
	beq	MC32321_post2
MC32321_post1
	MASKCOPYONEPIXEL	7, r0, r1
MC32321_post2
	MASKCOPYONEPIXEL	6, r0, r1
MC32321_post3
	MASKCOPYONEPIXEL	5, r0, r1
MC32321_post4
	MASKCOPYONEPIXEL	4, r0, r1
MC32321_post5
	MASKCOPYONEPIXEL	3, r0, r1
MC32321_post6
	MASKCOPYONEPIXEL	2, r0, r1
MC32321_post7
	MASKCOPYONEPIXEL	1, r0, r1


MC32321_linedone
	add	srcptr, srcptr, srcstep
	add	dstptr, dstptr, dststep
	add	mskptr, mskptr, mskstep
	subs	dstheight, dstheight, #1
	bgt	MC32321_lineloop

MC32321_done
	ldmfd	sp!, {r4-r12,pc}

 ELSE
	MACRO
	MaskCopy32321	$mask
	vdup.8	d8, $mask	; duplicate mask value in all lanes
	vtst.8	d8, d8, d15	; duplicate one bit in each mask to fill lane
	vmovl.s8	q4, d8	; extend bits into 16 bits
	vmovl.s16	q5, d9	; extend bits...
	vmovl.s16	q4, d8	; ...into 32 bits
	vbsl	q5, q3, q1	; choose source or...
	vbsl	q4, q2, q0	; ...destination based on masks
	MEND

MaskCopy32to32withA1
	stmfd	sp!, {r4-r12,lr}
	THREESURFINIT	32, 32, 1, 10
	
	ldr	r12, [sp, #((10 + 8) * 4)]	; save mskleft for the start of each line
	and	r12, r12, #7		; determine # of pixels out of 8 to skip

; Load masks into d15
 IF REVERSESUBBYTEPIXELBITS
 	mov	r0, #128
	vmov.u8	d15[0], r0
	mov	r0, r0, lsr #1
	vmov.u8	d15[1], r0
	mov	r0, r0, lsr #1
	vmov.u8	d15[2], r0
	mov	r0, r0, lsr #1
	vmov.u8	d15[3], r0
	mov	r0, r0, lsr #1
	vmov.u8	d15[4], r0
	mov	r0, r0, lsr #1
	vmov.u8	d15[5], r0
	mov	r0, r0, lsr #1
	vmov.u8	d15[6], r0
	mov	r0, r0, lsr #1
	vmov.u8	d15[7], r0
 ELSE
 	mov	r0, #1
	vmov.u8	d15[0], r0
	mov	r0, r0, lsl #1
	vmov.u8	d15[1], r0
	mov	r0, r0, lsl #1
	vmov.u8	d15[2], r0
	mov	r0, r0, lsl #1
	vmov.u8	d15[3], r0
	mov	r0, r0, lsl #1
	vmov.u8	d15[4], r0
	mov	r0, r0, lsl #1
	vmov.u8	d15[5], r0
	mov	r0, r0, lsl #1
	vmov.u8	d15[6], r0
	mov	r0, r0, lsl #1
	vmov.u8	d15[7], r0
 ENDIF

MC32321_lineloop
	PRELOADLINE32bpp	dstptr, dstwidth
	PRELOADLINE32bpp	srcptr, dstwidth
	PRELOADLINE1bpp		mskptr, dstwidth

	movs	r12, r12		; are there any pixels to skip
	beq	MC32321_groupentry	; if not, do groups of 8

; r0 = mask
; r1 = strays
; r2 = dstptr & ~31
; r3 = srcptr & ~31
MC32321_pre
;	ldrb	r0, [mskptr], #1	; load mask byte
;	mov	r1, #0xFF
;	and	r0, r0, r1, lsr r12	; mask off leftmost bits
;	rsb	r1, r12, #7		; get # of stray pixels to handle
;	subs	pels, pels, r1		; subtract from pixel count
;	bic	dstptr, dstptr, #31	; move back to 8 pixel (32 byte) boundary
;	bic	srcptr, srcptr, #31	; move back to 8 pixel (32 byte) boundary
;	vld1.32	{d0,d1,d2,d3}, [dstptr]
;	vld1.32	{d4,d5,d6,d7}, [srcptr]!
;	MaskCopy32321	r0
;	vst1.32	{d8,d9,d10,d11}, [dstptr]!

	ldrb	r0, [mskptr], #1
 IF REVERSESUBBYTEPIXELBITS
	mov	r1, #1
	mov	r1, r1, lsl r12
 ELSE
	mov	r1, #128		; set mask to leftmost pixel
	mov	r1, r1, lsr r12		; move mask to skip pixels
 ENDIF
MC32321_preloop
 	tst	r0, r1			; check bit
 	ldrne	r2, [srcptr]		; load source if bit set
 	strne	r2, [dstptr]		; store source if bit set
 	subs	pels, pels, #1
 	add	dstptr, dstptr, #4
 	add	srcptr, srcptr, #4
 	beq	MC32321_linedone
 IF REVERSESUBBYTEPIXELBITS
 	mov	r1, r1, lsl #1
 	cmp	r1, #256
 ELSE
 	movs	r1, r1, lsr #1
 ENDIF
 	bne	MC32321_preloop

MC32321_groupentry
	subs	pels, dstwidth, #8
	blt	MC32321_post
	
MC32321_grouploop
	ldrb	r0, [mskptr], #1		; load mask byte
	vld1.32	{d0,d1,d2,d3}, [dstptr]		; load 8 32-bit pixels from destination
	vld1.32	{d4,d5,d6,d7}, [srcptr]!	; load 8 32-bit pixels from source
	MaskCopy32321	r0
	vst1.32	{d8,d9,d10,d11}, [dstptr]!
	subs	pels, pels, #8
	bge	MC32321_grouploop

MC32321_post
	adds	pels, pels, #8
	ble	MC32321_linedone
	
	ldrb	r0, [mskptr], #1
 IF REVERSESUBBYTEPIXELBITS
	mov	r1, #128
	mov	r1, r1, lsr pels
 ELSE
	mov	r1, #1			; set mask to leftmost pixel
	mov	r1, r1, lsl pels	; move mask to skip pixels
 ENDIF
MC32321_postloop
 	tst	r0, r1			; check bit
 	ldrne	r2, [srcptr]		; load source if bit set
 	strne	r2, [dstptr]		; store source if bit set
 	add	dstptr, dstptr, #4
 	add	srcptr, srcptr, #4
 IF REVERSESUBBYTEPIXELBITS
 	mov	r1, r1, lsl #1
 ELSE
 	mov	r1, r1, lsr #1
 ENDIF
 	subs	pels, pels, #1
 	bne	MC32321_postloop

 IF {FALSE}	
MC32321_singles
	adds	pels, pels, #8
	ble	MC32321_linedone
	
; Load however many stray pixels there are (1 to 7) into register lanes
	ldrb	r0, [mskptr], #1		; get mask byte
	vld1.32	{d0,d1,d2,d3}, [dstptr]		; load 8 32-bit pixels from destination
	vld1.32	{d4,d5,d6,d7}, [srcptr]!	; load 8 32-bit pixels from source
	
MC32321_singlesloaddone
	MaskCopy32321

; Store however many stray pixels there are (1 to 7) into memory
	vst1.32	{d0[0]}, [dstptr]!
	subs	pels, pels, #1
	ble	MC32321_linedone
	vst1.32	{d0[1]}, [dstptr]!
	subs	pels, pels, #1
	ble	MC32321_linedone
	vst1.32	{d1[0]}, [dstptr]!
	subs	pels, pels, #1
	ble	MC32321_linedone
	vst1.32	{d1[1]}, [dstptr]!
	subs	pels, pels, #1
	ble	MC32321_linedone
	vst1.32	{d2[0]}, [dstptr]!
	subs	pels, pels, #1
	ble	MC32321_linedone
	vst1.32	{d2[1]}, [dstptr]!
	subs	pels, pels, #1
	ble	MC32321_linedone
	vst1.32	{d3[0]}, [dstptr]!
 ENDIF
 
MC32321_linedone
	add	srcptr, srcptr, srcstep
	add	dstptr, dstptr, dststep
	add	mskptr, mskptr, mskstep
	subs	dstheight, dstheight, #1
	bgt	MC32321_lineloop

MC32321_done
	ldmfd	sp!, {r4-r12,pc}
 ENDIF
 ENDIF

;----------------------------------------------------------------------------
; void MaskCopy16to16withA1(void*       dstptr,
;                           int32       dststride,
;                           uint32      dstleft,
;                           uint32      dsttop,
;                           uint32      dstwidth,
;                           uint32      dstheight,
;                           void const* srcptr,
;                           int32       srcstride,
;                           uint32      srcleft,
;                           uint32      srctop,
;                           void const* mskptr,
;                           int32       mskstride,
;                           uint32      mskleft,
;                           uint32      msktop)
;
; Performs a per-pixel transparent copy between rectangles of 16-bit surfaces
;  using a 1-bit mask.
;
	MACRO
	MaskCopy16161	$dst, $mask, $src0, $src1, $tmp
	vdup.8	$tmp, $mask	; duplicate mask value in all lanes
	vtst.8	$tmp, $tmp, d15	; duplicate one bit in each mask to fill lane
	vmovl.s8	$dst, $tmp	; extend bits into 16 bits
	vbsl	$dst, $src0, $src1	; select source0 or source1 based on masks
	MEND

MaskCopy16to16withA1
	stmfd	sp!, {r4-r12,lr}
	THREESURFINIT	16, 16, 1, 10
	
	ldr	r12, [sp, #((10 + 8) * 4)]	; save mskleft for the start of each line
	and	r12, r12, #7		; determine # of pixels out of 8 to skip

; Load masks into d15
 IF REVERSESUBBYTEPIXELBITS
 	mov	r0, #128
	vmov.u8	d15[0], r0
	mov	r0, r0, lsr #1
	vmov.u8	d15[1], r0
	mov	r0, r0, lsr #1
	vmov.u8	d15[2], r0
	mov	r0, r0, lsr #1
	vmov.u8	d15[3], r0
	mov	r0, r0, lsr #1
	vmov.u8	d15[4], r0
	mov	r0, r0, lsr #1
	vmov.u8	d15[5], r0
	mov	r0, r0, lsr #1
	vmov.u8	d15[6], r0
	mov	r0, r0, lsr #1
	vmov.u8	d15[7], r0
 ELSE
 	mov	r0, #1
	vmov.u8	d15[0], r0
	mov	r0, r0, lsl #1
	vmov.u8	d15[1], r0
	mov	r0, r0, lsl #1
	vmov.u8	d15[2], r0
	mov	r0, r0, lsl #1
	vmov.u8	d15[3], r0
	mov	r0, r0, lsl #1
	vmov.u8	d15[4], r0
	mov	r0, r0, lsl #1
	vmov.u8	d15[5], r0
	mov	r0, r0, lsl #1
	vmov.u8	d15[6], r0
	mov	r0, r0, lsl #1
	vmov.u8	d15[7], r0
 ENDIF

TC16161_lineloop
	PRELOADLINE16bpp	dstptr, dstwidth
	PRELOADLINE16bpp	srcptr, dstwidth
	PRELOADLINE1bpp		mskptr, dstwidth

	movs	r12, r12		; are there any pixels to skip
	beq	TC16161_groupentry	; if not, do groups of 8

TC16161_pre

	ldrb	r0, [mskptr], #1
 IF REVERSESUBBYTEPIXELBITS
	mov	r1, #1
	mov	r1, r1, lsl r12
 ELSE
	mov	r1, #128		; set mask to leftmost pixel
	mov	r1, r1, lsr r12		; move mask to skip pixels
 ENDIF
TC16161_preloop
 	tst	r0, r1			; check bit
 	ldrneh	r2, [srcptr]		; load source if bit set
 	strneh	r2, [dstptr]		; store source if bit set
 	subs	pels, pels, #1
 	add	dstptr, dstptr, #2
 	add	srcptr, srcptr, #2
 	beq	TC16161_linedone
 IF REVERSESUBBYTEPIXELBITS
 	mov	r1, r1, lsl #1
 	cmp	r1, #256
 ELSE
 	movs	r1, r1, lsr #1
 ENDIF
 	bne	TC16161_preloop

TC16161_groupentry
	subs	pels, dstwidth, #8
	blt	TC16161_post
	
TC16161_grouploop
	ldrb	r0, [mskptr], #1	; load mask byte
	vld1.16	{d0,d1}, [dstptr]	; load 8 32-bit pixels from destination
	vld1.16	{d2,d3}, [srcptr]!	; load 8 32-bit pixels from source
	MaskCopy16161	q2, r0, q0, q1, d6
	vst1.16	{d4,d5}, [dstptr]!
	subs	pels, pels, #8
	bge	TC16161_grouploop

TC16161_post
	adds	pels, pels, #8
	ble	TC16161_linedone
	
	ldrb	r0, [mskptr], #1
 IF REVERSESUBBYTEPIXELBITS
	mov	r1, #128
	mov	r1, r1, lsr pels
 ELSE
	mov	r1, #1			; set mask to leftmost pixel
	mov	r1, r1, lsl pels	; move mask to skip pixels
 ENDIF
TC16161_postloop
 	tst	r0, r1			; check bit
 	ldrneh	r2, [srcptr]		; load source if bit set
 	strneh	r2, [dstptr]		; store source if bit set
 	add	dstptr, dstptr, #2
 	add	srcptr, srcptr, #2
 IF REVERSESUBBYTEPIXELBITS
 	mov	r1, r1, lsl #1
 ELSE
 	mov	r1, r1, lsr #1
 ENDIF
 	subs	pels, pels, #1
 	bne	TC16161_postloop

TC16161_linedone
	add	srcptr, srcptr, srcstep
	add	dstptr, dstptr, dststep
	add	mskptr, mskptr, mskstep
	subs	dstheight, dstheight, #1
	bgt	TC16161_lineloop

TC16161_done
	ldmfd	sp!, {r4-r12,pc}

	END

