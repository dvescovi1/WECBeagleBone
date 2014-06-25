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
//  File:  constraintadapter.cpp
//

#include <windows.h>
#include <ceddk.h>
#include <ceddkex.h>
#include <_constants.h>
#include <ConstraintAdapter.h>

#define FUNCTION_FMT            _T("%s_%s")
#define INIT_NAME               _T("InitConstraint")
#define DEINIT_NAME             _T("DeinitConstraint")
#define CREATE_NAME             _T("CreateConstraint")
#define UPDATE_NAME             _T("UpdateConstraint")
#define CLOSE_NAME              _T("CloseConstraint")
#define CREATECB_NAME           _T("InsertConstraintCallback")
#define REMOVECB_NAME           _T("RemoveConstraintCallback")

//-----------------------------------------------------------------------------
//
//  Method: IsHex
//
BOOL
IsHex(
    _TCHAR c
    )
{
    if (c >= _T('0') && c <= _T('9')) return TRUE;
    if (c >= _T('a') && c <= _T('f')) return TRUE;
    if (c >= _T('A') && c <= _T('F')) return TRUE;
    return FALSE;
}

//-----------------------------------------------------------------------------
//
//  Method: ConstraintAdapter::ParseConstraintClassification
//
BOOL 
ConstraintAdapter::
ParseConstraintClassification(
    _TCHAR const *szClasses,
    DWORD nLen
    )
{
    BOOL rc = FALSE;
    DWORD count;
    DWORD nStartPos = 0;
    DWORD nEndPos = 0;
    DWORD nClassIndex = 0;
    
    // get end of string
    while (nEndPos < nLen)
        {
        count = 0;
        do
            {
            if (IsHex(szClasses[nEndPos]))
                {
                count++;
                nEndPos++;
                }
            else if (szClasses[nEndPos] == _T(',') ||
                     szClasses[nEndPos] == _T(' ') ||
                     szClasses[nEndPos] == _T('\0'))
                {
                // got end marker
                break;
                }
            else
                {
                // unexpected character
                goto cleanUp;
                }

            // check for too long hex values
            if (count > 8 || count == 0) goto cleanUp;
            }
            while (nEndPos < nLen);

        // check for double NULL
        if (count == 0) break;

        m_rgClasses[nClassIndex] = _tcstoul(szClasses + nStartPos, NULL, 16);

        // update for next class identifier
        nEndPos++;
        nStartPos = nEndPos;
        nClassIndex++;
        }

    m_nClassIds = nClassIndex;
    rc = TRUE;
    
cleanUp:
    return rc;
}

//-----------------------------------------------------------------------------
//
//  Method: ConstraintAdapter::Initialize
//
BOOL
ConstraintAdapter::
Initialize(
    LPCWSTR     szContext
    )
{
    LONG code;
    DWORD size;
    DWORD rc = FALSE;    
    HKEY hKey = NULL;
    _TCHAR szBuffer[MAX_PATH];

    // Save device registry key for later use
    if (_tcslen(szContext) >= sizeof(m_szRegKey) / sizeof(m_szRegKey[0])) goto cleanUp;
    _tcscpy_s(m_szRegKey,_countof(m_szRegKey), szContext);

    // Open device registry key
    code = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, szContext, 0, 0, &hKey);
    if (code != ERROR_SUCCESS) goto cleanUp;

    // read load order
    size = sizeof(m_dwOrder);
    code = RegQueryValueEx(hKey, REGEDIT_CONSTRAINT_ORDER, 0, 0, (BYTE*)&m_dwOrder, &size);
    if (code != ERROR_SUCCESS) goto cleanUp;

    // read constraint name
    size = sizeof(m_szConstraintName);
    code = RegQueryValueEx(hKey, REGEDIT_CONSTRAINT_NAME, 0, 0, (BYTE*)&m_szConstraintName, &size);
    if (code != ERROR_SUCCESS) goto cleanUp;

    // read dll name
    size = sizeof(m_szDll);
    code = RegQueryValueEx(hKey, REGEDIT_CONSTRAINT_DLL, 0, 0, (BYTE*)&m_szDll, &size);
    if (code != ERROR_SUCCESS) goto cleanUp;

    // read constraint classifications
    size = sizeof(szBuffer);
    code = RegQueryValueEx(hKey, REGEDIT_CONSTRAINT_CLASSES, 0, 0, (BYTE*)szBuffer, &size);
    if (code == ERROR_SUCCESS)
        {
        if (ParseConstraintClassification(szBuffer, size >> 1) == FALSE)
            {
            goto cleanUp;
            }
        }
    
    // Done
    rc = TRUE;
    
