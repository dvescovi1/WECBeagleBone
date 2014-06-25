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
;   File:  omap_optbltasm_blend.s
;
;   Revision:  April 27, 2009
;              + Added RGB16 to RGB16 blend with constant alpha
;
;   Revision:  March 31, 2009
;

	AREA	omap_optbltasm,CODE,READONLY

	EXTERN	PreloadLine
	EXTERN	BlockCopyBytes

	EXPORT	AlphaBlendRGB16toRGB16withConst
	EXPORT	AlphaBlendpBGRA32toRGB16	; premultiplied BGRA into RGB16
	EXPORT	AlphaBlendRGB16toBGRx32withA8
	EXPORT	AlphaBlendXYZx32toXYZx32withA8
	EXPORT	AlphaBlendpBGRA32topBGRA32
	EXPORT	AlphaBlendBGRA32topBGRA32

	INCLUDE	omap_optbltasm.inc

;----------------------------------------------------------------------------
; ASSUMPTIONS
;----------------------------------------------------------------------------
; All destination memory has write combining enabled.  This mitigates the
;  need to align writes.
;
; Sources are cached.  This mitigates the need to align reads.
;
; Destinations may not be cached, which will make reads less efficient.
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
srcptr	rn	r6
srcstep	rn	r7
mskptr	rn	r8
mskstep rn	r9
dstwidth	rn	r10
dstheight	rn	r11
lut	rn	r12
pels	rn	lr


