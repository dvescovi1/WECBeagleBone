// All rights reserved ADENEO EMBEDDED 2010
/*
===============================================================================
*             Texas Instruments OMAP(TM) Platform Software
* (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
*
* Use of this software is controlled by the terms and conditions found
* in the license agreement under which this software has been supplied.
*
===============================================================================
*/
//
// apm.cpp
//  
#include "omap.h"
#include "omap_pmext.h"
#include "oalex.h"
#include "ceddkex.h"
//-----------------------------------------------------------------------------
//  global defines
//
#define PMEXT_MODULE_PATH    _T("\\SYSTEM\\CurrentControlSet\\Control\\Power\\Extension\\Omap")
#define PMEXT_MODULE_KEY     _T("Dll")
#define IsPmExtValid()       (_PmIfc.bInitialized != FALSE ? TRUE : LoadPmExt())

//-----------------------------------------------------------------------------
//  globals variables

// flag used to ensure only 1 instance of policy manager is initialized
//
static HMODULE                          _hPmExt = NULL;
static OMAP_PMEXT_IFC                   _PmIfc = {FALSE};

//-----------------------------------------------------------------------------
//
//  Function:  HalContextUpdateDirtyRegister
//
//  update context save mask to indicate registers need to be saved before
//  off
//
void
HalContextUpdateDirtyRegister(
    UINT32 ffRegister
    )
{
#if 1
#if (_WINCEOSVER<600)
    BOOL bOldMode = SetKMode(TRUE);
#endif

    static UINT32 *pKernelContextSaveMask = NULL;

    if (pKernelContextSaveMask == NULL)
        {
        KernelIoControl(IOCTL_HAL_CONTEXTSAVE_GETBUFFER, 
            NULL, 
            0, 
            &pKernelContextSaveMask, 
            sizeof(UINT**), 
            0
            );
        }

    *pKernelContextSaveMask |= ffRegister;

#if (_WINCEOSVER<600)
    SetKMode(bOldMode);
#endif
#else
    UNREFERENCED_PARAMETER(ffRegister);
#endif
}

//-----------------------------------------------------------------------------
//
//  Function:  LoadPmExt
//
//  loads the power management extension
//
static 
BOOL 
LoadPmExt()
{
    DWORD dwType;
    DWORD dwSize;
    BOOL rc = FALSE;
    HKEY hKey = NULL;
    _TCHAR szBuffer[MAX_PATH];
    fnOmapPmExtensions OmapPmExtensions;

    // check if an attempt was already made, if so don't do it again
    if (_PmIfc.bInitialized == TRUE) goto cleanUp;

    // open registery key to the power management extension
    RegOpenKeyEx(HKEY_LOCAL_MACHINE, PMEXT_MODULE_PATH, 0, 0, &hKey);
    if (hKey == NULL) goto cleanUp;

    // get power management extension module which containts the framework
    dwType = REG_SZ;
    dwSize = sizeof(szBuffer);
    if (RegQueryValueEx(hKey, PMEXT_MODULE_KEY, NULL, &dwType, 
            (BYTE*)szBuffer, &dwSize) != ERROR_SUCCESS)
        {
        goto cleanUp;
        }

    // load module
    _hPmExt = (HMODULE)LoadLibrary(szBuffer);
    if (_hPmExt == NULL) goto cleanUp;

    // extract exported functions
    OmapPmExtensions = (fnOmapPmExtensions)GetProcAddress(
                                    _hPmExt, 
                                    L"OmapPmExtensions"
                                    );

    // validate library and routine
    if (OmapPmExtensions == NULL) goto cleanUp;
    if (OmapPmExtensions(&_PmIfc, sizeof(OMAP_PMEXT_IFC)) == FALSE)
        {
        _PmIfc.bInitialized = FALSE;
        goto cleanUp;
        }

    _PmIfc.bInitialized = TRUE;
    rc = TRUE;
    
cleanUp:    
    if (hKey != NULL) RegCloseKey(hKey);
    if (rc == FALSE)
        {
        // something failed, don't attempt to reload module
        FreeLibrary(_hPmExt);
        _hPmExt = (HMODULE)INVALID_HANDLE_VALUE;
        }
            
    return rc;
}

