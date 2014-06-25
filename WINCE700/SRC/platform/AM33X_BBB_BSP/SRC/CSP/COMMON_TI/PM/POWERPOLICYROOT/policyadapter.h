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
//  File:  policyadapter.h
//

#ifndef __POLICYADAPTER_H__
#define __POLICYADAPTER_H__

//------------------------------------------------------------------------------

typedef BOOL (*PolicyCallback)(HANDLE hPolicy, DWORD msg, void *pParam, UINT size);

//------------------------------------------------------------------------------

class PolicyRoot;

//------------------------------------------------------------------------------
//
//  Class:  PolicyAdapter
//
class PolicyAdapter
{
public:
    //-------------------------------------------------------------------------
    // typedefs
    //-------------------------------------------------------------------------
    typedef HANDLE (*fnInitPolicy)(_TCHAR const *szContext);
    typedef BOOL (*fnDeinitPolicy)(HANDLE hPolicyAdapter);
    typedef HANDLE (*fnOpenPolicy)(HANDLE hPolicyAdapter);
    typedef BOOL (*fnNotifyPolicy)(HANDLE hPolicyContext, DWORD msg, void *pParam, UINT size);
    typedef BOOL (*fnClosePolicy)(HANDLE hPolicyContext);
    typedef BOOL (*fnDeviceStateChange)(HANDLE hPolicyAdapter, UINT dev, UINT oldState, UINT newState);
    typedef BOOL (*fnSystemStateChange)(HANDLE hPolicyAdapter, DWORD dwExtContext, LPCTSTR lpNewStateName, DWORD dwFlags);

    typedef struct {
        fnInitPolicy            InitPolicy;
        fnDeinitPolicy          DeinitPolicy;
        fnOpenPolicy            OpenPolicy;
        fnNotifyPolicy          NotifyPolicy;
        fnClosePolicy           ClosePolicy;
        fnDeviceStateChange     PreDeviceStateChange;
        fnDeviceStateChange     PostDeviceStateChange;
        fnSystemStateChange     PreSystemStateChange;
        fnSystemStateChange     PostSystemStateChange;
    } PolicyAdapterFns;
    
protected:
    //-------------------------------------------------------------------------
    // member variables
    //-------------------------------------------------------------------------
    
    HMODULE                     m_hModule;
    DWORD                       m_dwOrder;
    _TCHAR                      m_szDll[MAX_PATH];
    HANDLE                      m_hPolicyAdapter;
    _TCHAR                      m_szRegKey[MAX_PATH];
    _TCHAR                      m_szPolicyName[MAX_PATH];
    
    PolicyAdapterFns            m_fns;

public:
    //-------------------------------------------------------------------------
    // member variables
    //-------------------------------------------------------------------------
    
    PolicyAdapter              *m_pNextDeviceChangeObserver;
    PolicyAdapter              *m_pNextSystemChangeObserver;
        
public:
    //-------------------------------------------------------------------------
    // constructor/destructor
    //-------------------------------------------------------------------------

    PolicyAdapter() : m_hModule(NULL), m_dwOrder(0), m_hPolicyAdapter(NULL),
                      m_pNextDeviceChangeObserver(NULL),
                      m_pNextSystemChangeObserver(NULL)
    {
        *m_szDll = NULL;
        *m_szRegKey = NULL;
        *m_szPolicyName = NULL;
        memset(&m_fns, 0, sizeof(PolicyAdapterFns));
    }

    
public:
    //-------------------------------------------------------------------------
    // inline public routines
    //-------------------------------------------------------------------------
    
    DWORD        
    GetLoadOrder()
    {
        return m_dwOrder;
    }

    BOOL        
    IsAdapterId(
        LPCWSTR szId
        )
    {
        if (szId == NULL || _tcslen(szId) >= MAX_PATH) return FALSE;
        return _tcsicmp(m_szPolicyName, szId) == 0;
    }

    _TCHAR const*        
    GetAdapterId() const
    {
        return m_szPolicyName;
    }

    HANDLE
    OpenPolicy()
    {
        if (m_fns.OpenPolicy == NULL) return NULL;
        return m_fns.OpenPolicy(m_hPolicyAdapter);
    }
    
    BOOL 
    NotifyPolicy(
        HANDLE hPolicyContext, 
        DWORD msg, 
        void *pParam, 
        UINT  size)
    {
        if (m_fns.NotifyPolicy == NULL) return NULL;
        return m_fns.NotifyPolicy(hPolicyContext, msg, pParam, size);
    }
    
    BOOL 
    ClosePolicy(
        HANDLE hPolicyContext
        )
    {
        if (m_fns.ClosePolicy == NULL) return FALSE;
        return m_fns.ClosePolicy(hPolicyContext);
    }

    DWORD 
    PreSystemStateChange(
        DWORD dwExtContext, 
        LPCTSTR lpNewStateName, 
        DWORD dwFlags
        )
    {
        if (m_fns.PreSystemStateChange == NULL) return FALSE;
        return m_fns.PreSystemStateChange(m_hPolicyAdapter, dwExtContext, lpNewStateName, dwFlags);
    }

    DWORD 
    PostSystemStateChange(
        DWORD dwExtContext, 
        LPCTSTR lpNewStateName, 
        DWORD dwFlags
        )
    {
        if (m_fns.PostSystemStateChange == NULL) return FALSE;
        return m_fns.PostSystemStateChange(m_hPolicyAdapter, dwExtContext, lpNewStateName, dwFlags);
    }

    BOOL 
    PreDeviceStateChange(
        UINT dev, 
        UINT oldState, 
        UINT newState
        )
    {
        if (m_fns.PreDeviceStateChange == NULL) return FALSE;
        return m_fns.PreDeviceStateChange(m_hPolicyAdapter, dev, oldState, newState);
    }

    BOOL 
    PostDeviceStateChange(
        UINT dev, 
        UINT oldState, 
        UINT newState
        )
    {
        if (m_fns.PostDeviceStateChange == NULL) return FALSE;
        return m_fns.PostDeviceStateChange(m_hPolicyAdapter, dev, oldState, newState);
    }

        
public:
    //-------------------------------------------------------------------------
    // public routines
    //-------------------------------------------------------------------------

    BOOL
    Initialize(
        LPCWSTR     szContext
        );

    BOOL
    PostInitialize(
        PolicyRoot* pPolicyRoot
        );

    void
    Uninitialize();
};

//------------------------------------------------------------------------------
#endif //__POLICYADAPTER_H__