;----------------------------------------------------------------------------
; void AlphaBlendRGB16toRGB16withConst(void*       dstptr,
;                                      int32       dststride,
;                                      uint32      dstleft,
;                                      uint32      dsttop,
;                                      uint32      dstwidth,
;                                      uint32      dstheight,
;                                      void const* srcptr,
;                                      int32       srcstride,
;                                      uint32      srcleft,
;                                      uint32      srctop
;                                      uint8       alpha)
;
; Performs a global alpha blend of an RGB 5:6:5 source image into another
;  RGB 5:6:5 destination, using a global alpha value.
;
AlphaBlendRGB16toRGB16withConst
	stmfd	sp!, {r4-r12,lr}
	TWOSURFINIT	16, 16, 10

	ldrb	r0, [sp, #((10 + 6) * 4)]	; get global alpha value
	cmp	r0, #0
	beq	AB1616K_done			; alpha of zero means no copy
	cmp	r0, #255
	addeq	dstwidth, dstwidth, dstwidth
	beq	BlockCopyBytes			; alpha of 255 (1.0) means copy all

	vdup.8	d31, r0
	vmvn.8	d30, d31

AB1616K_lineloop
	PRELOADLINE16bpp	srcptr, dstwidth
	PRELOADLINE16bpp	dstptr, dstwidth
AB1616K_shortline
	subs	pels, dstwidth, #8
	blt	AB1616K_singles

AB1616K_grouploop
	vld1.16	{d0,d1}, [dstptr]
	vld1.16	{d2,d3}, [srcptr]!
	RGB565toRGB888v2	d4, d6, d5, q2, q0	; d4=r,d6=g,d5=b,q2=rb,q0=dst in
	RGB565toRGB888v2	d8, d7, d9, q4, q1	; d8=r,d7=g,d9=b,q4=rb,q1=src in
	BLEND	d4, d6, d5, d30, d8, d7, d9, d31, q5, q6, q7, q8, q9, q10
	RGB888toRGB565	q0, d4, d6, d5, q1, q4
	vst1.16	{d0,d1}, [dstptr]!
	subs	pels, pels, #8
	bge	AB1616K_grouploop

AB1616K_singles
	adds	pels, pels, #8
	beq	AB1616K_linedone

	mov	r0, dstptr
	LOADSTRAYS_16	pels, r0, d0, d1
	LOADSTRAYS_16	pels, srcptr, d2, d3
	RGB565toRGB888v2	d4, d6, d5, q2, q0	; d4=r,d6=g,d5=b,q2=rb,q0=dst in
	RGB565toRGB888v2	d8, d7, d9, q4, q1	; d8=r,d7=g,d9=b,q4=rb,q1=src in
	BLEND	d4, d6, d5, d30, d8, d7, d9, d31, q5, q6, q7, q8, q9, q10
	RGB888toRGB565	q0, d4, d6, d5, q1, q4
	STORESTRAYS_16	pels, dstptr, d0, d1

AB1616K_linedone
	add	dstptr, dstptr, dststep
	add	srcptr, srcptr, srcstep
	subs	dstheight, dstheight, #1
	bgt	AB1616K_lineloop

AB1616K_done
	ldmfd	sp!, {r4-r12,pc}

;----------------------------------------------------------------------------
; void AlphaBlendRGB16toBGRx32withA8(void*       dstptr,
;                                    int32       dststride,
;                                    uint32      dstleft,
;                                    uint32      dsttop,
;                                    uint32      dstwidth,
;                                    uint32      dstheight,
;                                    void const* srcptr,
;                                    int32       srcstride,
;                                    uint32      srcleft,
;                                    uint32      srctop,
;                                    void const* mskptr,
;                                    int32       mskstride,
;                                    uint32      mskleft,
;                                    uint32      msktop)
;
; Performs a per-pixel alpha blend of an RGB 5:6:5 source image into an
;  unpacked 24-bit (32 bits including padding byte) destination, using an
;  8-bit mask surface.  Since the alpha is independent of the source surface,
;  the source surface is assumed not to be premultiplied.  The destination
;  alpha is ignored.
;
AlphaBlendRGB16toBGRx32withA8
	stmfd	sp!, {r4-r12,lr}
	THREESURFINIT	32, 16, 8, 10

AB16r328_lineloop
	PRELOADLINE32bpp	dstptr, dstwidth
	PRELOADLINE16bpp	srcptr, dstwidth
	PRELOADLINE8bpp		mskptr, dstwidth
	
	subs	pels, dstwidth, #8
	blt	AB16r328_singles

AB16r328_octetloop
	vld4.8	{d0,d1,d2,d3}, [dstptr]
	vld1.16	{d4,d5}, [srcptr]!		; vld1.16 {q0}, [srcptr]!
	vld1.8	{d6}, [mskptr]!
	RGB565toRGB888v2	d9, d7, d8, q4, q2
	vmvn.u8	d10, d6
	BLEND	d2, d1, d0, d10, d9, d7, d8, d6, q6, q7, q8, q9, q10, q11
	vmov.u8	d3, #0xFF
	vst4.8	{d0,d1,d2,d3}, [dstptr]!
	subs	pels, pels, #8
	bge	AB16r328_octetloop

AB16r328_singles
	adds	pels, pels, #8
	beq	AB16r328_linedone

	mov	r0, dstptr
	LOADSTRAYS_4x8	pels, r0, d0, d1, d2, d3
	LOADSTRAYS_16	pels, srcptr, d4, d5
	LOADSTRAYS_8	pels, mskptr, d6
	RGB565toRGB888v2	d9, d7, d8, q4, q2
	vmvn.u8	d10, d6
	BLEND	d2, d1, d0, d10, d9, d7, d8, d6, q6, q7, q8, q9, q10, q11
	vmov.u8	d3, #0xFF
	STORESTRAYS_4x8	pels, dstptr, d0, d1, d2, d3

AB16r328_linedone
	add	dstptr, dstptr, dststep
	add	srcptr, srcptr, srcstep
	add	mskptr, mskptr, mskstep
	subs	dstheight, dstheight, #1
	bgt	AB16r328_lineloop

	ldmfd	sp!, {r4-r12,pc}
 
;----------------------------------------------------------------------------
; void AlphaBlendXYZx32toXYZx32withA8(void*       dstptr,
;                                     int32       dststride,
;                                     uint32      dstleft,
;                                     uint32      dsttop,
;                                     uint32      dstwidth,
;                                     uint32      dstheight,
;                                     void const* srcptr,
;                                     int32       srcstride,
;                                     uint32      srcleft,
;                                     uint32      srctop,
;                                     void const* mskptr,
;                                     int32       mskstride,
;                                     uint32      mskleft,
;                                     uint32      msktop)
;
; Performs a per-pixel alpha blend of an unpacked 24-bit (32 bits with
;  padding byte) surface toto another unpacked 24-bit (32 bits including
;  padding byte) destination, using an 8-bit mask surface.  Since the alpha
;  is independent of the source surface, the source surface is assumed not to
;  be premultiplied.  Both the source nor the destination alphas are ignored.
;
AlphaBlendXYZx32toXYZx32withA8
	stmfd	sp!, {r4-r12,lr}
	THREESURFINIT	32, 32, 8, 10

ABr32r328_lineloop
	PRELOADLINE32bpp	srcptr, dstwidth
	PRELOADLINE32bpp	dstptr, dstwidth
	PRELOADLINE8bpp		mskptr, dstwidth
	
	subs	pels, dstwidth, #8
	blt	ABr32r328_singles

ABr32r328_octetloop
	vld4.8	{d0,d1,d2,d3}, [dstptr]
	vld4.8	{d4,d5,d6,d7}, [srcptr]!
	vld1.8	{d8}, [mskptr]!
	vmvn.u8	d9, d8
	BLEND	d2, d1, d0, d9, d6, d5, d4, d8, q5, q6, q7, q8, q9, q10
	vmov.u8	d3, #0xFF
	vst4.8	{d0,d1,d2,d3}, [dstptr]!
	subs	pels, pels, #8
	bge	ABr32r328_octetloop

ABr32r328_singles
	adds	pels, pels, #8
	beq	ABr32r328_linedone

	mov	r0, dstptr
	LOADSTRAYS_4x8	pels, r0, d0, d1, d2, d3
	LOADSTRAYS_4x8	pels, srcptr, d4, d5, d6, d7
	LOADSTRAYS_8	pels, mskptr, d8
	vmvn.u8	d9, d8
	BLEND	d2, d1, d0, d9, d6, d5, d4, d8, q5, q6, q7, q8, q9, q10
	vmov.u8	d3, #0xFF
	STORESTRAYS_4x8	pels, dstptr, d0, d1, d2, d3

ABr32r328_linedone
	add	dstptr, dstptr, dststep
	add	srcptr, srcptr, srcstep
	add	mskptr, mskptr, mskstep
	subs	dstheight, dstheight, #1
	bgt	ABr32r328_lineloop

	ldmfd	sp!, {r4-r12,pc}
 
;----------------------------------------------------------------------------
; void AlphaBlendpBGRA32toRGB16(void*       dstptr,
;                               int32       dststride,
;                               uint32      dstleft,
;                               uint32      dsttop,
;                               uint32      dstwidth,
;                               uint32      dstheight,
;                               void const* srcptr,
;                               int32       srcstride,
;                               uint32      srcleft,
;                               uint32      srctop)
;
; Performs a per-pixel alpha blend of a reverse 32-bit image into an
;  RGB 5:6:5 destination.  Since the alpha is part of the source surface,
;  the source surface is assumed to be premultiplied.
;
AlphaBlendpBGRA32toRGB16
	stmfd	sp!, {r4-r12,lr}
	TWOSURFINIT	16, 32, 10

	;vmov.u8	d31, #0xFF	; alpha of 1.0 for premultiplied source

ABpr3216_lineloop
	PRELOADLINE32bpp	srcptr, dstwidth
	PRELOADLINE16bpp	dstptr, dstwidth
	
	subs	pels, dstwidth, #8
	blt	ABpr3216_singles

ABpr3216_octetloop
	vld4.8	{d2,d3,d4,d5}, [srcptr]!
	vld1.16	{d0,d1}, [dstptr]		; vld1.16 {q0}, [dstptr]
	vmvn.u8 d9, d5	; destination alpha = 1 - source alpha
	RGB565toRGB888v2	d6, d8, d7, q3, q0
	BLEND	d6, d8, d7, d9, d4, d3, d2, d5, q9, q10, q11, q12, q13, q14
	RGB888toRGB565	q0, d6, d8, d7, q13, q14
	vst1.16	{d0,d1}, [dstptr]!		; vst1.16 {q0}, [dstptr]!
	subs	pels, pels, #8
	bge	ABpr3216_octetloop

ABpr3216_singles
	adds	pels, pels, #8
	beq	ABpr3216_linedone

	mov	r0, dstptr
	LOADSTRAYS_4x8	pels, srcptr, d2, d3, d4, d5
	LOADSTRAYS_16	pels, r0, d0, d1
	vmvn.u8 d9, d5	; destination alpha = 1 - source alpha
	RGB565toRGB888v2	d6, d8, d7, q3, q0
	BLEND	d6, d8, d7, d9, d4, d3, d2, d5, q9, q10, q11, q12, q13, q14
	RGB888toRGB565	q0, d6, d8, d7, q13, q14
	STORESTRAYS_16	pels, dstptr, d0, d1

ABpr3216_linedone
	add	dstptr, dstptr, dststep
	add	srcptr, srcptr, srcstep
	add	mskptr, mskptr, mskstep
	subs	dstheight, dstheight, #1
	bgt	ABpr3216_lineloop

	ldmfd	sp!, {r4-r12,pc}

;----------------------------------------------------------------------------
; void AlphaBlendpBGRA32topBGRA32(void*       dstptr,
;                                 int32       dststride,
;                                 uint32      dstleft,
;                                 uint32      dsttop,
;                                 uint32      dstwidth,
;                                 uint32      dstheight,
;                                 void const* srcptr,
;                                 int32       srcstride,
;                                 uint32      srcleft,
;                                 uint32      srctop)
;
AlphaBlendpBGRA32topBGRA32
	stmfd	sp!, {r4-r12,lr}
	TWOSURFINIT	32, 32, 10

	pld	[srcptr]
	pld	[dstptr]
	pld	[srcptr, #64]
	pld	[dstptr, #64]

	vmov.u8	d31, #0xFF	; destinationalpha of 1.0 for premultiplied destination

ABpr32r32_lineloop
	subs	pels, dstwidth, #8
	blt	ABpr32r32_singles

ABpr32r32_octetloop
	pld	[srcptr, #128]
	pld	[dstptr, #128]
	vld4.8	{d2,d3,d4,d5}, [srcptr]!
	vld4.8	{d6,d7,d8,d9}, [dstptr]
	PREMULTTOPREMULTBLEND	d8,d7,d6,d9, d4,d3,d2,d5, d31, q5,q6,q7,q8,q9,q10,q11,q12
	vst4.8	{d6,d7,d8,d9}, [dstptr]!
	subs	pels, pels, #8
	bge	ABpr32r32_octetloop

ABpr32r32_singles
	adds	pels, pels, #8
	beq	ABpr32r32_linedone

	mov	r0, dstptr
	LOADSTRAYS_4x8	pels, srcptr, d2, d3, d4, d5
	LOADSTRAYS_4x8	pels, r0, d6, d7, d8, d9
;	vmvn.u8 d9, d5	; destination alpha = 1 - source alpha
	PREMULTTOPREMULTBLEND	d8,d7,d6,d9, d4,d3,d2,d5, d31, q5,q6,q7,q8,q9,q10,q11,q12
	STORESTRAYS_4x8	pels, dstptr, d6, d7, d8, d9

ABpr32r32_linedone
	add	dstptr, dstptr, dststep
	add	srcptr, srcptr, srcstep
	subs	dstheight, dstheight, #1
	bgt	ABpr32r32_lineloop
	
	ldmfd	sp!, {r4-r12,pc}

;----------------------------------------------------------------------------
; void AlphaBlendBGRA32topBGRA32(void*       dstptr,
;                                int32       dststride,
;                                uint32      dstleft,
;                                uint32      dsttop,
;                                uint32      dstwidth,
;                                uint32      dstheight,
;                                void const* srcptr,
;                                int32       srcstride,
;                                uint32      srcleft,
;                                uint32      srctop)
;
AlphaBlendBGRA32topBGRA32
	stmfd	sp!, {r4-r12,lr}
	TWOSURFINIT	32, 32, 10

	pld	[srcptr]
	pld	[dstptr]
	pld	[srcptr, #64]
	pld	[dstptr, #64]

ABr32r32_lineloop
	subs	pels, dstwidth, #8
	blt	ABr32r32_singles

ABr32r32_octetloop
	pld	[srcptr, #128]
	pld	[dstptr, #128]
	vld4.8	{d2,d3,d4,d5}, [srcptr]!
	vld4.8	{d6,d7,d8,d9}, [dstptr]
	vmvn.u8 d9, d5	; destination alpha = 1 - source alpha
	PREMULTTOPREMULTBLEND	d8,d7,d6,d9, d4,d3,d2,d5, d31, q5,q6,q7,q8,q9,q10,q11,q12
	vst4.8	{d6,d7,d8,d9}, [dstptr]!
	subs	pels, pels, #8
	bge	ABr32r32_octetloop

ABr32r32_singles
	adds	pels, pels, #8
	beq	ABr32r32_linedone

	mov	r0, dstptr
	LOADSTRAYS_4x8	pels, srcptr, d2, d3, d4, d5
	LOADSTRAYS_4x8	pels, r0, d6, d7, d8, d9
	vmvn.u8 d9, d5	; destination alpha = 1 - source alpha
	PREMULTTOPREMULTBLEND	d8,d7,d6,d9, d4,d3,d2,d5, d31, q5,q6,q7,q8,q9,q10,q11,q12
	STORESTRAYS_4x8	pels, dstptr, d6, d7, d8, d9

ABr32r32_linedone
	add	dstptr, dstptr, dststep
	add	srcptr, srcptr, srcstep
	subs	dstheight, dstheight, #1
	bgt	ABr32r32_lineloop

	ldmfd	sp!, {r4-r12,pc}

	END

