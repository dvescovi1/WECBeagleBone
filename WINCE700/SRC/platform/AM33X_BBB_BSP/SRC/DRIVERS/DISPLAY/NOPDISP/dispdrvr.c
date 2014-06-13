//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this sample source code is subject to the terms of the Microsoft
// license agreement under which you licensed this sample source code. If
// you did not accept the terms of the license agreement, you are not
// authorized to use this sample source code. For the terms of the license,
// please see the license agreement between you and Microsoft or, if applicable,
// see the LICENSE.RTF on your install media or the root of your tools installation.
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
//------------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//  
//------------------------------------------------------------------------------
//
//  Stub display driver. Does nothing.
//      
//------------------------------------------------------------------------------
#include <windows.h>
#if  (_WINCEOSVER<=700)
#include <types.h>
#else
#include <cetypes.h>
#endif
#include <memory.h>

// Zero tells GDI the physical frame buffer is not in DIB format
void * DispDrvrPhysicalFrameBuffer = (void *)0;    
int DispDrvr_cxScreen = 800;
int DispDrvr_cyScreen = 600;
int DispDrvr_HORZSIZE = 64;   // Width, in millimeters, of the physical screen
int DispDrvr_VERTSIZE = 60;   // Height,in millimeters, of the physical screen
int DispDrvr_LOGPIXELSX = 90; // 25.4 (mm/in) / 0.3 dot pitch (mm/pixel)
int DispDrvr_LOGPIXELSY = 90;
int DispDrvr_cdwStride = 20;


void DispDrvrInitialize (void) {}
void DispDrvrDirtyRectDump(LPCRECT prc) {}    
void DispDrvrSetDibBuffer (void * pv) {}
void DispDrvrPowerHandler(BOOL bOff) {}
BOOL DispDrvrContrastControl(int Cmd, DWORD *pValue) {return TRUE;}
void DispDrvrMoveCursor(INT32 xLocation, INT32 yLocation) {}


