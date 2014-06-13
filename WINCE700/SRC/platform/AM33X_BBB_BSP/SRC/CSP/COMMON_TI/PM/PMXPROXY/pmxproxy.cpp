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
//  File: pmxproxy.cpp
//
#include "omap.h"
#include <windows.h>
#include <oal.h>
#include <oalex.h>
#include <ceddk.h>
#include <ceddkex.h>
#include <pmxproxy.h>

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
    L"PmxProxyDriver", {
        L"Errors",      L"Warnings",    L"Function",    L"Info",
        L"Undefined" ,  L"Undefined",   L"Undefined",   L"Undefined",
        L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined",
        L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined"
    },
    0x000B
};

#endif


#if (_WINCEOSVER<600)

/* marshal.hpp */
#define ARG_O_BIT   0x8
#define ARG_I_BIT   0x4

#define ARG_I_PTR           (ARG_I_BIT)             // input only pointer, size in the next argument
#define ARG_I_WSTR          (ARG_I_BIT|1)           // input only, unicode string
#define ARG_I_ASTR          (ARG_I_BIT|2)           // input only, ascii string
#define ARG_I_PDW           (ARG_I_BIT|3)           // input only, ptr to DWORD
                                                    //(can be used for constant sized structure for kernelPSL)
#define ARG_O_PTR           (ARG_O_BIT)             // output only pointer, size in the next argument
#define ARG_O_PDW           (ARG_O_BIT|1)           // output only, pointer to DWORD
#define ARG_O_PI64          (ARG_O_BIT|2)           // output only, pointer to 64 bit value
#define ARG_IO_PTR          (ARG_I_BIT|ARG_O_BIT)   // I/O pointer, size in the next argument
#define ARG_IO_PDW          (ARG_IO_PTR|1)          // I/O pointer to DWORD
#define ARG_IO_PI64         (ARG_IO_PTR|2)          // I/O pointer to 64 bit value

#define MARSHAL_DATA_POINTER(x, y, h)          MapCallerPtr((void*)x, y)
#define UNMARSHAL_DATA_POINTER(a, b, c, h)
#else
#include <marshal.hpp>
__inline
void*
MarshalDataPointerHelper(PVOID pSrcUnmarshalled,
                         DWORD cbSrc,
                         DWORD desc
                         )
{
    HRESULT hr;
    void* pDestMarshalled;

    hr = CeOpenCallerBuffer(&pDestMarshalled, pSrcUnmarshalled, cbSrc, desc, FALSE);
    if (FAILED(hr))
        return NULL;

    return pDestMarshalled;
}

#define MARSHAL_DATA_POINTER(x, y, h)           MarshalDataPointerHelper((void*)x, y, h)
#define UNMARSHAL_DATA_POINTER(a, b, c, h)      CeCloseCallerBuffer((void*)a, (void*)b, c, h)

#endif

//------------------------------------------------------------------------------
//  Local Definitions
//
#define PMX_DEVICE_COOKIE       'pmxy'

//------------------------------------------------------------------------------
//  Local Structures

typedef struct {
    DWORD               cookie;
    HANDLE              hPmxConstraint;
    HANDLE              hPmxPolicy;
} Instance_t;

//------------------------------------------------------------------------------
//  Local Functions
BOOL PMX_Deinit(DWORD context);

//------------------------------------------------------------------------------
//
//  Function:  PMX_Init
//
//  Called by device manager to initialize device.
//
DWORD
PMX_Init(
    LPCTSTR szContext,
    LPCVOID pBusContext
    )
{
    UNREFERENCED_PARAMETER(pBusContext);
    UNREFERENCED_PARAMETER(szContext);
    return PMX_DEVICE_COOKIE;
}

//------------------------------------------------------------------------------
//
//  Function:  PMX_Deinit
//
//  Called by device manager to uninitialize device.
//
BOOL
PMX_Deinit(
    DWORD context
    )
{
    return PMX_DEVICE_COOKIE == context;
}

