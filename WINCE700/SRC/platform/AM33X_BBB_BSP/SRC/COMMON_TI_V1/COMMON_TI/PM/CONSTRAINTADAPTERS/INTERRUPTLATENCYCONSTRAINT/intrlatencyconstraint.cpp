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
//  File:  intrlatencyconstraint.cpp
//
//
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

#ifndef MAX_INT
#define MAX_INT
#endif

using namespace std;

//-----------------------------------------------------------------------------
//  registry key names
//
#define IL_CONSTRAINT_NULL              (-1.0f)

//-----------------------------------------------------------------------------
//  local structures
typedef struct {
    CRITICAL_SECTION                    cs;
    float                               current;    
} InterruptLatencyConstraintInfo_t;

//-----------------------------------------------------------------------------
//  local variables
static IndexList<float>                 s_IndexList;
static InterruptLatencyConstraintInfo_t s_ilInfo;

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
    EnterCriticalSection(&s_ilInfo.cs);
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
    LeaveCriticalSection(&s_ilInfo.cs);
}

//-----------------------------------------------------------------------------
// 
//  Function:  UpdateConstraint
//
//  Updates the operating mode based on current constraints
//
BOOL
UpdateConstraint()
{
    int i;
    BOOL rc = FALSE;    
    float *pDataNode;
    float *pMinLatency = NULL;
    float nullConstraint = IL_CONSTRAINT_NULL;    
        
    // get highest operatin mode if not forced
    for (i = 0; i < s_IndexList.MaxIndex(); ++i)
        {
        pDataNode = s_IndexList.GetIndex(i);
        if (pDataNode != NULL && *pDataNode >= 0.0f)
            {
            if (pMinLatency == NULL || *pDataNode < *pMinLatency)
                {
                pMinLatency = pDataNode;
                }
            }
        }

    // if no minimum latency then use a null constraint to release
    // all constraints
    if (pMinLatency == NULL)
        {
        pMinLatency = &nullConstraint;
        }

    // Prepare to send notifications
    if (s_ilInfo.current != *pMinLatency)
        {                         
        // change operating points
        rc = KernelIoControl(IOCTL_INTERRUPT_LATENCY_CONSTRAINT, pMinLatency, 
                sizeof(float), 0, 0, 0
                );
        if (rc == FALSE) goto cleanUp;
        s_ilInfo.current = *pMinLatency;
        }
    rc = TRUE;

cleanUp:
    return rc;
}

//-----------------------------------------------------------------------------
// 
//  Function:  INTRLAT_InitConstraint
//
//  Constraint initialization
//
HANDLE
INTRLAT_InitConstraint(
    _TCHAR const *szContext
    )
{
    LONG code;
    HANDLE rc = NULL;
    HKEY hKey = NULL;
    
    // Initialize data structure
    memset(&s_ilInfo, 0, sizeof(InterruptLatencyConstraintInfo_t));
    s_ilInfo.current = -1.0f;

    // read registry to get ceiling value    
    code = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, szContext, 0, 0, &hKey);
    if (code != ERROR_SUCCESS) goto cleanUp;

    // intialize synchronization object
    InitializeCriticalSection(&s_ilInfo.cs);
    
    rc = (HANDLE)&s_ilInfo;

cleanUp:
    return rc;
} 

//-----------------------------------------------------------------------------
// 
//  Function:  INTRLAT_DeinitConstraint
//
//  Constraint initialization
//
BOOL
INTRLAT_DeinitConstraint(
    HANDLE hConstraintAdapter
    )
{
    BOOL rc = FALSE;

    // validate parameters
    if (hConstraintAdapter != (HANDLE)&s_ilInfo) goto cleanUp;
    DeleteCriticalSection(&s_ilInfo.cs);

    rc = TRUE;

cleanUp:
    return rc;
} 

//-----------------------------------------------------------------------------
// 
//  Function:  INTRLAT_CreateConstraint
//
//  Constraint uninitialization
//
HANDLE
INTRLAT_CreateConstraint(
    HANDLE hConstraintAdapter
    )
{
    DWORD id;
    float *pDataNode;
    HANDLE rc = NULL;
    
    // validate parameters
    if (hConstraintAdapter != (HANDLE)&s_ilInfo) goto cleanUp;

    // get new index
    if (s_IndexList.NewIndex(&pDataNode, &id) == FALSE)
        {
        goto cleanUp;
        }

    // initialize values
    rc = (HANDLE)(id + 1);
    *pDataNode = IL_CONSTRAINT_NULL;

cleanUp:    
    return rc;
} 

//-----------------------------------------------------------------------------
// 
//  Function:  INTRLAT_UpdateConstraint
//
//  Constraint update
//
BOOL
INTRLAT_UpdateConstraint(
    HANDLE hConstraintContext, 
    DWORD msg, 
    void *pParam, 
    UINT  size
    )
{
    float constraintNew;
    BOOL rc = FALSE;
    float *pDataNode;
    DWORD id = (DWORD)hConstraintContext - 1;
    
    // validate parameters
    if (size != sizeof(float)) goto cleanUp;
    if (id > s_IndexList.MaxIndex()) goto cleanUp;

    // get data node
    pDataNode = s_IndexList.GetIndex(id);
    if (pDataNode == NULL) goto cleanUp;

    // determine new opm
    constraintNew = *(float*)pParam;
    if (CONSTRAINT_STATE_NULL == *(DWORD*)pParam)
        {
        constraintNew = IL_CONSTRAINT_NULL;
        }
    else if (CONSTRAINT_STATE_FLOOR == *(DWORD*)pParam)
        {
        constraintNew = IL_CONSTRAINT_NULL;
        }

    // serialize access
    Lock();
           
    // process constraint message
    switch (msg)
        {
        case CONSTRAINT_MSG_INTRLAT_REQUEST:
            *pDataNode = constraintNew;
            break;
        }
    
    // update operating mode
    rc = UpdateConstraint();
    
    Unlock();

cleanUp:
    return rc;
} 

//-----------------------------------------------------------------------------
// 
//  Function:  INTRLAT_CloseConstraint
//
//  Constraint releasing constraint handle
//
BOOL
INTRLAT_CloseConstraint(
    HANDLE hConstraintContext
    )
{
    BOOL rc = FALSE;
    float *pDataNode;    
    DWORD id = (DWORD)hConstraintContext - 1;

    // validate
    if (id > s_IndexList.MaxIndex()) goto cleanUp;

    // get data node
    pDataNode = s_IndexList.GetIndex(id);
    if (pDataNode == NULL) goto cleanUp;

    Lock();       
    s_IndexList.DeleteIndex(id);
    rc = UpdateConstraint();
    Unlock();
    
cleanUp:    
    return rc;
} 
//------------------------------------------------------------------------------
