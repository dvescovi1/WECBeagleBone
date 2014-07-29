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
;  Function:  OALGetIdCode
;
;  The ID Code value will be returned in r0.
;
        LEAF_ENTRY OALGetIdCode

        mrc     p15, 0, r0, c0, c0, 0

        RETURN

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

