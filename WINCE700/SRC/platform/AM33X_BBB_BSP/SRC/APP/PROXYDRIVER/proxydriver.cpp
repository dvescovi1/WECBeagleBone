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

#pragma warning(push)
#pragma warning(disable:4201)
//
//  File: proxydriver.cpp
//
#include <windows.h>
#include <oal.h>
#include <oalex.h>
#include <ceddk.h>
#include <ceddkex.h>
#include <proxyapi.h>

#pragma warning(pop)
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
#define ZONE_FUNCTION       DEBUGZONE(2)
#define ZONE_INFO           DEBUGZONE(3)
#define ZONE_DVFS           DEBUGZONE(4)

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
    0x000B
};

#endif

//------------------------------------------------------------------------------
//  Local Definitions

#define PROXY_DEVICE_COOKIE       'prxy'

//#define CONSTRAINT_MSG_DVFS_FORCE   (0x80000000)

//------------------------------------------------------------------------------
//  Local Structures

typedef struct {
    HANDLE              hParent;
} Device_t;

typedef struct {
    DWORD               cookie;
    CRITICAL_SECTION    cs;
    Device_t           *pDevice;
    HANDLE              hDvfsConstraint;
    HANDLE              hDomainConstraint;
    HANDLE              hInterruptLatencyConstraint;
} Instance_t;

//------------------------------------------------------------------------------
//  Local Functions
BOOL PXY_Deinit(DWORD context);

