/*
================================================================================
*             Texas Instruments OMAP(TM) Platform Software
* (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
*
* Use of this software is controlled by the terms and conditions found
* in the license agreement under which this software has been supplied.
*
================================================================================
*/
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
/* File:    oalioctl.cpp
 *
 * Purpose: OAL updatabale module which will be called from kernel for user
 * mode threads. OEMs can update the list of ioctls supported by user mode
 * threads by updating the IOControl function in this module.
 *
 * OEMs can do the following updates:
 *
 * -- Add/Delete from the list of ioctls in IOControl function below: This entry point
 * is called whenever a user mode thread calls into kernel via KernelIoControl or
 * via KernelLibIoControl (with KMOD_OAL). For kernel mode threads, all the 
 * handling for KernelIoControl and KernelLibIoControl happens inside kernel and
 * this module is not involved.
 *
 */

#include <windows.h>
#include <oalioctl.h>

#include <ceddk.h>
#include <ceddkex.h>

#pragma warning(push)
#pragma warning (disable:4201)
#include <oal.h>
#pragma warning(pop)

#include <oalex.h>


PFN_Ioctl g_pfnExtOALIoctl;

//------------------------------------------------------------------------------
// Function: IOControl
//
// Arguments: Same signature as KernelIoControl
//    DWORD dwIoControlCode: io control code
//    PBYTE pInBuf: pointer to input buffer
//    DWORD nInBufSize: length of input buffer in bytes
//    PBYTE pOutBuf: pointer to out buffer
//    DWORD nOutBufSize: length of output buffer in bytes
//    PDWORD pBytesReturned: number of bytes returned in output buffer
//
// Return Values:
// If the function call is successful, TRUE is returned from this API call.
// If the function call is not successful, FALSE is returned from this API
// call and the last error is set to:
// a) ERROR_INVALID_PARAMETER: any of the input arguments are invalid
// b) ERROR_NOT_SUPPORTED: given ioctl is not supported
// c) any other ioctl set by OAL code
//
// Abstract:
// This is called by kernel whenever a user mode thread makes a call to
// KernelIoControl or KernelLibIoControl with io control code being an OAL
// io control code. OEMs can override what ioctls a user mode thread can call
// by enabling or disabling ioctl codes in this function.
//
//------------------------------------------------------------------------------
EXTERN_C
BOOL
IOControl(
    DWORD dwIoControlCode,
    PBYTE pInBuf,
    DWORD nInBufSize,
    PBYTE pOutBuf,
    DWORD nOutBufSize,
    PDWORD pBytesReturned
    )
{
    BOOL fRet = FALSE;

    //
    // By default the following ioctls are supported for user mode threads.
    // If a new ioctl is being added to this list, make sure the corresponding
    // data associated with that ioctl is marshalled properly to the OAL
    // ioctl implementation. In normal cases, one doesn't need any 
    // marshaling as first level user specified buffers are already validated 
    // by kernel that:
    // -- the buffers are within the user process space
    // Check out IsValidUsrPtr() function in vmlayout.h for details on kernel
    // validation of user specified buffers. Kernel doesn't validate that the
    // buffers are accessible; it only checks that the buffer start and end
    // addresses are within the user process space.
    //
    switch (dwIoControlCode) {     
        //  MSFT Standard kernel IOCTLs
        case IOCTL_HAL_GET_CACHE_INFO:
        case IOCTL_HAL_GET_DEVICE_INFO:
        case IOCTL_HAL_GET_DEVICEID:
        case IOCTL_HAL_GET_UUID:
        case IOCTL_PROCESSOR_INFORMATION:
        case IOCTL_HAL_REBOOT:
        case IOCTL_HAL_ENABLE_WAKE:
        case IOCTL_HAL_DISABLE_WAKE:
        case IOCTL_HAL_GET_WAKE_SOURCE:
            // request is to service the ioctl - forward the call to OAL code
            // OAL code will set the last error if there is a failure
            fRet = (*g_pfnExtOALIoctl)(dwIoControlCode, pInBuf, nInBufSize, pOutBuf, nOutBufSize, pBytesReturned);
        break;

#if (_WINCEOSVER==700)
        //  "Seven" specific IOCTLs
        case IOCTL_HAL_UPDATE_MODE:
        case IOCTL_HAL_GET_CELOG_PARAMETERS:
        case IOCTL_HAL_GET_POWERONREASON:
            // request is to service the ioctl - forward the call to OAL code
            // OAL code will set the last error if there is a failure
            fRet = (*g_pfnExtOALIoctl)(dwIoControlCode, pInBuf, nInBufSize, pOutBuf, nOutBufSize, pBytesReturned);
        break;
#endif

        //  OMAP35XX OAL Extension IOCTLs
        case IOCTL_HAL_GET_CPUID:
        case IOCTL_HAL_GET_DIEID:
        case IOCTL_HAL_GET_BSP_VERSION:
        case IOCTL_HAL_GET_DSP_INFO:
        case IOCTL_HAL_GET_CPUFAMILY:
//        case IOCTL_HAL_GET_CPUREVISION:
        case IOCTL_HAL_DUMP_REGISTERS:        
        case IOCTL_HAL_GET_CPUSPEED:            
		case IOCTL_HAL_GET_DISPLAY_RES:		
        case IOCTL_HAL_GET_ECC_TYPE: 
        case IOCTL_HAL_GET_PLATFORM_ID: 
		case IOCTL_HAL_GET_BOOT_INFO: 
		case IOCTL_HAL_GET_IRQ_COUNTERS: 
        case IOCTL_HAL_GET_EBOOT_CFG:
            // request is to service the ioctl - forward the call to OAL code
            // OAL code will set the last error if there is a failure
            fRet = (*g_pfnExtOALIoctl)(dwIoControlCode, pInBuf, nInBufSize, pOutBuf, nOutBufSize, pBytesReturned);
        break;

        default:
            SetLastError(ERROR_NOT_SUPPORTED);
        break;
    }

    return fRet;
}

BOOL
WINAPI
DllMain(HANDLE hDll, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls((HINSTANCE)hDll);
            g_pfnExtOALIoctl = (PFN_Ioctl) lpReserved;
        break;
        case DLL_PROCESS_DETACH:
        default:
        break;
    }

    return TRUE;
}

