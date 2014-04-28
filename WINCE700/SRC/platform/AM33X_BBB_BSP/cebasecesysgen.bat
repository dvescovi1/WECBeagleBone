@REM
@REM Copyright (c) Microsoft Corporation.  All rights reserved.
@REM
@REM
@REM Use of this sample source code is subject to the terms of the Microsoft
@REM license agreement under which you licensed this sample source code. If
@REM you did not accept the terms of the license agreement, you are not
@REM authorized to use this sample source code. For the terms of the license,
@REM please see the license agreement between you and Microsoft or, if applicable,
@REM see the LICENSE.RTF on your install media or the root of your tools installation.
@REM THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
@REM
@REM Use of this source code is subject to the terms of the Microsoft end-user
@REM license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
@REM If you did not accept the terms of the EULA, you are not authorized to use
@REM this source code. For a copy of the EULA, please see the LICENSE.RTF on your
@REM install media.
@REM
REM Platform specific sysgen settings

if /i not "%1"=="preproc" goto :Not_Preproc
    goto :EOF
:Not_Preproc



REM Pass1. Add new platform specific settings here.
if /i not "%1"=="pass1" goto :Not_Pass1

    rem  set __SYSGEN_DEVLOAD=1

    goto :EOF

:Not_Pass1

if /i not "%1"=="pass2" goto :Not_Pass2
    goto :EOF
:Not_Pass2

if /i not "%1"=="report" goto :Not_Report
    goto :EOF
:Not_Report


