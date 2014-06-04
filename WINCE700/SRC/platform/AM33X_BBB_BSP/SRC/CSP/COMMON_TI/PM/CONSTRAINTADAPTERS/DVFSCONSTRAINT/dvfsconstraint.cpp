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
//  File:  dvfsconstraint.cpp
//
//
#pragma warning (push)
#pragma warning (disable:4245)
#include "omap.h"
#include <windows.h>
#include <oal.h>
#include <oalex.h>
#include <ceddk.h>
#include <ceddkex.h>
#include "omap_dvfs.h"
#include <list>
#include <indexlist.h>
#include "_constants.h"
#pragma warning (pop)
using namespace std;
//#error sdf
//-----------------------------------------------------------------------------
//  registry key names
//
#define DVFS_FORCE                  (0x80000000)

#define REGEDIT_DVFS_MPU1           (_T("MPU1Map"))
#define REGEDIT_DVFS_CORE1          (_T("CORE1Map"))
#define REGEDIT_DVFS_DEFAULTOPM     (_T("OpmInit"))
#define REGEDIT_DVFS_CEILING        (_T("OpmCeiling"))
#define REGEDIT_DVFS_FLOOR          (_T("OpmFloor"))

#define DVFS_PRENOTIFY_MASK         (DVFS_CORE1_PRE_NOTICE | \
                                     DVFS_MPU1_PRE_NOTICE \
                                     )

#define DVFS_CANCELNOTIFY_MASK      (DVFS_CORE1_CANCEL_NOTICE | \
                                     DVFS_MPU1_CANCEL_NOTICE \
                                     )
                                     
#define DVFS_POSTNOTIFY_MASK        (DVFS_CORE1_POST_NOTICE | \
                                     DVFS_MPU1_POST_NOTICE \
                                     )

//-----------------------------------------------------------------------------
//  local structures
typedef struct {
    CRITICAL_SECTION        cs;
    DWORD                   rgVddMPU1[kOpmCount];
    DWORD                   rgVddCORE1[kOpmCount];
    DWORD                   opmTable[kOpmCount];
    DWORD                   opmCeiling;
    DWORD                   opmFloor;
    DWORD                   idxForce;
    DWORD                   opmCurrent;    
} DVFSConstraintInfo_t;

typedef struct {
    DWORD                   idContext;
    ConstraintCallback      fnCallback;
    HANDLE                  hRefContext;
    HANDLE                  hConstraintContext;
    HANDLE                  hCallbackContext;
} DVFSCallbackInfo_t;  
    

//-----------------------------------------------------------------------------
//  local variables
static IndexList<DWORD>        s_IndexList;
static list<DVFSCallbackInfo_t*> s_CallbackList;
static DVFSConstraintInfo_t    s_OpmInfo;

//-----------------------------------------------------------------------------
// 
//  Function:  Lock
//
//  enter critical section
//
static
void
Lock()
{
    EnterCriticalSection(&s_OpmInfo.cs);
}