//------------------------------------------------------------------------------
//
//  Function:  PMX_Open
//
//  Called by device manager to open a device for reading and/or writing.
//
DWORD
PMX_Open(
    DWORD context,
    DWORD accessCode,
    DWORD shareMode
    )
{
    DWORD dw = 0;
    Instance_t *pInstance;

	UNREFERENCED_PARAMETER(context);
	UNREFERENCED_PARAMETER(accessCode);
	UNREFERENCED_PARAMETER(shareMode);

    RETAILMSG(ZONE_FUNCTION, (L"+PMX_Open(0x%08x, 0x%08x, 0x%08x)\r\n",
        context, accessCode, shareMode
        ));

    // Create device structure
    pInstance = (Instance_t *)LocalAlloc(LPTR, sizeof(Instance_t));
    if (pInstance == NULL)
        {
        RETAILMSG(ZONE_ERROR, (L"ERROR: PMX_Open: "
            L"Failed allocate Proxy driver structure\r\n"
            ));

        goto cleanUp;
        }

    memset(pInstance, 0, sizeof(Instance_t));

    // Set cookie
    pInstance->cookie = PMX_DEVICE_COOKIE;

    // Return non-null value
    dw = (DWORD)pInstance;

cleanUp:
    RETAILMSG(ZONE_FUNCTION, (L"-PMX_Open(0x%08x, 0x%08x, 0x%08x) == %d\r\n",
        context, accessCode, shareMode, dw
        ));

    return dw;
}

