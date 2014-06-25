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
//  File:  systemstate.cpp
//

//******************************************************************************
//  #INCLUDES
//******************************************************************************
#include "omap.h"
#include <windows.h>
#include <oal.h>
#include <oalex.h>
#include <ceddk.h>
#include <ceddkex.h>
#include "omap_dvfs.h"
#include <ceddkex.h>

//------------------------------------------------------------------------------
//
//  Global:  dpCurSettings
//
//  Set debug zones names and initial setting for driver
//
#ifndef SHIP_BUILD

#ifndef ZONE_ERROR
#define ZONE_ERROR          DEBUGZONE(0)
#endif

#define ZONE_WARN           DEBUGZONE(1)
#define ZONE_FUNCTION    DEBUGZONE(2)
#define ZONE_INFO            DEBUGZONE(3)

//------------------------------------------------------------------------------
//
//  Global:  dpCurSettings
//
DBGPARAM dpCurSettings = {
    L"ProxyDriver", {
        L"Errors",      L"Warnings",    L"Function",    L"Info",
        L"Undefined" ,  L"Undefined",   L"Undefined",   L"Undefined",
        L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined",
        L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined"
    },
    0x0000
};

#endif

//------------------------------------------------------------------------------
//  Local Definitions
#define POLICY_CONTEXT_COOKIE       'syst'

#define CONSTRAINT_ID_DVFS          L"DVFS"

#define QUEUE_ENTRIES       3
#define MAX_NAMELEN         200
#define QUEUE_SIZE        (QUEUE_ENTRIES * (sizeof(POWER_BROADCAST) + MAX_NAMELEN))

//-----------------------------------------------------------------------------
//  local structures

typedef enum 
{
    UserOpm = 0,
    ThreadPriority,
    BootEventName,
    Total,
} Registry_e;

typedef struct
{
    HANDLE                           hDvfsConstraint;
    HANDLE                           hSystemSateThread;
    HANDLE                           hPowerNotificationQueue;
    HANDLE                           hPowerNotificationsHandle;
    HANDLE                           hBootEvent;
    DWORD                           dwUserOpm;
    DWORD                           dwThreadPriority;
} SystenStatePolicyInfo_t;

typedef struct
{
    LPTSTR                          lpRegname;
    DWORD                         dwSize;
} REGISTRY_PARAMETERS;

//-----------------------------------------------------------------------------
//  local variables
static SystenStatePolicyInfo_t          s_SystenStatePolicyInfo;

REGISTRY_PARAMETERS                 g_reg[]=
{
    {
        L"UserOpm",   sizeof(DWORD)
    },{
        L"priority256",     sizeof(DWORD)
    }
};


//-----------------------------------------------------------------------------
// 
//  Function:  GetRegVal
//
//  Reads policy values from the registry
//
BOOL GetRegVal(
        WCHAR *szRegPath
        )
{
    BOOL rc=FALSE;
    HKEY hOpenedKey=NULL;

    if(ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                        szRegPath,
                                        0,
                                        0,
                                        &hOpenedKey))
        {
        goto cleanUp;
        }
    
    if (ERROR_SUCCESS != RegQueryValueEx(hOpenedKey,
                                        g_reg[UserOpm].lpRegname,
                                        NULL,
                                        NULL,
                                        (UCHAR *)&s_SystenStatePolicyInfo.dwUserOpm,
                                        &g_reg[UserOpm].dwSize))
        {
        goto closeKey;
        }

    if (ERROR_SUCCESS != RegQueryValueEx(hOpenedKey,
                                        g_reg[ThreadPriority].lpRegname,
                                        NULL,
                                        NULL,
                                        (UCHAR *)&s_SystenStatePolicyInfo.dwThreadPriority,
                                        &g_reg[ThreadPriority].dwSize))
        {
        goto closeKey;
        }

    rc = TRUE;

closeKey:

    if(ERROR_SUCCESS != RegCloseKey(hOpenedKey))
        {
        rc = FALSE;
        goto cleanUp;
        }

cleanUp:

    return rc;
}

//-----------------------------------------------------------------------------
// 
//  Function:  UpdateDVFSConstraint
//
//  Updates the OPM level
//
VOID
UpdateDVFSConstraint(
    DWORD dwOpm
    )
{
    PmxUpdateConstraint(
        s_SystenStatePolicyInfo.hDvfsConstraint, 
        CONSTRAINT_MSG_DVFS_REQUEST, 
        (void*)&dwOpm, 
        sizeof(DWORD)
        );
}