//-----------------------------------------------------------------------------
// 
//  Function:  Unlock
//
//  exit critical section
//
static
void
Unlock()
{
    LeaveCriticalSection(&s_OpmInfo.cs);
}

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
ParseOppToOpmMap(
    _TCHAR const *szOpp,
    DWORD   nLen,
    DWORD *rgOppMap,
    DWORD   nMaxIndex
    )
{
    BOOL rc = FALSE;
    DWORD count;
    DWORD nStartPos = 0;
    DWORD nEndPos = 0;
    DWORD nOppIndex = 0;
    
    // get end of string
    while (nEndPos < nLen && nOppIndex < nMaxIndex)
        {
        count = 0;
        do
            {
            if (IsHex(szOpp[nEndPos]))
                {
                count++;
                nEndPos++;
                }
            else if (szOpp[nEndPos] == _T(',') ||
                     szOpp[nEndPos] == _T(' ') ||
                     szOpp[nEndPos] == _T('\0'))
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

        rgOppMap[nOppIndex] = _tcstoul(szOpp + nStartPos, NULL, 16);

        // update for next class identifier
        nEndPos++;
        nStartPos = nEndPos;
        nOppIndex++;
        }

    rc = TRUE;
    
cleanUp:
    return rc;
}

//-----------------------------------------------------------------------------
//
//  Method: NotifyCallbacks
//
void
NotifyCallbacks(
    DWORD newOpm,
    DWORD idContext
    )
{
    list<DVFSCallbackInfo_t*>::iterator iter;
    
    for (iter = s_CallbackList.begin(); iter != s_CallbackList.end(); ++iter)
        {
        // don't notify the caller who triggered the change
        if ((*iter)->idContext == idContext) continue;

        // notify callbacks
        (*iter)->fnCallback((*iter)->hRefContext, 
            CONSTRAINT_MSG_DVFS_NEWOPM,
            (void*)newOpm,
            sizeof(DWORD)
            );
        }
}

//-----------------------------------------------------------------------------
// 
//  Function:  UpdateOpm
//
//  Updates the operating mode based on current constraints
//
BOOL
UpdateOpm(
    DWORD idContext
    )
{
    int i;
    DWORD opmOld;
    DWORD opmNew = (DWORD) kOpmUndefined;
    BOOL rc = FALSE;
    DWORD dwNotificationMask = 0;
    
    IOCTL_DVFS_OPPNOTIFY_IN info;
    IOCTL_OPP_REQUEST_IN oppRequest;
    
    // first check if there is a constraint being forced
    if (s_OpmInfo.idxForce != -1)
        {
        DWORD *pDataNode;
        pDataNode = s_IndexList.GetIndex(s_OpmInfo.idxForce);
        if (pDataNode != NULL)
            {
            opmNew = *pDataNode;
            }
        }

    // get highest operating mode if not forced
    if (opmNew == kOpmUndefined)
        {
        for (i = (signed)s_OpmInfo.opmCeiling; i >= (signed)s_OpmInfo.opmFloor; --i)
            {
            if (s_OpmInfo.opmTable[i] > 0)
                {
                opmNew = (Dvfs_OperatingMode_e)i;
                break;
                }
            }
        }

    // Prepare to send notifications
    if (opmNew != kOpmUndefined && s_OpmInfo.opmCurrent != opmNew)
        {        
        OALMSG(1, (L"OPM%d%s\r\n", opmNew, s_OpmInfo.idxForce == -1 ? L"" : L" -f"));

        opmOld = s_OpmInfo.opmCurrent;

        // initialize notification structure
        info.dwCount = 0;
        oppRequest.dwCount = 0;
        info.size = sizeof(IOCTL_DVFS_OPPNOTIFY_IN);
        oppRequest.size = sizeof(IOCTL_OPP_REQUEST_IN);
        
        // initialize structures      
        if (s_OpmInfo.rgVddMPU1[opmOld] != s_OpmInfo.rgVddMPU1[opmNew])
            {
            //OALMSG(1, (L"MPU OPP%d -> OPP%d\r\n", s_OpmInfo.rgVddMPU1[opmOld], s_OpmInfo.rgVddMPU1[opmNew]));
            dwNotificationMask |= DVFS_MPU1_PRE_NOTICE | 
                                  DVFS_MPU1_POST_NOTICE | 
                                  DVFS_MPU1_CANCEL_NOTICE;

            // update notification structure
            info.rgOppInfo[info.dwCount].newOpp = s_OpmInfo.rgVddMPU1[opmNew];
            info.rgOppInfo[info.dwCount].oldOpp = s_OpmInfo.rgVddMPU1[opmOld];
            info.rgOppInfo[info.dwCount].domain = DVFS_MPU1_OPP;
            info.dwCount++;

            // update kernel structure
            oppRequest.rgOpps[oppRequest.dwCount] = s_OpmInfo.rgVddMPU1[opmNew];
            oppRequest.rgDomains[oppRequest.dwCount] = DVFS_MPU1_OPP;
            oppRequest.dwCount++;
            }

        if (s_OpmInfo.rgVddCORE1[opmOld] != s_OpmInfo.rgVddCORE1[opmNew])
            {
            //OALMSG(1, (L"CORE OPP%d -> OPP%d\r\n", s_OpmInfo.rgVddCORE1[opmOld], s_OpmInfo.rgVddCORE1[opmNew]));
            dwNotificationMask |= DVFS_CORE1_PRE_NOTICE | 
                                  DVFS_CORE1_POST_NOTICE |
                                  DVFS_CORE1_CANCEL_NOTICE;

            // update notification structure
            info.rgOppInfo[info.dwCount].newOpp = s_OpmInfo.rgVddCORE1[opmNew];
            info.rgOppInfo[info.dwCount].oldOpp = s_OpmInfo.rgVddCORE1[opmOld];
            info.rgOppInfo[info.dwCount].domain = DVFS_CORE1_OPP;
            info.dwCount++;

            // update kernel structure
            oppRequest.rgOpps[oppRequest.dwCount] = s_OpmInfo.rgVddCORE1[opmNew];
            oppRequest.rgDomains[oppRequest.dwCount] = DVFS_CORE1_OPP;
            oppRequest.dwCount++;
            }

        // send pre-dvfs notification
        info.ffInfo = dwNotificationMask & DVFS_PRENOTIFY_MASK;
        rc = PmxSendDeviceNotification(DEVICEMEDIATOR_DVFS_LIST,
                info.ffInfo, IOCTL_DVFS_OPPNOTIFY, &info, sizeof(info),
                NULL, 0, NULL);
        if (rc == FALSE) goto cleanUp;
                  
        // change operating points
        rc = KernelIoControl(IOCTL_OPP_REQUEST, &oppRequest, 
                sizeof(IOCTL_OPP_REQUEST_IN), 0, 0, 0
                );
        if (rc == FALSE) goto cleanUp;

        // send post-dvfs notification
        info.ffInfo = dwNotificationMask & DVFS_POSTNOTIFY_MASK;
        PmxSendDeviceNotification(DEVICEMEDIATOR_DVFS_LIST,
                info.ffInfo, IOCTL_DVFS_OPPNOTIFY, &info, sizeof(info),
                NULL, 0, NULL);

        s_OpmInfo.opmCurrent = opmNew;

        NotifyCallbacks(opmNew, idContext);
        }
    else
        {
        rc = TRUE;
        }

cleanUp:
    if (rc == FALSE)
        {
        info.ffInfo = dwNotificationMask & DVFS_CANCELNOTIFY_MASK;
        PmxSendDeviceNotification(DEVICEMEDIATOR_DVFS_LIST,
                info.ffInfo, IOCTL_DVFS_OPPNOTIFY, &info, sizeof(info),
                NULL, 0, NULL);
        }

    return rc;
}

//-----------------------------------------------------------------------------
// 
//  Function:  DVFS_InitConstraint
//
//  Constraint initialization
//
HANDLE
DVFS_InitConstraint(
    _TCHAR const *szContext
    )
{
    LONG code;
    DWORD opm;
    DWORD size;
    HANDLE rc = NULL;
    HKEY hKey = NULL;
    _TCHAR szBuffer[MAX_PATH];
    _TCHAR szRegKey[MAX_PATH];
    DWORD cpuFamily = CPU_FAMILY_OMAP35XX;
    
    // Initialize data structure
    memset(&s_OpmInfo, 0, sizeof(DVFSConstraintInfo_t));
    s_OpmInfo.opmCeiling = kOpm9;
    s_OpmInfo.opmFloor = kOpm0;    
    s_OpmInfo.idxForce = (DWORD) -1;
    s_OpmInfo.opmCurrent = (DWORD) kOpmUndefined;

    KernelIoControl(
        IOCTL_HAL_GET_CPUFAMILY,
        &cpuFamily,
        sizeof(DWORD),
        &cpuFamily,
        sizeof(DWORD),
        NULL
        );

    _tcscpy_s(szRegKey, _countof(szRegKey), szContext);

    if( cpuFamily == CPU_FAMILY_DM37XX)
    {
        _tcscat(szRegKey, _T("\\37xx"));
    }
    else if( cpuFamily == CPU_FAMILY_OMAP35XX)
    {
        _tcscat(szRegKey, _T("\\35xx"));
    }
    else if( cpuFamily == CPU_FAMILY_AM35XX)
    {
        _tcscat(szRegKey, _T("\\3517"));
    }
    else
    {
        RETAILMSG(ZONE_ERROR,(L"DVFS_InitConstraint: Unsupported CPU family=(%x)", cpuFamily));
        goto cleanUp;
    }
	 
    // read registry to get ceiling value    
    code = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, szRegKey, 0, 0, &hKey);
    if (code != ERROR_SUCCESS) goto cleanUp;

    // get default opm
    size = sizeof(Dvfs_OperatingMode_e);
    code = RegQueryValueEx(hKey, REGEDIT_DVFS_DEFAULTOPM, 0, 0, 
            (BYTE*)&opm, &size
            );
    if (code != ERROR_SUCCESS) goto cleanUp;
    s_OpmInfo.opmCurrent= opm;
    
    // get ceiling value
    size = sizeof(Dvfs_OperatingMode_e);
    code = RegQueryValueEx(hKey, REGEDIT_DVFS_CEILING, 0, 0, 
            (BYTE*)&opm, &size
            );
    if (code != ERROR_SUCCESS) goto cleanUp;
    s_OpmInfo.opmCeiling = min(opm, s_OpmInfo.opmCeiling);

    // get Opp --> Opm map for mpu1
    size = sizeof(szBuffer);
    code = RegQueryValueEx(hKey, REGEDIT_DVFS_MPU1, 0, 0, 
            (BYTE*)szBuffer, &size
            );
    if (code != ERROR_SUCCESS) goto cleanUp;
    if (ParseOppToOpmMap(szBuffer, size/sizeof(_TCHAR), 
            s_OpmInfo.rgVddMPU1, kOpmCount) == FALSE
            )
        {
        goto cleanUp;
        }

    // get Opp --> Opm map for core1
    size = sizeof(szBuffer);
    code = RegQueryValueEx(hKey, REGEDIT_DVFS_CORE1, 0, 0, 
            (BYTE*)szBuffer, &size
            );
    if (code != ERROR_SUCCESS) goto cleanUp;
    if (ParseOppToOpmMap(szBuffer, size/sizeof(_TCHAR), 
            s_OpmInfo.rgVddCORE1, kOpmCount) == FALSE
            )
        {
        goto cleanUp;
        }

    // get floor value
    size = sizeof(Dvfs_OperatingMode_e);
    code = RegQueryValueEx(hKey, REGEDIT_DVFS_FLOOR, 0, 0, 
            (BYTE*)&opm, &size
            );
    if (code == ERROR_SUCCESS)
        {
        s_OpmInfo.opmFloor = min(s_OpmInfo.opmCeiling, opm);
        }

    RETAILMSG(1,(L"DVFS_InitConstraint: opmCurrent=%d, opmCeiling=%d, opmFloor=%d\r\n", 
		s_OpmInfo.opmCurrent,
		s_OpmInfo.opmCeiling,
		s_OpmInfo.opmFloor
		));

    InitializeCriticalSection(&s_OpmInfo.cs);
    
    rc = (HANDLE)&s_OpmInfo;

cleanUp:
    if (hKey != NULL) 
        RegCloseKey(hKey);
    return rc;
} 

