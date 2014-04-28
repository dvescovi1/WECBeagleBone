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
//  File:  constraintadapter.h
//

#ifndef __CONSTRAINTADAPTER_H__
#define __CONSTRAINTADAPTER_H__

#define MAX_CONSTRAINT_CLASSES          (256)

//------------------------------------------------------------------------------
//
//  Class:  ConstraintAdapter
//
class ConstraintAdapter
{
public:
    //-------------------------------------------------------------------------
    // typedefs
    //-------------------------------------------------------------------------
    typedef HANDLE (*fnInitConstraint)(_TCHAR const *szContext);
    typedef BOOL (*fnDeinitConstraint)(HANDLE hConstraintAdapter);
    typedef HANDLE (*fnCreateConstraint)(HANDLE hConstraintAdapter);
    typedef BOOL (*fnUpdateConstraint)(HANDLE hConstraintContext, DWORD msg, void *pParam, UINT size);
    typedef BOOL (*fnCloseConstraint)(HANDLE hConstraintContext);
    typedef HANDLE (*fnInsertConstraintCallback)(HANDLE hConstraintContext, void* pCallback, void *pParam, UINT size, HANDLE hRefContext);
    typedef BOOL (*fnRemoveConstraintCallback)(HANDLE hConstraintCallback);

    typedef struct {
        fnInitConstraint            InitConstraint;
        fnDeinitConstraint          DeinitConstraint;
        fnCreateConstraint          CreateConstraint;
        fnUpdateConstraint          UpdateConstraint;
        fnCloseConstraint           CloseConstraint;
        fnInsertConstraintCallback  InsertConstraintCallback;
        fnRemoveConstraintCallback  RemoveConstraintCallback;
    } ConstraintAdapterFns;

    
protected:
    //-------------------------------------------------------------------------
    // member variables
    //-------------------------------------------------------------------------
    
    HMODULE                     m_hModule;
    DWORD                       m_dwOrder;
    INT                         m_nClassIds;
    _TCHAR                      m_szDll[MAX_PATH];
    HANDLE                      m_hConstraintAdapter;
    _TCHAR                      m_szRegKey[MAX_PATH];
    _TCHAR                      m_szConstraintName[MAX_PATH];
    DWORD                       m_rgClasses[MAX_CONSTRAINT_CLASSES];
    
    ConstraintAdapterFns        m_fns;

        
public:
    //-------------------------------------------------------------------------
    // constructor/destructor
    //-------------------------------------------------------------------------

    ConstraintAdapter() : m_hModule(NULL), m_dwOrder(0), m_nClassIds(0),
                          m_hConstraintAdapter(NULL)
    {
        *m_szDll = NULL;
        *m_szRegKey = NULL;
        *m_szConstraintName = NULL;
        memset(&m_fns, 0, sizeof(ConstraintAdapterFns));
        memset(m_rgClasses, 0, sizeof(DWORD)*MAX_CONSTRAINT_CLASSES);
    }


protected:
    //-------------------------------------------------------------------------
    // inline protected routines
    //-------------------------------------------------------------------------

    BOOL ParseConstraintClassification(_TCHAR const *szClasses, DWORD nLen);
    
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
        return _tcscmp(m_szConstraintName, szId) == 0;
    }

    _TCHAR const*        
    GetAdapterId() const
    {
        return m_szConstraintName;
    }

    BOOL
    IsAdapterClass(
        DWORD adapterClass
        )
    {
        for (int i = 0; i < m_nClassIds; ++i)
            {
            if (m_rgClasses[i] == adapterClass) return TRUE;
            }
        return FALSE;
    }

    HANDLE
    CreateConstraint()
    {
        if (m_fns.CreateConstraint == NULL) return NULL;
        return m_fns.CreateConstraint(m_hConstraintAdapter);
    }
    
    BOOL 
    UpdateConstraint(
        HANDLE hConstraintContext, 
        DWORD msg, 
        void *pParam, 
        UINT  size)
    {
        if (m_fns.UpdateConstraint == NULL) return NULL;
        return m_fns.UpdateConstraint(hConstraintContext, msg, pParam, size);
    }
    
    BOOL 
    CloseConstraint(
        HANDLE hConstraintContext
        )
    {
        if (m_fns.CloseConstraint == NULL) return FALSE;
        return m_fns.CloseConstraint(hConstraintContext);
    }
    
    HANDLE 
    InsertConstraintCallback(
        HANDLE hConstraintContext,
        ConstraintCallback fnCallback, 
        void *pParam, 
        UINT  size, 
        HANDLE hRefContext
        )
    {
        if (m_fns.InsertConstraintCallback == NULL) return FALSE;
        return m_fns.InsertConstraintCallback(hConstraintContext, 
                fnCallback, pParam, size, hRefContext
                );
    }
    
    BOOL 
    RemoveConstraintCallback(
        HANDLE hConstraintCallback
        )
    {
        if (m_fns.RemoveConstraintCallback == NULL) return FALSE;
        return m_fns.RemoveConstraintCallback(hConstraintCallback);
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
    PostInitialize();

    void
    Uninitialize();
};

//------------------------------------------------------------------------------
#endif //__CONSTRAINTADAPTER_H__