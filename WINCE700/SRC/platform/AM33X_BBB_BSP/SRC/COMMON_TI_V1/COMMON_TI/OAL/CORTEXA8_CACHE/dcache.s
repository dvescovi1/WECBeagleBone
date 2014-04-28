;
; Copyright (c) Microsoft Corporation.  All rights reserved.
;
;
; Use of this sample source code is subject to the terms of the Microsoft
; license agreement under which you licensed this sample source code. If
; you did not accept the terms of the license agreement, you are not
; authorized to use this sample source code. For the terms of the license,
; please see the license agreement between you and Microsoft or, if applicable,
; see the LICENSE.RTF on your install media or the root of your tools installation.
; THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
;
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
;-------------------------------------------------------------------------------
;
;  File:  dcache.s
;
;
        INCLUDE kxarm.h
        INCLUDE armmacros.s
        INCLUDE oal_cache.inc

        IMPORT g_oalCacheInfo

        TEXTAREA


;-------------------------------------------------------------------------------
;
;  Function:  OALCleanDCache
;
;  Entire cache operatations must be performed using set/way method operation at
;  first level of the cache.
;
        LEAF_ENTRY OALCleanDCache

        stmfd   sp!, {r4,r5}                    ; store off registers to stack

        ldr     r0, =g_oalCacheInfo

        ; first we need to get index bit
        ldr     r1, [r0, #L1DNumWays]
        mov     r5, #32
10      movs    r1, r1, lsr #1
        beq     %F20
        sub     r5, r5, #1
        b       %B10
20

        ldr     r1, [r0, #L1DNumWays]
        sub     r1, r1, #1
        mov     r1, r1, lsl r5
        mov     r2, #1
        mov     r2, r2, lsl r5

        ldr     r3, [r0, #L1DLineSize]
30      ldr     r4, [r0, #L1DSetsPerWay]

        mov     r5, r1

40      mcr     p15, 0, r5, c7, c10, 2          ; clean by set/way

        ; add the set index
        add     r5, r5, r3

        ; decrement the set number
        subs    r4, r4, #1
        bgt     %b40

        ; test last index
        cmp     r1, #0
        beq     %f50

        ; decrement index
        sub     r1, r1, r2
        b       %b30

50      mov     r0, #0
        mcr     p15, 0, r0, c7, c10, 4          ; data sync barrier operation

        ldmfd   sp!, {r4,r5}                    ; restore registers

        RETURN




;-------------------------------------------------------------------------------
;
;  Function:  OALCleanL2Cache
;
;  Entire cache operatations must be performed using set/way method operation at
;  second level of the cache.
;
        LEAF_ENTRY OALCleanL2Cache

        stmfd   sp!, {r4,r5}                    ; store off registers to stack

        ldr     r0, =g_oalCacheInfo

        ; first we need to get index bit
        ldr     r1, [r0, #L2DNumWays]
        mov     r5, #32
10      movs    r1, r1, lsr #1
        beq     %F20
        sub     r5, r5, #1
        b       %B10
20

        ldr     r1, [r0, #L2DNumWays]
        sub     r1, r1, #1
        mov     r1, r1, lsl r5
        mov     r2, #1
        mov     r2, r2, lsl r5

        ldr     r3, [r0, #L2DLineSize]
30      ldr     r4, [r0, #L2DSetsPerWay]

        orr     r5, r1, #2                     ; specify the cache level: L2

40      mcr     p15, 0, r5, c7, c10, 2         ; clean by set/way

        ; add the set index
        add     r5, r5, r3

        ; decrement the set number
        subs    r4, r4, #1
        bgt     %b40

        ; test last index
        cmp     r1, #0
        beq     %f50

        ; decrement index
        sub     r1, r1, r2
        b       %b30

50      mov     r0, #0
        mcr     p15, 0, r0, c7, c10, 4          ; data sync barrier operation

        ldmfd   sp!, {r4,r5}                    ; restore registers

        RETURN




;-------------------------------------------------------------------------------
;
;  Function:  OALCleanDCacheLines
;
        LEAF_ENTRY OALCleanDCacheLines

        ldr     r2, =g_oalCacheInfo
        ldr     r3, [r2, #L1DLineSize]

10      mcr     p15, 0, r0, c7, c10, 1          ; clean entry
        add     r0, r0, r3                      ; move to next entry
        subs    r1, r1, r3
        bgt     %b10                            ; loop while > 0 bytes left

        mov     r2, #0
        mcr     p15, 0, r2, c7, c10, 4          ; data sync barrier operation

        RETURN

;-------------------------------------------------------------------------------
;
;  Function:  OALCleanL2CacheLines
;
        LEAF_ENTRY OALCleanL2CacheLines

        ldr     r2, =g_oalCacheInfo
        ldr     r3, [r2, #L2DLineSize]

10      mcr     p15, 0, r0, c7, c10, 1          ; clean entry
        add     r0, r0, r3                      ; move to next entry
        subs    r1, r1, r3
        bgt     %b10                            ; loop while > 0 bytes left

        mov     r2, #0
        mcr     p15, 0, r2, c7, c10, 4          ; data sync barrier operation

        RETURN


;-------------------------------------------------------------------------------
;
;  Function:  OALFlushDCache
;
;  Entire cache operatations must be performed using set/way method operation at
;  fisrt level of the cache.
;
        LEAF_ENTRY OALFlushDCache

        stmfd   sp!, {r4,r5}                    ; store off registers to stack

        ldr     r0, =g_oalCacheInfo

        ; first we need to get index bit
        ldr     r1, [r0, #L1DNumWays]
        mov     r5, #32
10      movs    r1, r1, lsr #1
        beq     %F20
        sub     r5, r5, #1
        b       %B10
20

        ldr     r1, [r0, #L1DNumWays]
        sub     r1, r1, #1
        mov     r1, r1, lsl r5
        mov     r2, #1
        mov     r2, r2, lsl r5

        ldr     r3, [r0, #L1DLineSize]
30      ldr     r4, [r0, #L1DSetsPerWay]

        mov     r5, r1

40      mcr     p15, 0, r5, c7, c14, 2          ; clean and invalidate by set/way

        ; add the set index
        add     r5, r5, r3

        ; decrement the set number
        subs    r4, r4, #1
        bgt     %b40

        ; test last index
        cmp     r1, #0
        beq     %f50

        ; decrement index
        sub     r1, r1, r2
        b       %b30

50      mov     r0, #0
        mcr     p15, 0, r0, c7, c10, 4          ; data sync barrier operation

        ldmfd   sp!, {r4,r5}                    ; restore registers

        RETURN



;-------------------------------------------------------------------------------
;
;  Function:  OALFlushL2Cache
;
;  Entire cache operatations must be performed using set/way method operation at
;  second level of the cache.
;
        LEAF_ENTRY OALFlushL2Cache

        stmfd   sp!, {r4,r5}                    ; store off registers to stack

        ldr     r0, =g_oalCacheInfo

        ; first we need to get index bit
        ldr     r1, [r0, #L2DNumWays]
        mov     r5, #32
10      movs    r1, r1, lsr #1
        beq     %F20
        sub     r5, r5, #1
        b       %B10
20

        ldr     r1, [r0, #L2DNumWays]
        sub     r1, r1, #1
        mov     r1, r1, lsl r5
        mov     r2, #1
        mov     r2, r2, lsl r5

        ldr     r3, [r0, #L2DLineSize]
30      ldr     r4, [r0, #L2DSetsPerWay]

        orr     r5, r1, #2                      ; specify the cache level: L2

40      mcr     p15, 0, r5, c7, c14, 2          ; clean and invalidate by set/way

        ; add the set index
        add     r5, r5, r3

        ; decrement the set number
        subs    r4, r4, #1
        bgt     %b40

        ; test last index
        cmp     r1, #0
        beq     %f50

        ; decrement index
        sub     r1, r1, r2
        b       %b30

50      mov     r0, #0
        mcr     p15, 0, r0, c7, c10, 4          ; data sync barrier operation


        ldmfd   sp!, {r4,r5}                    ; restore registers

        RETURN


;-------------------------------------------------------------------------------
;
;  Function:  OALFlushDCacheLines
;
        LEAF_ENTRY OALFlushDCacheLines

        ldr     r2, =g_oalCacheInfo
        ldr     r3, [r2, #L1DLineSize]

10      mcr     p15, 0, r0, c7, c14, 1          ; clean and invalidate entry
        add     r0, r0, r3                      ; move to next
        subs    r1, r1, r3
        bgt     %b10                            ; loop while > 0 bytes left

        mov     r2, #0
        mcr     p15, 0, r2, c7, c10, 4          ; data sync barrier operation

        RETURN


;-------------------------------------------------------------------------------
;  Function:  OALInvalidateDCacheLines
;
        LEAF_ENTRY OALInvalidateDCacheLines

        ldr     r2, =g_oalCacheInfo
        ldr     r3, [r2, #L1DLineSize]

10      mcr     p15, 0, r0, c7, c6, 1           ; invalidate entry
        add     r0, r0, r3                      ; move to next
        subs    r1, r1, r3
        bgt     %b10                            ; loop while > 0 bytes left

        mov     r2, #0
        mcr     p15, 0, r2, c7, c10, 4          ; data sync barrier operation

        RETURN


        END