//-----------------------------------------------------------------------------
// 
//  Function:  SystenStateThreadFn
//
//  Contains the Power state transition listener
//
DWORD WINAPI
SystenStateThreadFn(LPVOID pvParam)
{
    union
    {
        UCHAR buf[QUEUE_SIZE];
        POWER_BROADCAST powerBroadcast;
    } u;

    UNREFERENCED_PARAMETER(pvParam);

    MSGQUEUEOPTIONS msgOptions = {0}; 
    POWER_BROADCAST powerInfo;
    DWORD dwOpm;    
    DWORD dwPowerFlags = 0;
    DWORD dwBytesRead = 0;
    
    msgOptions.dwSize = sizeof(MSGQUEUEOPTIONS);
    msgOptions.dwFlags = 0;
    msgOptions.dwMaxMessages = QUEUE_ENTRIES;
    msgOptions.cbMaxMessage = sizeof(POWER_BROADCAST) + MAX_NAMELEN;
    msgOptions.bReadAccess = TRUE;

    // Create Message Queue to revice system power state changes
    s_SystenStatePolicyInfo.hPowerNotificationQueue = CreateMsgQueue(NULL, &msgOptions);
    if (!s_SystenStatePolicyInfo.hPowerNotificationQueue) 
        {
        RETAILMSG(ZONE_ERROR,(L"System State Policy: ERROR: MSGQUEUE creation fail"));
        goto cleanUp;
        }

    HANDLE hPMReady = CreateEvent(NULL, TRUE, FALSE, _T("SYSTEM/PowerManagerReady"));
    if (hPMReady)
    {
        WaitForSingleObject(hPMReady,2000);
        CloseHandle(hPMReady);
    }
    

    // Register the Message Queue to the PM
    s_SystenStatePolicyInfo.hPowerNotificationsHandle = 
        RequestPowerNotifications(
            s_SystenStatePolicyInfo.hPowerNotificationQueue,
            PBT_TRANSITION
            );
    if (!s_SystenStatePolicyInfo.hPowerNotificationsHandle) 
        {
        RETAILMSG(ZONE_ERROR,(L"System State Policy:"
            L" ERROR: Power Notification request fail"));
        goto cleanUp;
        }
    
    for(;;)
        {
        WaitForSingleObject(s_SystenStatePolicyInfo.hPowerNotificationQueue, INFINITE);

        while (ReadMsgQueue(s_SystenStatePolicyInfo.hPowerNotificationQueue, 
            u.buf, QUEUE_SIZE, &dwBytesRead, INFINITE, &dwPowerFlags))
            {
            
            if(dwBytesRead >= sizeof(POWER_BROADCAST)) 
                {
                powerInfo =  u.powerBroadcast;
                dwPowerFlags = powerInfo.Flags;
                
                switch (powerInfo.Message) 
                    {
                    case PBT_TRANSITION:
                    
                        if((dwPowerFlags  & 
                            (POWER_STATE_BACKLIGHTON | 
                            POWER_STATE_PASSWORD | 
                            POWER_STATE_ON)) != 0
                            )
                            {
                            RETAILMSG(ZONE_INFO,(L"System State Policy: INFO: User Active"));
                            dwOpm = s_SystenStatePolicyInfo.dwUserOpm;
                            UpdateDVFSConstraint(dwOpm);
                            }
                        else 
                            {
                            RETAILMSG(ZONE_INFO,(L"System State Policy: INFO: User Inactive"));
                            dwOpm = CONSTRAINT_STATE_NULL;
                            UpdateDVFSConstraint(dwOpm);
                            }
                        
                    break;
                    }
                
                dwPowerFlags = 0;
                dwBytesRead = 0;
                memset(u.buf, 0, QUEUE_SIZE);
                }
            
            }

        }
cleanUp:
    RETAILMSG(ZONE_ERROR,(L"System State Policy: ERROR: Exiting monitor thread!"));
    return 0;
}

//-----------------------------------------------------------------------------
// 
//  Function:  SYSTEMSTATE_InitPolicy
//
//  Policy  initialization
//

HANDLE
SYSTEMSTATE_InitPolicy(
    _TCHAR const *szContext
    )
{
    DWORD dwLevel;

    // initializt global structure
    memset(&s_SystenStatePolicyInfo, 0, sizeof(SystenStatePolicyInfo_t));       

    // Read registries
    if (GetRegVal((WCHAR*)szContext) == FALSE) return NULL;
    
    // Set boot opm value
    dwLevel = CONSTRAINT_STATE_NULL;

    // Obtain DVFS constraint handler 
    s_SystenStatePolicyInfo.hDvfsConstraint = PmxSetConstraintById(
                                    CONSTRAINT_ID_DVFS, 
                                    CONSTRAINT_MSG_DVFS_REQUEST, 
                                    (void*)&dwLevel, 
                                    sizeof(DWORD)
                                    );

    // Start running the thread that will check for the cpu load
    s_SystenStatePolicyInfo.hSystemSateThread = CreateThread(NULL, 
                                            0,
                                            SystenStateThreadFn,
                                            NULL,
                                            0,
                                            NULL
                                            );
    
    SetThreadPriority(s_SystenStatePolicyInfo.hSystemSateThread,
                           s_SystenStatePolicyInfo.dwThreadPriority
                           );

    return (HANDLE)&s_SystenStatePolicyInfo;
} 

//-----------------------------------------------------------------------------
// 
//  Function:  SYSTEMSTATE_DeinitPolicy
//
//  Policy uninitialization
//
BOOL
SYSTEMSTATE_DeinitPolicy(
    HANDLE hPolicyAdapter
    )
{
    BOOL rc = FALSE;

    // validate parameters
    if (hPolicyAdapter != (HANDLE)&s_SystenStatePolicyInfo) goto cleanUp;

    // release all resoureces	
    if (s_SystenStatePolicyInfo.hDvfsConstraint != NULL)
        {
        PmxReleaseConstraint(s_SystenStatePolicyInfo.hDvfsConstraint);
        }
    
    if (s_SystenStatePolicyInfo.hSystemSateThread != NULL)
        {
        TerminateThread(s_SystenStatePolicyInfo.hSystemSateThread,0);
        }

    if (s_SystenStatePolicyInfo.hPowerNotificationsHandle)
        {
        StopPowerNotifications(s_SystenStatePolicyInfo.hPowerNotificationsHandle);        
        }

    if (s_SystenStatePolicyInfo.hPowerNotificationQueue) 
        {
        CloseMsgQueue(s_SystenStatePolicyInfo.hPowerNotificationQueue);
        }

    // clear structures
    memset(&s_SystenStatePolicyInfo, 0, sizeof(SystenStatePolicyInfo_t));
    
    rc = TRUE;

cleanUp:
    return rc;
} 

//-----------------------------------------------------------------------------

