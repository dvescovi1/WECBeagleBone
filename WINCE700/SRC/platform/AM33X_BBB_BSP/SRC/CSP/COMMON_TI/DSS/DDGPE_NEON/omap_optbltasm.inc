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
;   File:  omap_optbltasm.inc
;
;   Revision:  April 27, 2009
;              + Added YUV to RGB conversion macro
;              + Added RGB to YUV conversion macro
;              + Changed LOADSTRAYS_16 to allow only one register
;              + Changed STORESTRAYS_16 to allow only one register
;
;   Revision:  March 31, 2009
;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; GLOBALs
;

;============================================================================
; O/S Specific Flags
;
; Set the following for the specific environment.  These values are used
;  below to handle specific differences between the environments.  These
;  may also be set on the command line by using the argument
;  -predefine "WINCE SETL {TRUE}" for the Microsoft assembler or
;  --predefine "SYMBIAN SETL {TRUE}" for the ARM Ltd. assembler.
;
;	GBLL	SYMBIAN
;SYMBIAN	SETL	{FALSE}
	GBLL	WINCE
WINCE	SETL	{TRUE}

;---------------------------------------------------------------------------
; Here the customization flags are defined.  They are set to their
;  respective default values, which may be overridden below by a specific
;  O/S (environment) need.

; Some O/Ss took the little-endian/big-endian thing too far and also reverse
;  the bits within the bytes for pixel sizes less than a byte.
	GBLL	REVERSESUBBYTEPIXELBITS
REVERSESUBBYTEPIXELBITS	SETL	{FALSE}
	
; When converting from 5:6:5 to 8:8:8, the best compromise between speed
;  (simplicity) and accuracy is to replicate the msbs into the lsbs (e.g.
;  r4-r3-r2-r1-r0 -> r4-r3-r2-r1-r0-r4-r3-r2).  But some environments
;  prefer to have the lsbs filled with zeroes.
	GBLL	REPLICATEMSBS565TO888
REPLICATEMSBS565TO888	SETL	{TRUE}

; When writing an xRGB variant, these routines normally fill the x with
;  a value of 255, just in case the byte is interpreted as an alpha.  But
;  some environments need this value to be 0.
	GBLL	DONTCAREALPHAIS0
DONTCAREALPHAIS0	SETL	{FALSE}

;---------------------------------------------------------------------------
; O/S (environment) settings

 IF :DEF: SYMBIAN
REVERSESUBBYTEPIXELBITS	SETL	{TRUE}
 ENDIF

 IF :DEF: WINCE
;REPLICATEMSBS565TO888	SETL	{FALSE}	; Set lsbs of 565 converted to 888 to 0
DONTCAREALPHAIS0	SETL	{TRUE}
 ENDIF

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; Predefined registers (for reference)
;	
; r13 a.k.a. sp
; r14 a.k.a. lr
; r15 a.k.a. pc

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; MACROs
;

;
; VMOV_U8_ALPHA is used to load the specified register with the appropriate
;  alpha value for a destination format where the alpha is ignored.  It would
;  have been preferable to use just a defined constant, but a variable format
;  was not found that worked with all assemblers tested to be used as part of
;  the constant value (i.e. they wouldn't accept something like
;  "vmov.u8 dreg, #ALPHACONST", no matter what type was used for ALPHACONST).
;
; Arguments:
;  $reg (Dn) - output: register into the lanes of which the alpha value is
;              duplicated
;
	MACRO
	VMOV_U8_ALPHA	$reg
 IF DONTCAREALPHAIS0
	vmov.u8	$reg, #0x00
 ELSE
	vmov.u8	$reg, #0xFF
 ENDIF
	MEND

;
; SURFINIT is used to set up the registers for a single surface of the type
;  specified.  It calculates the offset from the left and top provided, and
;  calculates the step needed to move from the end of one span to the start
;  of the next.
;
; $ptrout and $step are outputs
; $ptrin, $stride, $left, $top, $bpp are inputs
; All are ARM registers (Rn) except $bpp, which is a constant
;
; Arguments:
;  $ptrout (Rn) - output: register into which the pointer to the upper left
;                 corner of the rectangle of interest is placed
;  $stepstrideout (Rn) - output: register into which the stride is copied
;                        (if $nostep is defined) or the step calculated is
;                        placed [the step is the number of bytes needed to
;                        move from the end of one line to the beginning of
;                        the next]
;  $ptrin (Rn) - input: pointer to the upper left corner of the surface
;  $stridein (Rn) - input: the stride of the surface [the stride is the
;                   number of bytes needed to move from a pixel to the
;                   pixel immediately below it on the screen]
;  $left (Rn) - input: the x pixel coordinate of the upper left corner of
;               the rectangle of interest
;  $top (Rn) - input: the y line coordinate of the upper left corner of the
;              rectangle of interest
;  $width (Rn) - input: the width in pixels of the rectangle of interest
;  $bpp (constant) - input: the number of bits in each pixel
;  $nostep (flag) - input: if not defined, $stepstrideout is filled with the
;                   step; if defined, $stepstrideout is a copy of $stridein
;
	MACRO
	SURFINIT	$ptrout, $stepstrideout, $ptrin, $stridein, $left, $top, $width, $bpp, $nostep
; ptrout = ptrin + ((left * bpp) / 8) + (top * stride)
	mla	$ptrout, $top, $stridein, $ptrin	; ptrout = ptrin + (top * stride)
	mov	$ptrin, #$bpp				; bpp (using ptrin as temporary register)
	mul	$top, $left, $ptrin			; left *= bpp (using $top as temporary register)
	add	$ptrout, $ptrout, $top, lsr #3		; ptrout += ((left * bpp) / 8)
 IF "$nostep" != ""
; keep stride
	mov	$stepstrideout, $stridein
 ELSE
; step = stride - ((dstwidth * $dstbpp) / 8)
	mul	$top, $ptrin, dstwidth			; dstwidth * $dstbpp (using $top as a temporary register)
	sub	$stepstrideout, $stridein, $top, lsr #3	; step = stride - ((dstwidth * bpp) / 8)
  IF $bpp < 8
