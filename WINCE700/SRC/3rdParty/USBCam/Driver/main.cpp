//-------------------------------------------------------------------------
// <copyright file="main.cpp" company="Microsoft">
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
//    USB camera driver for Windows Embedded CE 6.0
// </summary>
//-------------------------------------------------------------------------
//======================================================================
// USB camera driver for Windows Embedded CE 6.0
//======================================================================

#include <windows.h>
#include <pkfuncs.h>
#include <pm.h>
#include <devload.h>
#include <nkintr.h>

#include "Cs.h"
#include "Csmedia.h"

#include "CameraPDDProps.h"
#include "dstruct.h"
#include "dbgsettings.h"
#include "CameraDriver.h"

BOOL
__stdcall
DllMain(
   HANDLE   hDllHandle, 
   DWORD    dwReason, 
   VOID   * lpReserved
   ) 
{
    BOOL bRc = TRUE;
    
    UNREFERENCED_PARAMETER( lpReserved );
    
    switch ( dwReason )
    {
        case DLL_PROCESS_ATTACH: 
        {
           DEBUGREGISTER( reinterpret_cast<HMODULE>( hDllHandle ) );
           DEBUGMSG( ZONE_INIT, ( _T("*** DLL_PROCESS_ATTACH - Current Process: 0x%x, ID: 0x%x ***\r\n"), GetCurrentProcess(), GetCurrentProcessId() ) );

           DisableThreadLibraryCalls( reinterpret_cast<HMODULE>( hDllHandle ) );

           break;
        } 

        case DLL_PROCESS_DETACH: 
        {
            DEBUGMSG( ZONE_INIT, ( _T("*** DLL_PROCESS_DETACH - Current Process: 0x%x, ID: 0x%x ***\r\n"), GetCurrentProcess(), GetCurrentProcessId() ) );

            break;
        } 
            
        case DLL_THREAD_DETACH:
        {
            break;
        }
            
        case DLL_THREAD_ATTACH:
        {
            break;
        }
            
        default:
        {
            break;
        }
    }
    
    return bRc;
}
