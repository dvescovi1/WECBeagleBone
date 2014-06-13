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
;   File:  omap_optbltasm_copy.s
;
;   Revision:  April 27, 2009
;              + Export BlockCopyBytes for opaque blends
;              + Added UYVY to RGB16
;              + Added UYVY to BGR24
;              + Working on UYVY to BGRA32
;              + Working on BGR24 to UYVY
;
;   Revision:  March 31, 2009
;

	AREA	omap_optbltasm,CODE,READONLY

	EXTERN	PreloadLine
	
	EXPORT	BlockCopyBytes	; Not for use from C code!

	EXPORT	BlockCopy8
	EXPORT	BlockCopy16
	EXPORT	BlockCopy24
	EXPORT	BlockCopy32
	
	EXPORT	BlockCopyLUT8to16
	EXPORT	BlockCopyLUT8to24
	EXPORT	BlockCopyLUT8to32
	
	EXPORT	BlockCopyRGB16toBGR24
	EXPORT	BlockCopyRGB16toBGRx32

	EXPORT	BlockCopyBGR24toRGB16
	EXPORT	BlockCopyXYZ24toXYZx32
	EXPORT	BlockCopyBGR24toUYVY
	
	EXPORT	BlockCopyBGRx32toRGB16
	EXPORT	BlockCopyXYZx32toXYZ24

	EXPORT	BlockCopyUYVYtoRGB16
	EXPORT	BlockCopyUYVYtoBGR24
	EXPORT	BlockCopyUYVYtoBGRx32

	EXPORT	BlockCopyIYUVtoRGB16
	EXPORT	BlockCopyIYUVtoUYVY

	INCLUDE	omap_optbltasm.inc
	
;----------------------------------------------------------------------------
; ASSUMPTIONS
;----------------------------------------------------------------------------
; All destination memory has write combining enabled.  This mitigates the
;  need to align writes.
;
; Sources that are not the same format as the destination are cached.  This
;  mitigates the need to align reads when the source and destination don't
;  match.
;
; Sources that are the same format as the destination might not be cached.
;  The byte copy routine below aligns the source.
;

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
dststride	rn	dststep
srcptr	rn	r6
srcstep	rn	r7
srcstride	rn	srcstep
dstwidth	rn	r10
dstheight	rn	r11
dstline	rn	r8
srcline	rn	r9
lut	rn	r12
pels	rn	lr

;----------------------------------------------------------------------------
; BlockCopyBytes copies an 8-bit rectangle of bytes from one surface to
;  another.  It is not used directly from an external app, but is called by
;  the block copy routines below.
;
; On entry, this functiona assumes:
; r4 = dstptr
; r5 = dststep
; r6 = srcptr
; r7 = srcstep
; r10 = dstwidth in bytes
; r11 = dstheight in lines
; And that stmfd sp!, {r4-r12,lr} was issued on entry
;
; This routine is written to use the maximum transfer size allowed in case
; the source is not cached.  The destination is normally has write combining
; enabled, so performance does not rely on destination alignment.
;
; (Note entry point is in the middle of the function.)
;
BCB_pre
	rsb	r0, r0, #64		; prebytes = bytes to get aligned
	cmp	dstwidth, r0		; if(dstwidth in bytes < bytes to get aligned)
	movlt	r0, dstwidth		;	prebytes = dstwidth in bytes

	rsb	r1, srcptr, #64		; alignment flags
BCB_pre1
									;   1   2   3   4   5   6   7   8   9  10  11
	tst	r1, #1			; see if lsb is set
	beq	BCB_pre2		; if so...
	ldrb	r2, [srcptr], #1	; ...copy one...
	strb	r2, [dstptr], #1	; ...byte and...
	subs	r12, r12, #1		; ...decrement byte counter	;  (0) (1) (2) (3) (4) (5) (6) (7) (8) (9)(10)
	beq	BCB_linedone
	sub	r0, r0, #1						;  -2  -1   0   1   2   3   4   5   6   7   8
BCB_pre2
	cmp	r0, #2
	blt	BCB_groups
	tst	r1, #2
	beq	BCB_pre4
	vld2.8	{d0[0],d1[0]}, [srcptr]!
	vst2.8	{d0[0],d1[0]}, [dstptr]!	; Neon used in case dest isn't aligned
	subs	r12, r12, #2						;   -   -  (0) (1) (2) (3) (4) (5) (6) (7) (8)
	beq	BCB_linedone
	subs	r0, r0, #2						;   -   -  -4  -3  -2  -1   0   1   2   3   4
BCB_pre4
	cmp	r0, #4
	blt	BCB_groups
	tst	r1, #4
	beq	BCB_pre8
	vld4.8	{d0[0],d1[0],d2[0],d3[0]}, [srcptr]!
	vst4.8	{d0[0],d1[0],d2[0],d3[0]}, [dstptr]!		; Neon used in case dest isn't aligned
	subs	r12, r12, #4
	beq	BCB_linedone
	subs	r0, r0, #4
BCB_pre8
	cmp	r0, #8
	blt	BCB_groups
	tst	r1, #8
	beq	BCB_pre16
	vld1.8	{d0}, [srcptr@64]!
	vst1.8	{d0}, [dstptr]!			; Neon used in case dest isn't aligned
	subs	r12, r12, #8
	beq	BCB_linedone
	subs	r0, r0, #8
BCB_pre16
	cmp	r0, #16
	blt	BCB_groups
	tst	r1, #16
	beq	BCB_pre32
	vld2.8	{d0,d1}, [srcptr@128]!
	vst2.8	{d0,d1}, [dstptr]!		; Neon used in case dest isn't aligned
	subs	r12, r12, #16
	beq	BCB_linedone
	subs	r0, r0, #16
BCB_pre32
	cmp	r0, #32
	blt	BCB_groups
	tst	r1, #32
	beq	BCB_groups
	vld4.8	{d0,d1,d2,d3}, [srcptr@128]!
	vst4.8	{d0,d1,d2,d3}, [dstptr]!	; Neon used in case dest isn't aligned
	subs	r12, r12, #32
	beq	BCB_linedone
	b	BCB_groups
	
BlockCopyBytes
BCB_lineloop
	mov	r0, srcptr
	mov	r1, dstwidth
	bl	PreloadLine
	
	mov	r12, dstwidth

	ands	r0, srcptr, #63
	bne	BCB_pre

BCB_groups
	movs	r1, r12, lsr #6
	beq	BCB_post

BCB_grouploop
	vld4.8	{d0,d1,d2,d3}, [srcptr@128]!	; vld4.8 {q0,q1,q2,q3}, [src@256]!
	vld4.8	{d4,d5,d6,d7}, [srcptr@128]!
	vst4.8	{d0,d1,d2,d3}, [dstptr]!	; vst4.8 {q0,q1,q2,q3}, [dst]!
	vst4.8	{d4,d5,d6,d7}, [dstptr]!

	subs	r1, r1, #1
	bne	BCB_grouploop

BCB_post
BCB_post32
	movs	r12, r12, lsl #(32-6)
	beq	BCB_linedone
	bpl	BCB_post16
	vld4.8	{d0,d1,d2,d3}, [srcptr@128]!
	vst4.8	{d0,d1,d2,d3}, [dstptr]!	; 8-bit Neon used in case dest isn't aligned
BCB_post16
	movs	r12, r12, lsl #1
	beq	BCB_linedone
	bpl	BCB_post8
	vld2.8	{d0,d1}, [srcptr@128]!
	vst2.8	{d0,d1}, [dstptr]!		; 8-bit Neon used in case dest isn't aligned
