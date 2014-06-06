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
/*++


Module Name:

    mon.c

Abstract:

    Power Manager Monitor.
    Run in the background to display power state changes in debugger.

Notes:

Revision History:

--*/
#pragma warning(push)
#pragma warning(disable : 4115 4057 4127 6320 6322)

#include <windows.h>
#include <msgqueue.h>
#include <pnp.h>
#include <guiddef.h>
#include <pm.h>

#define QUEUE_ENTRIES   3
#define MAX_NAMELEN     200
#define QUEUE_SIZE      (QUEUE_ENTRIES * (sizeof(POWER_BROADCAST) + MAX_NAMELEN))

// device notification queue parameters
#define PNP_QUEUE_ENTRIES       3
#define PNP_MAX_NAMELEN         128
#define PNP_QUEUE_SIZE          (PNP_QUEUE_ENTRIES * (sizeof(DEVDETAIL) + (PNP_MAX_NAMELEN * sizeof(TCHAR))))

#define PWRMGR_REG_KEY      TEXT("SYSTEM\\CurrentControlSet\\Control\\Power")

// global variables
HANDLE ghPowerNotifications = NULL;
HANDLE ghDeviceNotifications = NULL;
HANDLE ghevTerminate = NULL;

// this macro goes with the sFlagInfo structure below
#define FLAGINFO(x)     { x, L#x }
#define dim(x)          (sizeof(x) / sizeof(x[0]))

void RetailPrint(wchar_t *pszFormat, ...);

void ProcessPowerNotification(HANDLE hMsgQ)
{
    union {
        UCHAR buf[QUEUE_SIZE];
        POWER_BROADCAST powerBroadcast;
    } u;
    int iBytesInQueue;
    DWORD dwFlags = 0;
    int dwCount = 0;
    LPTSTR pszFname = _T("PMMON!ProcessPowerNotification");

    iBytesInQueue = 0;
    memset(u.buf, 0, QUEUE_SIZE);

    if ( !ReadMsgQueue(hMsgQ,
                       u.buf,
                       QUEUE_SIZE,
                       &iBytesInQueue,
                       INFINITE,    // Timeout
                       &dwFlags))
    {
        DWORD dwErr = GetLastError();
        RetailPrint  (TEXT("%s: ReadMsgQueue: ERROR:%d\n"), pszFname, dwErr);
        ASSERT(0);
    } else if(iBytesInQueue >= sizeof(POWER_BROADCAST)) {
        //
        // process power notifications
        //
        PPOWER_BROADCAST pB = &u.powerBroadcast;
        dwFlags = pB->Flags;
        
        RetailPrint  (TEXT("%s: *** Power Notification @ Tick:%u, Count:%d***\n"), pszFname,
            GetTickCount(), dwCount++);
        switch (pB->Message) 
        {
        case PBT_TRANSITION:
            RetailPrint  (TEXT("%s:\tPBT_TRANSITION to system power state: '%s'\n"), pszFname, pB->SystemPowerState);
            RetailPrint  (TEXT("%s:\t0x%08x "), pszFname, dwFlags);
            {
                struct {
                    DWORD dwFlag;
                    LPCTSTR pszFlagName;
                } sFlagInfo[] = {
                    FLAGINFO(POWER_STATE_ON),
                    FLAGINFO(POWER_STATE_OFF),
                    FLAGINFO(POWER_STATE_CRITICAL),
                    FLAGINFO(POWER_STATE_BOOT),
                    FLAGINFO(POWER_STATE_IDLE),
                    FLAGINFO(POWER_STATE_SUSPEND),
                    FLAGINFO(POWER_STATE_RESET),
                    FLAGINFO(POWER_STATE_PASSWORD),
                };
                int i;

                // display each bit that's in the mask
                for(i = 0; i < dim(sFlagInfo); i++) {
                    if((dwFlags & sFlagInfo[i].dwFlag) != 0) {
                        RetailPrint  (TEXT("%s "), sFlagInfo[i].pszFlagName);
                        dwFlags &= ~sFlagInfo[i].dwFlag;        // mask off the bit
                    }
                }
                if(dwFlags != 0) {
                    RetailPrint  (TEXT("***UNKNOWN***"));
                }
            }
            RetailPrint  (TEXT("\r\n"));
            break;
            
            case PBT_RESUME:
                RetailPrint  (TEXT("%s:\tPBT_RESUME\n"), pszFname);
                break;
                
            case PBT_POWERSTATUSCHANGE:
                RetailPrint  (TEXT("%s:\tPBT_POWERSTATUSCHANGE\n"), pszFname);
                break;
                
            case PBT_POWERINFOCHANGE:
                {
                    PPOWER_BROADCAST_POWER_INFO ppbpi = (PPOWER_BROADCAST_POWER_INFO) pB->SystemPowerState;

                    RetailPrint  (TEXT("%s:\tPBT_POWERINFOCHANGE\n"), pszFname);
                    RetailPrint  (TEXT("%s:\t\tAC line status %u, battery flag %u, backup flag %u, %d levels\n"), pszFname,
                        ppbpi->bACLineStatus, ppbpi->bBatteryFlag, ppbpi->bBackupBatteryFlag, ppbpi->dwNumLevels);
                    RetailPrint  (TEXT("%s:\t\tbattery life %d, backup life %d\n"), pszFname,
                        ppbpi->bBatteryLifePercent, ppbpi->bBackupBatteryLifePercent);
                    RetailPrint  (TEXT("%s:\t\tlifetime 0x%08x, full lifetime 0x%08x\n"), pszFname, 
                        ppbpi->dwBatteryLifeTime, ppbpi->dwBatteryFullLifeTime);
                    RetailPrint  (TEXT("%s:\t\tbackup lifetime 0x%08x, backup full lifetime 0x%08x\n"), pszFname, 
                        ppbpi->dwBackupBatteryLifeTime, ppbpi->dwBackupBatteryFullLifeTime);
                }
                break;
                
            default:
                RetailPrint  (TEXT("%s:\tUnknown Message:%d\n"), pszFname, pB->Message);
                break;
        }

        RetailPrint  (TEXT("%s:\tMessage: 0x%x\n"), pszFname, pB->Message);
        RetailPrint  (TEXT("%s:\tFlags: 0x%x\n"), pszFname, pB->Flags);
        RetailPrint  (TEXT("%s:\tdwLen: %d\n"), pszFname, pB->Length);
        
        RetailPrint  (TEXT("%s:***********************\n"), pszFname);
    } else {
        RetailPrint  (TEXT("%s:\tReceived short message: %d bytes\n"), pszFname, iBytesInQueue);
        ASSERT(0);
    }
}

