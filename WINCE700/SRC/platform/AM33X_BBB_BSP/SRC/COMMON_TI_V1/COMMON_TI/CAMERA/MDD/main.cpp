// All rights reserved ADENEO EMBEDDED 2010
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
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES.
//

#pragma warning(push)
#pragma warning(disable : 4201 6244)
#include <windows.h>
#include <pm.h>
#include <devload.h>
#include <nkintr.h>

#include "Cs.h"
#include "Csmedia.h"
#pragma warning(pop)

#include "CameraPDDProps.h"
#include "dstruct.h"
#include "dbgsettings.h"
#include "CameraDriver.h"

BOOL WINAPI 
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
