	AREA	BltOptAsm,CODE,READONLY

	EXPORT	UnpremultXYZA32toXYZA32
	EXPORT	UnpremultXYZA32
	
CACHELINEWIDTH	EQU	64

; r13 a.k.a. sp
; r14 a.k.a. lr
; r15 a.k.a. pc

;----------------------------------------------------------------------------
; void UnpremultXYZA32toXYZA32(void*          dstptr,
;                              long           dststride,
;                              void const*    srcptr,
;                              long           srcstride,
;                              unsigned long  width,
;                              unsigned long  height,
;                              unsigned long* lut);
UnpremultXYZA32toXYZA32
	stmfd	sp!, {r4-r12,lr}

dstptr	rn	r4
dststrd	rn	r5
srcptr	rn	r6
srcstrd	rn	r7
widthoff	equ	(10 * 4)
heightoff	equ	(11 * 4)
lutoff	equ	(12 * 4)
height	rn	r8
lut	rn	r9

	mov	dstptr, r0
	mov	dststrd, r1
	mov	srcptr, r2
	mov	srcstrd, r3

	ldr	height, [sp, #heightoff]
	ldr	lut, [sp, #lutoff]

pels	rn	lr
	ldr	pels, [sp, #widthoff]

srcstep	rn	srcstrd
	sub	srcstep, srcstrd, pels lsl #2	; convert src stride to step

dststep	rn	dststrd
	sub	dststep, dststrd, pels lsl #2	; convert dest stride to step

UPMr32r32_lineloop
	pld	[srcptr]
	pld	[srcptr, #64]
	pld	[srcptr, #128]
	
	mov	r0, srcptr
	ldr	r1, [sp, #widthoff]
	mov	r1, r1, lsl #2

	ldr	pels, [sp, #widthoff]

alf0	rn	r0
alf1	rn	r1
alf2	rn	r2
alf3	rn	r3
add0	rn	alf0
add1	rn	alf1
add2	rn	alf2
add3	rn	alf3
recs16	qn	q0
recs16hi	dn	d0
recs16lo	dn	d1
blus8	dn	d2
grns8	dn	d3
reds8	dn	d4
alfs8	dn	d5
blus16	qn	q3
blus16hi	dn	d6
blus16lo	dn	d7
grns16	qn	q4
grns16hi	dn	d8
grns16lo	dn	d9
reds16	qn	q5
reds16hi	dn	d10
reds16lo	dn	d11

UPMr32r32_grouploop
	pld	[srcptr, #192]
	
 vld4.8	{blus8,grns8,reds8,alfs8}, [srcptr]!
 
	ldrb	alf0, [srcptr, #(3-32)]
		ldrb	alf1, [srcptr, #(7-32)]
			ldrb	alf2, [srcptr, #(11-32)]
				ldrb	alf3, [srcptr, #(15-32)]
UPMr32r32_unpremultiply
	subs	pels, pels, #8
	
	add	add0, lut, alf0, lsl #2
		add	add1, lut, alf1, lsl #2
	vld1.16	{recs16hi[0]}, [add0]
			add	add2, lut, alf2, lsl #2
 vmovl.u8	blus16, blus8
		vld1.16	{recs16hi[1]}, [add1]
				add	add3, lut, alf3, lsl #2
 vmovl.u8	grns16, grns8
			vld1.16	{recs16hi[2]}, [add2]
 vmovl.u8	reds16, reds8
				vld1.16	{recs16hi[3]}, [add3]
					ldrb	alf0, [srcptr, #(19-32)]
						ldrb	alf1, [srcptr, #(23-32)]
							ldrb	alf2, [srcptr, #(27-32)]
								ldrb	alf3, [srcptr, #(31-32)]
					add	add0, lut, alf0, lsl #2
						add	add1, lut, alf1, lsl #2
					vld1.16	{recs16lo[0]}, [add0]
							add	add2, lut, alf2, lsl #2
 vmul.u16	blus16hi, recs16hi, blus16hi
								add	add3, lut, alf3, lsl #2
						vld1.16	{recs16lo[1]}, [add1]
 vmul.u16	grns16hi, recs16hi, grns16hi
							vld1.16	{recs16lo[2]}, [add2]
 vmul.u16	reds16hi, recs16hi, reds16hi
								vld1.16	{recs16lo[3]}, [add3]

 vmul.u16	blus16lo, recs16lo, blus16lo
 vmul.u16	grns16lo, recs16lo, grns16lo
 vmul.u16	reds16lo, recs16lo, reds16lo

	vrshrn.u16	blus8, blus16, #8
	vrshrn.u16	grns8, grns16, #8
	vrshrn.u16	reds8, reds16, #8

	vst4.8	{blus8,grns8,reds8,alfs8}, [dstptr]!

	bgt	UPMr32r32_grouploop
 
UPMr32r32_linedone
	add	srcptr, srcptr, srcstep
	subs	height, height, #1
	add	dstptr, dstptr, dststep
	bne	UPMr32r32_lineloop

	ldmfd	sp!, {r4-r12,pc}


;----------------------------------------------------------------------------
; void UnpremultXYZA32(void*          dstptr,
;                      long           dststride,
;                      unsigned long  width,
;                      unsigned long  height,
;                      unsigned long* lut);
;
; If the source and destination are the same, if the alpha is 255, the pixel
; can be left alone.  Likewise, if the alpha is 0, the pixel will be 0 and
; can be left alone.  So this version previews the alphas and skips groups of
; 8 pixels if all the alphas are 0 or 255.
UnpremultXYZA32
	stmfd	sp!, {r4-r12,lr}

	mov	dstptr, r0
	mov	dststrd, r1
	mov	srcptr, r0
	mov	srcstrd, r1

	ldr	height, [sp, #heightoff]
	ldr	lut, [sp, #lutoff]

pels	rn	lr
	ldr	pels, [sp, #widthoff]

srcstep	rn	srcstrd
	sub	srcstep, srcstrd, pels lsl #2	; convert src stride to step

dststep	rn	dststrd
	sub	dststep, dststrd, pels lsl #2	; convert dest stride to step

UPMr32_lineloop
	pld	[srcptr]
	pld	[srcptr, #64]
	pld	[srcptr, #128]
	
	mov	r0, srcptr
	ldr	r1, [sp, #widthoff]
	mov	r1, r1, lsl #2

	ldr	pels, [sp, #widthoff]

alf0	rn	r0
alf1	rn	r1
alf2	rn	r2
alf3	rn	r3
add0	rn	alf0
add1	rn	alf1
add2	rn	alf2
add3	rn	alf3
recs16	qn	q0
recs16hi	dn	d0
recs16lo	dn	d1
blus8	dn	d2
grns8	dn	d3
reds8	dn	d4
alfs8	dn	d5
blus16	qn	q3
blus16hi	dn	d6
blus16lo	dn	d7
grns16	qn	q4
grns16hi	dn	d8
grns16lo	dn	d9
reds16	qn	q5
reds16hi	dn	d10
reds16lo	dn	d11

UPMr32_grouploop
	pld	[srcptr, #192]
	
 vld4.8	{blus8,grns8,reds8,alfs8}, [srcptr]!
 
	ldrb	alf0, [srcptr, #(3-32)]
		ldrb	alf1, [srcptr, #(7-32)]
			ldrb	alf2, [srcptr, #(11-32)]
				ldrb	alf3, [srcptr, #(15-32)]
	cmp	alf0, #0
	cmpne	alf0, #0xFF
	beq	UPMr32_testmorealphas
UPMr32_unpremultiply
	subs	pels, pels, #8
	
	add	add0, lut, alf0, lsl #2
		add	add1, lut, alf1, lsl #2
	vld1.16	{recs16hi[0]}, [add0]
			add	add2, lut, alf2, lsl #2
 vmovl.u8	blus16, blus8
		vld1.16	{recs16hi[1]}, [add1]
				add	add3, lut, alf3, lsl #2
 vmovl.u8	grns16, grns8
			vld1.16	{recs16hi[2]}, [add2]
 vmovl.u8	reds16, reds8
				vld1.16	{recs16hi[3]}, [add3]
					ldrb	alf0, [srcptr, #(19-32)]
						ldrb	alf1, [srcptr, #(23-32)]
							ldrb	alf2, [srcptr, #(27-32)]
								ldrb	alf3, [srcptr, #(31-32)]
					add	add0, lut, alf0, lsl #2
						add	add1, lut, alf1, lsl #2
					vld1.16	{recs16lo[0]}, [add0]
							add	add2, lut, alf2, lsl #2
 vmul.u16	blus16hi, recs16hi, blus16hi
								add	add3, lut, alf3, lsl #2
						vld1.16	{recs16lo[1]}, [add1]
 vmul.u16	grns16hi, recs16hi, grns16hi
							vld1.16	{recs16lo[2]}, [add2]
 vmul.u16	reds16hi, recs16hi, reds16hi
								vld1.16	{recs16lo[3]}, [add3]

 vmul.u16	blus16lo, recs16lo, blus16lo
 vmul.u16	grns16lo, recs16lo, grns16lo
 vmul.u16	reds16lo, recs16lo, reds16lo

	vrshrn.u16	blus8, blus16, #8
	vrshrn.u16	grns8, grns16, #8
	vrshrn.u16	reds8, reds16, #8

	vst4.8	{blus8,grns8,reds8,alfs8}, [dstptr]!

	bgt	UPMr32_grouploop
 
UPMr32_linedone
	add	srcptr, srcptr, srcstep
	subs	height, height, #1
	add	dstptr, dstptr, dststep
	bne	UPMr32_lineloop

	ldmfd	sp!, {r4-r12,pc}

UPMr32_testmorealphas
	cmp	alf1, #0
	cmpne	alf1, #0xFF
	bne	UPMr32_unpremultiply
	cmp	alf2, #0
	cmpne	alf2, #0xFF
	bne	UPMr32_unpremultiply
	ldrb	r10, [srcptr, #(19-32)]
	cmp	alf3, #0
	cmpne	alf3, #0xFF
	bne	UPMr32_unpremultiply
	ldrb	r11, [srcptr, #(23-32)]
	cmp	r10, #0
	cmpne	r10, #0xFF
	bne	UPMr32_unpremultiply
	ldrb	r12, [srcptr, #(27-32)]
	cmp	r11, #0
	cmpne	r11, #0xFF
	bne	UPMr32_unpremultiply
	ldrb	r10, [srcptr, #(31-32)]
	cmp	r12, #0
	cmpne	r12, #0xFF
	bne	UPMr32_unpremultiply
	cmp	r10, #0
	cmpne	r10, #0xFF
	bne	UPMr32_unpremultiply
	subs	pels, pels, #8
	add	dstptr, dstptr, #(8 * 4)
	bgt	UPMr32_grouploop
	b	UPMr32_linedone

	END