//------------------------------------------------------------------------------
//
//  Function:  PXY_Init
//
//  Called by device manager to initialize device.
//
DWORD
PXY_Init(
    LPCTSTR szContext, 
    LPCVOID pBusContext
    )
{
    DWORD rc = (DWORD)NULL;
    Device_t *pDevice;

	UNREFERENCED_PARAMETER(szContext);
	UNREFERENCED_PARAMETER(pBusContext);

    // Create device structure
    pDevice = (Device_t *)LocalAlloc(LPTR, sizeof(Device_t));
    if (pDevice == NULL)
        {
        RETAILMSG(ZONE_ERROR, (L"ERROR: PXY_Init: "
            L"Failed allocate Proxy driver structure\r\n"
            ));

        goto cleanUp;
        }

    memset(pDevice, 0, sizeof(Device_t));
   
#if 0
    // get handle to rootbus for notification to dvfs policy
    // Open clock device
    pDevice->hParent = CreateFile(L"BUS1:", 0, 0, NULL, 0, 0, NULL);
    if (pDevice->hParent == INVALID_HANDLE_VALUE)
    {
        RETAILMSG(ZONE_ERROR, (L"ERROR: PXY_Init: "
            L"Failed open parent bus device '%s'\r\n", L"BUS1:"
            ));
        pDevice->hParent = NULL;
        goto cleanUp;
    }
#endif

    // Return non-null value
    rc = (DWORD)pDevice;

cleanUp:
    if (rc == 0) PXY_Deinit((DWORD)pDevice);

    RETAILMSG(ZONE_FUNCTION, (L"-PXY_Init(rc = %d\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  PXY_Deinit
//
//  Called by device manager to uninitialize device.
//
BOOL
PXY_Deinit(
    DWORD context
    )
{
    BOOL rc = FALSE;
    Device_t *pDevice = (Device_t*)context;

    RETAILMSG(ZONE_FUNCTION, (L"+PXY_Deinit(0x%08x)\r\n", context));

    // Check if we get correct context
    if ((pDevice == NULL))
        {
        RETAILMSG (ZONE_ERROR, (L"TLD: !ERROR(PXY_Deinit) - "
            L"Incorrect context parameter\r\n"
            ));
        goto cleanUp;
        }

    //Close handle to bus driver
    if (pDevice->hParent != NULL)
        {
        CloseHandle(pDevice->hParent);
        }
    
    // Free device structure
    LocalFree(pDevice);

    // Done
    rc = TRUE;

cleanUp:
    RETAILMSG(ZONE_FUNCTION, (L"-PXY_Deinit(rc = %d)\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  PXY_Open
//
//  Called by device manager to open a device for reading and/or writing.
//
DWORD
PXY_Open(
    DWORD context, 
    DWORD accessCode, 
    DWORD shareMode
    )
{
    DWORD dw = 0;
    Instance_t *pInstance;
    Device_t *pDevice = (Device_t*)context;
		
	UNREFERENCED_PARAMETER(shareMode);
	UNREFERENCED_PARAMETER(accessCode);

    RETAILMSG(ZONE_FUNCTION, (L"+PXY_Open(0x%08x, 0x%08x, 0x%08x)\r\n", 
        context, accessCode, shareMode
        ));
    
    if ((pDevice == NULL))
        {
        RETAILMSG (ZONE_ERROR, (L"TLD: !ERROR(PXY_Open) - "
            L"Incorrect context parameter\r\n"
            ));
        goto cleanUp;
        }

    
    // Create device structure
    pInstance = (Instance_t *)LocalAlloc(LPTR, sizeof(Instance_t));
    if (pInstance == NULL)
        {
        RETAILMSG(ZONE_ERROR, (L"ERROR: PXY_Open: "
            L"Failed allocate Proxy driver structure\r\n"
            ));

        goto cleanUp;
        }

    memset(pInstance, 0, sizeof(Instance_t));
    
    // Set cookie
    pInstance->cookie = PROXY_DEVICE_COOKIE;

    // Initialize critical sections
    InitializeCriticalSection(&pInstance->cs);

    pInstance->pDevice = pDevice;

    // Return non-null value
    dw = (DWORD)pInstance;

cleanUp:
    RETAILMSG(ZONE_FUNCTION, (L"-PXY_Open(0x%08x, 0x%08x, 0x%08x) == %d\r\n", 
        context, accessCode, shareMode, dw
        ));

    return dw;
}

//------------------------------------------------------------------------------
//
//  Function:  PXY_Close
//
//  This function closes the device context.
//
BOOL
PXY_Close(
    DWORD context
    )
{
    BOOL rc = FALSE;
    Instance_t *pInstance = (Instance_t*)context;
    
    
    RETAILMSG(ZONE_FUNCTION, (L"+PXY_Close(0x%08x)\r\n", 
        context
        ));
    
    if ((pInstance == NULL))
        {
        RETAILMSG (ZONE_ERROR, (L"TLD: !ERROR(PXY_Close) - "
            L"Incorrect context parameter\r\n"
            ));
        goto cleanUp;
        }

    // Delete critical sections
    DeleteCriticalSection(&pInstance->cs);

#if 0
    // close constraint handles
    if (pInstance->hDvfsConstraint != NULL)
        {
        PmxReleaseConstraint(pInstance->hDvfsConstraint);
        }

    if (pInstance->hDomainConstraint != NULL)
        {
        PmxReleaseConstraint(pInstance->hDomainConstraint);
        }
#endif

    // Free device structure
    LocalFree(pInstance);

    // Done
    rc = TRUE;

cleanUp:
    RETAILMSG(ZONE_FUNCTION, (L"-PXY_Close(0x%08x)\r\n", 
        context
        ));

    return rc;
}


//------------------------------------------------------------------------------
//
//  Function:  PXY_IOControl
//
//  This function sends a command to a device.
//
BOOL
PXY_IOControl(
    DWORD context, DWORD code, 
    UCHAR *pInBuffer, DWORD inSize, 
    UCHAR *pOutBuffer, DWORD outSize, 
    DWORD *pOutSize
    )
{
    BOOL rc = FALSE;
    DWORD dwParam;    
    Instance_t *pInstance = (Instance_t*)context;
    POWERDOMAIN_CONSTRAINT_INFO *pConstraintInfo;

    RETAILMSG(ZONE_FUNCTION, (
        L"+PXY_IOControl(0x%08x, 0x%08x, 0x%08x, %d, 0x%08x, %d, 0x%08x)\r\n",
        context, code, pInBuffer, inSize, pOutBuffer, outSize, pOutSize
        ));

    // Check if we get correct context
    if ((pInstance == NULL))
        {
        RETAILMSG(ZONE_ERROR, (L"ERROR: PXY_IOControl: "
            L"Incorrect context parameter\r\n"
            ));
        goto cleanUp;
        }

    switch (code)
        {
        case IOCTL_VIRTUAL_COPY_EX:
            {
            void * pvLocal;            
            PHYSICAL_ADDRESS PhysAddr;
            VIRTUAL_COPY_EX_DATA *pData = (VIRTUAL_COPY_EX_DATA*)pInBuffer;
            
            PhysAddr.QuadPart = pData->physAddr;
            pvLocal = MmMapIoSpace(PhysAddr, pData->size, FALSE);
            if (pvLocal != NULL)
                {
                HANDLE hDstProc = OpenProcess(0, FALSE, pData->idDestProc);
                rc = VirtualCopyEx(hDstProc, pData->pvDestMem, GetCurrentProcess(), 
                        pvLocal, pData->size, PAGE_READWRITE | PAGE_NOCACHE
                        );
                CloseHandle(hDstProc);                    
                }
            }
        break;

#if 0
        case IOCTL_DVFS_REQUEST:
            if (pInstance->hDvfsConstraint == NULL)
                {
                dwParam = CONSTRAINT_STATE_NULL;
                pInstance->hDvfsConstraint = PmxSetConstraintByClass(
                                    CONSTRAINT_CLASS_DVFS, 
                                    CONSTRAINT_MSG_DVFS_REQUEST, 
                                    (void*)&dwParam, 
                                    sizeof(DWORD)
                                    );
                }

            PmxUpdateConstraint(pInstance->hDvfsConstraint, 
                CONSTRAINT_MSG_DVFS_REQUEST, 
                (void*)pInBuffer, 
                inSize
                );
            break;

        case IOCTL_DVFS_FORCE:
            if (pInstance->hDvfsConstraint == NULL)
                {
                dwParam = CONSTRAINT_STATE_NULL;
                pInstance->hDvfsConstraint = PmxSetConstraintByClass(
                                    CONSTRAINT_CLASS_DVFS, 
                                    CONSTRAINT_MSG_DVFS_REQUEST, 
                                    (void*)&dwParam, 
                                    sizeof(DWORD)
                                    );
                }

            PmxUpdateConstraint(pInstance->hDvfsConstraint, 
                CONSTRAINT_MSG_DVFS_FORCE, 
                (void*)pInBuffer, 
                inSize
                );
            break;

        case IOCTL_POWERDOMAIN_REQUEST:
            pConstraintInfo = (POWERDOMAIN_CONSTRAINT_INFO*)pInBuffer;
            if (pInstance->hDomainConstraint == NULL)
                {
                dwParam = CONSTRAINT_STATE_NULL;
                pInstance->hDomainConstraint = PmxSetConstraintById(
                                    L"PWRDOM",
                                    CONSTRAINT_MSG_POWERDOMAIN_REQUEST, 
                                    (void*)pConstraintInfo, 
                                    sizeof(POWERDOMAIN_CONSTRAINT_INFO)
                                    );

                if (pInstance->hDomainConstraint == NULL) rc = FALSE;
                }
            else
                {
                rc = PmxUpdateConstraint(
                                    pInstance->hDomainConstraint, 
                                    CONSTRAINT_MSG_POWERDOMAIN_REQUEST, 
                                    (void*)pConstraintInfo, 
                                    sizeof(POWERDOMAIN_CONSTRAINT_INFO)
                                    );
                }
            break;
#endif

        /*case IOCTL_INTERRUPT_LATENCY_CONSTRAINT:
            if (pInstance->hInterruptLatencyConstraint == NULL)
                {
                pInstance->hInterruptLatencyConstraint = PmxSetConstraintById(
                                    L"INTRLAT",
                                    CONSTRAINT_MSG_INTRLAT_REQUEST, 
                                    pInBuffer, 
                                    inSize
                                    );

                if (pInstance->hInterruptLatencyConstraint != NULL) rc = TRUE;
                }
            else
                {
                rc = PmxUpdateConstraint(
                                    pInstance->hInterruptLatencyConstraint, 
                                    CONSTRAINT_MSG_INTRLAT_REQUEST, 
                                    pInBuffer, 
                                    inSize
                                    );
                }
            break;*/

		/*
        case IOCTL_PROFILE_DVFS_CORE:
            RunOppLatencyProfiler(*(int*)pInBuffer);
            rc = TRUE;
            break;
		*/

		/*
        case IOCTL_PROFILE_INTERRUPTLATENCY:
            if (pInstance->hDomainConstraint == NULL)
                {
                constraintInfo.size = sizeof(POWERDOMAIN_CONSTRAINT_INFO);
                constraintInfo.powerDomain = POWERDOMAIN_MPU;
                constraintInfo.state = CONSTRAINT_STATE_NULL;
                pInstance->hDomainConstraint = PmxSetConstraintById(
                                    L"PWRDOM",
                                    CONSTRAINT_MSG_POWERDOMAIN_REQUEST, 
                                    (void*)&constraintInfo, 
                                    sizeof(POWERDOMAIN_CONSTRAINT_INFO)
                                    );

                if (pInstance->hDomainConstraint == NULL)
                    {
                    rc = FALSE;
                    break;
                    }
                }
            ProfileInterruptLatency(pInstance->hDomainConstraint, *(int*)pInBuffer);
            PmxReleaseConstraint(pInstance->hDomainConstraint);
            pInstance->hDomainConstraint = NULL;
            break;
			*/

		/*
        case IOCTL_PROFILE_DVFS:
            if (pInstance->hDvfsConstraint == NULL)
                {
                dwParam = CONSTRAINT_STATE_NULL;
                pInstance->hDvfsConstraint = PmxSetConstraintByClass(
                                    CONSTRAINT_CLASS_DVFS, 
                                    CONSTRAINT_MSG_DVFS_REQUEST, 
                                    (void*)&dwParam, 
                                    sizeof(DWORD)
                                    );

                if (pInstance->hDvfsConstraint == NULL)
                    {
                    rc = FALSE;
                    break;
                    }
                }
            ProfileDVFSLatency(pInstance->hDvfsConstraint, (DVFS_STRESS_TEST_PARAMETERS*)pInBuffer);
            break;
			*/

		/*
        case IOCTL_PROFILE_WAKEUPACCURACY:
            if (pInstance->hDomainConstraint == NULL)
                {
                constraintInfo.size = sizeof(POWERDOMAIN_CONSTRAINT_INFO);
                constraintInfo.powerDomain = POWERDOMAIN_MPU;
                constraintInfo.state = CONSTRAINT_STATE_NULL;
                pInstance->hDomainConstraint = PmxSetConstraintById(
                                    L"PWRDOM",
                                    CONSTRAINT_MSG_POWERDOMAIN_REQUEST, 
                                    (void*)&constraintInfo, 
                                    sizeof(POWERDOMAIN_CONSTRAINT_INFO)
                                    );

                if (pInstance->hDomainConstraint == NULL)
                    {
                    rc = FALSE;
                    break;
                    }
                }
            
            ProfileWakupAccuracy(pInstance->hDomainConstraint, (WAKEUPACCURACY_TEST_PARAMETERS*)pInBuffer);
            PmxReleaseConstraint(pInstance->hDomainConstraint);
            pInstance->hDomainConstraint = NULL;
            break;
			*/
        
#if 0
        case IOCTL_HAL_OEM_PROFILER:
        case IOCTL_OPP_REQUEST:
        case IOCTL_PRCM_DEVICE_GET_DEVICESTATUS:
        case IOCTL_PRCM_DEVICE_GET_SOURCECLOCKINFO:
        case IOCTL_PRCM_CLOCK_GET_SOURCECLOCKINFO:            
            rc = KernelIoControl(code, pInBuffer, inSize, pOutBuffer, outSize, pOutSize);
            break;
#endif
    }

cleanUp:

    RETAILMSG(ZONE_FUNCTION, (L"-PXY_IOControl(rc = %d)\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  DllMain
//
//  DLL entry point.
//
BOOL WINAPI DllMain(HANDLE hDLL, ULONG Reason, LPVOID Reserved)
{
	UNREFERENCED_PARAMETER(Reserved);

    switch(Reason) 
        {
        case DLL_PROCESS_ATTACH:
            RETAILREGISTERZONES((HMODULE)hDLL);
            DisableThreadLibraryCalls((HMODULE)hDLL);
            break;
        }
    return TRUE;
}


