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
;   File:  omap_optbltasm_rotate.s
;
;   Revision:  June 1, 2009
;

	AREA	omap_optbltasm,CODE,READONLY

	EXPORT	Rotate_8_90
	EXPORT	Rotate_16_90
	EXPORT	Rotate_16_180
	EXPORT	Rotate_16_270
	EXPORT	Rotate_UY_90
	EXPORT	Rotate_UY_270
	EXPORT	Rotate_UY_180


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
dststride	rn	dststep
srcptr	rn	r6
srcstep	rn	r7
srcstride	rn	srcstep
dstwidth	rn	r10
dstheight	rn	r11
srcheight	rn	dstwidth
srcwidth	rn	dstheight
srcrows	rn r8
srccolumns rn	r9
pels	rn	lr

; | | | | || | | | || | |
; |_|_|_|_||_|_|_|_||_|_|
; | | | | || | | | || | |
; | | | | || | | | || | |
; | | | | || | | | || | |
; |_|_|_|_||_|_|_|_||_|_|
;  _ _ _ _  _ _ _ _  _ _
; | | | | || | | | || | |
; | | | | || | | | || | |
; | | | | || | | | || | |
; |_|_|_|_||_|_|_|_||_|_|
; | | | | || | | | || | |
; | | | | || | | | || | |
; | | | | || | | | || | |
; |_|_|_|_||_|_|_|_||_|_|
; | | | | || | | | || | |
; | | | | || | | | || | |
; | | | | || | | | || | |
; |_|_|_|_||_|_|_|_||_|_|
; | | | | || | | | || | |
; | | | | || | | | || | |
; | | | | || | | | || | |
; |_|_|_|_||_|_|_|_||_|_|
;  _ _ _ _  _ _ _ _  _ _
; | | | | || | | | || | |
; | | |1|1|| | | | || | |
; |3|7|1|5|| | | | || | |
; |_|_|_|_||_|_|_|_||_|_|
; | | | | || | | | || | |
; | | |1|1|| | | | || | |
; |2|6|0|4|| | | | || | |
; |_|_|_|_||_|_|_|_||_|_|
; | | | | || | | | || | |
; | | | |1|| | | | || | |
; |1|5|9|3|| | | | || | |
; |_|_|_|_||_|_|_|_||_|_|
; | | | | || | | | || | |
; | | | |1|| | | | || | |
; |0|4|8|2|| | | | || | |
; |_|_|_|_||_|_|_|_||_|_|
; 
;  ________ ________ ________ ________  ________ ________ ________ ________  ________ __
; |____0___|____1___|____2___|____3___||________|________|________|________||________|__
; |____4___|____5___|____6___|____7___||________|________|________|________||________|__
; |____8___|____9___|___10___|___11___||________|________|________|________||________|__
; |___12___|___13___|___14___|___15___||________|________|________|________||________|__
;  ________ ________ ________ ________  ________ ________ ________ ________  ________ __
; |________|________|________|________||________|________|________|________||________|__
; |________|________|________|________||________|________|________|________||________|__
; |________|________|________|________||________|________|________|________||________|__
; |________|________|________|________||________|________|________|________||________|__
;  ________ ________ ________ ________  ________ ________ ________ ________  ________ __
; |________|________|________|________||________|________|________|________||________|__
; |________|________|________|________||________|________|________|________||________|__
;



