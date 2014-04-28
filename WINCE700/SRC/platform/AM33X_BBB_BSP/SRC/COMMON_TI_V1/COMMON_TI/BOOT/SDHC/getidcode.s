;
; Copyright (c) Microsoft Corporation.  All rights reserved.
;
;
; Use of this source code is subject to the terms of the Microsoft end-user
; license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
; If you did not accept the terms of the EULA, you are not authorized to use
; this source code. For a copy of the EULA, please see the LICENSE.RTF on your
; install media.
;
;-------------------------------------------------------------------------------
;
;  File: getidcode.s
;
;  This file implement OALGetIdCode function. This implementaion should
;  work on most ARM based SoC.
;
        INCLUDE kxarm.h
        INCLUDE armmacros.s

        TEXTAREA

;-------------------------------------------------------------------------------
;
;  Function:  OALGetSiliconIdCode
;
;  The Silicon ID Code value will be returned in r0.
;
        LEAF_ENTRY OALGetSiliconIdCode

        mrc     p15, 1, r0, c0, c0, 7

        RETURN

        END

