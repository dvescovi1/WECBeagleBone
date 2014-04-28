//-------------------------------------------------------------------------
// <copyright file="debug.cpp" company="Microsoft">
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//    The use and distribution terms for this software are covered by the
//    Microsoft Limited Permissive License (Ms-LPL) 
//    which can be found in the file MS-LPL.txt at the root of this distribution.
//    By using this software in any fashion, you are agreeing to be bound by
//    the terms of this license.
//
//    THE SOFTWARE IS LICENSED "AS-IS" WITH NO WARRANTIES OR INDEMNITIES. 
//
//    You must not remove this notice, or any other, from this software.
// </copyright>
// 
// <summary>
//    MJPEG DirectShow Filter for use with USB camera driver for Windows 
//    Embedded CE 6.0
// </summary>
//-------------------------------------------------------------------------
//======================================================================
// USB camera driver for Windows Embedded CE 6.0
//======================================================================

#include "debug.h"

#ifdef DEBUG
DBGPARAM dpCurSettings =
{
    TEXT("jpegfilter"),
    {
        TEXT("Errors"),         \
        TEXT("Warnings"),       \
        TEXT("Info (min)"),     \
        TEXT("Info (max)"),     \
        TEXT("Initialize"),     \
        TEXT("Enter,Exit"),     \
        TEXT("Timing"),         \
        TEXT("Memory"),         \
        TEXT("Locking"),        \
        TEXT("Media Types"),    \
        TEXT("Undefined"),      \
        TEXT("RefCount"),       \
        TEXT("Performance"),    \
        TEXT("CELOG"),          \
        TEXT("Temporary tests"),\
        TEXT("Undefined")
    },
    ZONEMASK_ERROR | ZONEMASK_WARNING | ZONEMASK_INIT
};
#endif // DEBUG