//-----------------------------------------------------------------------------
// 
//  Function:  DVFS_DeinitConstraint
//
//  Constraint initialization
//
BOOL
DVFS_DeinitConstraint(
    HANDLE hConstraintAdapter
    )
{
    BOOL rc = FALSE;

    // validate parameters
    if (hConstraintAdapter != (HANDLE)&s_OpmInfo) goto cleanUp;

    // reset structure
    memset(&s_OpmInfo, 0, sizeof(DVFSConstraintInfo_t));
    s_OpmInfo.opmFloor = kOpm0;
    s_OpmInfo.opmCeiling = kOpm9;
    s_OpmInfo.idxForce = (DWORD) -1;

    DeleteCriticalSection(&s_OpmInfo.cs);

    rc = TRUE;

cleanUp:
    return rc;
} 

//-----------------------------------------------------------------------------
// 
//  Function:  DVFS_CreateConstraint
//
//  Constraint uninitialization
//
HANDLE
DVFS_CreateConstraint(
    HANDLE hConstraintAdapter
    )
{
    DWORD id;
    DWORD *pDataNode;
    HANDLE rc = NULL;
    
    // validate parameters
    if (hConstraintAdapter != (HANDLE)&s_OpmInfo) goto cleanUp;

    // get new index
    if (s_IndexList.NewIndex(&pDataNode, &id) == FALSE)
        {
        goto cleanUp;
        }

    // initialize values
    rc = (HANDLE)(id + 1);
    *pDataNode = (DWORD) kOpmUndefined;

cleanUp:    
    return rc;
} 