cleanUp:    
    if (hKey != NULL) RegCloseKey(hKey);
    return rc;
}

//-----------------------------------------------------------------------------
//
//  Method: ConstraintAdapter::PostInitialize
//
BOOL
ConstraintAdapter::
PostInitialize()
{
    BOOL rc = FALSE;
    _TCHAR szBuffer[MAX_PATH];
    
    // Load library
    m_hModule = ::LoadLibrary(m_szDll);
    if (m_hModule == NULL) goto cleanUp;

    //
    // load functions
    
    // xxx_InitConstraint
    _stprintf_s(szBuffer,_countof(szBuffer), FUNCTION_FMT, m_szConstraintName, INIT_NAME);
    m_fns.InitConstraint = reinterpret_cast<fnInitConstraint>(
                            ::GetProcAddress(m_hModule, szBuffer)
                            );
    if (m_fns.InitConstraint == NULL) goto cleanUp;

    // xxx_DeinitConstraint
    _stprintf_s(szBuffer,_countof(szBuffer), FUNCTION_FMT, m_szConstraintName, DEINIT_NAME);
    m_fns.DeinitConstraint = reinterpret_cast<fnDeinitConstraint>(
                            ::GetProcAddress(m_hModule, szBuffer)
                            );
    if (m_fns.DeinitConstraint == NULL) goto cleanUp;

    // xxx_CreateConstraint
    _stprintf_s(szBuffer,_countof(szBuffer), FUNCTION_FMT, m_szConstraintName, CREATE_NAME);
    m_fns.CreateConstraint = reinterpret_cast<fnCreateConstraint>(
                            ::GetProcAddress(m_hModule, szBuffer)
                            );
    if (m_fns.CreateConstraint == NULL) goto cleanUp;

    // xxx_UpdateConstraint
    _stprintf_s(szBuffer,_countof(szBuffer), FUNCTION_FMT, m_szConstraintName, UPDATE_NAME);
    m_fns.UpdateConstraint = reinterpret_cast<fnUpdateConstraint>(
                            ::GetProcAddress(m_hModule, szBuffer)
                            );
    if (m_fns.UpdateConstraint == NULL) goto cleanUp;

    // xxx_CloseConstraint
    _stprintf_s(szBuffer,_countof(szBuffer), FUNCTION_FMT, m_szConstraintName, CLOSE_NAME);
    m_fns.CloseConstraint = reinterpret_cast<fnCloseConstraint>(
                            ::GetProcAddress(m_hModule, szBuffer)
                            );
    if (m_fns.CloseConstraint == NULL) goto cleanUp;

    // xxx_CreateConstraintCallback
    _stprintf_s(szBuffer,_countof(szBuffer), FUNCTION_FMT, m_szConstraintName, CREATECB_NAME);
    m_fns.InsertConstraintCallback = reinterpret_cast<fnInsertConstraintCallback>(
                            ::GetProcAddress(m_hModule, szBuffer)
                            );

    // xxx_RemoveConstraintCallback
    _stprintf_s(szBuffer,_countof(szBuffer), FUNCTION_FMT, m_szConstraintName, REMOVECB_NAME);
    m_fns.RemoveConstraintCallback = reinterpret_cast<fnRemoveConstraintCallback>(
                            ::GetProcAddress(m_hModule, szBuffer)
                            );

    // try to initialize constraint adapter
    m_hConstraintAdapter = m_fns.InitConstraint(m_szRegKey);
    rc = m_hConstraintAdapter != NULL;
    
cleanUp:
    return rc;
}

//-----------------------------------------------------------------------------
//
//  Method: ConstraintAdapter::Initialize
//
void
ConstraintAdapter::
Uninitialize()
{
    if (m_fns.CloseConstraint != NULL && m_hConstraintAdapter != NULL)
        {
        m_fns.CloseConstraint(m_hConstraintAdapter);
        }

    if (m_hModule != NULL)
        {
        FreeLibrary(m_hModule);
        }

    // reset member variables
    *m_szDll = NULL;
    m_hModule = NULL;
    m_dwOrder = 0;
    m_nClassIds = 0;
    *m_szRegKey = NULL;
    *m_szConstraintName = NULL;
    m_hConstraintAdapter = NULL;
    memset(&m_fns, 0, sizeof(ConstraintAdapterFns));
    memset(m_rgClasses, 0, sizeof(DWORD)*MAX_CONSTRAINT_CLASSES);
}

//-----------------------------------------------------------------------------