void
ProcessDeviceNotification(HANDLE hMsgQ)
{
    union {
        UCHAR deviceBuf[PNP_QUEUE_SIZE];
        DEVDETAIL devDetail;
    } u;
    DWORD iBytesInQueue = 0;
    DWORD dwFlags = 0;
    LPTSTR pszFname = _T("PMMON!ProcessDeviceNotification");

    // read a message from the message queue -- it should be a device advertisement
    memset(u.deviceBuf, 0, PNP_QUEUE_SIZE);
    if ( !ReadMsgQueue(hMsgQ, u.deviceBuf, PNP_QUEUE_SIZE, &iBytesInQueue, 0, &dwFlags)) {
        // nothing in the queue
        RetailPrint  (_T("%s: ReadMsgQueue() failed %d\r\n"), pszFname,
            GetLastError());
    } else if(iBytesInQueue >= sizeof(DEVDETAIL)) {
        // process the message
        PDEVDETAIL pDevDetail = &u.devDetail;
        //DWORD dwMessageSize = sizeof(DEVDETAIL) + pDevDetail->cbName;
        
        // check for overlarge names
        if(pDevDetail->cbName > ((PNP_MAX_NAMELEN - 1) * sizeof(pDevDetail->szName[0]))) {
            RetailPrint  (_T("%s: device name longer than %d characters\r\n"), 
                pszFname, PNP_MAX_NAMELEN - 1);
        } else {
            LPCTSTR pszName = pDevDetail->szName;
            LPCGUID guidDevClass = &pDevDetail->guidDevClass;
            TCHAR szClassDescription[MAX_PATH];
            TCHAR szClassName[MAX_PATH];
            HKEY hk;
            DWORD dwStatus;

            // format the class name
            _stprintf(szClassName, _T("{%08x-%04x-%04x-%04x-%02x%02x%02x%02x%02x%02x}"), 
                guidDevClass->Data1, guidDevClass->Data2, guidDevClass->Data3,
                (guidDevClass->Data4[0] << 8) + guidDevClass->Data4[1], guidDevClass->Data4[2], guidDevClass->Data4[3], 
                guidDevClass->Data4[4], guidDevClass->Data4[5], guidDevClass->Data4[6], guidDevClass->Data4[7]);

            // (inefficiently) look up the class description whenever a device registers
            wsprintf(szClassDescription, _T("%s\\Interfaces"), PWRMGR_REG_KEY);
            dwStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szClassDescription, 0, 0, &hk);
            if(dwStatus == ERROR_SUCCESS) {
                DWORD dwSize = sizeof(szClassDescription);
                dwStatus = RegQueryValueEx(hk, szClassName, NULL, NULL, (LPBYTE) szClassDescription, &dwSize);
                RegCloseKey(hk);
            }
            if(dwStatus != ERROR_SUCCESS) {
                _tcscpy(szClassDescription, (_T("<unknown class>")));
            }
            
            RetailPrint  
                (_T("%s: device '%s' %s class %s ('%s')\r\n"), pszFname,
                pszName, pDevDetail->fAttached ? _T("joining") : _T("leaving"),
                szClassName, szClassDescription);
        }
    } else {
        // not enough bytes for a message
        RetailPrint  (_T("%s: got runt message (%d bytes)\r\n"), pszFname, 
            iBytesInQueue);
    }
}

