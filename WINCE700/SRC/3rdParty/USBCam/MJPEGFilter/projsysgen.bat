@REM -------------------------------------------------------------------------
@REM  <copyright file="projsysgen.bat" company="Microsoft">
@REM   Copyright (c) Microsoft Corporation.  All rights reserved.
@REM 
@REM     The use and distribution terms for this software are covered by the
@REM     Microsoft Limited Permissive License (Ms-LPL) 
@REM     which can be found in the file MS-LPL.txt at the root of this distribution.
@REM     By using this software in any fashion, you are agreeing to be bound by
@REM     the terms of this license.
@REM 
@REM     THE SOFTWARE IS LICENSED "AS-IS" WITH NO WARRANTIES OR INDEMNITIES. 
@REM 
@REM     You must not remove this notice, or any other, from this software.
@REM  </copyright>
@REM  
@REM  <summary>
@REM     MJPEG DirectShow Filter for use with USB camera driver for Windows Embedded CE 6.0
@REM  </summary>
@REM -------------------------------------------------------------------------
@REM ======================================================================
@REM  USB camera driver for Windows Embedded CE 6.0
@REM ======================================================================


if /i not "%1"=="preproc" goto :Not_Preproc
    goto :EOF
:Not_Preproc
if /i not "%1"=="pass1" goto :Not_Pass1

    REM ==============================================================================================
    REM
    REM Required SYSGENs for jpeg2rgb filter
    REM
    REM ==============================================================================================
        set SYSGEN_DCOM_STG=1
        set SYSGEN_IMAGING=1
        set SYSGEN_IMAGING_BMP_DECODE=1
        set SYSGEN_IMAGING_BMP_ENCODE=1
        set SYSGEN_IMAGING_JPG_DECODE=1
        set SYSGEN_IMAGING_JPG_ENCODE=1
		set SYSGEN_DSHOW=1
        set SYSGEN_DSHOW_DISPLAY=1
        set SYSGEN_DSHOW_VIDREND=1
    goto :EOF
:Not_Pass1
if /i not "%1"=="pass2" goto :Not_Pass2
    goto :EOF
:Not_Pass2
if /i not "%1"=="report" goto :Not_Report
    goto :EOF
:Not_Report
echo %0 Invalid parameter %1
