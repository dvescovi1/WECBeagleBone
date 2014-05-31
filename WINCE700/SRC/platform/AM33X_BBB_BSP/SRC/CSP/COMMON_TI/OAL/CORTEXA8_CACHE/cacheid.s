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
;  File:  cacheid.s
;
;
        INCLUDE kxarm.h
        INCLUDE armmacros.s

        TEXTAREA

;-------------------------------------------------------------------------------
;
;  Function:  OALGetFCSE
;
;  Get FCSE register value
;
        LEAF_ENTRY OALGetFCSE

        mov     r0, #0
        mrc     p15, 0, r0, c13, c0, 0   ; read FCSE

        RETURN

;-------------------------------------------------------------------------------
;
;  Function:  OALGetContextID
;
;  Get ContextID register value
;
        LEAF_ENTRY OALGetContextID

        mov     r0, #0
        mrc     p15, 0, r0, c13, c0, 1   ; read context ID

        RETURN

;-------------------------------------------------------------------------------
;
;  Function:  OALGetCacheType
;
;  Get Cache Type register value
;
        LEAF_ENTRY OALGetCacheType

        mov     r0, #0
        mrc     p15, 0, r0, c0, c0, 1   ; read cache type reg

        RETURN

;-------------------------------------------------------------------------------
;
;  Function:  OALGetCacheLevelID
;
;  Get Cache Level ID register value
;
        LEAF_ENTRY OALGetCacheLevelID

        mov     r0, #0
        mrc     p15, 1, r0, c0, c0, 1   ; read cache level ID reg

        RETURN

;-------------------------------------------------------------------------------
;
;  Function:  OALGetCacheSizeID
;
;  Get Cache size and attributes for the selected cache level
;
        LEAF_ENTRY OALGetCacheSizeID

        and     r0, r0, #0x0f
        mov     r1, #0
        mcr     p15, 2, r0, c0, c0, 0   ; write the Cache Size selection register
        mcr     p15, 0, r1, c7, c5, 4   ; PrefetchFlush to sync the change to the CacheSizeID reg

        mov     r0, #0
        mrc     p15, 1, r0, c0, c0, 0   ; reads current Cache Size ID register
        
        RETURN

;-------------------------------------------------------------------------------

        END

        