BCB_post8
	movs	r12, r12, lsl #1
	beq	BCB_linedone
	bpl	BCB_post4
	vld1.8	{d0}, [srcptr@64]!
	vst1.8	{d0}, [dstptr]!			; 8-bit Neon used in case dest isn't aligned
BCB_post4
	movs	r12, r12, lsl #1
	beq	BCB_linedone
	bpl	BCB_post2
	vld4.8	{d0[0],d1[0],d2[0],d3[0]}, [srcptr]!
	vst4.8	{d0[0],d1[0],d2[0],d3[0]}, [dstptr]!	; 8-bit Neon used in case dest isn't aligned
BCB_post2
	movs	r12, r12, lsl #1
	beq	BCB_linedone
	bpl	BCB_post1
	vld2.8	{d0[0],d1[0]}, [srcptr]!
	vst2.8	{d0[0],d1[0]}, [dstptr]!	; 8-bit Neon used in case dest isn't aligned
BCB_post1
	movs	r12, r12, lsl #1
	ldrmib	r1, [srcptr], #1
	strmib	r1, [dstptr], #1

BCB_linedone
	add	srcptr, srcptr, srcstep
	subs	dstheight, dstheight, #1
	add	dstptr, dstptr, dststep
	bne	BCB_lineloop

	ldmfd	sp!, {r4-r12,pc}

;----------------------------------------------------------------------------
; void BlockCopy8(void*       dstptr,
;                 int32       dststride,
;                 uint32      dstleft,
;                 uint32      dsttop,
;                 void const* srcptr,
;                 int32       srcstride,
;                 uint32      srcleft,
;                 uint32      srctop,
;                 uint32      dstwidth,
;                 uint32      dstheight)
;
; Performs a block copy of a rectangle of 8-bit pixels from one surface to
;  another.
;
BlockCopy8
	stmfd	sp!, {r4-r12,lr}
	TWOSURFINIT	8, 8, 10
	b	BlockCopyBytes

;----------------------------------------------------------------------------
; void BlockCopy16(void*       dstptr,
;                  int32       dststride,
;                  uint32      dstleft,
;                  uint32      dsttop,
;                  void const* srcptr,
;                  int32       srcstride,
;                  uint32      srcleft,
;                  uint32      srctop,
;                  uint32      dstwidth,
;                  uint32      dstheight)
;
; Performs a block copy of a rectangle of 16-bit pixels from one surface to
;  another.
;
BlockCopy16
	stmfd	sp!, {r4-r12,lr}
	TWOSURFINIT	16, 16, 10
	add	dstwidth, dstwidth, dstwidth
	b	BlockCopyBytes

;----------------------------------------------------------------------------
; void BlockCopy24(void*       dstptr,
;                  int32       dststride,
;                  uint32      dstleft,
;                  uint32      dsttop,
;                  void const* srcptr,
;                  int32       srcstride,
;                  uint32      srcleft,
;                  uint32      srctop,
;                  uint32      dstwidth,
;                  uint32      dstheight)
;
; Performs a block copy of a rectangle of 24-bit pixels from one surface to
;  another.
;
BlockCopy24
	stmfd	sp!, {r4-r12,lr}
	TWOSURFINIT	24,24, 10
	add	dstwidth, dstwidth, dstwidth, lsl #1
	b	BlockCopyBytes

;----------------------------------------------------------------------------
; void BlockCopy32(void*       dstptr,
;                  int32       dststride,
;                  uint32      dstleft,
;                  uint32      dsttop,
;                  void const* srcptr,
;                  int32       srcstride,
;                  uint32      srcleft,
;                  uint32      srctop,
;                  uint32      dstwidth,
;                  uint32      dstheight)
;
; Performs a block copy of a rectangle of 32-bit pixels from one surface to
;  another.
;
BlockCopy32
	stmfd	sp!, {r4-r12,lr}
	TWOSURFINIT	32,32, 10
	mov	dstwidth, dstwidth, lsl #2
	b	BlockCopyBytes