;  O _ _ _ 
; | | | | |
; |0|1|2|3|
; | | | | |
; |I|_|_|_|
; 
	MACRO
	Rotate_8_90_4x8_Read	$Dreg0, $Dreg1, $Dreg2, $Dreg3, $ptr, $stride
 pld [$ptr, #4]
	sub	r0, $ptr, $stride
	vld4.8	{$Dreg0[0],$Dreg1[0],$Dreg2[0],$Dreg3[0]}, [$ptr]
 pld [r0, #4]
	sub	$ptr, $ptr, $stride, lsl #1
	vld4.8	{$Dreg0[1],$Dreg1[1],$Dreg2[1],$Dreg3[1]}, [r0]
 pld [$ptr, #4]
	sub	r0, r0, $stride, lsl #1
	vld4.8	{$Dreg0[2],$Dreg1[2],$Dreg2[2],$Dreg3[2]}, [$ptr]
 pld [r0, #4]
	sub	$ptr, $ptr, $stride, lsl #1
	vld4.8	{$Dreg0[3],$Dreg1[3],$Dreg2[3],$Dreg3[3]}, [r0]
 pld [$ptr, #4]
	sub	r0, r0, $stride, lsl #1
	vld4.8	{$Dreg0[4],$Dreg1[4],$Dreg2[4],$Dreg3[4]}, [$ptr]
 pld [r0, #4]
	sub	$ptr, $ptr, $stride, lsl #1
	vld4.8	{$Dreg0[5],$Dreg1[5],$Dreg2[5],$Dreg3[5]}, [r0]
 pld [$ptr, #4]
	sub	r0, r0, $stride, lsl #1
	vld4.8	{$Dreg0[6],$Dreg1[6],$Dreg2[6],$Dreg3[6]}, [$ptr]
 pld [r0, #4]
	sub	$ptr, $ptr, $stride, lsl #1
	vld4.8	{$Dreg0[7],$Dreg1[7],$Dreg2[7],$Dreg3[7]}, [r0]
	MEND

;  _ _ _ _
; |D|D|D|D|
; |1|1|1|1|
; |2|3|4|5|
; |_|_|_|_|
; | | |D|D|
; |D|D|1|1|
; |8|8|0|1|
; |_|_|_|_|
; | | | | |
; |D|D|D|D|
; |4|5|6|7|
; |_|_|_|_|
; | | | | |
; |D|D|D|D|
; |0|1|2|3|
; |I|_|_|_|O
; 
;  ________ ________ ________ ________
; |I__D0___|___D4___|___D8___|__D12___|
; |___D1___|___D5___|___D9___|__D13___|
; |___D2___|___D6___|__D10___|__D14___|
; |___D3___|___D7___|__D11___|__D15___|
;  O
;
	MACRO
	Rotate_8_90_4x32	$dst, $dststrd, $src, $srcstrd

	Rotate_8_90_4x8_Read	d0, d1, d2, d3, $src, $srcstrd
	Rotate_8_90_4x8_Read	d4, d5, d6, d7, $src, $srcstrd
	Rotate_8_90_4x8_Read	d8, d9, d10, d11, $src, $srcstrd
	Rotate_8_90_4x8_Read	d12, d13, d14, d15, $src, $srcstrd

	add	$src, $src, $srcstrd, lsl #5	; move from top to bottom of 4x32
	add	$src, $src, #4			; move to next column

	add	r0, $dst, #8
	add	r1, $dst, #16
	add	r2, $dst, #24
 	vst1.8	{d0}, [$dst], $dststrd
 	vst1.8	{d4}, [r0], $dststrd
 	vst1.8	{d8}, [r1], $dststrd
 	vst1.8	{d12}, [r2], $dststrd
 	vst1.8	{d1}, [$dst], $dststrd
 	vst1.8	{d5}, [r0], $dststrd
 	vst1.8	{d9}, [r1], $dststrd
 	vst1.8	{d13}, [r2], $dststrd
 	vst1.8	{d2}, [$dst], $dststrd
 	vst1.8	{d6}, [r0], $dststrd
 	vst1.8	{d10}, [r1], $dststrd
 	vst1.8	{d14}, [r2], $dststrd
 	vst1.8	{d3}, [$dst], $dststrd
 	vst1.8	{d7}, [r0]
 	vst1.8	{d11}, [r1]
 	vst1.8	{d15}, [r2]
	MEND

;  _ _ _ _  _ _ _ _
; | | | | || | | | |
; | | | | || | | | |
; | | | | || | | | |
; |_|_|_|_||_|_|_|_|
; | | | | || | | | |
; | | | | || | | | |
; | | | | || | | | |
; |_|_|_|_||_|_|_|_|
; | | | | || | | | |
; | | | | || | | | |
; | | | | || | | | |
; |_|_|_|_||_|_|_|_|
; | | | | || | | | |
; | | | | || | | | |
; | | | | || | | | |
; |_|_|_|_||_|_|_|_|
;  _ _ _ _  _ _ _ _
; | | | | || | | | |
; | | |1|1|| | | | |
; |3|7|1|5|| | | | |
; |_|_|_|_||_|_|_|_|
; | | | | || | | | |
; | | |1|1|| | | | |
; |2|6|0|4|| | | | |
; |_|_|_|_||_|_|_|_|
; | | | | || | | | |
; | | | |1|| | | | |
; |1|5|9|3|| | | | |
; |_|_|_|_||_|_|_|_|
; | | | | || | | | |
; | | | |1|| | | | |
; |0|4|8|2|| | | | |
; |_|_|_|_||_|_|_|_|
; 
;  ________ ________ ________ ________  ________ ________ ________ ________
; |____0___|____1___|____2___|____3___||________|________|________|________|
; |____4___|____5___|____6___|____7___||________|________|________|________|
; |____8___|____9___|___10___|___11___||________|________|________|________|
; |___12___|___13___|___14___|___15___||________|________|________|________|
;  ________ ________ ________ ________  ________ ________ ________ ________
; |________|________|________|________||________|________|________|________|
; |________|________|________|________||________|________|________|________|
; |________|________|________|________||________|________|________|________|
; |________|________|________|________||________|________|________|________|
;
;
Rotate_8_90
	stmfd	sp!, {r4-r12,lr}
	TWOSURFINIT	8, 8, 10, nostep

	mla	srcptr, srcheight, srcstride, srcptr
	sub	srcptr, srcptr, srcstride

 pld [srcptr]
 sub r0, srcptr, srcstride
 pld [r0]
 sub r0, srcptr, srcstride
 pld [r0]
 sub r0, srcptr, srcstride
 pld [r0]

	mov	srccolumns, srcwidth, lsr #2
	movs	srcrows, srcheight, lsr #5
	beq	R890_done

R890_rowloop
	movs	lr, srccolumns
	beq	R890_rowdone
	
R890_columnloop
	Rotate_8_90_4x32	dstptr, dststride, srcptr, srcstride
	subs	lr, lr, #1
	bne	R890_columnloop

R890_rowdone
	sub	srcptr, srcptr, srcwidth		; move back to left of image
	sub	srcptr, srcptr, srcstride, lsl #5	; move up to next row

	mls	dstptr, dstheight, dststride, dstptr	; move to top of image
	add	dstptr, dstptr, #32			; move right to next column

	subs	srcrows, srcrows, #1
	bne	R890_rowloop

R890_done
	ldmfd	sp!, {r4-r12,pc}

;  _ _ _ _ _ _ _ _
; | | | | | | | | |
; | | |1|1|1|1|1|1|
; |8|9|0|1|2|3|4|5|
; |_|_|_|_|_|_|_|_|
; | | | | | | | | |
; | | | | | | | | |
; |0|1|2|3|4|5|6|7|
; |I|_|_|_|_|_|_|_|O
;
;  ________ ________
; |I___0___|____8___|
; |____1___|____9___|
; |____2___|___10___|
; |____3___|___11___|
; |____4___|___12___|
; |____5___|___13___|
; |____6___|___14___|
; |____7___|___15___|
;  O
	MACRO
	Rotate_16_90_8x8	$dstptr, $dststride, $srcptr, $srcstride
 IF {TRUE}
  IF {TRUE}
	sub	r0, $srcptr, $srcstride
	mov	r1, $srcstride, asl #1
	rsb	r1, r1, #0
	vld1.16	{d0,d1}, [$srcptr], r1		; q0 = 00 01 02 03 04 05 06 07
	vld1.16	{d2,d3}, [r0], r1		; q1 = 10 11 12 13 14 15 16 17
	vld1.16	{d4,d5}, [$srcptr], r1		; q2 = 20 21 22 23 24 25 26 27
	vld1.16	{d6,d7}, [r0], r1		; q3 = 30 31 32 33 34 35 36 37
	vld1.16	{d8,d9}, [$srcptr], r1		; q4 = 40 41 42 43 44 45 46 47
	vld1.16	{d10,d11}, [r0], r1		; q5 = 50 51 52 53 54 55 56 57
	vld1.16	{d12,d13}, [$srcptr], r1	; q6 = 60 61 62 63 64 65 66 67
	vld1.16	{d14,d15}, [r0]			; q7 = 70 71 72 73 74 75 76 77

	add	$srcptr, $srcptr, $srcstride, lsl #3
	add	$srcptr, $srcptr, #16

	vtrn.16	q0, q1				; q1 = 10 00 12 02 14 04 16 06
						; q0 = 11 01 13 03 15 05 17 07
	vtrn.16	q2, q3				; q3 = 30 20 32 22 34 24 36 26
						; q2 = 31 21 33 23 35 25 37 27
	vtrn.16	q4, q5				; q5 = 50 40 52 42 54 44 56 46
						; q4 = 51 41 53 43 55 45 57 47
	vtrn.16	q6, q7				; q7 = 70 60 72 62 74 64 76 66
						; q6 = 71 61 73 63 75 65 77 67

	vtrn.32	q0, q2
	vtrn.32	q1, q3
	vtrn.32	q4, q6
	vtrn.32	q5, q7
						; q3 = 30 20 10 00 34 24 14 04 = d6 d7
						; q2 = 31 21 11 01 35 25 15 05 = d4 d5
						; q1 = 32 22 12 02 36 26 16 06 = d2 d3
						; q0 = 33 23 13 03 37 27 17 07 = d0 d1
						; q7 = 70 60 50 40 74 64 54 44 = d14 d15
						; q6 = 71 61 51 41 75 65 55 45 = d12 d13
						; q5 = 72 62 52 42 76 66 56 46 = d10 d11
						; q4 = 73 63 53 43 77 67 57 47 = d8 d9
	mov	r0, $dststride, lsl #1
	add	r1, $dstptr, $dststride
	add	r2, $dstptr, #8
	add	r3, r1, #8
	vst1.16	{d14}, [$dstptr], r0
	vst1.16	{d6}, [r1], r0
	vst1.16	{d12}, [r2], r0
	vst1.16	{d4}, [r3], r0
	vst1.16	{d10}, [$dstptr], r0
	vst1.16	{d2}, [r1], r0
	vst1.16	{d8}, [r2], r0
	vst1.16 {d0}, [r3], r0
	vst1.16	{d15}, [$dstptr], r0
	vst1.16	{d7}, [r1], r0
	vst1.16	{d13}, [r2], r0
	vst1.16 {d5}, [r3], r0
	vst1.16	{d11}, [$dstptr], r0
	vst1.16 {d3}, [r1]
	vst1.16	{d9}, [r2]
	vst1.16 {d1}, [r3]
  ELSE
	sub	r0, $srcptr, $srcstride
	mov	r1, $srcstride, asl #1
	rsb	r1, r1, #0
	vld1.16	{d0,d1}, [$srcptr], r1		; q0 = 00 01 02 03 04 05 06 07
	vld1.16	{d2,d3}, [r0], r1		; q1 = 10 11 12 13 14 15 16 17
	vld1.16	{d4,d5}, [$srcptr], r1		; q2 = 20 21 22 23 24 25 26 27
	vld1.16	{d6,d7}, [r0], r1		; q3 = 30 31 32 33 34 35 36 37
	vld1.16	{d8,d9}, [$srcptr], r1		; q4 = 40 41 42 43 44 45 46 47
	vld1.16	{d10,d11}, [r0], r1		; q5 = 50 51 52 53 54 55 56 57
	vld1.16	{d12,d13}, [$srcptr], r1	; q6 = 60 61 62 63 64 65 66 67
	vld1.16	{d14,d15}, [r0]			; q7 = 70 71 72 73 74 75 76 77

	add	$srcptr, $srcptr, $srcstride, lsl #3
	add	$srcptr, $srcptr, #16

	vtrn.16	q0, q1				; q1 = 10 00 12 02 14 04 16 06
						; q0 = 11 01 13 03 15 05 17 07
	vtrn.16	q2, q3				; q3 = 30 20 32 22 34 24 36 26
						; q2 = 31 21 33 23 35 25 37 27
	vtrn.16	q4, q5				; q5 = 50 40 52 42 54 44 56 46
						; q4 = 51 41 53 43 55 45 57 47
	vtrn.16	q6, q7				; q7 = 70 60 72 62 74 64 76 66
						; q6 = 71 61 73 63 75 65 77 67

	vtrn.32	q0, q2
	vtrn.32	q1, q3
	vtrn.32	q4, q6
	vtrn.32	q5, q7
						; q3 = 30 20 10 00 34 24 14 04 = d6 d7
						; q2 = 31 21 11 01 35 25 15 05 = d4 d5
						; q1 = 32 22 12 02 36 26 16 06 = d2 d3
						; q0 = 33 23 13 03 37 27 17 07 = d0 d1
						; q7 = 70 60 50 40 74 64 54 44 = d14 d15
						; q6 = 71 61 51 41 75 65 55 45 = d12 d13
						; q5 = 72 62 52 42 76 66 56 46 = d10 d11
						; q4 = 73 63 53 43 77 67 57 47 = d8 d9
	mov	r0, $dststride, lsl #1
	add	r1, $dstptr, $dststride
	add	r2, $dstptr, #8
	add	r3, r1, #8
	vst1.16	{d14}, [$dstptr], r0
	vst1.16	{d6}, [r1], r0
	vst1.16	{d12}, [r2], r0
	vst1.16	{d4}, [r3], r0
	vst1.16	{d10}, [$dstptr], r0
	vst1.16	{d2}, [r1], r0
	vst1.16	{d8}, [r2], r0
	vst1.16 {d0}, [r3], r0
	vst1.16	{d15}, [$dstptr], r0
	vst1.16	{d7}, [r1], r0
	vst1.16	{d13}, [r2], r0
	vst1.16 {d5}, [r3], r0
	vst1.16	{d11}, [$dstptr], r0
	vst1.16 {d3}, [r1]
	vst1.16	{d9}, [r2]
	vst1.16 {d1}, [r3]
  ENDIF
 ELSE
	add	r0, $srcptr, #(4 * 2)
	vld4.16	{d0[0],d1[0],d2[0],d3[0]}, [$srcptr]
	sub	$srcptr, $srcptr, $srcstride
	vld4.16	{d4[0],d5[0],d6[0],d7[0]}, [r0]
	sub	r0, r0, $srcstride
	vld4.16	{d0[1],d1[1],d2[1],d3[1]}, [$srcptr]
	sub	$srcptr, $srcptr, $srcstride
	vld4.16	{d4[1],d5[1],d6[1],d7[1]}, [r0]
	sub	r0, r0, $srcstride
	vld4.16	{d0[2],d1[2],d2[2],d3[2]}, [$srcptr]
	sub	$srcptr, $srcptr, $srcstride
	vld4.16	{d4[2],d5[2],d6[2],d7[2]}, [r0]
	sub	r0, r0, $srcstride
	vld4.16	{d0[3],d1[3],d2[3],d3[3]}, [$srcptr]
	sub	$srcptr, $srcptr, $srcstride
	vld4.16	{d4[3],d5[3],d6[3],d7[3]}, [r0]
	sub	r0, r0, $srcstride
	vld4.16	{d8[0],d9[0],d10[0],d11[0]}, [$srcptr]
	sub	$srcptr, $srcptr, $srcstride
	vld4.16	{d12[0],d13[0],d14[0],d15[0]}, [r0]
	sub	r0, r0, $srcstride
	vld4.16	{d8[1],d9[1],d10[1],d11[1]}, [$srcptr]
	sub	$srcptr, $srcptr, $srcstride
	vld4.16	{d12[1],d13[1],d14[1],d15[1]}, [r0]
	sub	r0, r0, $srcstride
	vld4.16	{d8[2],d9[2],d10[2],d11[2]}, [$srcptr]
	sub	$srcptr, $srcptr, $srcstride
	vld4.16	{d12[2],d13[2],d14[2],d15[2]}, [r0]
	sub	r0, r0, $srcstride
	vld4.16	{d8[3],d9[3],d10[3],d11[3]}, [$srcptr]
	sub	$srcptr, $srcptr, $srcstride
	vld4.16	{d12[3],d13[3],d14[3],d15[3]}, [r0]
	add	r0, $dstptr, #(4 * 2)
	vst1.16	{d0}, [$dstptr], $dststride
	add	$srcptr, $srcptr, $srcstride, lsl #3
	vst1.16	{d8}, [r0], $dststride
	add	$srcptr, $srcptr, #(8 * 2)
	vst1.16	{d1}, [$dstptr], $dststride
	vst1.16	{d9}, [r0], $dststride
	vst1.16	{d2}, [$dstptr], $dststride
	vst1.16	{d10}, [r0], $dststride
	vst1.16	{d3}, [$dstptr], $dststride
	vst1.16	{d11}, [r0], $dststride
	vst1.16	{d4}, [$dstptr], $dststride
	vst1.16	{d12}, [r0], $dststride
	vst1.16	{d5}, [$dstptr], $dststride
	vst1.16	{d13}, [r0], $dststride
	vst1.16	{d6}, [$dstptr], $dststride
	vst1.16	{d14}, [r0], $dststride
	vst1.16	{d7}, [$dstptr], $dststride
	vst1.16	{d15}, [r0]
 ENDIF
	MEND

;  ____ ____ ____ ____
; |    |    |    |    |
; |    |    |    |    |
; |  8 |  9 | 10 | 11 |
; |____|____|____|____|
; |    |    |    |    |
; |    |    |    |    |
; |  4 |  5 |  6 |  7 |
; |____|____|____|____|
; |    |    |    |    |
; |    |    |    |    |
; |  0 |  1 |  2 |  3 |
; |____|____|____|____|
;
;  ____ ____ ____
; |    |    |    |
; |    |    |    |
; |  0 |  4 |  8 |
; |____|____|____|
; |    |    |    |
; |    |    |    |
; |  1 |  5 |  9 |
; |____|____|____|
; |    |    |    |
; |    |    |    |
; |  2 |  6 | 10 |
; |____|____|____|
; |    |    |    |
; |    |    |    |
; |  3 |  7 | 11 |
; |____|____|____|
;
Rotate_16_90
	stmfd	sp!, {r4-r12,lr}
	TWOSURFINIT	16, 16, 10, nostep

; move source pointer to left edge of bottom line of source image
	mla	srcptr, srcheight, srcstride, srcptr
	sub	srcptr, srcptr, srcstride

; destination pointer is where it belongs, at the upper left of the destination image

 pld [srcptr]
 sub r0, srcptr, srcstride
 pld [r0]
 sub r0, r0, srcstride
 pld [r0]
 sub r0, r0, srcstride
 pld [r0]

; round size to 8x8 - @@@ to be extended for all resolutions
	bics	srcheight, srcheight, #7
	bicnes	srcwidth, srcwidth, #7
	beq	R1690_done

R1690_rowloop
	mov	srccolumns, srcwidth

R1690_columnloop
	Rotate_16_90_8x8	dstptr, dststride, srcptr, srcstride
	subs	srccolumns, srccolumns, #8
	bgt	R1690_columnloop

; move source to the start of the next row of 8x8 block going up
	sub	srcptr, srcptr, srcwidth, lsl #1	; move to left side
	sub	srcptr, srcptr, srcstride, lsl #3	; move up 8 lines

; move destination to the top of the next column
	add	dstptr, dstptr, #(8 * 2)		; move to next column
	subs	srcheight, srcheight, #8
	mls	dstptr, dststride, dstheight, dstptr	; move to top of image

	bgt	R1690_rowloop
	
R1690_done
	ldmfd	sp!, {r4-r12,pc}


;  _ _ _ _ _ _ _ _
; |I| | | | | | | |O
; | | | | | | | | |
; |0|1|2|3|4|5|6|7|
; |_|_|_|_|_|_|_|_|
; | | | | | | | | |
; | | | |1|1|1|1|1|
; |8|9|0|1|2|3|4|5|
; |_|_|_|_|_|_|_|_|
;
;  O_______ ________
; |____7___|___15___|
; |____6___|___14___|
; |____5___|___13___|
; |____4___|___12___|
; |____3___|___11___|
; |____2___|___10___|
; |____1___|____9___|
; |I___0___|____8___|
;  
	MACRO
	Rotate_16_270_8x8	$dstptr, $dststride, $srcptr, $srcstride
	add	r0, $srcptr, #(4 * 2)				; r0 = src + 8
	vld4.16	{d0[0],d1[0],d2[0],d3[0]}, [$srcptr]
	add	$srcptr, $srcptr, $srcstride			; ptr = src + (1 * stride)
	vld4.16	{d4[0],d5[0],d6[0],d7[0]}, [r0]
	add	r0, r0, $srcstride				; r0 = src + (1 * stride) + 8
	vld4.16	{d0[1],d1[1],d2[1],d3[1]}, [$srcptr]
	add	$srcptr, $srcptr, $srcstride			; ptr = src + (2 * stride)
	vld4.16	{d4[1],d5[1],d6[1],d7[1]}, [r0]
	add	r0, r0, $srcstride				; r0 = src + (2 * stride) + 8
	vld4.16	{d0[2],d1[2],d2[2],d3[2]}, [$srcptr]
	add	$srcptr, $srcptr, $srcstride			; ptr = src + (3 * stride)
	vld4.16	{d4[2],d5[2],d6[2],d7[2]}, [r0]
	add	r0, r0, $srcstride				; r0 = src + (3 * stride) + 8
	vld4.16	{d0[3],d1[3],d2[3],d3[3]}, [$srcptr]
	add	$srcptr, $srcptr, $srcstride			; ptr = src + (4 * stride)
	vld4.16	{d4[3],d5[3],d6[3],d7[3]}, [r0]
	add	r0, r0, $srcstride				; r0 = src + (4 * stride) + 8
	vld4.16	{d8[0],d9[0],d10[0],d11[0]}, [$srcptr]
	add	$srcptr, $srcptr, $srcstride			; ptr = src + (5 * stride)
	vld4.16	{d12[0],d13[0],d14[0],d15[0]}, [r0]
	add	r0, r0, $srcstride				; r0 = src + (5 * stride) + 8
	vld4.16	{d8[1],d9[1],d10[1],d11[1]}, [$srcptr]
	add	$srcptr, $srcptr, $srcstride			; ptr = src + (6 * stride)
	vld4.16	{d12[1],d13[1],d14[1],d15[1]}, [r0]
	add	r0, r0, $srcstride				; r0 = src + (6 * stride) + 8
	vld4.16	{d8[2],d9[2],d10[2],d11[2]}, [$srcptr]
	add	$srcptr, $srcptr, $srcstride			; ptr = src + (7 * stride)
	vld4.16	{d12[2],d13[2],d14[2],d15[2]}, [r0]
	add	r0, r0, $srcstride				; r0 = src + (7 * stride) + 8
	vld4.16	{d8[3],d9[3],d10[3],d11[3]}, [$srcptr]
	add	$srcptr, $srcptr, $srcstride			; ptr = src + (8 * stride)
	vld4.16	{d12[3],d13[3],d14[3],d15[3]}, [r0]

	sub	$srcptr, $srcptr, $srcstride, lsl #3		; ptr = src
	rsb	r1, $dststride, #0				; r1 = -stride
	add	r0, $dstptr, #(4 * 2)				; r0 = dst + 8
	add	$srcptr, $srcptr, #(8 * 2)			; ptr = src + 16

	vst1.16	{d0}, [$dstptr], r1				; ptr = dst - (1 * stride)
	vst1.16	{d8}, [r0], r1					; r0 = dst - (1 * stride) + 8
	vst1.16	{d1}, [$dstptr], r1				; ptr = dst - (2 * stride)
	vst1.16	{d9}, [r0], r1					; r0 = dst - (2 * stride) + 8
	vst1.16	{d2}, [$dstptr], r1				; ptr = dst - (3 * stride)
	vst1.16	{d10}, [r0], r1					; r0 = dst - (3 * stride) + 8
	vst1.16	{d3}, [$dstptr], r1				; ptr = dst - (4 * stride)
	vst1.16	{d11}, [r0], r1					; r0 = dst - (4 * stride) + 8
	vst1.16	{d4}, [$dstptr], r1				; ptr = dst - (5 * stride)
	vst1.16	{d12}, [r0], r1					; r0 = dst - (5 * stride) + 8
	vst1.16	{d5}, [$dstptr], r1				; ptr = dst - (6 * stride)
	vst1.16	{d13}, [r0], r1					; r0 = dst - (6 * stride) + 8
	vst1.16	{d6}, [$dstptr], r1				; ptr = dst - (7 * stride)
	vst1.16	{d14}, [r0], r1					; r0 = dst - (7 * stride) + 8
	vst1.16	{d7}, [$dstptr], r1				; ptr = dst - (8 * stride)
	vst1.16	{d15}, [r0]
	MEND

;  ____ ____ ____ ____
; |    |    |    |    |
; |    |    |    |    |
; |  0 |  1 |  2 |  3 |
; |____|____|____|____|
; |    |    |    |    |
; |    |    |    |    |
; |  4 |  5 |  6 |  7 |
; |____|____|____|____|
; |    |    |    |    |
; |    |    |    |    |
; |  8 |  9 | 10 | 11 |
; |____|____|____|____|
;
;  ____ ____ ____
; |    |    |    |
; |    |    |    |
; |  3 |  7 | 11 |
; |____|____|____|
; |    |    |    |
; |    |    |    |
; |  2 |  6 | 10 |
; |____|____|____|
; |    |    |    |
; |    |    |    |
; |  1 |  5 |  9 |
; |____|____|____|
; |    |    |    |
; |    |    |    |
; |  0 |  4 |  8 |
; |____|____|____|
;
Rotate_16_270
	stmfd	sp!, {r4-r12,lr}
	TWOSURFINIT	16, 16, 10, nostep

; source pointer is where it belongs, at the upper left of the source image

; put destination pointer at lower left corner of the image
	mla	dstptr, dstheight, dststride, dstptr
	sub	dstptr, dstptr, dststride

; round size to 8x8 - @@@ to be extended for all resolutions
	bics	srcheight, srcheight, #7
	bicnes	srcwidth, srcwidth, #7
	beq	R16270_done

R16270_rowloop
	mov	srccolumns, srcwidth

R16270_columnloop
	Rotate_16_270_8x8	dstptr, dststride, srcptr, srcstride
	subs	srccolumns, srccolumns, #8
	bgt	R16270_columnloop

; move source to the start of the next row of 8x8 blocks
	sub	srcptr, srcptr, srcwidth, lsl #1	; move to left side
	add	srcptr, srcptr, srcstride, lsl #3	; move down 8 lines

; move destination to the bottom of the next column
	add	dstptr, dstptr, #(8 * 2)		; move to next column
	subs	srcheight, srcheight, #8
	mla	dstptr, dststride, dstheight, dstptr	; move to bottom of image

	bgt	R16270_rowloop
	
R16270_done
	ldmfd	sp!, {r4-r12,pc}

;
;
;
;

Rotate_16_180
	stmfd	sp!, {r4-r12,lr}
	TWOSURFINIT	16, 16, 10

	pld	[srcptr]
	pld	[srcptr, #64]
	pld	[srcptr, #128]

; source pointer is where it belongs, at the upper left of the source image

; put destination pointer at lower right corner of the image, minus 8 pixels
	add	r0, dststep, dstwidth, lsl #1
	mla	dstptr, dstheight, r0, dstptr
	sub	dstptr, dstptr, #(8 * 2)

; round size to 8x1 - @@@ to be extended for all resolutions
	bics	dstwidth, dstwidth, #7
	beq	R16180_done

R16180_rowloop
	mov	pels, dstwidth

R16180_columnloop
	pld	[srcptr, #192]
	vld1.16	{d0,d1}, [srcptr]!
	vrev64.16	q0, q0
	vst1.16	{d1}, [dstptr]!			; +4 pixels
	subs	pels, pels, #8
	vst1.16	{d0}, [dstptr]			; +0 pixels
	sub	dstptr, dstptr, #((8 + 4) * 2)	; -12 pixels
						;-----------
						; -8 pixels
	bgt	R16180_columnloop

	add	srcptr, srcptr, srcstep
	sub	dstptr, dstptr, dststep
	subs	dstheight, dstheight, #1
	bgt	R16180_rowloop
	
R16180_done
	ldmfd	sp!, {r4-r12,pc}


	MACRO
	ROTATE_UY_90_2x8	$dstptr, $dststride, $srcptr, $srcstride
; Uef Ye Vef Yf
; Ucd Yc Vcd Yd
; Uab Ya Vab Yb
; U89 Y8 V89 Y9
; U67 Y6 V67 Y7
; U45 Y4 V45 Y5
; U23 Y2 V23 Y3
; U01 Y0 V01 Y1

; U0123 Y0 V0123 Y2 U4567 Y4 V4567 Y6 U89ab Y8 V89ab Ya Ucdef Yc Vcdef Ye
; U0123 Y1 V0123 Y3 U4567 Y5 V4567 Y7 U89ab Y9 V89ab Yb Ucdef Yd Vcdef Yf

	sub	r0, $srcptr, $srcstride
	vld4.8	{d0[0],d1[0],d2[0],d3[0]}, [$srcptr]
	sub	$srcptr, $srcptr, $srcstride, asl #1
	vld4.8	{d0[1],d1[1],d2[1],d3[1]}, [r0]
	sub	r0, r0, $srcstride, asl #1
	vld4.8	{d0[2],d1[2],d2[2],d3[2]}, [$srcptr]
	sub	$srcptr, $srcptr, $srcstride, asl #1
	vld4.8	{d0[3],d1[3],d2[3],d3[3]}, [r0]
	sub	r0, r0, $srcstride, asl #1
	vld4.8	{d0[4],d1[4],d2[4],d3[4]}, [$srcptr]
	sub	$srcptr, $srcptr, $srcstride, asl #1
	vld4.8	{d0[5],d1[5],d2[5],d3[5]}, [r0]
	vld4.8	{d0[6],d1[6],d2[6],d3[6]}, [$srcptr]!
	sub	r0, r0, $srcstride, asl #1
	sub	$srcptr, $srcptr, $srcstride, asl #1
	vld4.8	{d0[7],d1[7],d2[7],d3[7]}, [r0]
	add	$srcptr, $srcptr, $srcstride, asl #3

; d0 = U01 U23 U45 U67 U89 Uab Ucd Uef
; d1 = Y0  Y2  Y4  Y6  Y8  Ya  Yc  Ye
; d2 = V01 V23 V45 V67 V89 Vab Vcd Vef
; d3 = Y1  Y3  Y5  Y7  Y9  Yb  Yd  Yf

	vtrn.i8	d0, d2

; d0 = U01 V01 U45 V45 U89 V89 Ucd Vcd
; d1 = Y0  Y2  Y4  Y6  Y8  Ya  Yc  Ye
; d2 = U23 V23 U67 V67 Uab Vab Uef Vef
; d3 = Y1  Y3  Y5  Y7  Y9  Yb  Yd  Yf

	vhadd.u8	d0, d0, d2
	vmov	d2, d0

; d0 = U0123 V0123 U4567 V4567 U89ab V89ab Ucdef Vcdef
; d1 = Y0  Y2  Y4  Y6  Y8  Ya  Yc  Ye
; d2 = U0123 V0123 U4567 V4567 U89ab V89ab Ucdef Vcdef
; d3 = Y1  Y3  Y5  Y7  Y9  Yb  Yd  Yf

	add	r0, $dstptr, $dststride
	vst2.8	{d0,d1}, [$dstptr]
	add	$dstptr, $dstptr, $dststride, asl #1
	vst2.8	{d2,d3}, [r0]

	MEND

Rotate_UY_90
	stmfd	sp!, {r4-r12,lr}
	TWOSURFINIT	16, 16, 10, nostep

; move source pointer to left edge of bottom line of source image
	mla	srcptr, srcheight, srcstride, srcptr
	sub	srcptr, srcptr, srcstride

; destination pointer is where it belongs, at the upper left of the destination image

; pld [srcptr]
; sub r0, srcptr, srcstride
; pld [r0]
; sub r0, r0, srcstride
; pld [r0]
; sub r0, r0, srcstride
; pld [r0]

; round size to 2x8 - @@@ to be extended for all resolutions
	bics	srcheight, srcheight, #7
	bicnes	srcwidth, srcwidth, #1
	beq	RUY90_done

RUY90_rowloop
	mov	srccolumns, srcwidth

RUY90_columnloop
	ROTATE_UY_90_2x8	dstptr, dststride, srcptr, srcstride
	subs	srccolumns, srccolumns, #2
	bgt	RUY90_columnloop

; move source to the start of the next row of 2x8 block going up
	sub	srcptr, srcptr, srcwidth, lsl #1	; move to left side
	sub	srcptr, srcptr, srcstride, lsl #3	; move up 8 lines

; move destination to the top of the next column
	add	dstptr, dstptr, #(8 * 2)		; move to next column
	subs	srcheight, srcheight, #8
	mls	dstptr, dststride, dstheight, dstptr	; move to top of image

	bgt	RUY90_rowloop
	
RUY90_done
	ldmfd	sp!, {r4-r12,pc}


	MACRO
	ROTATE_UY_270_2x8	$dstptr, $dststride, $srcptr, $srcstride
; U01 Y0 V01 Y1
; U23 Y2 V23 Y3
; U45 Y4 V45 Y5
; U67 Y6 V67 Y7
; U89 Y8 V89 Y9
; Uab Ya Vab Yb
; Ucd Yc Vcd Yd
; Uef Ye Vef Yf

; U0123 Y1 V0123 Y3 U4567 Y5 V4567 Y7 U89ab Y9 V89ab Yb Ucdef Yd Vcdef Yf
; U0123 Y0 V0123 Y2 U4567 Y4 V4567 Y6 U89ab Y8 V89ab Ya Ucdef Yc Vcdef Ye

	add	r0, $srcptr, $srcstride
	vld4.8	{d0[0],d1[0],d2[0],d3[0]}, [$srcptr]
	add	$srcptr, $srcptr, $srcstride, asl #1
	vld4.8	{d0[1],d1[1],d2[1],d3[1]}, [r0]
	add	r0, r0, $srcstride, asl #1
	vld4.8	{d0[2],d1[2],d2[2],d3[2]}, [$srcptr]
	add	$srcptr, $srcptr, $srcstride, asl #1
	vld4.8	{d0[3],d1[3],d2[3],d3[3]}, [r0]
	add	r0, r0, $srcstride, asl #1
	vld4.8	{d0[4],d1[4],d2[4],d3[4]}, [$srcptr]
	add	$srcptr, $srcptr, $srcstride, asl #1
	vld4.8	{d0[5],d1[5],d2[5],d3[5]}, [r0]
	vld4.8	{d0[6],d1[6],d2[6],d3[6]}, [$srcptr]!
	add	r0, r0, $srcstride, asl #1
	add	$srcptr, $srcptr, $srcstride, asl #1
	vld4.8	{d0[7],d1[7],d2[7],d3[7]}, [r0]
	sub	$srcptr, $srcptr, $srcstride, asl #3

; d0 = U01 U23 U45 U67 U89 Uab Ucd Uef
; d1 = Y0  Y2  Y4  Y6  Y8  Ya  Yc  Ye
; d2 = V01 V23 V45 V67 V89 Vab Vcd Vef
; d3 = Y1  Y3  Y5  Y7  Y9  Yb  Yd  Yf

	vtrn.i8	d0, d2

; d0 = U01 V01 U45 V45 U89 V89 Ucd Vcd
; d1 = Y0  Y2  Y4  Y6  Y8  Ya  Yc  Ye
; d2 = U23 V23 U67 V67 Uab Vab Uef Vef
; d3 = Y1  Y3  Y5  Y7  Y9  Yb  Yd  Yf

	vhadd.u8	d0, d0, d2
	vmov	d2, d0

; d0 = U0123 V0123 U4567 V4567 U89ab V89ab Ucdef Vcdef
; d1 = Y0  Y2  Y4  Y6  Y8  Ya  Yc  Ye
; d2 = U0123 V0123 U4567 V4567 U89ab V89ab Ucdef Vcdef
; d3 = Y1  Y3  Y5  Y7  Y9  Yb  Yd  Yf

	sub	r0, $dstptr, $dststride
	vst2.8	{d0,d1}, [$dstptr]
	sub	$dstptr, $dstptr, $dststride, asl #1
	vst2.8	{d2,d3}, [r0]

	MEND

Rotate_UY_270
	stmfd	sp!, {r4-r12,lr}
	TWOSURFINIT	16, 16, 10, nostep

; source pointer is where it belongs, at the upper left of the source image

; put destination pointer at lower left corner of the image
	mla	dstptr, dstheight, dststride, dstptr
	sub	dstptr, dstptr, dststride

; round size to 2x8 - @@@ to be extended for all resolutions
	bics	srcheight, srcheight, #7
	bicnes	srcwidth, srcwidth, #1
	beq	RUY270_done

RUY270_rowloop
	mov	srccolumns, srcwidth

RUY270_columnloop
	ROTATE_UY_270_2x8	dstptr, dststride, srcptr, srcstride
	subs	srccolumns, srccolumns, #2
	bgt	RUY270_columnloop

; move source to the start of the next row of 8x8 blocks
	sub	srcptr, srcptr, srcwidth, lsl #1	; move to left side
	add	srcptr, srcptr, srcstride, lsl #3	; move down 8 lines

; move destination to the bottom of the next column
	add	dstptr, dstptr, #(8 * 2)		; move to next column
	subs	srcheight, srcheight, #8
	mla	dstptr, dststride, dstheight, dstptr	; move to bottom of image

	bgt	RUY270_rowloop
	
RUY270_done
	ldmfd	sp!, {r4-r12,pc}

Rotate_UY_180
	stmfd	sp!, {r4-r12,lr}
	TWOSURFINIT	16, 16, 10

	pld	[srcptr]
	pld	[srcptr, #64]
	pld	[srcptr, #128]

; source pointer is where it belongs, at the upper left of the source image

; put destination pointer at lower right corner of the image, minus 8 pixels
	add	r0, dststep, dstwidth, lsl #1
	mla	dstptr, dstheight, r0, dstptr
	sub	dstptr, dstptr, #(8 * 2)

; round size to 8x1 - @@@ to be extended for all resolutions
	bics	dstwidth, dstwidth, #7
	beq	RUY180_done

RUY180_rowloop
	mov	pels, dstwidth

RUY180_columnloop
	pld	[srcptr, #192]
	vld2.8	{d0,d1}, [srcptr]!
	vrev64.16	d0, d0
	vrev64.8	d1, d1
	vst2.8	{d0,d1}, [dstptr]
	subs	pels, pels, #8
	sub	dstptr, dstptr, #(8 * 2)
	bgt	RUY180_columnloop

	add	srcptr, srcptr, srcstep
	sub	dstptr, dstptr, dststep
	subs	dstheight, dstheight, #1
	bgt	RUY180_rowloop
	
RUY180_done
	ldmfd	sp!, {r4-r12,pc}

	
	END