//-----------------------------------------------------------------------------
// 
//  Function:  DVFS_UpdateConstraint
//
//  Constraint update
//
BOOL
DVFS_UpdateConstraint(
    HANDLE hConstraintContext, 
    DWORD msg, 
    void *pParam, 
    UINT  size
    )
{
    DWORD opmNew;
    DWORD dwParam;    
    BOOL rc = FALSE;
    DWORD *pDataNode;
    DWORD id = (DWORD)hConstraintContext - 1;
    
    // validate parameters
    if (size != sizeof(DWORD)) goto cleanUp;
    if (id > s_IndexList.MaxIndex()) goto cleanUp;

    // get data node
    pDataNode = s_IndexList.GetIndex(id);
    if (pDataNode == NULL) goto cleanUp;

    // determine new opm
    dwParam = *(DWORD*)pParam;
    if (CONSTRAINT_STATE_NULL == dwParam)
        {
        //OALMSG(1, (L"DVFS_UpdateConstraint - undefined\r\n"));
        opmNew = (DWORD) kOpmUndefined;
        }
    else if (CONSTRAINT_STATE_FLOOR == dwParam)
        {
        //OALMSG(1, (L"DVFS_UpdateConstraint - FLOOR\r\n"));
        opmNew = s_OpmInfo.opmFloor;
        }
    else 
        {
        //OALMSG(1, (L"DVFS_UpdateConstraint - OPM%d\r\n", dwParam));
        opmNew = min(dwParam, s_OpmInfo.opmCeiling);
        }

    if (s_OpmInfo.idxForce == id)
        {
        s_OpmInfo.idxForce = (DWORD) -1;
        }

    // serialize access
    Lock();
           
    // process constraint message
    switch (msg)
        {
        case DVFS_FORCE:
            s_OpmInfo.idxForce = id;

            //fall-through
            
        case CONSTRAINT_MSG_DVFS_REQUEST:
            // update new constraint
            if (*pDataNode != kOpmUndefined)
                {
                s_OpmInfo.opmTable[*pDataNode] -= 1;
                }

            if (opmNew != kOpmUndefined)
                {
                s_OpmInfo.opmTable[opmNew] += 1;
                }
            *pDataNode = opmNew;
            break;
        }
    
    // update operating mode
    rc = UpdateOpm(id);
    
    Unlock();

cleanUp:
    return rc;
} 