int WINAPI MonThreadProc(LPVOID pvParam)
{
    HANDLE hEvents[3];
    LPTSTR pszFname = _T("PMMON!MonThreadProc");

    UNREFERENCED_PARAMETER(pvParam);

    ASSERT(ghPowerNotifications != NULL);
    ASSERT(ghDeviceNotifications != NULL);
    ASSERT(ghevTerminate != NULL);

    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

    hEvents[0] = ghPowerNotifications;
    hEvents[1] = ghDeviceNotifications;
    hEvents[2] = ghevTerminate;

    while (1) 
    {
        DWORD dwStatus;

        // Block on our message queue.
        // This thread runs when the power manager writes a notification into the queue.
        dwStatus = WaitForMultipleObjects(3, hEvents, FALSE, INFINITE);
        if(dwStatus == WAIT_OBJECT_0) {
            ProcessPowerNotification(ghPowerNotifications);
        } else if(dwStatus == (WAIT_OBJECT_0 + 1)) {
            ProcessDeviceNotification(ghDeviceNotifications);
        } else if(dwStatus == (WAIT_OBJECT_0 + 2)) {
            RetailPrint  (TEXT("%s: termination event signaled\r\n"), pszFname);
        } else {
            RetailPrint  (TEXT("%s: WaitForMultipleObjects returned %d (error %d)\r\n"), pszFname,
                dwStatus, GetLastError());
            break;
        }
    }

    return 0;
}