; sub-byte pixel sizes present a special issue for calculating the step
	and	$top, $left, #((8 / $bpp) - 1)		; get lsbs of left (using $top as temporary register)
	and	$left, $width, #((8 / $bpp) - 1)	; get lsbs of dstwidth (using $left as a temporary register)
	add	$top, $top, $left			; add lsbs of left to lsbs of dstwidth
	cmp	$top, #(8 / $bpp)			; if the result isn't more than bits that fit into a byte...
	sublt	$stepstrideout, $stepstrideout, #1	; ...there's one less byte in the dstwidth than we thought
  ENDIF
 ENDIF
	MEND

;----------------------------------------------------------------------------
; ONESURFINIT handles entry for a single surface operation.  It pulls the
;  associated parameters off of the stack and initializes the following
;  common registers:
;   dstptr
;   dststep
;   dstwidth
;   dstheight
;
; $dstbpp provides the bits per pixel of the destination surface
; $regspushed provides the number of registers pushed on the stack since
;  the parameters were pushed on the stack, so that on entry, this macro
;  assumes:
;   r0 = dstptr
;   r1 = dststride in bytes
;   r2 = dstleft in pixels
;   r3 = dsttop in lines
;   sp + (($regspushed + 0) * 4) = dstwidth in pixels
;   sp + (($regspushed + 1) * 4) = dstheight in lines
;
; Arguments:
;
	MACRO
	ONESURFINIT	$dstbpp, $regspushed, $nostep
	ldr	dstwidth, [sp, #(($regspushed + 0) * 4)]	; get dstwidth
	ldr	dstheight, [sp, #(($regspushed + 1) * 4)]	; get dstheight
	SURFINIT	dstptr, dststep, r0, r1, r2, r3, dstwidth, $dstbpp, $nostep
	MEND

;----------------------------------------------------------------------------
; TWOSURFINIT handles entry for a dual surface operation.  It pulls the
;  associated parameters off of the stack and initializes the following
;  common registers:
;   dstptr
;   dststep
;   srcptr
;   srcstep
;   dstwidth
;   dstheight
;
; $dstbpp provides the bits per pixel of the destination surface
; $srcbpp provides the bits per pixel of the source surface
; $regspushed provides the number of registers pushed on the stack since
;  the parameters were pushed on the stack, so that on entry, this macro
;  assumes:
;   r0 = dstptr
;   r1 = dststride in bytes
;   r2 = dstleft in pixels
;   r3 = dsttop in lines
;   sp + (($regspushed + 0) * 4) = dstwidth in pixels
;   sp + (($regspushed + 1) * 4) = dstheight in lines
;   sp + (($regspushed + 2) * 4) = srcptr
;   sp + (($regspushed + 3) * 4) = srcstride in bytes
;   sp + (($regspushed + 4) * 4) = srcleft in pixels
;   sp + (($regspushed + 5) * 4) = srctop in lines
;
	MACRO
	TWOSURFINIT	$dstbpp, $srcbpp, $regspushed, $nostep
	ldr	dstwidth, [sp, #(($regspushed + 0) * 4)]	; get dstwidth
	ldr	dstheight, [sp, #(($regspushed + 1) * 4)]	; get dstheight
	SURFINIT	dstptr, dststep, r0, r1, r2, r3, dstwidth, $dstbpp, $nostep
	ldr	r0, [sp, #(($regspushed + 2) * 4)]	; srcptr
	ldr	r1, [sp, #(($regspushed + 3) * 4)]	; srcstride
	ldr	r2, [sp, #(($regspushed + 4) * 4)]	; srcleft
	ldr	r3, [sp, #(($regspushed + 5) * 4)]	; srctop
	SURFINIT	srcptr, srcstep, r0, r1, r2, r3, dstwidth, $srcbpp, $nostep
	MEND

;----------------------------------------------------------------------------
; THREESURFINIT handles entry for a three surface operation.  It pulls the
;  associated parameters off of the stack and initializes the following
;  common registers:
;   dstptr
;   dststep
;   srcptr
;   srcstep
;   mskptr
;   mskstep
;   dstwidth
;   dstheight
;
; $dstbpp provides the bits per pixel of the destination surface
; $srcbpp provides the bits per pixel of the source surface
; $mskbpp provides the bits per pixel of the mask surface
; $regspushed provides the number of registers pushed on the stack since
;  the parameters were pushed on the stack, so that on entry, this macro
;  assumes:
;   r0 = dstptr
;   r1 = dststride in bytes
;   r2 = dstleft in pixels
;   r3 = dsttop in lines
;   sp + (($regspushed + 0) * 4) = dstwidth in pixels
;   sp + (($regspushed + 1) * 4) = dstheight in lines
;   sp + (($regspushed + 2) * 4) = srcptr
;   sp + (($regspushed + 3) * 4) = srcstride in bytes
;   sp + (($regspushed + 4) * 4) = srcleft in pixels
;   sp + (($regspushed + 5) * 4) = srctop in lines
;   sp + (($regspushed + 6) * 4) = mskptr
;   sp + (($regspushed + 7) * 4) = mskstride in bytes
;   sp + (($regspushed + 8) * 4) = mskleft in pixels
;   sp + (($regspushed + 9) * 4) = msktop in lines
;
	MACRO
	THREESURFINIT	$dstbpp, $srcbpp, $mskbpp, $regspushed, $nostep
	ldr	dstwidth, [sp, #(($regspushed + 0) * 4)]	; get dstwidth
	ldr	dstheight, [sp, #(($regspushed + 1) * 4)]	; get dstheight
	SURFINIT	dstptr, dststep, r0, r1, r2, r3, dstwidth, $dstbpp, $nostep
	ldr	r0, [sp, #(($regspushed + 2) * 4)]		; srcptr
	ldr	r1, [sp, #(($regspushed + 3) * 4)]	; srcstride
	ldr	r2, [sp, #(($regspushed + 4) * 4)]	; srcleft
	ldr	r3, [sp, #(($regspushed + 5) * 4)]	; srctop
	SURFINIT	srcptr, srcstep, r0, r1, r2, r3, dstwidth, $srcbpp, $nostep
	ldr	r0, [sp, #(($regspushed + 6) * 4)]	; mskptr
	ldr	r1, [sp, #(($regspushed + 7) * 4)]	; mskstride
	ldr	r2, [sp, #(($regspushed + 8) * 4)]	; mskleft
	ldr	r3, [sp, #(($regspushed + 9) * 4)]	; msktop
	SURFINIT	mskptr, mskstep, r0, r1, r2, r3, dstwidth, $mskbpp, $nostep
	MEND


;----------------------------------------------------------------------------
;
; Load and store macros simplify single Neon register lane loading below.
;

; Load 1 to 7 stray 8-bit pixels
	MACRO
	LOADSTRAYS_2x8	$strays, $ptr, $reg0, $reg1
	vld2.8	{$reg0[0],$reg1[0]}, [$ptr]!	; >= 1
	cmp	$strays, #2
	blt	%f80				; == 1
	vld2.8	{$reg0[1],$reg1[1]}, [$ptr]!	; >= 2
	beq	%f80				; == 2
	cmp	$strays, #4
	vld2.8	{$reg0[2],$reg1[2]}, [$ptr]!	; >= 3
	blt	%f80				; == 3
	vld2.8	{$reg0[3],$reg1[3]}, [$ptr]!	; >= 4
	beq	%f80				; == 4
	cmp	$strays, #6
	vld2.8	{$reg0[4],$reg1[4]}, [$ptr]!	; >= 5
	blt	%f80				; == 5
	vld2.8	{$reg0[5],$reg1[5]}, [$ptr]!	; >=6
	beq	%f80				; == 6
	vld2.8	{$reg0[6],$reg1[6]}, [$ptr]!	; == 7 or >= 7
80
	MEND

; Load 1 to 7 stray 8-bit pixels
	MACRO
	LOADSTRAYS_8	$strays, $ptr, $reg0
	vld1.8	{$reg0[0]}, [$ptr]!	; >= 1
	cmp	$strays, #2
	blt	%f80			; == 1
	vld1.8	{$reg0[1]}, [$ptr]!	; >= 2
	beq	%f80			; == 2
	cmp	$strays, #4
	vld1.8	{$reg0[2]}, [$ptr]!	; >= 3
	blt	%f80			; == 3
	vld1.8	{$reg0[3]}, [$ptr]!	; >= 4
	beq	%f80			; == 4
	cmp	$strays, #6
	vld1.8	{$reg0[4]}, [$ptr]!	; >= 5
	blt	%f80			; == 5
	vld1.8	{$reg0[5]}, [$ptr]!	; >=6
	beq	%f80			; == 6
	vld1.8	{$reg0[6]}, [$ptr]!	; == 7 or >= 7
80
	MEND
	
; Load 1 to 7 stray 16-bit pixels
	MACRO
	LOADSTRAYS_16	$strays, $ptr, $reg0, $reg1
	vld1.16	{$reg0[0]}, [$ptr]!	; >= 1
	cmp	$strays, #2
	blt	%f160			; == 1
	vld1.16	{$reg0[1]}, [$ptr]!	; >= 2
	beq	%f160			; == 2
 IF "$reg1" != ""
	cmp	$strays, #4
 ENDIF
	vld1.16	{$reg0[2]}, [$ptr]!	; >= 3
 IF "$reg1" != ""
	blt	%f160			; == 3
	vld1.16	{$reg0[3]}, [$ptr]!	; >= 4
	beq	%f160			; == 4
	cmp	$strays, #6
	vld1.16	{$reg1[0]}, [$ptr]!	; >= 5
	blt	%f160			; == 5
	vld1.16	{$reg1[1]}, [$ptr]!	; >= 6
	beq	%f160			; == 6
	vld1.16	{$reg1[2]}, [$ptr]!	; == 7
 ENDIF
160
	MEND

; Load 1 to 7 stray 3 x 8-bit pixels
; $reg0, $reg1, and $reg2 must be sequential by 1s or 2s
	MACRO
	LOADSTRAYS_3x8	$strays, $ptr, $reg0, $reg1, $reg2
	vld3.8	{$reg0[0],$reg1[0],$reg2[0]}, [$ptr]!
	cmp	$strays, #2
	blt	%f240
	vld3.8	{$reg0[1],$reg1[1],$reg2[1]}, [$ptr]!
	ble	%f240
	cmp	$strays, #4
	vld3.8	{$reg0[2],$reg1[2],$reg2[2]}, [$ptr]!
	blt	%f240
	vld3.8	{$reg0[3],$reg1[3],$reg2[3]}, [$ptr]!
	ble	%f240
	cmp	$strays, #6
	vld3.8	{$reg0[4],$reg1[4],$reg2[4]}, [$ptr]!
	blt	%f240
	vld3.8	{$reg0[5],$reg1[5],$reg2[5]}, [$ptr]!
	ble	%f240
	vld3.8	{$reg0[6],$reg1[6],$reg2[6]}, [$ptr]!
240
	MEND

; Load 1 to 7 stray 4 x 8-bit pixels
; $reg0, $reg1, $reg2, and $reg3 must be sequential by 1s or 2s
	MACRO
	LOADSTRAYS_4x8	$strays, $ptr, $reg0, $reg1, $reg2, $reg3
	vld4.8	{$reg0[0],$reg1[0],$reg2[0],$reg3[0]}, [$ptr]!
	cmp	$strays, #2
	blt	%f320
	vld4.8	{$reg0[1],$reg1[1],$reg2[1],$reg3[1]}, [$ptr]!
	ble	%f320
	cmp	$strays, #4
	vld4.8	{$reg0[2],$reg1[2],$reg2[2],$reg3[2]}, [$ptr]!
	blt	%f320
	vld4.8	{$reg0[3],$reg1[3],$reg2[3],$reg3[3]}, [$ptr]!
	ble	%f320
	cmp	$strays, #6
	vld4.8	{$reg0[4],$reg1[4],$reg2[4],$reg3[4]}, [$ptr]!
	blt	%f320
	vld4.8	{$reg0[5],$reg1[5],$reg2[5],$reg3[5]}, [$ptr]!
	ble	%f320
	vld4.8	{$reg0[6],$reg1[6],$reg2[6],$reg3[6]}, [$ptr]!
320
	MEND

; Store 1 to 7 stray 16-bit pixels
	MACRO
	STORESTRAYS_16	$strays, $ptr, $reg0, $reg1
	cmp	$strays, #2
	vst1.16	{$reg0[0]}, [$ptr]!
	blt	%f161
	vst1.16	{$reg0[1]}, [$ptr]!
	ble	%f161
 IF "$reg1" != ""
	cmp	$strays, #4
 ENDIF
	vst1.16	{$reg0[2]}, [$ptr]!
 IF "$reg1" != ""
	blt	%f161
	vst1.16	{$reg0[3]}, [$ptr]!
	ble	%f161
  	cmp	$strays, #6
	vst1.16	{$reg1[0]}, [$ptr]!
	blt	%f161
 	vst1.16	{$reg1[1]}, [$ptr]!
 	ble	%f161
 	vst1.16	{$reg1[2]}, [$ptr]!
 ENDIF
161
	MEND

; Store 1 to 3 pairs of (2 to 6) stray 2 x 16-bit pixels
; $reg0 and $reg1 must be sequential by 1s or 2s
; Numer of strays must be multiple of 2
	MACRO
	STORESTRAYS_2x16	$strays, $ptr, $reg0, $reg1
	cmp	$strays, #4
	vst2.16	{$reg0[0],$reg1[0]}, [$ptr]!
	blt	%f2161
	vst2.16	{$reg0[1],$reg1[1]}, [$ptr]!
	ble	%f2161
	vst2.16	{$reg0[2],$reg1[2]}, [$ptr]!
2161
	MEND

; Store 1 to 7 stray 3 x 8-bit pixels
; $reg0, $reg1, and $reg2 must be sequential by 1s or 2s
	MACRO
	STORESTRAYS_3x8	$strays, $ptr, $reg0, $reg1, $reg2
	cmp	$strays, #2
	vst3.8	{$reg0[0],$reg1[0],$reg2[0]}, [$ptr]!
	blt	%f241
	vst3.8	{$reg0[1],$reg1[1],$reg2[1]}, [$ptr]!
	ble	%f241
	cmp	$strays, #4
	vst3.8	{$reg0[2],$reg1[2],$reg2[2]}, [$ptr]!
	blt	%f241
	vst3.8	{$reg0[3],$reg1[3],$reg2[3]}, [$ptr]!
	ble	%f241
	cmp	$strays, #6
	vst3.8	{$reg0[4],$reg1[4],$reg2[4]}, [$ptr]!
	blt	%f241
	vst3.8	{$reg0[5],$reg1[5],$reg2[5]}, [$ptr]!
	ble	%f241
	vst3.8	{$reg0[6],$reg1[6],$reg2[6]}, [$ptr]!
241
	MEND

; Store 1 to 7 stray 4 x 8-bit pixels
; $reg0, $reg1, $reg2, and $reg3 must be sequential by 1s or 2s
	MACRO
	STORESTRAYS_4x8	$strays, $ptr, $reg0, $reg1, $reg2, $reg3
	cmp	$strays, #2
	vst4.8	{$reg0[0],$reg1[0],$reg2[0],$reg3[0]}, [$ptr]!
	blt	%f321
	vst4.8	{$reg0[1],$reg1[1],$reg2[1],$reg3[1]}, [$ptr]!
	ble	%f321
	cmp	$strays, #4
	vst4.8	{$reg0[2],$reg1[2],$reg2[2],$reg3[2]}, [$ptr]!
	blt	%f321
	vst4.8	{$reg0[3],$reg1[3],$reg2[3],$reg3[3]}, [$ptr]!
	ble	%f321
	cmp	$strays, #6
	vst4.8	{$reg0[4],$reg1[4],$reg2[4],$reg3[4]}, [$ptr]!
	blt	%f321
	vst4.8	{$reg0[5],$reg1[5],$reg2[5],$reg3[5]}, [$ptr]!
	ble	%f321
	vst4.8	{$reg0[6],$reg1[6],$reg2[6],$reg3[6]}, [$ptr]!
321
	MEND

;----------------------------------------------------------------------------
; RGB565toRGB888 converts a set of eight 16-bit RGB 5:6:5 pixels to
;  RGB 8:8:8.  Three Dn registers and a Qn register are passed in.  The Qn
;  register contains the input pixels, while the Dn registers will contain
;  the converted pixels, separated into R, G, and B channels.
;
; Arguments:
;  $outred8x8 (Dn) - output: 8 8-bit red components
;  $outgrn8x8 (Dn) - output: 8 8-bit green components
;  $outblu8x8 (Dn) - output: 8 8-bit blue components
;  $in16x8 (Qn) - input: 8 16-bit RGB16 (5:6:5) pixels
	MACRO
	RGB565toRGB888	$outred8x8, $outgrn8x8, $outblu8x8, $in16x8
 IF REPLICATEMSBS565TO888
	vmovn.u16	$outblu8x8, $in16x8		; gggbbbbb x 8
	vshrn.u16	$outred8x8, $in16x8, #8		; rrrrrggg x 8
	vshrn.u16	$outgrn8x8, $in16x8, #3		; ggggggbb x 8
	vshl.u8		$outblu8x8, $outblu8x8, #3	; bbbbb000 x 8
	vsri.u8		$outgrn8x8, $outgrn8x8, #6	; gggggggg x 8
	vsri.u8		$outred8x8, $outred8x8, #5	; rrrrrrrr x 8
	vsri.u8		$outblu8x8, $outblu8x8, #5	; bbbbbbbb x 8
 ELSE
	vmovn.u16	$outblu8x8, $in16x8		; gggbbbbb x 8
	vshrn.u16	$outred8x8, $in16x8, #8		; rrrrrggg x 8
	vshrn.u16	$outgrn8x8, $in16x8, #5		; rrgggggg x 8
	vshr.u8		$outred8x8, $outred8x8, #3	; 000rrrrr x 8
	vshl.u8		$outblu8x8, $outblu8x8, #3	; bbbbb000 x 8
	vshl.u8		$outred8x8, $outred8x8, #3	; rrrrr000 x 8
	vshl.u8		$outgrn8x8, $outgrn8x8, #2	; gggggg00 x 8
 ENDIF
	MEND

; V2
; If the red and green Dn registers are chosen to be in the same Qn
;  register, this macro can eliminate an instruction.
;
; Arguments:
;  $outred8x8 (Dn) - output: 8 8-bit red components; register should be
;                    paired with blue register
;  $outgrn8x8 (Dn) - output: 8 8-bit green components
;  $outblu8x8 (Dn) - output: 8 8-bit blue components; register should be
;                    paired with red register
;  $outredblu8x8x2 (Qn) - output: quad-word register corresponding to
;                         the double-word registers holding the red and
;                         green pixels
;  $in16x8 (Qn) - input: 8 16-bit RGB16 (5:6:5) pixels

	MACRO
	RGB565toRGB888v2	$outred8x8, $outgrn8x8, $outblu8x8, $outredblu8x8x2, $in16x8
	vmovn.u16	$outblu8x8, $in16x8		; gggbbbbb x 8
	vshrn.u16	$outred8x8, $in16x8, #8		; rrrrrggg x 8
	vshrn.u16	$outgrn8x8, $in16x8, #3		; ggggggbb x 8
	vshl.u8		$outblu8x8, $outblu8x8, #3	; bbbbb000 x 8
	vsri.u8		$outgrn8x8, $outgrn8x8, #6	; gggggggg x 8
	vsri.u8		$outredblu8x8x2, $outredblu8x8x2, #5
	MEND

;----------------------------------------------------------------------------
; RGB888toRGB565 converts a set of eight 24-bit RGB 8:8:8 pixels to
;  RGB 5:6:5.  Three Dn registers and a Qn register are passed in.  The Dn
;  registers contains the input pixels, while the Qn register will contain
;  the converted pixels.
;
; Arguments:
;  $out16x8 (Qn) - output: 8 16-bit RGB5:6:5 pixels
;  $inred8x8 (Dn) - input: 8 8-bit red components
;  $ingrn8x8 (Dn) - input: 8 8-bit green components
;  $inblu8x8 (Dn) - input: 8 8-bit blue components
;  $tmpgrn16x8 (Qn) - working: temporary register holding intermediate green values
;  $tmpblu16x8 (Qn) - working: temporary register holding intermediate blue values
	MACRO
	RGB888toRGB565	$out16x8, $inred8x8, $ingrn8x8, $inblu8x8, $tmpgrn16x8, $tmpblu16x8
	vshll.u8	$out16x8, $inred8x8, #8
	vshll.u8	$tmpgrn16x8, $ingrn8x8, #8
	vshll.u8	$tmpblu16x8, $inblu8x8, #8
	vsri.u16	$out16x8, $tmpgrn16x8, #5
	vsri.u16	$out16x8, $tmpblu16x8, #11
	MEND

;----------------------------------------------------------------------------
; RGB888toYUV422_PREP sets up the constants needed for the RGB888toYUV422
;  macro below.  q12-q15 are initialized.
;
; Arguments:
;  none
	MACRO
	RGB888toYUV422_PREP
	vmov.i8	d31, #66	; Kry = .257
	vmov.i8	d30, #129	; Kgy = .504
	vmov.i8	d29, #25	; Kby = .098
	vmov.i8	d28, #38	; -Kru = .148
	vmov.i8	d27, #74	; -Kgu = .291
	vmov.i8	d26, #112	; Kbu & Krv = .439
	vmov.i8	d25, #94	; -Kgv = .368
	vmov.i8	d24, #18	; -Kbv = .071
	vmov.i16	q11, #0x1000	; 16 * 256
	vmov.i16	q10, #0x8000	; 128 * 256
	MEND

;----------------------------------------------------------------------------
; RGB888toYUV422 converts 24-bit RGB pixels to YUV 4:2:2 pixels.  The input
;  should be separated into even and odd pixels as well as color components.
;  The output will have the even and odd Y values separated.  The output
;  registers should not conflict with input registers.
;
; Arguments:
;  $dstYe (Dn) - output: 8 even Y components
;  $dstU (Dn) - output: 8 U components
;  $dstYo (Dn) - output: 8 odd Y components
;  $dstV (Dn) - output: 8 V components
;  $srcRe (Dn) - input: 8 even red components
;  $srcRo (Dn) - input: 8 odd red components
;  $srcGe (Dn) - input: 8 even green components
;  $srcGo (Dn) - input: 8 odd green components
;  $srcBe (Dn) - input: 8 even blue components
;  $srcBo (Dn) - input: 8 odd blue components
;  $tmpYe (Qn) - working: temp reg for even Y values
;  $tmpYo (Qn) - working: temp reg for odd Y values
;  $tmpU (Qn) - working: temp reg for U values
;  $tmpV (Qn) - working: temp reg for V values
	MACRO
	RGB888toYUV422	$dstYe, $dstU, $dstYo, $dstV, $srcRe, $srcRo, $srcGe, $srcGo, $srcBe, $srcBo, $tmpYe, $tmpYo, $tmpU, $tmpV
	; recommended	d7,     d6,    d9,     d8,    d4,     d5,     d2,     d3,     d0,     d1,     q5,     q6,     q7,    q8
	vmull.u8	$tmpYe, $srcRe, d31	; even r * Kry
	vmull.u8	$tmpYo, $srcRo, d31	; odd r * Kry

	vmlal.u8	$tmpYe, $srcGe, d30	; even r * Kry + even g * Kgy
	vmlal.u8	$tmpYo, $srcGo, d30	; odd r * Kry + odd g * Kgy

	vmlal.u8	$tmpYe, $srcBe, d29	; even r * Kry + even g * Kgy + even b * Kby
	vmlal.u8	$tmpYo, $srcBo, d29	; odd r * Kry + odd g * Kgy + odd b * Kby

	vaddhn.u16	$dstYe, $tmpYe, q11	; even r * Kry + even g * Kgy + even b * Kby + 16 = even Y
	vaddhn.u16	$dstYo, $tmpYo, q11	; odd r * Kry + odd g * Kgy + odd b * Kby + 16 = odd Y

	vhadd.u8	$srcBe, $srcBe, $srcBo	; even b + odd b / 2
	vhadd.u8	$srcRe, $srcRe, $srcRo	; even r + odd r / 2
	vhadd.u8	$srcGe, $srcGe, $srcGe	; even g + odd g / 2

	vmull.u8	$tmpU, $srcBe, d26	; b * Kbu
	vmull.u8	$tmpV, $srcRe, d26	; r * Krv

	vmlsl.u8	$tmpU, $srcGe, d27	; b * Kbu - (g * -Kgu)
	vmlsl.u8	$tmpV, $srcGe, d25	; r * Krv - (g * -Kgv)

	vmlsl.u8	$tmpU, $srcRe, d28	; b * Kbu - (g * -Kgu) - (r * -Kru)
	vmlsl.u8	$tmpV, $srcBe, d24	; r * Krv - (g * -Kgv) - (b * -Kbv)

	vaddhn.u16	$dstU, $tmpU, q10	; r * Kru + g * Kgu + b * Kbu + 128 = U
	vaddhn.u16	$dstV, $tmpV, q10	; r * Krv + g * Kgv + b * Kbv + 128 = V
	MEND

;--------------------------------------------------------------------------
; YUV422toRGB888_PREP sets up the constants needed for the RGB888toYUV422
;  macro below.  q10-q15 are initialized.
;
; Arguments:
;  none
	MACRO
	YUV422toRGB888_PREP
	vmov.i8	d31, #16
	vmov.i8	d30, #128
	vmov.i8	d29, #74	; (1.164 * 64)
	vmov.i16	q13, #129	; (2.017 * 64)
	vmov.i16	q12, #0
	vmov.i16	q11, #52
	vsub.s16	q12, q12, q11	; (-0.813 * 64)
	vmov.i16	q11, #0
	vmov.i16	q10, #25
	vsub.s16	q11, q11, q10	; (-0.392 * 64)
	vmov.i16	q10, #102	; (1.596 * 64)
	MEND

;----------------------------------------------------------------------------
; YUV422toRGB888 converts YUV 4:2:2 pixels to 24-bit RGB.  The input Y
;  values should be separated into even and odd.  The output will have the
;  even and odd pixels grouped, as well as the color channels separated.
;  YUV422toRGB888_PRE should be called earlier in the function that uses
;  this macro to allow setting up the constants needed.  Note that the
;  constants use q10-q15 (d20-d31), so those registers should be preserved
;  for the call to this macro.
;
; Arguments:
;  $dstRe (Dn) - output: 8 even red components
;  $dstRo (Dn) - output: 8 odd red components
;  $dstGe (Dn) - output: 8 even green components
;  $dstGo (Dn) - output: 8 odd green components
;  $dstBe (Dn) - output: 8 even blue components
;  $dstBo (Dn) - output: 8 odd blue components
;  $srcYe (Dn) - input: 8 even Y components
;  $srcU (Dn) - input: 8 U components
;  $srcYo (Dn) - input: 8 odd Y components
;  $srcV (Dn) - input: 8 V components
;  $tmpYe (Qn) - working: temp reg for even Y values
;  $tmpYo (Qn) - working: temp reg for odd Y values
;  $tmpU (Qn) - working: temp reg for U values
;  $tmpV (Qn) - working: temp reg for V values
;  $tmpR (Qn) - working: temp reg for red
;  $tmpG (Qn) - working: temp reg for green
;  $tmpB (Qn) - working: temp reg for blue
	MACRO
	; source and destination registers can duplicate
	YUV422toRGB888	$dstRe, $dstRo, $dstGe, $dstGo, $dstBe, $dstBo, $srcYe, $srcU, $srcYo, $srcV, $tmpYe, $tmpYo, $tmpU, $tmpV, $tmpR, $tmpG, $tmpB
	;recommended	d4,     d5,     d2,     d3,     d0,     d1,     d1,     d0,    d3,     d2,    q3,     q4,     q5,    q6,    q7,    q8,    q9
; vmov r0, r1, $srcYe
; vmov r2, r3, d31
	vqsub.u8	$srcYe, $srcYe, d31		; even y - 16
; vmov r0, r1, $srcYe
; vmov r0, r1, $srcYo
	vqsub.u8	$srcYo, $srcYo, d31		; odd y - 16
; vmov r0, r1, $srcYo
; vmov r0, r1, $srcU
; vmov r2, r3, d30
 	vsub.u8	$srcU, $srcU, d30		; u - 128
; vmov r0, r1, $srcU
; vmov r0, r1, $srcV
 	vsub.u8	$srcV, $srcV, d30		; v - 128
; vmov r0, r1, $srcV
 	vmovl.s8	$tmpU, $srcU		; extend to 16 bits
; vmov r0, r1, d10 ; 1/2 $tmpU
; vmov r2, r3, d11 ; 1/2 $tmpU
 	vmovl.s8	$tmpV, $srcV		; extend to 16 bits
; vmov r0, r1, d12 ; 1/2 $tmpV
; vmov r2, r3, d13 ; 1/2 $tmpV

; vmov r0, r1, d22 ; 1/2 q11
; vmov r2, r3, d23 ; 1/2 q11
	vmul.s16	$tmpG, $tmpU, q11	; Guv = (u - 128) * -0.391
; vmov r0, r1, d16 ; 1/2 $tmpG
; vmov r2, r3, d17 ; 1/2 $tmpG
; vmov r0, r1, d24 ; 1/2 q12
; vmov r2, r3, d25 ; 1/2 q12
	vmla.s16	$tmpG, $tmpV, q12	;       + (v - 128) * -0.813	; not interleaved to save a register
; vmov r0, r1, d16 ; 1/2 $tmpG
; vmov r2, r3, d17 ; 1/2 $tmpG
; vmov r0, r1, d26 ; 1/2 q13
; vmov r2, r3, d27 ; 1/2 q13
	vmul.s16	$tmpU, $tmpU, q13	; Buv = (u - 128) * 2.018
; vmov r0, r1, d10 ; 1/2 $tmpU
; vmov r2, r3, d11 ; 1/2 $tmpU
; vmov r0, r1, d20 ; 1/2 q10
; vmov r2, r3, d21 ; 1/2 q10
	vmul.s16	$tmpV, $tmpV, q10	; Ruv = (v - 128) * 1.596
; vmov r0, r1, d12 ; 1/2 $tmpV
; vmov r2, r3, d13 ; 1/2 $tmpV

; vmov r0, r1, d29
	vmull.u8	$tmpYe, $srcYe, d29	; (even y - 16) * 1.164
; vmov r0, r1, d6 ; 1/2 $tmpYe
; vmov r2, r3, d7 ; 1/2 $tmpYe
	vmull.u8	$tmpYo, $srcYo, d29	; (odd y - 16) * 1.164
; vmov r0, r1, d8 ; 1/2 $tmpYo
; vmov r2, r3, d9 ; 1/2 $tmpYo

	vadd.s16	$tmpR, $tmpV, $tmpYe	; Re = (even y - 16) * 1.164 + (v - 128) * 1.596
; vmov r0, r1, d14 ; 1/2 $tmpR
; vmov r2, r3, d15 ; 1/2 $tmpR
	vadd.s16	$tmpB, $tmpU, $tmpYe	; Be = (even y - 16) * 1.164 + (u - 128) * 2.018
; vmov r0, r1, d18 ; 1/2 $tmpB
; vmov r2, r3, d19 ; 1/2 $tmpB
	vadd.s16	$tmpYe, $tmpG, $tmpYe	; Ge = (even y - 16) * 1.164 + (u - 128) * -0.391 + (v - 128) * -0.813
; vmov r0, r1, d6 ; 1/2 $tmpYe
; vmov r2, r3, d7 ; 1/2 $tmpYe

	vadd.s16	$tmpV, $tmpV, $tmpYo	; Ro = (odd y - 16) * 1.164 + (v - 128) * 1.596
; vmov r0, r1, d12 ; 1/2 $tmpV
; vmov r2, r3, d13 ; 1/2 $tmpV
	vadd.s16	$tmpU, $tmpU, $tmpYo	; Bo = (odd y - 16) * 1.164 + (u - 128) * 2.018
; vmov r0, r1, d10 ; 1/2 $tmpU
; vmov r2, r3, d11 ; 1/2 $tmpU
	vadd.s16	$tmpG, $tmpG, $tmpYo	; Go = (odd y - 16) * 1.164 + (u - 128) * -0.391 + (v - 128) * -0.813
; vmov r0, r1, d16 ; 1/2 $tmpG
; vmov r2, r3, d17 ; 1/2 $tmpG

	vqshrun.s16	$dstRe, $tmpR, #6	; Re
	vqshrun.s16	$dstRo, $tmpV, #6	; Ro
; vmov r0, r1, $dstRe
; vmov r2, r3, $dstRo
	vqshrun.s16	$dstGe, $tmpYe, #6	; Ge
	vqshrun.s16	$dstGo, $tmpG, #6	; Go
; vmov r0, r1, $dstGe
; vmov r2, r3, $dstGo
	vqshrun.s16	$dstBe, $tmpB, #6	; Be
	vqshrun.s16	$dstBo, $tmpU, #6	; Bo
; vmov r0, r1, $dstBe
; vmov r2, r3, $dstBo
	MEND

;----------------------------------------------------------------------------
; BLEND blends one set of eight ARGB 8:8:8:8 pixels into another set of eight
;  ARGB 8:8:8:8 pixels.  Note that this macro DOES NOT calculate the resulting
;  alpha of the destination (and alphas are not modified).  For alpha
;  calculation use BLENDWITHALPHA instead.
;
; Co = AaCa + AbCb
;
; $dst* are input and output registers (except alpha)
; $src* are input and working registers
; $tmp* are working registers, and should not conflict with other arguments
;
; Sample 1:  Premultiplied ARGB blended to RGB
;	vmov.u8	srcalf8x8, #0xFF	; 1.0 used for source (created outside octet loop)
; ...
;	vmvn.u8	dstalf8x8, srcalf8x8	; (1.0 - srcalf) used for destination
;	BLEND	dstred8x8, dstgrn8x8, dstblu8x8, dstalf8x8, srcred8x8, srcgrn8x8, srcblu8x8, srcalf8x8, qa, qb, qc
;
; Sample 2:  Non-premultiplied RGB blended to RGB using separate 8-bit alpha
;					; mskalf used for source
;	vmvn.u8	dstalf8x8, mskalf8x8	; (1.0 - mskalf) used for destination
;	BLEND	dstred8x8, dstgrn8x8, dstblu8x8, dstalf8x8, srcred8x8, srcgrn8x8, srcblu8x8, mskalf8x8, qa, qb, qc
;
	MACRO
	BLEND	$dstred8x8, $dstgrn8x8, $dstblu8x8, $dstalf8x8, $srcred8x8, $srcgrn8x8, $srcblu8x8, $srcalf8x8, $tmpred16x8, $tmpgrn16x8, $tmpblu16x8, $tmp2red16x8, $tmp2grn16x8, $tmp2blu16x8
; Multiply destination by destination alpha
	vmull.u8	$tmpred16x8, $dstred8x8, $dstalf8x8
	vmull.u8	$tmpgrn16x8, $dstgrn8x8, $dstalf8x8
	vmull.u8	$tmpblu16x8, $dstblu8x8, $dstalf8x8
; Multiply source by source alpha and add to destination
	vmlal.u8	$tmpred16x8, $srcred8x8, $srcalf8x8
	vmlal.u8	$tmpgrn16x8, $srcgrn8x8, $srcalf8x8
	vmlal.u8	$tmpblu16x8, $srcblu8x8, $srcalf8x8
; Get value / 256
	vrshr.u16	$tmp2red16x8, $tmpred16x8, #8
	vrshr.u16	$tmp2grn16x8, $tmpgrn16x8, #8
	vrshr.u16	$tmp2blu16x8, $tmpblu16x8, #8	
; Adjust value by 257/256
	vraddhn.u16	$dstred8x8, $tmp2red16x8, $tmpred16x8
	vraddhn.u16	$dstgrn8x8, $tmp2grn16x8, $tmpgrn16x8
	vraddhn.u16	$dstblu8x8, $tmp2blu16x8, $tmpblu16x8
	MEND

;
; BLENDWITHALPHA is alpha blending with modification of the destination alpha.
;
; Co = Ca + Cb(1-Aa)
; Ao = Aa + Ab(1-Aa)
;
	MACRO
	BLENDWITHALPHA	$dstred8x8, $dstgrn8x8, $dstblu8x8, $dstalf8x8, $srcred8x8, $srcgrn8x8, $srcblu8x8, $srcalf8x8, $negsrcalf8x8, $tmpred16x8, $tmpgrn16x8, $tmpblu16x8, $tmpalf16x8, $tmp2red16x8, $tmp2grn16x8, $tmp2blu16x8, $tmp2alf16x8
; Get (1 - source alpha)
	vmvn.u8		$negsrcalf8x8, $srcalf8x8
; Multiply destination by (1 - source alpha)
	vmull.u8	$tmpred16x8, $dstred8x8, $negsrcalf8x8
	vmull.u8	$tmpgrn16x8, $dstgrn8x8, $negsrcalf8x8
	vmull.u8	$tmpblu16x8, $dstblu8x8, $negsrcalf8x8
	vmull.u8	$tmpalf16x8, $dstalf8x8, $negsrcalf8x8
; Multiply results by 257/256
	vrshr.u16	$tmp2red16x8, $tmpred16x8, #8
	vrshr.u16	$tmp2grn16x8, $tmpgrn16x8, #8
	vrshr.u16	$tmp2blu16x8, $tmpblu16x8, #8
	vrshr.u16	$tmp2alf16x8, $tmpalf16x8, #8
; Shrink to 8 bits and round
	vraddhn.u16	$dstred8x8, $tmp2red16x8, $tmpred16x8
	vraddhn.u16	$dstgrn8x8, $tmp2grn16x8, $tmpgrn16x8
	vraddhn.u16	$dstblu8x8, $tmp2blu16x8, $tmpblu16x8
	vraddhn.u16	$dstalf8x8, $tmp2alf16x8, $tmpalf16x8
; Add premultiplied source
	vqadd.u8	$dstred8x8, $dstred8x8, $srcred8x8
	vqadd.u8	$dstgrn8x8, $dstgrn8x8, $srcgrn8x8
	vqadd.u8	$dstblu8x8, $dstblu8x8, $srcblu8x8
	vqadd.u8	$dstalf8x8, $dstalf8x8, $srcalf8x8
	MEND

;
; PREMULTTOPREMULTBLEND is alpha blending with modification of the destination alpha.
;
; Co = Ca + Cb(1-Aa)
; Ao = Aa + Ab(1-Aa)
;
	MACRO
	PREMULTTOPREMULTBLEND	$dstred8x8, $dstgrn8x8, $dstblu8x8, $dstalf8x8, $srcred8x8, $srcgrn8x8, $srcblu8x8, $srcalf8x8, $negsrcalf8x8, $tmpred16x8, $tmpgrn16x8, $tmpblu16x8, $tmpalf16x8, $tmp2red16x8, $tmp2grn16x8, $tmp2blu16x8, $tmp2alf16x8
; Get (1 - source alpha)
	vmvn.u8		$negsrcalf8x8, $srcalf8x8
; Multiply destination by (1 - source alpha)
	vmull.u8	$tmpred16x8, $dstred8x8, $negsrcalf8x8
	vmull.u8	$tmpgrn16x8, $dstgrn8x8, $negsrcalf8x8
	vmull.u8	$tmpblu16x8, $dstblu8x8, $negsrcalf8x8
	vmull.u8	$tmpalf16x8, $dstalf8x8, $negsrcalf8x8
; Multiply results by 257/256
	vrshr.u16	$tmp2red16x8, $tmpred16x8, #8
	vrshr.u16	$tmp2grn16x8, $tmpgrn16x8, #8
	vrshr.u16	$tmp2blu16x8, $tmpblu16x8, #8
	vrshr.u16	$tmp2alf16x8, $tmpalf16x8, #8
; Shrink to 8 bits and round
	vraddhn.u16	$dstred8x8, $tmp2red16x8, $tmpred16x8
	vraddhn.u16	$dstgrn8x8, $tmp2grn16x8, $tmpgrn16x8
	vraddhn.u16	$dstblu8x8, $tmp2blu16x8, $tmpblu16x8
	vraddhn.u16	$dstalf8x8, $tmp2alf16x8, $tmpalf16x8
; Add premultiplied source
	vqadd.u8	$dstred8x8, $dstred8x8, $srcred8x8
	vqadd.u8	$dstgrn8x8, $dstgrn8x8, $srcgrn8x8
	vqadd.u8	$dstblu8x8, $dstblu8x8, $srcblu8x8
	vqadd.u8	$dstalf8x8, $dstalf8x8, $srcalf8x8
	MEND


;
; NONPREMULTTOPREMULTBLEND is alpha blending with modification of the destination alpha.
;
; Co = AaCa + Cb(1-Aa)
; Ao = Aa + Ab(1-Aa)
;
	MACRO
	NONPREMULTTOPREMULTBLEND	$dstred8x8, $dstgrn8x8, $dstblu8x8, $dstalf8x8, $srcred8x8, $srcgrn8x8, $srcblu8x8, $srcalf8x8, $negsrcalf8x8, $tmpred16x8, $tmpgrn16x8, $tmpblu16x8, $tmpalf16x8, $tmp2red16x8, $tmp2grn16x8, $tmp2blu16x8, $tmp2alf16x8
; Get (1 - source alpha)
	vmvn.u8		$negsrcalf8x8, $srcalf8x8
; Multiply source by source alpha
	vmull.u8	$tmpred16x8, $srcred8x8, $srcalf8x8
	vmull.u8	$tmpgrn16x8, $srcgrn8x8, $srcalf8x8
	vmull.u8	$tmpblu16x8, $srcblu8x8, $srcalf8x8
; Add in destination multiplied by (1 - source alpha)
	vmlal.u8	$tmpred16x8, $dstred8x8, $negsrcalf8x8
	vmlal.u8	$tmpgrn16x8, $dstgrn8x8, $negsrcalf8x8
	vmlal.u8	$tmpblu16x8, $dstblu8x8, $negsrcalf8x8
	vmull.u8	$tmpalf16x8, $dstalf8x8, $negsrcalf8x8
; Multiply results by 257/256
	vrshr.u16	$tmp2red16x8, $tmpred16x8, #8
	vrshr.u16	$tmp2grn16x8, $tmpgrn16x8, #8
	vrshr.u16	$tmp2blu16x8, $tmpblu16x8, #8
	vrshr.u16	$tmp2alf16x8, $tmpalf16x8, #8
; Shrink to 8 bits and round
	vraddhn.u16	$dstred8x8, $tmp2red16x8, $tmpred16x8
	vraddhn.u16	$dstgrn8x8, $tmp2grn16x8, $tmpgrn16x8
	vraddhn.u16	$dstblu8x8, $tmp2blu16x8, $tmpblu16x8
	vraddhn.u16	$dstalf8x8, $tmp2alf16x8, $tmpalf16x8
	MEND


;----------------------------------------------------------------------------
; PRELOADLINEnbpp are macros to ease use of the PreloadLine() function.
;
	MACRO
	PRELOADLINE1bpp	$ptr, $pels
	mov		r0, $ptr
	mov		r1, $pels, lsr #3
	bl		PreloadLine
	MEND
	
;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	MACRO
	PRELOADLINE8bpp	$ptr, $pels
	mov		r0, $ptr
	mov		r1, $pels
	bl		PreloadLine
	MEND

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	MACRO
	PRELOADLINE16bpp	$ptr, $pels
	mov		r0, $ptr
	mov		r1, $pels, lsl #1
	bl		PreloadLine
	MEND

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	MACRO
	PRELOADLINE24bpp	$ptr, $pels
	mov		r0, $ptr
	add		r1, $pels, $pels, lsl #2
	bl		PreloadLine
	MEND

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	MACRO
	PRELOADLINE32bpp	$ptr, $pels
	mov		r0, $ptr
	mov		r1, $pels, lsl #2
	bl		PreloadLine
	MEND

	END

