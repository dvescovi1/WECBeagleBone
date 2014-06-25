// All rights reserved ADENEO EMBEDDED 2010
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
//  File:  policyadapter.cpp
//

#include <windows.h>
#include <_constants.h>
#include <policyadapter.h>
#include <policyroot.h>

#define FUNCTION_FMT            _T("%s_%s")
#define INIT_NAME               _T("InitPolicy")
#define DEINIT_NAME             _T("DeinitPolicy")
#define OPEN_NAME               _T("OpenPolicy")
#define NOTIFY_NAME             _T("NotifyPolicy")
#define CLOSE_NAME              _T("ClosePolicy")
#define PREDEVICESTATE_NAME     _T("PreDeviceStateChange")
#define POSTDEVICESTATE_NAME    _T("PostDeviceStateChange")
#define PRESYSTEMSTATE_NAME     _T("PreSystemStateChange")
#define POSTSYSTEMSTATE_NAME    _T("PostSystemStateChange")

//-----------------------------------------------------------------------------
//
//  Method: PolicyAdapter::Initialize
//
BOOL
PolicyAdapter::
Initialize(
    LPCWSTR     szContext
    )
{
    LONG code;
    DWORD size;
    DWORD rc = FALSE;    
    HKEY hKey = NULL;

    // Save device registry key for later use
    if (_tcslen(szContext) >= sizeof(m_szRegKey) / sizeof(m_szRegKey[0])) goto cleanUp;
    _tcscpy_s(m_szRegKey,_countof(m_szRegKey), szContext);

    // Open device registry key
    code = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, szContext, 0, 0, &hKey);
    if (code != ERROR_SUCCESS) goto cleanUp;

    // read load order
    size = sizeof(m_dwOrder);
    code = RegQueryValueEx(hKey, REGEDIT_POLICY_ORDER, 0, 0, (BYTE*)&m_dwOrder, &size);
    if (code != ERROR_SUCCESS) goto cleanUp;

    // read constraint name
    size = sizeof(m_szPolicyName);
    code = RegQueryValueEx(hKey, REGEDIT_POLICY_NAME, 0, 0, (BYTE*)&m_szPolicyName, &size);
    if (code != ERROR_SUCCESS) goto cleanUp;

    // read dll name
    size = sizeof(m_szDll);
    code = RegQueryValueEx(hKey, REGEDIT_POLICY_DLL, 0, 0, (BYTE*)&m_szDll, &size);
    if (code != ERROR_SUCCESS) goto cleanUp;
    
    // Done
    rc = TRUE;
    
cleanUp:    
    if (hKey != NULL) RegCloseKey(hKey);
    return rc;
}