//------------------------------------------------------------------------------
//
//  Function:  PMX_Close
//
//  This function closes the device context.
//
BOOL
PMX_Close(
    DWORD context
    )
{
    BOOL rc = FALSE;
    Instance_t *pInstance = (Instance_t*)context;

    RETAILMSG(ZONE_FUNCTION, (L"+PMX_Close(0x%08x)\r\n",
        context
        ));

    if ((pInstance == NULL))
        {
        RETAILMSG (ZONE_ERROR, (L"TLD: !ERROR(PMX_Close) - "
            L"Incorrect context parameter\r\n"
            ));
        goto cleanUp;
        }

    // close constraint handles
    if (pInstance->hPmxConstraint != NULL)
        {
        PmxReleaseConstraint(pInstance->hPmxConstraint);
        }

    if (pInstance->hPmxPolicy != NULL)
        {
        PmxClosePolicy(pInstance->hPmxPolicy);
        }

    // Free device structure
    LocalFree(pInstance);

    // Done
    rc = TRUE;

cleanUp:
    RETAILMSG(ZONE_FUNCTION, (L"-PMX_Close(0x%08x)\r\n",
        context
        ));

    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  PMX_IOControl
//
//  This function sends a command to a device.
//
BOOL
PMX_IOControl(
    DWORD context, DWORD code,
    void  *pInBuffer, DWORD inSize,
    void  *pOutBuffer, DWORD outSize,
    DWORD *pOutSize
    )
{
    BOOL rc = FALSE;
    Instance_t *pInstance = (Instance_t*)context;
    IOCTL_PMX_PARAMETERS  tempPmxInfo;
    IOCTL_PMX_PARAMETERS *pPmxInfo;

	UNREFERENCED_PARAMETER(inSize);
	UNREFERENCED_PARAMETER(pOutBuffer);
	UNREFERENCED_PARAMETER(outSize);
	UNREFERENCED_PARAMETER(pOutSize);

    RETAILMSG(ZONE_FUNCTION, (
        L"+PMX_IOControl(0x%08x, 0x%08x, 0x%08x, %d, 0x%08x, %d, 0x%08x)\r\n",
        context, code, pInBuffer, inSize, pOutBuffer, outSize, pOutSize
        ));

    // Check if we get correct context
    if ((pInstance == NULL))
        {
        RETAILMSG(ZONE_ERROR, (L"ERROR: PMX_IOControl: "
            L"Incorrect context parameter\r\n"
            ));
        goto cleanUp;
        }

    switch (code)
        {
        case IOCTL_PMX_LOADPOLICY:
            if (pInstance->hPmxPolicy != NULL || pInBuffer == NULL) return FALSE;
            pInstance->hPmxPolicy = PmxLoadPolicy(pInBuffer);
            rc = pInstance->hPmxPolicy != NULL;
            break;

        case IOCTL_PMX_OPENPOLICY:
            if (pInstance->hPmxPolicy != NULL || pInBuffer == NULL) return FALSE;
            pInstance->hPmxPolicy = PmxOpenPolicy((LPCWSTR)pInBuffer);
            rc = pInstance->hPmxPolicy != NULL;
            break;

        case IOCTL_PMX_CLOSEPOLICY:
            if (pInstance->hPmxPolicy == NULL) return FALSE;
            PmxClosePolicy(pInstance->hPmxPolicy);
            pInstance->hPmxPolicy = NULL;
            rc = TRUE;
            break;

        case IOCTL_PMX_NOTIFYPOLICY:
            if (pInstance->hPmxPolicy == NULL || pInBuffer == NULL) return FALSE;
            pPmxInfo = (IOCTL_PMX_PARAMETERS*)pInBuffer;
            tempPmxInfo.pvParam = MARSHAL_DATA_POINTER(pPmxInfo->pvParam,
                                                       pPmxInfo->size,
                                                       ARG_I_PTR
                                                       );

            rc = PmxNotifyPolicy(pInstance->hPmxPolicy,
                                 pPmxInfo->msg,
                                 tempPmxInfo.pvParam,
                                 pPmxInfo->size
                                 );

            UNMARSHAL_DATA_POINTER(tempPmxInfo.pvParam,
                                   pPmxInfo->pvParam,
                                   pPmxInfo->size,
                                   ARG_I_PTR
                                   );
            break;

        case IOCTL_PMX_LOADCONSTRAINT:
            if (pInstance->hPmxConstraint != NULL || pInBuffer == NULL) return FALSE;
            pInstance->hPmxConstraint = PmxLoadConstraint(pInBuffer);
            rc = pInstance->hPmxConstraint != NULL;
            break;

        case IOCTL_PMX_RELEASECONSTRAINT:
            if (pInstance->hPmxConstraint == NULL) return FALSE;
            PmxReleaseConstraint(pInstance->hPmxConstraint);
            pInstance->hPmxConstraint = NULL;
            rc = TRUE;
            break;

        case IOCTL_PMX_SETCONSTRAINTBYID:
            if (pInstance->hPmxConstraint != NULL || pInBuffer == NULL) return FALSE;
            pPmxInfo = (IOCTL_PMX_PARAMETERS*)pInBuffer;
            tempPmxInfo.szId = (LPCWSTR)MARSHAL_DATA_POINTER(pPmxInfo->szId,
                                                             (sizeof(WCHAR) * pPmxInfo->strlen),
                                                             ARG_I_WSTR
                                                             );

            tempPmxInfo.pvParam = MARSHAL_DATA_POINTER(pPmxInfo->pvParam,
                                                       pPmxInfo->size,
                                                       ARG_I_PTR
                                                       );

            pInstance->hPmxConstraint = PmxSetConstraintById(tempPmxInfo.szId,
                                                             pPmxInfo->msg,
                                                             tempPmxInfo.pvParam,
                                                             pPmxInfo->size
                                                             );
            UNMARSHAL_DATA_POINTER(tempPmxInfo.pvParam,
                                   pPmxInfo->pvParam,
                                   pPmxInfo->size,
                                   ARG_I_PTR
                                   );

            UNMARSHAL_DATA_POINTER(tempPmxInfo.szId,
                                   pPmxInfo->szId,
                                   (sizeof(WCHAR) * pPmxInfo->strlen),
                                   ARG_I_WSTR
                                   );

            rc = pInstance->hPmxConstraint != NULL;
            break;

        case IOCTL_PMX_UPDATECONSTRAINT:
            if (pInstance->hPmxConstraint == NULL || pInBuffer == NULL) return FALSE;
            pPmxInfo = (IOCTL_PMX_PARAMETERS*)pInBuffer;

            tempPmxInfo.pvParam = MARSHAL_DATA_POINTER(pPmxInfo->pvParam,
                                                       pPmxInfo->size,
                                                       ARG_I_PTR
                                                       );
            rc = PmxUpdateConstraint(pInstance->hPmxConstraint,
                                     pPmxInfo->msg,
                                     tempPmxInfo.pvParam,
                                     pPmxInfo->size);

            UNMARSHAL_DATA_POINTER(tempPmxInfo.pvParam,
                                   pPmxInfo->pvParam,
                                   pPmxInfo->size,
                                   ARG_I_PTR
                                   );
            break;
        }

cleanUp:

    RETAILMSG(ZONE_FUNCTION, (L"-PMX_IOControl(rc = %d)\r\n", rc));
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