//-----------------------------------------------------------------------------
// 
//  Function:  DVFS_CloseConstraint
//
//  Constraint releasing constraint handle
//
BOOL
DVFS_CloseConstraint(
    HANDLE hConstraintContext
    )
{
    DWORD *pDataNode;
    BOOL rc = FALSE;
    DWORD id = (DWORD)hConstraintContext - 1;
    list<DVFSCallbackInfo_t*>::iterator iter;

    // validate
    if (id > s_IndexList.MaxIndex()) goto cleanUp;

    // get data node
    pDataNode = s_IndexList.GetIndex(id);
    if (pDataNode == NULL) goto cleanUp;

    Lock();

    // free any forced constraint
    if (s_OpmInfo.idxForce == id)
        {
        s_OpmInfo.idxForce = (DWORD) -1;
        }

    // update operating mode
    if (*pDataNode != kOpmUndefined)
        {
        s_OpmInfo.opmTable[*pDataNode] -= 1;
        UpdateOpm(id);
        }
       
    // remove any associated callbacks
    for (iter = s_CallbackList.begin(); iter != s_CallbackList.end(); ++iter)
        {
        if ((*iter)->hConstraintContext == hConstraintContext)
            {
            // free resources
            LocalFree(*iter);
            s_CallbackList.erase(iter);
            rc = TRUE;
            break;
            }
        }

    s_IndexList.DeleteIndex(id);    
    Unlock();
    
    
    rc = TRUE;
cleanUp:    
    return rc;
} 

