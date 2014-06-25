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
;  File:  icache.s
;
;
        INCLUDE kxarm.h
        INCLUDE armmacros.s
        INCLUDE oal_cache.inc

        IMPORT g_oalCacheInfo

        TEXTAREA

;-------------------------------------------------------------------------------
;
;  Function:  OALFlushICache
;
        LEAF_ENTRY OALFlushICache

        mov     r0, #0
        mcr     p15, 0, r0, c7, c5, 0       ; invalidate all instruction caches

        mov     r2, #0
        mcr     p15, 0, r2, c7, c5, 6       ; flush entire branch target cache
        mcr     p15, 0, r2, c7, c10, 4      ; data sync barrier operation
        mcr     p15, 0, r2, c7, c5, 4       ; instr sync barrier operation

        RETURN
        ENTRY_END OALFlushICache

;-------------------------------------------------------------------------------
;
;  Function:  OALFlushICacheLines
;
        LEAF_ENTRY OALFlushICacheLines

        ldr     r2, =g_oalCacheInfo
        ldr     r3, [r2, #L1ILineSize]

10      mcr     p15, 0, r0, c7, c5,  1      ; invalidate line from icache
        add     r0, r0, r3                  ; move to next entry
        subs    r1, r1, r3
        bgt     %b10                        ; loop while > 0 bytes left

        mov     r2, #0
        mcr     p15, 0, r2, c7, c5, 6       ; flush entire branch target cache
        mcr     p15, 0, r2, c7, c10, 4      ; data sync barrier operation
        mcr     p15, 0, r2, c7, c5, 4       ; instr sync barrier operation

        RETURN
        ENTRY_END OALFlushICacheLines

        END