;----------------------------------------------------------------------------
; void BlockCopyLUT8to16(void*         dstptr,
;                        int32         dststride,
;                        uint32        dstleft,
;                        uint32        dsttop,
;                        uint32        dstwidth,
;                        uint32        dstheight,
;                        void const*   srcptr,
;                        int32         srcstride,
;                        uint32        srcleft,
;                        uint32        srctop,
;                        uint32 const* lut)
;
; Utilizes a lookup table to convert an 8-bit rectangle from one surface to a
;  16-bit destination surface.
;
; Note that the table is assumed to contain the 16 bit values, but on 32 bit
;  boundary.  This saves one cycle for each of the reads into the cycle, due
;  to an optimization of the ARM for "lsl #2" modifications that doesn't
;  exist for any other shift.
;
BlockCopyLUT8to16
	stmfd	sp!, {r4-r12,lr}
	TWOSURFINIT	16, 8, 10, nostep
	ldr	lut, [sp, #((10 + 6) * 4)]

	mov	srcline, srcptr
	mov	dstline, dstptr

BC816_lineloop
	PRELOADLINE8bpp	srcline, dstwidth

	mov	pels, dstwidth
	mov	srcptr, srcline
	mov	dstptr, dstline

BC816_pelloop
; r0 = idx0, val0
; r1 = idx1, val1
; r2 = idx2, val2
; interleaved three pixels, twice unrolled loop
					; 1  2  3  4  5  6  7
	subs	pels, pels, #2		;-1  0  1  2  3  4  5
	ldrgeb	r1, [srcptr, #1]	; n  y  y  y  y  y  y
	ldrgtb	r2, [srcptr, #2]	; n  n  y  y  y  y  y
	ldrb	r0, [srcptr], #3	; y  y  y  y  y  y  y
	ldrge	r1, [lut, r1, lsl #2]	; n  y  y  y  y  y  y
	ldrgt	r2, [lut, r2, lsl #2]	; n  n  y  y  y  y  y
	ldr	r0, [lut, r0, lsl #2]	; y  y  y  y  y  y  y
	strgeh	r1, [dstptr, #2]	; n  y  y  y  y  y  y
	strgth	r2, [dstptr, #4]	; n  n  y  y  y  y  y
	strh	r0, [dstptr], #6	; y  y  y  y  y  y  y
	subs	pels, pels, #1		;-2 -1  0  1  2  3  4

	ble	BC816_linedone

	subs	pels, pels, #2		; x  x  x -1  0  1  2
	ldrgeb	r1, [srcptr, #1]	; n  y  y  y  y  y  y
	ldrgtb	r2, [srcptr, #2]	; n  n  y  y  y  y  y
	ldrb	r0, [srcptr], #3	; y  y  y  y  y  y  y
	ldrge	r1, [lut, r1, lsl #2]	; n  y  y  y  y  y  y
	ldrgt	r2, [lut, r2, lsl #2]	; n  n  y  y  y  y  y
	ldr	r0, [lut, r0, lsl #2]	; y  y  y  y  y  y  y
	strgeh	r1, [dstptr, #2]	; n  y  y  y  y  y  y
	strgth	r2, [dstptr, #4]	; n  n  y  y  y  y  y
	strh	r0, [dstptr], #6	; y  y  y  y  y  y  y
	subs	pels, pels, #1		;-3 -2 -1 -2 -1  0  1

	bgt	BC816_pelloop

BC816_linedone
	add	srcline, srcline, srcstride
	add	dstline, dstline, dststride
	subs	dstheight, dstheight, #1
	bne	BC816_lineloop

	ldmfd	sp!, {r4-r12,pc}

;----------------------------------------------------------------------------
; void BlockCopyLUT8to24(void*         dstptr,
;                        int32         dststride,
;                        uint32        dstleft,
;                        uint32        dsttop,
;                        uint32        dstwidth,
;                        uint32        dstheight,
;                        void const*   srcptr,
;                        int32         srcstride,
;                        uint32        srcleft,
;                        uint32        srctop,
;                        uint32 const* lut)
;
; Utilizes a lookup table to convert an 8-bit rectangle from one surface to a
;  24-bit destination surface.
;
; Note that the table is assumed to contain the 24 bit values, but on 32 bit
;  boundary.
;
BlockCopyLUT8to24
	stmfd	sp!, {r4-r12,lr}
	TWOSURFINIT	24, 8, 10
	ldr	lut, [sp, #((10 + 6) * 4)]

BC824_lineloop
	PRELOADLINE8bpp	srcline, dstwidth
	subs	pels, dstwidth, #8
	blt	BC824_singles

BC824_grouploop
	ldrb	r0, [srcptr, #7]
	ldrb	r1, [srcptr, #6]
	add	r0, lut, r0, lsl #2
	add	r1, lut, r1, lsl #2
	vld3.8	{d0[7],d1[7],d2[7]}, [r0]
	vld3.8	{d0[6],d1[6],d2[6]}, [r1]
	ldrb	r0, [srcptr, #5]
	ldrb	r1, [srcptr, #4]
	add	r0, lut, r0, lsl #2
	add	r1, lut, r1, lsl #2
	vld3.8	{d0[5],d1[5],d2[5]}, [r0]
	vld3.8	{d0[4],d1[4],d2[4]}, [r1]
	ldrb	r0, [srcptr, #3]
	ldrb	r1, [srcptr, #2]
	add	r0, lut, r0, lsl #2
	add	r1, lut, r1, lsl #2
	vld3.8	{d0[3],d1[3],d2[3]}, [r0]
	vld3.8	{d0[2],d1[2],d2[2]}, [r1]
	ldrb	r0, [srcptr, #1]
	ldrb	r1, [srcptr], #8
	add	r0, lut, r0, lsl #2
	add	r1, lut, r1, lsl #2
	vld3.8	{d0[1],d1[1],d2[1]}, [r0]
	vld3.8	{d0[0],d1[0],d2[0]}, [r1]
	subs	pels, pels, #8
	vst3.8	{d0,d1,d2}, [dstptr]!
	bge	BC824_grouploop
	
BC824_singles
	adds	pels, pels, #8
	ble	BC824_linedone

BC824_singleloop
	ldrb	r0, [srcptr], #1
	add	r1, lut, r0, lsl #2
	vld3.8	{d0[0],d1[0],d2[0]}, [r1]
	vst3.8	{d0[0],d1[0],d2[0]}, [dstptr]!
	subs	pels, pels, #1
	bgt	BC824_singleloop
	
BC824_linedone
	add	srcptr, srcptr, srcstep
	add	dstptr, dstptr, dststep
	subs	dstheight, dstheight, #1
	bne	BC824_lineloop

	ldmfd	sp!, {r4-r12,pc}

;----------------------------------------------------------------------------
; void BlockCopyLUT8to32(void*         dstptr,
;                        int32         dststride,
;                        uint32        dstleft,
;                        uint32        dsttop,
;                        uint32        dstwidth,
;                        uint32        dstheight,
;                        void const*   srcptr,
;                        int32         srcstride,
;                        uint32        srcleft,
;                        uint32        srctop,
;                        uint32 const* lut)
;
; Utilizes a lookup table to convert an 8-bit rectangle from one surface to a
;  32-bit destination surface.
;
BlockCopyLUT8to32
	stmfd	sp!, {r4-r12,lr}
	TWOSURFINIT 32, 8, 10, nostep
	ldr	lut, [sp, #((10 + 6) * 4)]

	mov	srcline, srcptr
	mov	dstline, dstptr

BC832_lineloop
	PRELOADLINE8bpp	srcline, dstwidth

	mov	pels, dstwidth
	mov	srcptr, srcline
	mov	dstptr, dstline

BC832_pelloop
; r0 = idx0, val0
; r1 = idx1, val1
; r2 = idx2, val2
; interleaved three pixels, twice unrolled loop
					; 1  2  3  4  5  6  7
	subs	pels, pels, #2		;-1  0  1  2  3  4  5
	ldrgeb	r1, [srcptr, #1]	; n  y  y  y  y  y  y
	ldrgtb	r2, [srcptr, #2]	; n  n  y  y  y  y  y
	ldrb	r0, [srcptr], #3	; y  y  y  y  y  y  y
	ldrge	r1, [lut, r1, lsl #2]	; n  y  y  y  y  y  y
	ldrgt	r2, [lut, r2, lsl #2]	; n  n  y  y  y  y  y
	ldr	r0, [lut, r0, lsl #2]	; y  y  y  y  y  y  y
	strge	r1, [dstptr, #4]	; n  y  y  y  y  y  y
	strgt	r2, [dstptr, #8]	; n  n  y  y  y  y  y
	str	r0, [dstptr], #12	; y  y  y  y  y  y  y
	subs	pels, pels, #1		;-2 -1  0  1  2  3  4

	ble	BC832_linedone

	subs	pels, pels, #2		; x  x  x -1  0  1  2
	ldrgeb	r1, [srcptr, #1]	; n  y  y  y  y  y  y
	ldrgtb	r2, [srcptr, #2]	; n  n  y  y  y  y  y
	ldrb	r0, [srcptr], #3	; y  y  y  y  y  y  y
	ldrge	r1, [lut, r1, lsl #2]	; n  y  y  y  y  y  y
	ldrgt	r2, [lut, r2, lsl #2]	; n  n  y  y  y  y  y
	ldr	r0, [lut, r0, lsl #2]	; y  y  y  y  y  y  y
	strge	r1, [dstptr, #4]	; n  y  y  y  y  y  y
	strgt	r2, [dstptr, #8]	; n  n  y  y  y  y  y
	str	r0, [dstptr], #12	; y  y  y  y  y  y  y
	subs	pels, pels, #1		;-3 -2 -1 -2 -1  0  1

	bgt	BC832_pelloop

BC832_linedone
	add	srcline, srcline, srcstride
	add	dstline, dstline, dststride
	subs	dstheight, dstheight, #1
	bne	BC832_lineloop

	ldmfd	sp!, {r4-r12,pc}

;----------------------------------------------------------------------------
; void BlockCopyRGB16toBGR24(void*       dstptr,
;                            int32       dststride,
;                            uint32      dstleft,
;                            uint32      dsttop,
;                            uint32      dstwidth,
;                            uint32      dstheight,
;                            void const* srcptr,
;                            int32       srcstride,
;                            uint32      srcleft,
;                            uint32      srctop)
;
; Converts a 16-bit (RGB 5:6:5) rectangle from one surface to a reverse order
;  (BGR byte order) 24 bit packed destination surface.
;
BlockCopyRGB16toBGR24
	stmfd	sp!, {r4-r12,lr}
	TWOSURFINIT	24, 16, 10

BC16r24_lineloop
	PRELOADLINE16bpp	srcptr, dstwidth
	
	subs	pels, dstwidth, #8
	blt	BC16r24_singles

BC16r24_octetloop
	vld1.16	{d0,d1}, [srcptr]!		; vld1.16 {q0}, [srcptr]!
	RGB565toRGB888	d4, d3, d2, q0
	vst3.8	{d2,d3,d4}, [dstptr]!
	subs	pels, pels, #8
	bge	BC16r24_octetloop

BC16r24_singles
	adds	pels, pels, #8
	beq	BC16r24_linedone

	LOADSTRAYS_16	pels, srcptr, d0, d1
	RGB565toRGB888	d4, d3, d2, q0
	STORESTRAYS_3x8	pels, dstptr, d2, d3, d4

BC16r24_linedone
	add	dstptr, dstptr, dststep
	add	srcptr, srcptr, srcstep
	subs	dstheight, dstheight, #1
	bgt	BC16r24_lineloop

	ldmfd	sp!, {r4-r12,pc}

;----------------------------------------------------------------------------
; void BlockCopyRGB16toBGRx32(void*       dstptr,
;                             int32       dststride,
;                             uint32      dstleft,
;                             uint32      dsttop,
;                             uint32      dstwidth,
;                             uint32      dstheight,
;                             void const* srcptr,
;                             int32       srcstride,
;                             uint32      srcleft,
;                             uint32      srctop)
;
; Converts a 16-bit (RGB 5:6:5) rectangle from one surface to a reverse order
;  (BGRx byte order) 24 bit unpacked (32 bits including padding byte)
;  destination surface.  The extra byte is filled in with the alpha specified
;  in the VMOV_U8_ALPHA macro above.
;
BlockCopyRGB16toBGRx32
	stmfd	sp!, {r4-r12,lr}
	TWOSURFINIT	32, 16, 10

	VMOV_U8_ALPHA	d5

BC16r32_lineloop
	PRELOADLINE16bpp	srcptr, dstwidth
	
	subs	pels, dstwidth, #8
	blt	BC16r32_singles

BC16r32_octetloop
	vld1.16	{d0,d1}, [srcptr]!		; vld1.16 {q0}, [srcptr]!
	RGB565toRGB888	d4, d3, d2, q0
	vst4.8	{d2,d3,d4,d5}, [dstptr]!
	subs	pels, pels, #8
	bge	BC16r32_octetloop

BC16r32_singles
	adds	pels, pels, #8
	beq	BC16r32_linedone

	LOADSTRAYS_16	pels, srcptr, d0, d1
	RGB565toRGB888	d4, d3, d2, q0
	STORESTRAYS_4x8	pels, dstptr, d2, d3, d4, d5

BC16r32_linedone
	add	dstptr, dstptr, dststep
	add	srcptr, srcptr, srcstep
	subs	dstheight, dstheight, #1
	bgt	BC16r32_lineloop

	ldmfd	sp!, {r4-r12,pc}
 
;----------------------------------------------------------------------------
; void BlockCopyBGR24toRGB16(void*       dstptr,
;                            int32       dststride,
;                            uint32      dstleft,
;                            uint32      dsttop,
;                            uint32      dstwidth,
;                            uint32      dstheight,
;                            void const* srcptr,
;                            int32       srcstride,
;                            uint32      srcleft,
;                            uint32      srctop)
;
; Converts a reverse order (BGR byte order) 24 bit packed rectangle from one
;  surface to an RGB 5:6:5 destination surface.
;
BlockCopyBGR24toRGB16
	stmfd	sp!, {r4-r12,lr}
	TWOSURFINIT	16, 24, 10

BCr2416_lineloop
	PRELOADLINE24bpp	srcptr, dstwidth
	
	subs	pels, dstwidth, #8
	blt	BCr2416_singles

BCr2416_octetloop
	vld3.8	{d0,d1,d2}, [srcptr]!
	RGB888toRGB565	q2, d2, d1, d0, q3, q4
	vst1.16	{d4,d5}, [dstptr]!		; vst1.16 {q2}, [dstptr]!
	subs	pels, pels, #8
	bge	BCr2416_octetloop

BCr2416_singles
	adds	pels, pels, #8
	beq	BCr2416_linedone

	LOADSTRAYS_3x8	pels, srcptr, d0, d1, d2
	RGB888toRGB565	q2, d2, d1, d0, q3, q4
	STORESTRAYS_16	pels, dstptr, d4, d5

BCr2416_linedone
	add	dstptr, dstptr, dststep
	add	srcptr, srcptr, srcstep
	subs	dstheight, dstheight, #1
	bgt	BCr2416_lineloop

	ldmfd	sp!, {r4-r12,pc}
 
;----------------------------------------------------------------------------
; void BlockCopyXYZ24toXYZx32(void*       dstptr,
;                             int32       dststride,
;                             uint32      dstleft,
;                             uint32      dsttop,
;                             uint32      dstwidth,
;                             uint32      dstheight,
;                             void const* srcptr,
;                             int32       srcstride,
;                             uint32      srcleft,
;                             uint32      srctop)
;
; Converts a 24-bit packed RGB or BGR rectangle from one surface to a RGBx or
;  BGRx (respectively) in a destination surface.
;
BlockCopyXYZ24toXYZx32
	stmfd	sp!, {r4-r12,lr}
	TWOSURFINIT	32, 24, 10

	VMOV_U8_ALPHA	d3

BC2432_lineloop
	PRELOADLINE24bpp	srcptr, dstwidth
	
	subs	pels, dstwidth, #8
	blt	BC2432_singles

BC2432_octetloop
	vld3.8	{d0,d1,d2}, [srcptr]!
	vst4.8	{d0,d1,d2,d3}, [dstptr]!
	subs	pels, pels, #8
	bge	BC2432_octetloop

BC2432_singles
	adds	pels, pels, #8
	beq	BC2432_linedone

	LOADSTRAYS_3x8	pels, srcptr, d0, d1, d2
	STORESTRAYS_4x8	pels, dstptr, d0, d1, d2, d3

BC2432_linedone
	add	dstptr, dstptr, dststep
	add	srcptr, srcptr, srcstep
	subs	dstheight, dstheight, #1
	bgt	BC2432_lineloop

	ldmfd	sp!, {r4-r12,pc}
 
;----------------------------------------------------------------------------
; void BlockCopyBGRx32toRGB16(void*       dstptr,
;                             int32       dststride,
;                             uint32      dstleft,
;                             uint32      dsttop,
;                             uint32      dstwidth,
;                             uint32      dstheight,
;                             void const* srcptr,
;                             int32       srcstride,
;                             uint32      srcleft,
;                             uint32      srctop)
;
; Converts a reverse order (BGRx byte order) 24 bit unpacked (32 bits
;  including padding byte) rectangle from one surface to an RGB 5:6:5
;  destination surface.
;
BlockCopyBGRx32toRGB16
	stmfd	sp!, {r4-r12,lr}
	TWOSURFINIT	16, 32, 10

BCr3216_lineloop
	PRELOADLINE32bpp	srcptr, dstwidth
	
	subs	pels, dstwidth, #8
	blt	BCr3216_singles

BCr3216_octetloop
	vld4.8	{d0,d1,d2,d3}, [srcptr]!
	RGB888toRGB565	q2, d2, d1, d0, q3, q4
	vst1.16	{d4,d5}, [dstptr]!		; vst1.16 {q2}, [dstptr]!
	subs	pels, pels, #8
	bge	BCr3216_octetloop

BCr3216_singles
	adds	pels, pels, #8
	beq	BCr3216_linedone

	LOADSTRAYS_4x8	pels, srcptr, d0, d1, d2, d3
	RGB888toRGB565	q2, d2, d1, d0, q3, q4
	STORESTRAYS_16	pels, dstptr, d4, d5

BCr3216_linedone
	add	dstptr, dstptr, dststep
	add	srcptr, srcptr, srcstep
	subs	dstheight, dstheight, #1
	bgt	BCr3216_lineloop

	ldmfd	sp!, {r4-r12,pc}
 
;----------------------------------------------------------------------------
; void BlockCopyXYZx32toXYZ24(void*       dstptr,
;                             int32       dststride,
;                             uint32      dstleft,
;                             uint32      dsttop,
;                             uint32      dstwidth,
;                             uint32      dstheight,
;                             void const* srcptr,
;                             int32       srcstride,
;                             uint32      srcleft,
;                             uint32      srctop)
;
; Converts a 32-bit (RGBX or BGRx) rectangle from one surface to a packed
;  24-bit surface (RGB or BGR respectively).
;
BlockCopyXYZx32toXYZ24
	stmfd	sp!, {r4-r12,lr}
	TWOSURFINIT	24, 32, 10

BC3224_lineloop
	PRELOADLINE32bpp	srcptr, dstwidth
	
	subs	pels, dstwidth, #8
	blt	BC3224_singles

BC3224_octetloop
	vld4.8	{d0,d1,d2,d3}, [srcptr]!
	vst3.8	{d0,d1,d2}, [dstptr]!
	subs	pels, pels, #8
	bge	BC3224_octetloop

BC3224_singles
	adds	pels, pels, #8
	beq	BC3224_linedone

	LOADSTRAYS_4x8	pels, srcptr, d0, d1, d2, d3
	STORESTRAYS_3x8	pels, dstptr, d0, d1, d2

BC3224_linedone
	add	dstptr, dstptr, dststep
	add	srcptr, srcptr, srcstep
	subs	dstheight, dstheight, #1
	bgt	BC3224_lineloop

	ldmfd	sp!, {r4-r12,pc}

;----------------------------------------------------------------------------
; void BlockCopyRGB16toUYVY(void*       dstptr,
;                           int32       dststride,
;                           uint32      dstleft,
;                           uint32      dsttop,
;                           uint32      dstwidth,
;                           uint32      dstheight,
;                           void const* srcptr,
;                           int32       srcstride,
;                           uint32      srcleft,
;                           uint32      srctop)
;
; Copies a rectangle from a 16-bit RGB (5:6:5) surface to a UYVY surface.
;  Left edge of source rectangle must be a multiple of 2.  Width must be
;  multiple of 2.
;
BlockCopyRGB16toUYVY
	stmfd	sp!, {r4-r12,lr}
	TWOSURFINIT	16, 16, 10

	RGB888toYUV422_PREP

BC16UY_lineloop
;	PRELOADLINE16bpp	srcptr, dstwidth

	subs	pels, dstwidth, #16
	blt	BC16UY_singles

BC16UY_grouploop
	vld1.16	{d6,d7,d8,d9}, [srcptr]!	; 16 16-bit pixels
	vuzp.16	q3, q4				; even 8, odd 8
	RGB565toRGB888	d0,d2,d4,q3
	RGB565toRGB888	d1,d3,d5,q4
	RGB888toYUV422	d7,d6,d9,d8,d4,d5,d2,d3,d0,d1,q5,q6,q7,q8	; d6-d9 => u, even y, v, odd y
	subs	pels, pels, #16
	vst4.8	{d6,d7,d8,d9}, [dstptr]!	; u, even y, v, odd y
	bge	BC16UY_grouploop
	
BC16UY_singles
	adds	pels, pels, #16
	beq	BC16UY_linedone

	mov	r0, pels, lsr #1	; calc half of pixels
	LOADSTRAYS_16	r0, srcptr, d6, d7	; load half of remaining 16-bit pixels
	LOADSTRAYS_16	r0, srcptr, d8, d9	; load other half of remaining 16-bit pixels
	vuzp.16	q3, q4				; even pixels, odd pixels
	RGB565toRGB888	d0,d2,d4,q3
	RGB565toRGB888	d1,d3,d5,q4
	RGB888toYUV422	d7,d6,d9,d8,d4,d5,d2,d3,d0,d1,q5,q6,q7,q8	; d6-d9 => u, even y, v, odd y
	STORESTRAYS_4x8	pels, dstptr, d6, d7, d8, d9	; u, even y, v, odd y

BC16UY_linedone
	add	dstptr, dstptr, dststep
	add	srcptr, srcptr, srcstep
	subs	dstheight, dstheight, #1
	bgt	BC16UY_lineloop

	ldmfd	sp!, {r4-r12,pc}

;----------------------------------------------------------------------------
; void BlockCopyUYVYtoRGB16(void*       dstptr,
;                           int32       dststride,
;                           uint32      dstleft,
;                           uint32      dsttop,
;                           uint32      dstwidth,
;                           uint32      dstheight,
;                           void const* srcptr,
;                           int32       srcstride,
;                           uint32      srcleft,
;                           uint32      srctop)
;
; Copies a rectangle from a UYVY (YUV 4:2:2) surface to a 16-bit RGB (5:6:5)
;  surface to a UYVY surface.  Left edge of source rectangle must be a
;  multiple of 2.  Width must be multiple of 2.
;
BlockCopyUYVYtoRGB16
	stmfd	sp!, {r4-r12,lr}
	TWOSURFINIT	16, 16, 10

	YUV422toRGB888_PREP

BCUY16_lineloop
;	PRELOADLINE16bpp	srcptr, dstwidth

	subs	pels, dstwidth, #16
	blt	BCUY16_singles

BCUY16_grouploop
	vld4.8	{d0,d1,d2,d3}, [srcptr]!	; u, even y, v odd y
	YUV422toRGB888	d4,d5,d2,d3,d0,d1,d1,d0,d3,d2,q3,q4,q5,q6,q7,q8,q9	; q0 = r, q1 = g, q2 = b (interleaved)
	RGB888toRGB565	q3, d4, d2, d0, q4, q5	; q3 = rgb16 (interleaved)
	RGB888toRGB565	q4, d5, d3, d1, q5, q6	; q4 = rgb16 (interleaved)
	subs	pels, pels, #16
	vst2.16	{d6,d8}, [dstptr]!
	vst2.16	{d7,d9}, [dstptr]!
	bge	BCUY16_grouploop
	
BCUY16_singles
	adds	pels, pels, #16
	beq	BCUY16_linedone

	mov	r0, pels, lsr #1	; calc number of pixel pairs
	LOADSTRAYS_4x8	r0, srcptr, d0, d1, d2, d3	; load pixel pairs
	YUV422toRGB888	d4,d5,d2,d3,d0,d1,d1,d0,d3,d2,q3,q4,q5,q6,q7,q8,q9	; q0 = r, q1 = g, q2 = b (interleaved)
	RGB888toRGB565	q3, d4, d2, d0, q4, q5	; q3 = rgb16 (interleaved)
	RGB888toRGB565	q4, d5, d3, d1, q5, q6	; q4 = rgb16 (interleaved)
 	STORESTRAYS_16	r0, dstptr, d6, d8	; half 16-bit pixels
 	STORESTRAYS_16	r0, dstptr, d7, d9	; other half 16-bit pixels

BCUY16_linedone
	add	dstptr, dstptr, dststep
	add	srcptr, srcptr, srcstep
	subs	dstheight, dstheight, #1
	bgt	BCUY16_lineloop

	ldmfd	sp!, {r4-r12,pc}
	
;----------------------------------------------------------------------------
; void BlockCopyBGR24toUYVY(void*       dstptr,
;                           int32       dststride,
;                           uint32      dstleft,
;                           uint32      dsttop,
;                           uint32      dstwidth,
;                           uint32      dstheight,
;                           void const* srcptr,
;                           int32       srcstride,
;                           uint32      srcleft,
;                           uint32      srctop)
;
; Copies a rectangle from a packed 24-bit BGR surface to a UYVY surface.
;  Left edge of source rectangle must be a multiple of 2.  Width must be
;  multiple of 2.
;
BlockCopyBGR24toUYVY
	stmfd	sp!, {r4-r12,lr}
	TWOSURFINIT	16, 24, 10

	pld	[srcptr]
	pld	[srcptr, #64]
	pld	[srcptr, #128]
 
	RGB888toYUV422_PREP

BCr24UY_lineloop
	subs	pels, dstwidth, #16
	blt	BCr24UY_singles

BCr24UY_grouploop
	pld	[srcptr, #192]
	vld3.8	{d0,d2,d4}, [srcptr]!	; b, g, r
	vld3.8	{d1,d3,d5}, [srcptr]!	; b, g, r
	vuzp.8	d0, d1			; even b, odd b
	vuzp.8	d4, d5			; even r, odd r
	vuzp.8	d2, d3			; even g, odd g
	RGB888toYUV422	d7,d6,d9,d8,d4,d5,d2,d3,d0,d1,q5,q6,q7,q8	; d6-d9 => u, even y, v, odd y
	subs	pels, pels, #16
	vst4.8	{d6,d7,d8,d9}, [dstptr]!
	bge	BCr24UY_grouploop
	
BCr24UY_singles
	adds	pels, pels, #16
	beq	BCr24UY_linedone

	mov	pels, pels, lsr #1	; calc half of pixels
	LOADSTRAYS_3x8	pels, srcptr, d0, d1, d2	; load half of remaining pixels
	LOADSTRAYS_3x8	pels, srcptr, d3, d4, d5	; load other half of remaining pixels
    vswp    d1, d3
    vswp    d2, d4
    vswp    d2, d3
	vuzp.8	d0, d1			; even b, odd b
	vuzp.8	d4, d5			; even r, odd r
	vuzp.8	d2, d3			; even g, odd g
	RGB888toYUV422	d7,d6,d9,d8,d4,d5,d2,d3,d0,d1,q5,q6,q7,q8	; d6-d9 => u, even y, v, odd y
	STORESTRAYS_4x8	pels, dstptr, d6, d7, d8, d9

BCr24UY_linedone
	add	dstptr, dstptr, dststep
	add	srcptr, srcptr, srcstep
	subs	dstheight, dstheight, #1
	bgt	BCr24UY_lineloop

	ldmfd	sp!, {r4-r12,pc}

;----------------------------------------------------------------------------
; void BlockCopyUYVYtoBGR24(void*       dstptr,
;                           int32       dststride,
;                           uint32      dstleft,
;                           uint32      dsttop,
;                           uint32      dstwidth,
;                           uint32      dstheight,
;                           void const* srcptr,
;                           int32       srcstride,
;                           uint32      srcleft,
;                           uint32      srctop)
;
; Copies a rectangle from a UYVY surface to a packed 24-bit BGR surface.
;  Left edge of source rectangle must be a multiple of 2.  Width must be
;  multiple of 2.
;
BlockCopyUYVYtoBGR24
	stmfd	sp!, {r4-r12,lr}
	TWOSURFINIT	16, 24, 10

	YUV422toRGB888_PREP

BCUYr24_lineloop
;	PRELOADLINE16bpp	srcptr, dstwidth

	subs	pels, dstwidth, #16
	blt	BCUYr24_singles

BCUYr24_grouploop
	vld4.8	{d0,d1,d2,d3}, [srcptr]!	; u even y, v, odd y
	YUV422toRGB888	d4,d5,d2,d3,d0,d1,d1,d0,d3,d2,q3,q4,q5,q6,q7,q8,q9
	vuzp.8	d4, d5	; r
	vuzp.8	d2, d3	; g
	vuzp.8	d0, d1	; b
	subs	pels, pels, #16
	vst3.8		{d0,d2,d4}, [dstptr]!
	vst3.8		{d1,d3,d5}, [dstptr]!
	bge	BCUYr24_grouploop
	
BCUYr24_singles
	adds	pels, pels, #16
	beq	BCUYr24_linedone

	LOADSTRAYS_4x8	pels, srcptr, d0, d1, d2, d3	; u even y, v, odd y
	YUV422toRGB888	d4,d5,d2,d3,d0,d1,d1,d0,d3,d2,q3,q4,q5,q6,q7,q8,q9
	vuzp.8	d4, d5	; r
	vuzp.8	d2, d3	; g
	vuzp.8	d0, d1	; b
    vswp    d1, d3
    vswp    d1, d2
    vswp    d2, d4
	mov	r0, pels, lsr #1	; store half at a time to interleave
	STORESTRAYS_3x8	r0, dstptr, d0, d1, d2
	STORESTRAYS_3x8	r0, dstptr, d3, d4, d5

BCUYr24_linedone
	add	dstptr, dstptr, dststep
	add	srcptr, srcptr, srcstep
	subs	dstheight, dstheight, #1
	bgt	BCUYr24_lineloop

	ldmfd	sp!, {r4-r12,pc}


;----------------------------------------------------------------------------
; void BlockCopyUYVYtoBGRx32(void*       dstptr,
;                            int32       dststride,
;                            uint32      dstleft,
;                            uint32      dsttop,
;                            uint32      dstwidth,
;                            uint32      dstheight,
;                            void const* srcptr,
;                            int32       srcstride,
;                            uint32      srcleft,
;                            uint32      srctop)
;
; Copies a rectangle from a UYVY surface to an unpacked 32-bit BGRA surface.
;  Left edge of source rectangle must be a multiple of 2.  Width must be
;  multiple of 2.
;
BlockCopyUYVYtoBGRx32
	stmfd	sp!, {r4-r12,lr}
	TWOSURFINIT	16, 32, 10

	YUV422toRGB888_PREP

BCUYr32_lineloop
;	PRELOADLINE16bpp	srcptr, dstwidth

	subs	pels, dstwidth, #16
	blt	BCUYr32_singles

BCUYr32_grouploop
	vld4.8	{d0,d1,d2,d3}, [srcptr]!	; u even y, v, odd y
	YUV422toRGB888	d4,d5,d2,d3,d0,d1,d1,d0,d3,d2,q3,q4,q5,q6,q7,q8,q9
	vuzp.8	d4, d5	; r
	vuzp.8	d2, d3	; g
	vuzp.8	d0, d1	; b
	vmov.i8	q3, #0xFF
	subs	pels, pels, #16
	vst4.8		{d0,d2,d4,d6}, [dstptr]!
	vst4.8		{d1,d3,d5,d7}, [dstptr]!
	bge	BCUYr32_grouploop
	
BCUYr32_singles
	adds	pels, pels, #16
	beq	BCUYr32_linedone

	LOADSTRAYS_4x8	pels, srcptr, d0, d1, d2, d3	; u even y, v, odd y
	YUV422toRGB888	d4,d5,d2,d3,d0,d1,d1,d0,d3,d2,q3,q4,q5,q6,q7,q8,q9
	vuzp.8	d4, d5	; r
	vuzp.8	d2, d3	; g
	vuzp.8	d0, d1	; b
	vmov.i8	q3, #0xFF
    vswp    d1, d4
    vswp    d1, d2
    vswp    d3, d6
    vswp    d5, d6
	mov	r0, pels, lsr #1	; store half at a time to interleave
	STORESTRAYS_4x8	r0, dstptr, d0, d1, d2, d3
	STORESTRAYS_4x8	r0, dstptr, d4, d5, d6, d7

BCUYr32_linedone
	add	dstptr, dstptr, dststep
	add	srcptr, srcptr, srcstep
	subs	dstheight, dstheight, #1
	bgt	BCUYr32_lineloop

	ldmfd	sp!, {r4-r12,pc}

;
; void BlockCopyIYUVtoUYVY(void*       dstptr,
;                          int32       dststride,
;                          uint32      dstleft,
;                          uint32      dsttop,
;                          uint32      dstwidth,	0
;                          uint32      dstheight,	1
;                          void const* srcptr,		2
;                          int32       srcstride,	3
;                          uint32      srcleft,		4
;                          uint32      srctop,		5
;                          uint32      srcsurfheight)	6
;
BlockCopyIYUVtoUYVY
	stmfd	sp!, {r4-r12,lr}
; Get UYVY destination surface working variables
	TWOSURFINIT	16, 8, 10		; dstptr, dststep, srcptr (yptr), srcstep (ystep), dstwidth, dstheight
; Now get YUV 4:2:0 source surface working variables
; Create generic variables assuming IYUV layout
; calculate u & v left/top offset
	ldr	r0, [sp, #((10 + 4) * 4)]	; srcleft
	ldr	r1, [sp, #((10 + 5) * 4)]	; srctop
	ldr	r2, [sp, #((10 + 3) * 4)]	; srcstride
	mov	r0, r0, lsr #1			; uvleft = srcleft / 2
	mov	r1, r1, lsr #1			; uvtop = srctop / 2
	mov	r2, r2, asr #1			; uvstride = ystride / 2
	mla	r0, r1, r2, r0			; uvoffset [r0] = uvleft + (uvtop * uvstride)

; calculate size of uv planes
	ldr	r1, [sp, #((10 + 3) * 4)]	; srcstride (ystride)
	ldr	r2, [sp, #((10 + 6) * 4)]	; srcsurfheight
	mov	r1, r1, asr #1			; uvstride = ystride / 2
	mov	r2, r2, lsr #1			; uvheight = srcsurfheight / 2
	mul	r1, r2, r1			; uvsize [r1] = uvheight * uvstride

; calculate u & v ptrs
	ldr	r8, [sp, #((10 + 2) * 4)]	; srcptr (yptr)
	ldr	r3, [sp, #((10 + 3) * 4)]	; srcstride (ystride)
	ldr	r2, [sp, #((10 + 6) * 4)]	; srcsurfheight
	mla	r8, r2, r3, r8			; r8 = yptr + (srcsurfheight * ystride)
	add	r8, r8, r0			; uptr = r8 + uvoffset
	add	r9, r8, r1			; vptr = uptr + uvsize

; calculate u & v step (two lines is one u or v line)
	ldr	r0, [sp, #((10 + 3) * 4)]	; srcstride (ystride)
	mov	r12, r0, asr #1			; uvstride = ystride / 2
	sub	r12, r12, dstwidth lsr #1	; uvstep = uvstride - (srcwidth / 2)

; calculate the y step (for two lines)
	ldr	r0, [sp, #((10 + 3) * 4)]	; srcstride (ystride)
	rsb	srcstep, dstwidth, r0, asl #1	; srcstep = (ystride + ystride) - srcwidth

; recalculate the destination step (for two lines)
	add	r0, dststep, dstwidth, lsl #1	; dststride = dststep + (2 x dstwidth)
	sub	dststep, r0, dstwidth
	mov	dststep, dststep, lsl #1	; dststep = (2 x dststride) - (2 x dstwidth)

; get the ystride
	ldr	r1, [sp, #((10 + 3) * 4)]	; srcstride (ystride)

	pld	[srcptr]
	pld	[r8]
	pld	[r9]
	add	r2, srcptr, r1
	pld	[r2]

;
; YUV 4:2:0 to UYVY entry point
;
; dstptr = UYVY
; dststep
; srcptr = yptr
; srcstep = ystep
; r8 = uptr
; r9 = vptr
; r12 = uvstep
; r0 = dststride
; r1 = ystride
;
BlockCopyYUV420toUYVY
BC420UY_linepairloop
	subs	pels, dstwidth, #16
	blt	BC420UY_singles
BC420UY_grouploop
	pld	[srcptr, #64]
	pld	[r8, #64]
	pld	[r9, #64]
	add	r3, srcptr, r1
	add	r2, dstptr, r0
	pld	[r3, #64]
	vld2.8	{d1,d3}, [srcptr]!		;d1 = y0 y2 y4 y6 y8 yA yC yE, d3 = y1 y3 y5 y7 y9 yB yD yF
	vld1.8	{d0}, [r8]!			;d0 = u01xx u23xx u45xx u67xx u89xx uABxx uCDxx uEFxx (xx = next line)
	vld1.8	{d2}, [r9]!			;d2 = v01xx v23xx v45xx v67xx v89xx vABxx vCDxx vEFxx
	vst4.8	{d0,d1,d2,d3}, [dstptr]!	;u01xx y0 v01xx y2...
	vld2.8	{d1,d3}, [r3]			;d1 = yx yx yx..., d3 = yx yx yx...
	vst4.8	{d0,d1,d2,d3}, [r2]		;u01xx yx v01xx yx...
	subs	pels, pels, #16
	bge	BC420UY_grouploop

	adds	pels, pels, #16
	ble	BC420UY_linepairdone
BC420UY_singles
	mov	pels, pels, lsr #1
	add	r2, dstptr, r0
	add	r3, srcptr, r1
	LOADSTRAYS_2x8	pels, srcptr, d4, d5
    vswp            d4, d1
    vswp            d5, d3
	LOADSTRAYS_8	pels, r8, d0
	LOADSTRAYS_8	pels, r9, d2
	STORESTRAYS_4x8	pels, dstptr, d0, d1, d2, d3
	LOADSTRAYS_2x8	pels, r3, d4, d5
    vswp            d4, d1
    vswp            d5, d3
	STORESTRAYS_4x8	pels, r2, d0, d1, d2, d3
BC420UY_linepairdone
	add	dstptr, dstptr, dststep
	add	srcptr, srcptr, srcstep
	add	r8, r8, r12
	add	r9, r9, r12
	subs	dstheight, dstheight, #2
	bgt	BC420UY_linepairloop

	ldmfd	sp!, {r4-r12,pc}


;
; void BlockCopyIYUVtoRGB16(void*       dstptr,
;                           int32       dststride,
;                           uint32      dstleft,
;                           uint32      dsttop,
;                           uint32      dstwidth,
;                           uint32      dstheight,
;                           void const* srcptr,
;                           int32       srcstride,
;                           uint32      srcleft,
;                           uint32      srctop,
;                           uint32      srcsurfheight)
;
BlockCopyIYUVtoRGB16
	stmfd	sp!, {r4-r12,lr}
; Get UYVY destination surface working variables
	TWOSURFINIT	16, 8, 10		; dstptr, dststep, srcptr (yptr), srcstep (ystep), dstwidth, dstheight
; Now get YUV 4:2:0 source surface working variables
; Create generic variables assuming IYUV layout
; calculate u & v left/top offset
	ldr	r0, [sp, #((10 + 4) * 4)]	; srcleft
	ldr	r1, [sp, #((10 + 5) * 4)]	; srctop
	ldr	r2, [sp, #((10 + 3) * 4)]	; srcstride
	mov	r0, r0, lsr #1			; uvleft = srcleft / 2
	mov	r1, r1, lsr #1			; uvtop = srctop / 2
	mov	r2, r2, asr #1			; uvstride = ystride / 2
	mla	r0, r1, r2, r0			; uvoffset [r0] = uvleft + (uvtop * uvstride)

; calculate size of uv planes
	ldr	r1, [sp, #((10 + 3) * 4)]	; srcstride (ystride)
	ldr	r2, [sp, #((10 + 6) * 4)]	; srcsurfheight
	mov	r1, r1, asr #1			; uvstride = ystride / 2
	mov	r2, r2, lsr #1			; uvheight = srcsurfheight / 2
	mul	r1, r2, r1			; uvsize [r1] = uvheight * uvstride

; calculate u & v ptrs
	ldr	r8, [sp, #((10 + 2) * 4)]	; srcptr (yptr)
	ldr	r3, [sp, #((10 + 3) * 4)]	; srcstride (ystride)
	ldr	r2, [sp, #((10 + 6) * 4)]	; srcsurfheight
	mla	r8, r2, r3, r8			; r8 = yptr + (srcsurfheight * ystride)
	add	r8, r8, r0			; uptr = r8 + uvoffset
	add	r9, r8, r1			; vptr = uptr + uvsize

; calculate u & v step (two lines is one u or v line)
	ldr	r0, [sp, #((10 + 3) * 4)]	; srcstride (ystride)
	mov	r12, r0, asr #1			; uvstride = ystride / 2
	sub	r12, r12, dstwidth lsr #1	; uvstep = uvstride - (srcwidth / 2)

; calculate the y step (for two lines)
	ldr	r0, [sp, #((10 + 3) * 4)]	; srcstride (ystride)
	rsb	srcstep, dstwidth, r0, asl #1	; srcstep = (ystride + ystride) - srcwidth

; recalculate the destination step (for two lines)
	add	r0, dststep, dstwidth, lsl #1	; dststride = dststep + (2 x dstwidth)
	sub	dststep, r0, dstwidth
	mov	dststep, dststep, lsl #1	; dststep = (2 x dststride) - (2 x dstwidth)

; get the ystride
	ldr	r1, [sp, #((10 + 3) * 4)]	; srcstride (ystride)

	pld	[srcptr]
	pld	[r8]
	pld	[r9]
	add	r2, srcptr, r1
	pld	[r2]

	YUV422toRGB888_PREP

;
; YUV 4:2:0 to RGB16 entry point
;
; dstptr = UYVY
; dststep
; srcptr = yptr
; srcstep = ystep
; r8 = uptr
; r9 = vptr
; r12 = uvstep
; r0 = dststride
; r1 = srcstride
;
BlockCopyYUV420toRGB16
BC42016_linepairloop
	subs	pels, dstwidth, #16
	blt	BC42016_singles
BC42016_grouploop
	pld	[srcptr, #64]
	pld	[r8, #64]
	pld	[r9, #64]
	add	r3, srcptr, r1
	add	r2, dstptr, r0
	pld	[r3, #64]
	vld2.8	{d1,d3}, [srcptr]!		;d1 = y0 y2 y4 y6 y8 yA yC yE, d3 = y1 y3 y5 y7 y9 yB yD yF
	vld1.8	{d0}, [r8]			;d0 = u01xx u23xx u45xx u67xx u89xx uABxx uCDxx uEFxx (xx = next line)
	vld1.8	{d2}, [r9]			;d2 = v01xx v23xx v45xx v67xx v89xx vABxx vCDxx vEFxx
	YUV422toRGB888	d4,d5,d2,d3,d0,d1,d1,d0,d3,d2,q3,q4,q5,q6,q7,q8,q9	; q0 = r, q1 = g, q2 = b (interleaved)
	RGB888toRGB565	q3, d4, d2, d0, q4, q5	; q3 = rgb16 (interleaved)
	RGB888toRGB565	q4, d5, d3, d1, q5, q6	; q4 = rgb16 (interleaved)
	vst2.16	{d6,d8}, [dstptr]!
	vst2.16	{d7,d9}, [dstptr]!
	vld2.8	{d1,d3}, [r3]			;d1 = yx yx yx..., d3 = yx yx yx...
; YUVtoRGB macro above is destructive, so reload U's and V's
	vld1.8	{d0}, [r8]!			;d0 = u01xx u23xx u45xx u67xx u89xx uABxx uCDxx uEFxx (xx = next line)
	vld1.8	{d2}, [r9]!			;d2 = v01xx v23xx v45xx v67xx v89xx vABxx vCDxx vEFxx
	YUV422toRGB888	d4,d5,d2,d3,d0,d1,d1,d0,d3,d2,q3,q4,q5,q6,q7,q8,q9	; q0 = r, q1 = g, q2 = b (interleaved)
	RGB888toRGB565	q3, d4, d2, d0, q4, q5	; q3 = rgb16 (interleaved)
	RGB888toRGB565	q4, d5, d3, d1, q5, q6	; q4 = rgb16 (interleaved)
	vst2.16	{d6,d8}, [r2]!
	vst2.16	{d7,d9}, [r2]
	subs	pels, pels, #16
	bge	BC42016_grouploop

	adds	pels, pels, #16
	ble	BC42016_linepairdone
BC42016_singles
	add	r2, dstptr, r0
	add	r3, srcptr, r1
	LOADSTRAYS_2x8	pels, srcptr, d4, d5
    vswp            d4, d1
    vswp            d5, d3
	LOADSTRAYS_8	pels, r8, d0
	LOADSTRAYS_8	pels, r9, d2
	YUV422toRGB888	d4,d5,d2,d3,d0,d1,d1,d0,d3,d2,q3,q4,q5,q6,q7,q8,q9	; q0 = r, q1 = g, q2 = b (interleaved)
	RGB888toRGB565	q3, d4, d2, d0, q4, q5	; q3 = rgb16 (interleaved)
	RGB888toRGB565	q4, d5, d3, d1, q5, q6	; q4 = rgb16 (interleaved)
	mov	pels, pels, lsr #1
	STORESTRAYS_2x16	pels, dstptr, d6, d8
	STORESTRAYS_2x16	pels, dstptr, d7, d9
	mov	pels, pels, lsl #1
	LOADSTRAYS_2x8	pels, r3, d4, d5
    vswp            d4, d1
    vswp            d5, d3
; YUVtoRGB macro above is destructive, so reload U's and V's
	LOADSTRAYS_8	pels, r8, d0
	LOADSTRAYS_8	pels, r9, d2
	YUV422toRGB888	d4,d5,d2,d3,d0,d1,d1,d0,d3,d2,q3,q4,q5,q6,q7,q8,q9	; q0 = r, q1 = g, q2 = b (interleaved)
	RGB888toRGB565	q3, d4, d2, d0, q4, q5	; q3 = rgb16 (interleaved)
	RGB888toRGB565	q4, d5, d3, d1, q5, q6	; q4 = rgb16 (interleaved)
	mov	pels, pels, lsr #1
	STORESTRAYS_2x16	pels, r2, d6, d8
	STORESTRAYS_2x16	pels, r2, d7, d9
BC42016_linepairdone
	add	dstptr, dstptr, dststep
	add	srcptr, srcptr, srcstep
	add	r8, r8, r12
	add	r9, r9, r12
	subs	dstheight, dstheight, #2
	bgt	BC42016_linepairloop

	ldmfd	sp!, {r4-r12,pc}

	END