//-----------------------------------------------------------------------------
//
//  Method: PolicyAdapter::PostInitialize
//
BOOL
PolicyAdapter::
PostInitialize(
    PolicyRoot* pPolicyRoot
    )
{
    BOOL rc = FALSE;
    _TCHAR szBuffer[MAX_PATH];
    
    // Load library
    m_hModule = ::LoadLibrary(m_szDll);
    if (m_hModule == NULL) goto cleanUp;

    //
    // load functions
    
    // xxx_InitPolicy
    _stprintf_s(szBuffer,_countof(szBuffer), FUNCTION_FMT, m_szPolicyName, INIT_NAME);
    m_fns.InitPolicy = reinterpret_cast<fnInitPolicy>(
                            ::GetProcAddress(m_hModule, szBuffer)
                            );
    if (m_fns.InitPolicy == NULL) goto cleanUp;

    // xxx_DeinitPolicy
    _stprintf_s(szBuffer,_countof(szBuffer), FUNCTION_FMT, m_szPolicyName, DEINIT_NAME);
    m_fns.DeinitPolicy = reinterpret_cast<fnDeinitPolicy>(
                            ::GetProcAddress(m_hModule, szBuffer)
                            );
    if (m_fns.DeinitPolicy == NULL) goto cleanUp;

    // xxx_OpenPolicy
    _stprintf_s(szBuffer,_countof(szBuffer), FUNCTION_FMT, m_szPolicyName, OPEN_NAME);
    m_fns.OpenPolicy = reinterpret_cast<fnOpenPolicy>(
                            ::GetProcAddress(m_hModule, szBuffer)
                            );

    if (m_fns.OpenPolicy != NULL)
            {
        // xxx_UpdatePolicy
        _stprintf_s(szBuffer,_countof(szBuffer), FUNCTION_FMT, m_szPolicyName, NOTIFY_NAME);
        m_fns.NotifyPolicy = reinterpret_cast<fnNotifyPolicy>(
                                ::GetProcAddress(m_hModule, szBuffer)
                                );
        if (m_fns.NotifyPolicy == NULL) goto cleanUp;

        // xxx_ClosePolicy
        _stprintf_s(szBuffer,_countof(szBuffer), FUNCTION_FMT, m_szPolicyName, CLOSE_NAME);
        m_fns.ClosePolicy = reinterpret_cast<fnClosePolicy>(
                                ::GetProcAddress(m_hModule, szBuffer)
                                );
        if (m_fns.ClosePolicy == NULL) goto cleanUp;
        }

    // device state change notification 
    _stprintf_s(szBuffer,_countof(szBuffer), FUNCTION_FMT, m_szPolicyName, PREDEVICESTATE_NAME);
    m_fns.PreDeviceStateChange = reinterpret_cast<fnDeviceStateChange>(
                            ::GetProcAddress(m_hModule, szBuffer)
                            );

    _stprintf_s(szBuffer,_countof(szBuffer), FUNCTION_FMT, m_szPolicyName, POSTDEVICESTATE_NAME);
    m_fns.PostDeviceStateChange = reinterpret_cast<fnDeviceStateChange>(
                            ::GetProcAddress(m_hModule, szBuffer)
                            );

    if (m_fns.PreDeviceStateChange != NULL || m_fns.PostDeviceStateChange != NULL)
        {
        pPolicyRoot->AddDeviceChangeObserver(this);
        }

    // device state change notification 
    _stprintf_s(szBuffer,_countof(szBuffer), FUNCTION_FMT, m_szPolicyName, PRESYSTEMSTATE_NAME);
    m_fns.PreSystemStateChange = reinterpret_cast<fnSystemStateChange>(
                            ::GetProcAddress(m_hModule, szBuffer)
                            );

    _stprintf_s(szBuffer,_countof(szBuffer), FUNCTION_FMT, m_szPolicyName, POSTSYSTEMSTATE_NAME);
    m_fns.PostSystemStateChange = reinterpret_cast<fnSystemStateChange>(
                            ::GetProcAddress(m_hModule, szBuffer)
                            );

    if (m_fns.PreSystemStateChange != NULL || m_fns.PostSystemStateChange != NULL)
        {
        pPolicyRoot->AddSystemChangeObserver(this);
        }
    
    // try to initialize constraint adapter
    m_hPolicyAdapter = m_fns.InitPolicy(m_szRegKey);
    rc = m_hPolicyAdapter != NULL;
    
cleanUp:
    return rc;
}

//-----------------------------------------------------------------------------
//
//  Method: PolicyAdapter::Initialize
//
void
PolicyAdapter::
Uninitialize()
{
    if (m_fns.ClosePolicy != NULL && m_hPolicyAdapter != NULL)
        {
        m_fns.ClosePolicy(m_hPolicyAdapter);
        }

    if (m_hModule != NULL)
        {
        FreeLibrary(m_hModule);
        }

    // reset member variables
    *m_szDll = NULL;
    m_hModule = NULL;
    m_dwOrder = 0;
    *m_szRegKey = NULL;
    *m_szPolicyName = NULL;
    m_hPolicyAdapter = NULL;
    memset(&m_fns, 0, sizeof(PolicyAdapterFns));
}

//-----------------------------------------------------------------------------