//-----------------------------------------------------------------------------
// 
//  Function:  DVFS_InsertConstraintCallback
//
//  Constraint callback registeration
//
HANDLE
DVFS_InsertConstraintCallback(
    HANDLE hConstraintContext, 
    void *pCallback, 
    void *pParam, 
    UINT  size,
    HANDLE hRefContext
    )
{
    HANDLE rc = NULL;
    DWORD *pDataNode;
    DWORD id = (DWORD)hConstraintContext - 1;
    DVFSCallbackInfo_t *pCallbackInfo;

    UNREFERENCED_PARAMETER(size);
    UNREFERENCED_PARAMETER(pParam);
    // validate
    if (id > s_IndexList.MaxIndex()) goto cleanUp;

    // get data node
    pDataNode = s_IndexList.GetIndex(id);
    if (pDataNode == NULL) goto cleanUp;

    // allocate callback structure
    pCallbackInfo = (DVFSCallbackInfo_t*)LocalAlloc(LPTR, sizeof(DVFSCallbackInfo_t));
    if (pCallbackInfo == NULL) goto cleanUp;

    // initialize structure
    pCallbackInfo->fnCallback = (ConstraintCallback)pCallback;
    pCallbackInfo->hConstraintContext = hConstraintContext;
    pCallbackInfo->hCallbackContext = pCallbackInfo;
    pCallbackInfo->hRefContext = hRefContext;
    
    // insert into callback list
    Lock();
    s_CallbackList.push_back(pCallbackInfo);
    Unlock();
    
cleanUp:
    return rc;
} 

//-----------------------------------------------------------------------------
// 
//  Function:  DVFS_RemoveConstraintCallback
//
//  Constraint callback unregisterations
//
BOOL
DVFS_RemoveConstraintCallback(
    HANDLE hConstraintCallback
    )
{
    BOOL rc = FALSE;
    list<DVFSCallbackInfo_t*>::iterator iter;
    
    // find entry with matching callback info
    Lock();
    for (iter = s_CallbackList.begin(); iter != s_CallbackList.end(); ++iter)
        {
        if ((*iter)->hCallbackContext == hConstraintCallback)
            {
            // free resources
            LocalFree(*iter);
            s_CallbackList.erase(iter);
            rc = TRUE;
            break;
            }
        }
    Unlock();

    return rc;
} 

//------------------------------------------------------------------------------