//-----------------------------------------------------------------------------
//
//  Global:  PmxSendDeviceNotification
//
//  sends a notification to a device driver
//
DWORD 
PmxSendDeviceNotification(
    UINT listId,
    UINT dwParam, 
    DWORD dwIoControlCode, 
    LPVOID lpInBuf, 
    DWORD nInBufSize, 
    LPVOID lpOutBuf, 
    DWORD nOutBufSize, 
    LPDWORD pdwBytesRet
    )
{
    DWORD rc = FALSE;

    // sanity check
    if (IsPmExtValid() == FALSE) goto cleanUp;

    // make call
    rc = _PmIfc.PmxSendDeviceNotification(listId, dwParam, 
                dwIoControlCode, lpInBuf, nInBufSize, lpOutBuf, 
                nOutBufSize, pdwBytesRet
                );

cleanUp:
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  PmxSetConstraintByClass
//
//  Imposes a constraint using an adapter identified by class.
//
HANDLE
PmxSetConstraintByClass(
    DWORD classId,
    DWORD msg,
    void *pParam,
    UINT  size
    )
{
    HANDLE rc = NULL;

    // sanity check
    if (IsPmExtValid() == FALSE) goto cleanUp;

    // make call
    rc = _PmIfc.PmxSetConstraintByClass(classId, msg, pParam, size);

cleanUp:
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  PmxSetConstraintById
//
//  Imposes a constraint using an adapter identified by id.
//
HANDLE
PmxSetConstraintById(
    LPCWSTR szId,
    DWORD msg,
    void *pParam,
    UINT  size
    )
{
    HANDLE rc = NULL;

    // sanity check
    if (IsPmExtValid() == FALSE) goto cleanUp;

    // make call
    rc = _PmIfc.PmxSetConstraintById(szId, msg, pParam, size);

cleanUp:
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  PmxUpdateConstraint
//
//  Updates a constraint context.
//
BOOL
PmxUpdateConstraint(
    HANDLE hConstraintContext,
    DWORD msg,
    void *pParam,
    UINT  size
    )
{
    DWORD rc = FALSE;

    // sanity check
    if (IsPmExtValid() == FALSE) goto cleanUp;

    // make call
    rc = _PmIfc.PmxUpdateConstraint(hConstraintContext, msg, pParam, size);

cleanUp:
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  PmxReleaseConstraint
//
//  Releases a constraint.
//
void
PmxReleaseConstraint(
    HANDLE hConstraintContext
    )
{
    // sanity check
    if (IsPmExtValid() == FALSE) return;

    // make call
    _PmIfc.PmxReleaseConstraint(hConstraintContext);
}

//------------------------------------------------------------------------------
//
//  Function:  PmxRegisterConstraintCallback
//
//  registers for constraint callback.
//
HANDLE
PmxRegisterConstraintCallback(
    HANDLE hConstraintContext,
    ConstraintCallback fnCallback, 
    void *pParam, 
    UINT  size, 
    HANDLE hRefContext
    )
{
    // sanity check
    if (IsPmExtValid() == FALSE) return NULL;

    // make call
    return _PmIfc.PmxRegisterConstraintCallback(hConstraintContext, 
                    fnCallback, 
                    pParam, 
                    size, 
                    hRefContext
                    );
}

//------------------------------------------------------------------------------
//
//  Function:  PmxUnregisterConstraintCallback
//
//  unregisters for constraint callback.
//
BOOL
PmxUnregisterConstraintCallback(
    HANDLE hConstraintContext,
    HANDLE hConstraintCallback
    )
{
    // sanity check
    if (IsPmExtValid() == FALSE) return FALSE;

    // make call
    return _PmIfc.PmxUnregisterConstraintCallback(hConstraintContext, 
                hConstraintCallback
                );
}

//-----------------------------------------------------------------------------
// 
//  Function:  PmxOpenPolicy
//
//  Called by external components to open an access to a policy
//
HANDLE
PmxOpenPolicy(
    LPCWSTR szId
    )
{
    // sanity check
    if (IsPmExtValid() == FALSE) return NULL;

    // make call
    return _PmIfc.PmxOpenPolicy(szId);    
}

//-----------------------------------------------------------------------------
// 
//  Function:  PmxNotifyPolicy
//
//  Called by external components to notify a policy
//
BOOL
PmxNotifyPolicy(
    HANDLE hPolicyContext,
    DWORD msg,
    void *pParam,
    UINT  size
    )
{
    // sanity check
    if (IsPmExtValid() == FALSE) return FALSE;

    // make call
    return _PmIfc.PmxNotifyPolicy(hPolicyContext, msg, pParam, size);    
}

//-----------------------------------------------------------------------------
// 
//  Function:  PmxClosePolicy
//
//  Called by external components to close a policy handle
//
void
PmxClosePolicy(
    HANDLE hPolicyContext
    )
{
    // sanity check
    if (IsPmExtValid() == FALSE) return;

    // make call
    _PmIfc.PmxClosePolicy(hPolicyContext);
}

//-----------------------------------------------------------------------------
// 
//  Function:  PmxLoadConstraint
//
//  Called by external components to dynamically load a constraint adapter
//
HANDLE
PmxLoadConstraint(
    void *context
    )
{
    // sanity check
    if (IsPmExtValid() == FALSE) return NULL;

    // make call
    return _PmIfc.PmxLoadConstraint(context);    
}

//-----------------------------------------------------------------------------
// 
//  Function:  PmxLoadPolicy
//
//  Called by external components to dynamically load a power policy
//
HANDLE
PmxLoadPolicy(
    void *context
    )
{
    // sanity check
    if (IsPmExtValid() == FALSE) return NULL;

    // make call
    return _PmIfc.PmxLoadPolicy(context);    
}

//-----------------------------------------------------------------------------