// this routine converts a string into a GUID and returns TRUE if the
// conversion was successful.
BOOL 
ConvertStringToGuid (LPCTSTR pszGuid, GUID *pGuid)
{
	UINT Data4[8];
	int  Count;
	BOOL fOk = FALSE;
	TCHAR *pszGuidFormat = _T("{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}");

	ASSERT(pGuid != NULL && pszGuid != NULL);
	__try
	{
		if (_stscanf(pszGuid, pszGuidFormat, &pGuid->Data1, 
		    &pGuid->Data2, &pGuid->Data3, &Data4[0], &Data4[1], &Data4[2], &Data4[3], 
		    &Data4[4], &Data4[5], &Data4[6], &Data4[7]) == 11)
		{
			for(Count = 0; Count < (sizeof(Data4) / sizeof(Data4[0])); Count++)
			{
	        	        pGuid->Data4[Count] = (UCHAR) Data4[Count];
			}
		}
		fOk = TRUE;
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{		
	}

	return fOk;
}

BOOL
RequestPMDeviceNotifications(DWORD cNotifications, HANDLE *phNotifications)
{
    BOOL fOk = TRUE;
    DWORD dwStatus;
    HKEY hk;
    TCHAR szBuf[MAX_PATH];
    LPTSTR pszFname = _T("PMMON!RequestPMDeviceNotifications");

    // enumerate all the device classes
    wsprintf(szBuf, _T("%s\\Interfaces"), PWRMGR_REG_KEY);
    dwStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szBuf, 0, 0, &hk);
    if(dwStatus == ERROR_SUCCESS) {
        DWORD dwIndex = 0;
        DWORD dwNotificationIndex = 0;
        do {
            DWORD cbValueName = MAX_PATH, dwType;
            GUID idInterface;

            dwStatus = RegEnumValue(hk, dwIndex, szBuf, &cbValueName, NULL,
                &dwType, NULL, NULL);
            if(dwStatus == ERROR_SUCCESS) {
                if(dwType != REG_SZ) {
                    RetailPrint  (_T("%s: invalid type for value '%s'\r\n"),
                        pszFname, szBuf);
                } else if(!ConvertStringToGuid(szBuf, &idInterface)) {
                    RetailPrint  (_T("%s: can't convert '%s' to GUID\r\n"),
                        pszFname, szBuf);
                } else if((phNotifications[dwNotificationIndex] = RequestDeviceNotifications(&idInterface, ghDeviceNotifications, TRUE)) == NULL) {
                    RetailPrint  (_T("%s: RequestDeviceNotifications('%s') failed\r\n"), 
                        pszFname, szBuf);
                } else {
                    dwNotificationIndex++;
                }

                // update the index
                dwIndex++;
            }
        } while(dwStatus == ERROR_SUCCESS && dwNotificationIndex < cNotifications);

        // check for abnormal termination of the loop
        if(dwStatus != ERROR_NO_MORE_ITEMS) {
            fOk = FALSE;
        }

        // close the registry handle
        RegCloseKey(hk);
    }

    return fOk;
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPWSTR lpCmdLine, int nCmShow)
{
    HANDLE hNotifications = NULL;
    HANDLE hDeviceNotifications[64];
    HANDLE ht;
    int i;
    LPTSTR pszFname = _T("PMMON:WinMain");
    
    MSGQUEUEOPTIONS msgOptions = {0};   

    UNREFERENCED_PARAMETER(hInst);
    UNREFERENCED_PARAMETER(hPrevInst);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmShow);

    // clear globals
    memset(hDeviceNotifications, 0, sizeof(hDeviceNotifications));

    // create a termination event
    ghevTerminate = CreateEvent(NULL, FALSE, FALSE, _T("SOFTWARE/PMTestPrograms/PMMON/Terminate"));
    if(ghevTerminate == NULL) {
        RetailPrint  (_T("%s: CreateEvent() failed %d for termination event\r\n"),
            pszFname, GetLastError());
        goto _Exit;
    }

    // did the event already exist?
    if(GetLastError() == ERROR_ALREADY_EXISTS) {
        // yes, kill the existing process
        RetailPrint  (_T("%s: Signaling termination event\r\n"), pszFname);
        SetEvent(ghevTerminate);
        goto _Exit;
    }
    
    // create a message queue for Power Manager notifications
    msgOptions.dwSize = sizeof(MSGQUEUEOPTIONS);
    msgOptions.dwFlags = 0;
    msgOptions.dwMaxMessages = QUEUE_ENTRIES;
    msgOptions.cbMaxMessage = sizeof(POWER_BROADCAST) + MAX_NAMELEN;
    msgOptions.bReadAccess = TRUE;

    ghPowerNotifications= CreateMsgQueue(NULL, &msgOptions);
    if (!ghPowerNotifications) {
        DWORD dwErr = GetLastError();
        RetailPrint  (TEXT("%s:CreateMessageQueue ERROR:%d\n"), pszFname, dwErr);
        goto _Exit;
    }

    // request Power notifications
    hNotifications = RequestPowerNotifications(ghPowerNotifications, POWER_NOTIFY_ALL); // Flags
    if (!hNotifications) {
        DWORD dwErr = GetLastError();
        RetailPrint  (TEXT("%s:RequestPowerNotifications ERROR:%d\n"), pszFname, dwErr);
        goto _Exit;
    }

    // create message queues
    memset(&msgOptions, 0, sizeof(msgOptions));
    msgOptions.dwSize = sizeof(MSGQUEUEOPTIONS);
    msgOptions.dwFlags = 0;
    msgOptions.cbMaxMessage = PNP_QUEUE_SIZE;
    msgOptions.bReadAccess = TRUE;
    ghDeviceNotifications = CreateMsgQueue(NULL, &msgOptions);
    if(ghDeviceNotifications == NULL) {
        DWORD dwStatus = GetLastError();
        RetailPrint  (_T("%s:CreateMsgQueue() failed %d\r\n"), pszFname, dwStatus);
        goto _Exit;
    }

    // create the monitoring thread
    ht = CreateThread(NULL, 0, MonThreadProc, NULL, 0, NULL);
    if(ht) {
        // request device notifications (do this after the thread is
        // created so we don't drop any notifications)
        RequestPMDeviceNotifications(64, hDeviceNotifications);

        // wait for the thread to exit
        WaitForSingleObject(ht, INFINITE);
        CloseHandle(ht);
    }

_Exit:
    if(hNotifications) StopPowerNotifications(hNotifications);
    if(ghPowerNotifications) CloseMsgQueue(ghPowerNotifications);
    for(i = 0; i < 64 && hDeviceNotifications[i] != NULL; i++) {
        StopDeviceNotifications(hDeviceNotifications[i]);
    }
    if(ghDeviceNotifications) CloseMsgQueue(ghDeviceNotifications);
    if(ghevTerminate) CloseHandle(ghevTerminate);
    RetailPrint  (_T("%s: exiting\r\n"), pszFname);

    return 0;
}



///////////////////////////////////////////////////////////////////////////////

void RetailPrint(wchar_t *pszFormat, ...)
{
    va_list al;
    wchar_t szTemp[2048];
    wchar_t szTempFormat[2048];

    va_start(al, pszFormat);

    // Show message on console
    vwprintf(pszFormat, al);

    // Show message on RETAILMSG
    swprintf(szTempFormat, L"PMGETD: %s", pszFormat);
    pszFormat = szTempFormat;
    vswprintf(szTemp, pszFormat, al);
    RETAILMSG(1, (szTemp));

    va_end(al);
}

// EOF

#pragma warning(pop)
