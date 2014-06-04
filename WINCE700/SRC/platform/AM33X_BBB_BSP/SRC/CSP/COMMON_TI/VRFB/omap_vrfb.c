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
//  File: dma.cpp
//
#include "omap.h"
#include "omap_vrfb_regs.h"
#include "ceddkex.h"
#include "oalex.h"
#include "oal_clock.h"
#include "soc_cfg.h"

#include "_debug.h"

#include <initguid.h>
#include <vrfb.h>


//------------------------------------------------------------------------------
//
//  Global:  dpCurSettings
//
//  Set debug zones names and initial setting for driver
//
#ifndef SHIP_BUILD

DBGPARAM dpCurSettings = {
    L"VRFB", {
        L"Errors",      L"Warnings",    L"Function",    L"Info",
        L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined",
        L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined",
        L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined"
    },
    0x000F
};

#endif

//------------------------------------------------------------------------------
//  Local Definitions

#define VRFB_DEVICE_COOKIE      'vrfD'
#define VRFB_INSTANCE_COOKIE    'vrfI'

#define VRFB_VIEW_HANDLE_BASE   0x60000000      // Arbitrary base value for view handle values

#define DEFAULT_PAGE_WIDTH      5               // Exponent based 2 value (eg. 2^5 = 32)
#define DEFAULT_PAGE_HEIGHT     5

#define PIXEL_SIZE_TO_BYTES(ps)     (1<<ps)                                                         // Converts pixel size to number of bytes per pixel
#define IMAGE_SIZE_ROUNDING(is,ps)  (((is % (1<<ps))==0) ? is : (is/(1<<ps))*(1<<ps) + (1<<ps))     // Rounds image size up to an even multiple of page size


//------------------------------------------------------------------------------
//  Local Structures

typedef struct {
    DWORD                   cookie;
    DWORD                   dwNumDisplayContexts;    
    CRITICAL_SECTION        cs;
    LONG                    instances;
    OMAP_DEVICE             device;
    OMAP_VRFB_REGS         *pVRFBRegs;
    VOID                   *pDisplayContextMem;
} Device_t;

typedef struct {
    DWORD                   cookie;
    Device_t               *pDevice;
} Instance_t;

typedef struct {
    BOOL                    bInUse;
    DWORD                   dwWidth;
    DWORD                   dwHeight;
    DWORD                   dwRotationAngle;
    DWORD                   dwPhysicalAddr;
    DWORD                   dwVirtualAddr;
    DWORD                   dwBufferPhysAddr;
    DWORD                   dwBufferVirtAddr;   // Only valid if buffer allocated by this driver
} VrfbContextInfo_t;


//------------------------------------------------------------------------------
//  Global Variables

VrfbContextInfo_t   g_VrfbContextList[VRFB_ROTATION_CONTEXTS] =
{
    { FALSE, 0, 0, 0, 0x70000000, 0, 0, 0 },
    { FALSE, 0, 0, 0, 0x74000000, 0, 0, 0 },
    { FALSE, 0, 0, 0, 0x78000000, 0, 0, 0 },
    { FALSE, 0, 0, 0, 0x7C000000, 0, 0, 0 },
    { FALSE, 0, 0, 0, 0xE0000000, 0, 0, 0 },
    { FALSE, 0, 0, 0, 0xE4000000, 0, 0, 0 },
    { FALSE, 0, 0, 0, 0xE8000000, 0, 0, 0 },
    { FALSE, 0, 0, 0, 0xEC000000, 0, 0, 0 },
    { FALSE, 0, 0, 0, 0xF0000000, 0, 0, 0 },
    { FALSE, 0, 0, 0, 0xF4000000, 0, 0, 0 },
    { FALSE, 0, 0, 0, 0xF8000000, 0, 0, 0 },
    { FALSE, 0, 0, 0, 0xFC000000, 0, 0, 0 }
};


//------------------------------------------------------------------------------
//  Local Functions

BOOL
VRF_Deinit(
    DWORD context
    );

HANDLE  
VRFB_AllocateView(
    HANDLE hContext, 
    DWORD dwPixelSize, 
    DWORD dwWidth, 
    DWORD dwHeight, 
    DWORD dwBufferPhysAddr
    );

BOOL    
VRFB_ReleaseView(
    HANDLE hContext, 
    HANDLE hView
    );
    
BOOL
VRFB_GetViewInfo(
    HANDLE hContext, 
    HANDLE hView, 
    VRFB_VIEW_INFO *pInfo
    );

BOOL
VRFB_RotateView(
    HANDLE hContext, 
    HANDLE hView, 
    DWORD dwRotateAngle
    );

BOOL
VRFB_UpdateView(
    HANDLE hContext, 
    HANDLE hView, 
    DWORD dwPixelSize, 
    DWORD dwWidth, 
    DWORD dwHeight, 
    DWORD dwBufferPhysAddr
    );
    
DWORD
VRFB_NumDisplayViews(
    HANDLE hContext
    );

HANDLE
VRFB_GetDisplayView(
    HANDLE hContext, 
    DWORD dwIndex
    );

BOOL
VRFB_GetDisplayViewInfo(
    HANDLE hContext, 
    DWORD dwIndex, 
    DWORD dwRotateAngle, 
    BOOL bMirror,
    VRFB_VIEW_INFO *pInfo
    );


VOID    
DebugPrintRegs(
    OMAP_VRFB_REGS *pVrfbRegs, 
    DWORD dwIndex
    );

//------------------------------------------------------------------------------
//  Device registry parameters

static const DEVICE_REGISTRY_PARAM s_deviceRegParams[] = {
    { 
        L"NumDisplayContexts", PARAM_DWORD, FALSE, offset(Device_t, dwNumDisplayContexts),
        fieldsize(Device_t, dwNumDisplayContexts), (VOID*)12
    }
};


//------------------------------------------------------------------------------
//
//  Function:  VRF_Init
//
//  Called by device manager to initialize device.
//
DWORD
VRF_Init(
    LPCWSTR szContext, 
    LPCVOID pBusContext
    )
{
    DWORD rc = (DWORD)NULL;
    PHYSICAL_ADDRESS pa;    
    Device_t *pDevice;
    DWORD i;

    UNREFERENCED_PARAMETER(pBusContext);

    DEBUGMSG(ZONE_FUNCTION, (
        L"+VRF_Init(%s, 0x%08x)\r\n", szContext, pBusContext
        ));

    // Create device structure
    pDevice = (Device_t *)LocalAlloc(LPTR, sizeof(Device_t));
    if (pDevice == NULL)
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: VRF_Init: "
            L"Failed allocate VRFB controller structure\r\n"
            ));
        goto cleanUp;
        }

    // initialize memory
    memset(pDevice, 0, sizeof(Device_t));

    // Set cookie
    pDevice->cookie = VRFB_DEVICE_COOKIE;

    // Initalize critical section
    InitializeCriticalSection(&pDevice->cs);

    // Read device parameters
    if (GetDeviceRegistryParams(
            szContext, pDevice, dimof(s_deviceRegParams), s_deviceRegParams
            ) != ERROR_SUCCESS)
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: VRF_Init: "
            L"Failed read VRFB driver registry parameters\r\n"
            ));
        goto cleanUp;
        }
    
    pDevice->device = SOCGetVRFBDevice();
    if (pDevice->device == OMAP_DEVICE_NONE)
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: VRF_Init: "
            L"Failed to get VRFB device\r\n"
            ));
        goto cleanUp;
    }
    
    // Set hardware to full power (VRFB has no power states)
    EnableDeviceClocks(pDevice->device,TRUE);

    
    //  Map VRFB control registers
    pa.QuadPart = GetAddressByDevice(pDevice->device);    
    pDevice->pVRFBRegs = (OMAP_VRFB_REGS*)MmMapIoSpace(pa, sizeof(OMAP_VRFB_REGS), FALSE);
    if (pDevice->pVRFBRegs == NULL)
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: VRF_Init: "
             L"Failed map VRFB control registers\r\n"
            ));
        goto cleanUp;
        }

    //  Validate the number of contexts reserved for display
    if( pDevice->dwNumDisplayContexts > VRFB_ROTATION_CONTEXTS )
        pDevice->dwNumDisplayContexts = VRFB_ROTATION_CONTEXTS;
        
    //  Reserve the contiguous vitual memory region for the display contexts
    if( pDevice->dwNumDisplayContexts > 0 )
    {
        //  Reserve virtual memory for the display driver views
        //  Note that this memory is not allocated, but is required by DDraw to be contiguous
        pDevice->pDisplayContextMem = VirtualAlloc( NULL, 
                                                    pDevice->dwNumDisplayContexts * VRFB_VIEW_SIZE, 
                                                    MEM_RESERVE, 
                                                    PAGE_NOACCESS );
                                                    
        if( pDevice->pDisplayContextMem == NULL )
            {
            DEBUGMSG(ZONE_ERROR, (L"ERROR: VRF_Init: "
                 L"Failed reserve display context memory\r\n"
                ));
            goto cleanUp;
            }

        DEBUGMSG(ZONE_INFO, (L"INFO: VRF_Init: "
             L"Reserved display memory - num contexts = %d, size = 0x%08X, va = 0x%08X\r\n", 
             pDevice->dwNumDisplayContexts, pDevice->dwNumDisplayContexts * VRFB_VIEW_SIZE, pDevice->pDisplayContextMem
            ));
        
        
        //  Associate the VRFB context addresses with the context virtual memory
        for( i = 0; i < pDevice->dwNumDisplayContexts; i++ )
        {
            BOOL    bResult;
            VOID*   pMem = (VOID*)((DWORD)pDevice->pDisplayContextMem + i*VRFB_VIEW_SIZE);
            
            //  Associate to the rotation angle 0 context by default
            bResult = VirtualCopy( pMem, 
                                   (VOID*)(g_VrfbContextList[i].dwPhysicalAddr/256), 
                                   VRFB_VIEW_SIZE, 
                                   PAGE_READWRITE | PAGE_PHYSICAL | PAGE_NOCACHE );

            if( bResult == FALSE )
            {
                DEBUGMSG(ZONE_ERROR, (L"ERROR: VRF_Init: "
                     L"Failed map display context memory for context # %d\r\n", i
                    ));
                goto cleanUp;
            }

            //  Change the attributes of the buffer for cache write thru
            bResult = CeSetMemoryAttributes( pMem, (VOID*)(g_VrfbContextList[i].dwPhysicalAddr/256), VRFB_VIEW_SIZE, PAGE_WRITECOMBINE );
            if( bResult == FALSE )
            {
                DEBUGMSG(ZONE_ERROR, (L"ERROR: VRF_Init: "
                     L"Failed CeSetMemoryAttributes for context # %d\r\n", i
                    ));
                goto cleanUp;
            }

            DEBUGMSG(ZONE_INFO, (L"INFO: VRF_Init: "
                 L"Context # %d: va = 0x%08X, pa = 0x%08X", 
                 i, pMem, g_VrfbContextList[i].dwPhysicalAddr
                ));
            
            //  Update the context table
            g_VrfbContextList[i].bInUse = TRUE;                                               
            g_VrfbContextList[i].dwRotationAngle = VRFB_ROTATE_ANGLE_0;                                               
            g_VrfbContextList[i].dwVirtualAddr = (DWORD) pMem;
        }   
        
    }

    rc = (DWORD)pDevice;
    
cleanUp:
    if (rc == 0) VRF_Deinit((DWORD)pDevice);
    DEBUGMSG(ZONE_FUNCTION, (L"-VRF_Init(rc = %d)\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  VRF_Deinit
//
//  Called by device manager to uninitialize device.
//
BOOL
VRF_Deinit(
    DWORD context
    )
{
    BOOL rc = FALSE;
    Device_t *pDevice = (Device_t*)context;

    DEBUGMSG(ZONE_FUNCTION, (L"+VRF_Deinit(0x%08x)\r\n", context));

    // Check if we get correct context
    if ((pDevice == NULL) || (pDevice->cookie != VRFB_DEVICE_COOKIE))
        {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: VRF_Deinit: "
            L"Incorrect context param\r\n"
            ));
        goto cleanUp;
        }

    // Check for open instances
    if (pDevice->instances > 0)
        {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: VRF_Deinit: "
            L"Deinit with active instance (%d instances active)\r\n",
            pDevice->instances
            ));
        goto cleanUp;
        }
    
    // Turn hardware off
    EnableDeviceClocks(pDevice->device,FALSE);
    
    //  Release the display context memory
    if (pDevice->pDisplayContextMem != NULL) 
    {
        VirtualFree( pDevice->pDisplayContextMem, VRFB_VIEW_SIZE*pDevice->dwNumDisplayContexts, MEM_DECOMMIT );
        VirtualFree( pDevice->pDisplayContextMem, 0, MEM_RELEASE );
    }                    

    //  Unmap VRFB registers
    if (pDevice->pVRFBRegs != NULL) 
    {
        MmUnmapIoSpace((VOID*)pDevice->pVRFBRegs, sizeof(OMAP_VRFB_REGS));
    }                    


    // Delete critical section
    DeleteCriticalSection(&pDevice->cs);
 
    // Free device structure
    LocalFree(pDevice);

    // Done
    rc = TRUE;

cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-VRF_Deinit(rc = %d)\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  VRF_Open
//
//  Called by device manager to open a device for reading and/or writing.
//
DWORD
VRF_Open(
    DWORD context, 
    DWORD accessCode, 
    DWORD shareMode
    )
{
    DWORD rc = (DWORD)NULL;
    Device_t *pDevice = (Device_t*)context;
    Instance_t *pInstance = NULL;

    UNREFERENCED_PARAMETER(accessCode);
    UNREFERENCED_PARAMETER(shareMode);

    DEBUGMSG(ZONE_FUNCTION, (
        L"+VRF_Open(0x%08x, 0x%08x, 0x%08x\r\n", context, accessCode, shareMode
        ));

    // Check if we get correct context
    if ((pDevice == NULL) || (pDevice->cookie != VRFB_DEVICE_COOKIE))
        {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: VRF_Open: "
            L"Incorrect context parameter\r\n"
            ));
        goto cleanUp;
        }

    // Create device structure
    pInstance = (Instance_t*)LocalAlloc(LPTR, sizeof(Instance_t));
    if (pInstance == NULL)
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: VRF_Open: "
            L"Failed allocate DMA instance structure\r\n"
            ));
        goto cleanUp;
        }

    // Set cookie
    memset(pInstance, 0, sizeof(Instance_t));
    pInstance->cookie = VRFB_INSTANCE_COOKIE;

    // Save device reference
    pInstance->pDevice = pDevice;

    // Increment number of open instances
    InterlockedIncrement(&pDevice->instances);

    // Sanity check number of instances
    ASSERT(pDevice->instances > 0);

    // Done...
    rc = (DWORD)pInstance;

cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-VRF_Open(rc = 0x%08x)\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  VRF_Close
//
//  This function closes the device context.
//
BOOL
VRF_Close(
    DWORD context
    )
{
    BOOL rc = FALSE;
    Device_t *pDevice;
    Instance_t *pInstance = (Instance_t*)context;

    DEBUGMSG(ZONE_FUNCTION, (L"+VRF_Close(0x%08x)\r\n", context));

    // Check if we get correct context
    if ((pInstance == NULL) || (pInstance->cookie != VRFB_INSTANCE_COOKIE))
        {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: VRF_Read: "
            L"Incorrect context param\r\n"
            ));
        goto cleanUp;
        }

    // Get device context
    pDevice = pInstance->pDevice;

    // Sanity check number of instances
    ASSERT(pDevice->instances > 0);

    // Decrement number of open instances
    InterlockedDecrement(&pDevice->instances);

    // Free instance structure
    LocalFree(pInstance);

    // Done...
    rc = TRUE;

cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-VRF_Close(rc = %d)\r\n", rc));
    return rc;
}


//------------------------------------------------------------------------------
//
//  Function:  VRF_IOControl
//
//  This function sends a command to a device.
//
BOOL
VRF_IOControl(
    DWORD context, 
    DWORD code, 
    BYTE  *pInBuffer, 
    DWORD inSize, 
    BYTE  *pOutBuffer,
    DWORD outSize, 
    DWORD *pOutSize
    )
{
    BOOL rc = FALSE;
    Instance_t *pInstance = (Instance_t*)context;
    Device_t *pDevice;
    DEVICE_IFC_VRFB ifc;

    DEBUGMSG(ZONE_FUNCTION, (
        L"+VRF_IOControl(0x%08x, 0x%08x, 0x%08x, %d, 0x%08x, %d, 0x%08x)\r\n",
        context, code, pInBuffer, inSize, pOutBuffer, outSize, pOutSize
        ));

    // Check if we get correct context
    if ((pInstance == NULL) || (pInstance->cookie != VRFB_INSTANCE_COOKIE))
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: VRF_IOControl: "
            L"Incorrect context param\r\n"
            ));
        goto cleanUp;
        }

    //  Get Device
    pDevice = pInstance->pDevice;


    switch (code)
        { 
        case IOCTL_DDK_GET_DRIVER_IFC:
            // We can give interface only to our peer in device process
            if (GetCurrentProcessId() != (DWORD)GetCallerProcess()) {
                DEBUGMSG(ZONE_ERROR, (L"ERROR: VRF_IOControl: "
                    L"IOCTL_DDK_GET_DRIVER_IFC can be called only from "
                    L"device process (caller process id 0x%08x)\r\n",
                    GetCallerProcess()
                ));
                SetLastError(ERROR_ACCESS_DENIED);
                goto cleanUp;
            }
            // Check input parameters
            if (pInBuffer == NULL || inSize < sizeof(GUID)) {
                SetLastError(ERROR_INVALID_PARAMETER);
                break;
            }
            if (IsEqualGUID((GUID*)pInBuffer, &DEVICE_IFC_VRFB_GUID)) {
                if (pOutSize != NULL) *pOutSize = sizeof(DEVICE_IFC_VRFB);
                if (pOutBuffer == NULL || outSize < sizeof(DEVICE_IFC_VRFB)) {
                    SetLastError(ERROR_INVALID_PARAMETER);
                    break;
                }
                ifc.context = (HANDLE) context;
                ifc.pfnAllocateView = VRFB_AllocateView;
                ifc.pfnReleaseView = VRFB_ReleaseView;
                ifc.pfnGetViewInfo = VRFB_GetViewInfo;
                ifc.pfnRotateView = VRFB_RotateView;
                ifc.pfnUpdateView = VRFB_UpdateView;
                ifc.pfnNumDisplayViews = VRFB_NumDisplayViews;
                ifc.pfnGetDisplayView = VRFB_GetDisplayView;
                ifc.pfnGetDisplayViewInfo = VRFB_GetDisplayViewInfo;
                if (!CeSafeCopyMemory(pOutBuffer, &ifc, sizeof(DEVICE_IFC_VRFB))) {
                    SetLastError(ERROR_INVALID_PARAMETER);
                    break;
                }
                rc = TRUE;
                break;
            }
            SetLastError(ERROR_INVALID_PARAMETER);
            break;

        case IOCTL_VRFB_ALLOCATEVIEW:
            {
                if ((pInBuffer == NULL) || (pOutBuffer == NULL) ||
                    (inSize < sizeof(VRFB_VIEW_INFO)) ||
                    (outSize < sizeof(HANDLE)))
                    {
                    SetLastError(ERROR_INVALID_PARAMETER);
                    }
                else
                    {
                    HANDLE          hContext = (HANDLE) context;
                    VRFB_VIEW_INFO *pViewInfo = (VRFB_VIEW_INFO*)pInBuffer;
                    HANDLE         *hView = (HANDLE*)pOutBuffer;
                    
                    *hView = VRFB_AllocateView( hContext, 
                                               pViewInfo->dwPixelSize, 
                                               pViewInfo->dwWidth,
                                               pViewInfo->dwHeight, 
                                               pViewInfo->dwPhysicalBufferAddr );

                    rc = TRUE;
                    }
            }
            break;
 
        case IOCTL_VRFB_RELEASEVIEW:
            {
                if ((pInBuffer == NULL) ||
                    (inSize < sizeof(HANDLE*)))
                    {
                    SetLastError(ERROR_INVALID_PARAMETER);
                    }
                else
                    {
                    HANDLE  hContext = (HANDLE) context;
                    HANDLE  hView = *(HANDLE*)pInBuffer;
                    
                    rc = VRFB_ReleaseView( hContext, hView );
                    }
            }
            break;
 
        case IOCTL_VRFB_GETVIEWINFO:
            {
                if ((pInBuffer == NULL) || (pOutBuffer == NULL) ||
                    (inSize < sizeof(HANDLE*)) ||
                    (outSize < sizeof(VRFB_VIEW_INFO)))
                    {
                    SetLastError(ERROR_INVALID_PARAMETER);
                    }
                else
                    {
                    HANDLE  hContext = (HANDLE) context;
                    HANDLE  hView = *(HANDLE*)pInBuffer;
                    VRFB_VIEW_INFO *pViewInfo = (VRFB_VIEW_INFO*)pOutBuffer;
                    
                    rc = VRFB_GetViewInfo( hContext, hView, pViewInfo ); 
                    }
            }
            break;
 
        case IOCTL_VRFB_ROTATEVIEW:
            {
                if ((pInBuffer == NULL)||
                    (inSize < sizeof(VRFB_VIEW_INFO)))
                    {
                    SetLastError(ERROR_INVALID_PARAMETER);
                    }
                else
                    {
                    HANDLE          hContext = (HANDLE) context;
                    VRFB_VIEW_INFO *pViewInfo = (VRFB_VIEW_INFO*)pInBuffer;
                    
                    rc = VRFB_RotateView( hContext, pViewInfo->hView, pViewInfo->dwRotationAngle ); 
                    }
            }
            break;
 
        case IOCTL_VRFB_UPDATEVIEW:
            {
                if ((pInBuffer == NULL)||
                    (inSize < sizeof(VRFB_VIEW_INFO)))
                    {
                    SetLastError(ERROR_INVALID_PARAMETER);
                    }
                else
                    {
                    HANDLE          hContext = (HANDLE) context;
                    VRFB_VIEW_INFO *pViewInfo = (VRFB_VIEW_INFO*)pInBuffer;
                    
                    rc = VRFB_UpdateView( hContext,
                                          pViewInfo->hView,
                                          pViewInfo->dwPixelSize, 
                                          pViewInfo->dwWidth,
                                          pViewInfo->dwHeight, 
                                          pViewInfo->dwPhysicalBufferAddr );
                    }
            }
            break;
 
        case IOCTL_VRFB_NUMDISPLAYVIEWS:
            {
                if ((pOutBuffer == NULL) ||
                    (outSize < sizeof(DWORD)))
                    {
                    SetLastError(ERROR_INVALID_PARAMETER);
                    }
                else
                    {
                    HANDLE  hContext = (HANDLE) context;
                    DWORD*  pNumViews = (DWORD*)pOutBuffer;
                    
                    *pNumViews = VRFB_NumDisplayViews( hContext ); 
                    rc = TRUE;
                    }
            }
            break;

        case IOCTL_VRFB_GETDISPLAYVIEW:
            {
                if ((pInBuffer == NULL) || (pOutBuffer == NULL) ||
                    (inSize < sizeof(DWORD)) ||
                    (outSize < sizeof(HANDLE)))
                    {
                    SetLastError(ERROR_INVALID_PARAMETER);
                    }
                else
                    {
                    HANDLE  hContext = (HANDLE) context;
                    DWORD  *dwIndex = (DWORD*)pInBuffer;
                    HANDLE *hView = (HANDLE*)pOutBuffer;
                    
                    *hView = VRFB_GetDisplayView( hContext, *dwIndex );
                    rc = TRUE;
                    }
            }
            break;
 
        case IOCTL_VRFB_GETDISPLAYVIEWINFO:
            {
                if ((pInBuffer == NULL) || (pOutBuffer == NULL) ||
                    (inSize < sizeof(IOCTL_VRFB_GETDISPLAYVIEWINFO_IN)) ||
                    (outSize < sizeof(VRFB_VIEW_INFO)))
                    {
                    SetLastError(ERROR_INVALID_PARAMETER);
                    }
                else
                    {
                    HANDLE                            hContext = (HANDLE) context;
                    IOCTL_VRFB_GETDISPLAYVIEWINFO_IN *pInfo = (IOCTL_VRFB_GETDISPLAYVIEWINFO_IN*)pInBuffer;
                    VRFB_VIEW_INFO                   *pViewInfo = (VRFB_VIEW_INFO*)pOutBuffer;
                    
                    rc = VRFB_GetDisplayViewInfo( hContext, 
                                                  pInfo->dwIndex,
                                                  pInfo->dwRotateAngle,
                                                  pInfo->bMirror,
                                                  pViewInfo );
                    }
            }
            break;
 
        default:
            DEBUGMSG(ZONE_WARNING, (
                L"VRFB: Unknown IOControl = 0x%08x\r\n",
                code
                ));
   
        }

cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-VRF_IOControl(rc = %d)\r\n", rc));
    return rc;
}



//------------------------------------------------------------------------------
//
//  Function:  VRFB_AllocateView
//
//  This allocates a VRFB view from the pool.  Returns NULL on failure.
//
HANDLE  VRFB_AllocateView(HANDLE hContext, DWORD dwPixelSize, DWORD dwWidth, DWORD dwHeight, DWORD dwBufferPhysAddr)
{
    HANDLE     hView = NULL;
    Instance_t *pInstance = (Instance_t*)hContext;
    Device_t   *pDevice;
    DWORD       index;
    DWORD       dwImageWidth,
                dwImageHeight;
    VOID       *pMem = NULL;
    DWORD       dwBufferVirtAddr = 0;

    DEBUGMSG(ZONE_FUNCTION, (
        L"+VRFB_AllocateView(0x%08x, %d, %d, %d, 0x%08x)\r\n", hContext, dwPixelSize, dwWidth, dwHeight, dwBufferPhysAddr
        ));

    // Check if we get correct context
    if ((pInstance == NULL) || (pInstance->cookie != VRFB_INSTANCE_COOKIE))
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: VRFB_AllocateView: "
            L"Incorrect context param\r\n"
            ));
        goto cleanUp;
        }

    //  Get Device
    pDevice = pInstance->pDevice;

    //  Validate parameters
    if( (dwPixelSize != VRFB_PIXELSIZE_1B) && (dwPixelSize != VRFB_PIXELSIZE_2B) && (dwPixelSize != VRFB_PIXELSIZE_4B) )
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: VRFB_AllocateView: "
            L"Invalid pixel size\r\n"
            ));
        goto cleanUp;
        }

    //  Ensure image width and height are multiple of page size
    dwImageWidth  = IMAGE_SIZE_ROUNDING(dwWidth, DEFAULT_PAGE_WIDTH);
    dwImageHeight = IMAGE_SIZE_ROUNDING(dwHeight, DEFAULT_PAGE_HEIGHT);

    if( (dwImageWidth < VRFB_IMAGE_WIDTH_MIN) || (dwImageWidth > VRFB_IMAGE_WIDTH_MAX) || 
        (dwImageHeight < VRFB_IMAGE_HEIGHT_MIN) || (dwImageHeight > VRFB_IMAGE_HEIGHT_MAX) )
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: VRFB_AllocateView: "
            L"Invalid image height or width\r\n"
            ));
        goto cleanUp;
        }


    //  Check for available context
    for( index = 0; index < VRFB_ROTATION_CONTEXTS; index++ )
    {
        //  See if slot is open
        if( g_VrfbContextList[index].bInUse == FALSE )
            break;
    }
    
    //  Allocate a context view
    if( index < VRFB_ROTATION_CONTEXTS )
    {
        BOOL    bResult;

        //  Check given buffer address; NULL indicates to alloc physical memory
        if( dwBufferPhysAddr == 0 )
        {
            //  Allocate physical memory for view
            dwBufferVirtAddr = (DWORD) AllocPhysMem( dwImageWidth*dwImageHeight*PIXEL_SIZE_TO_BYTES(dwPixelSize),
                                                     PAGE_READWRITE | PAGE_NOCACHE,
                                                     0,
                                                     0,
                                                     &dwBufferPhysAddr );

            if( dwBufferVirtAddr == 0 )
            {
                DEBUGMSG(ZONE_ERROR, (L"ERROR: VRFB_AllocateView: "
                     L"Failed to allocate physical memory for view buffer\n"
                    ));
                goto cleanUp;
            }
        } 

        //  Reserve virtual memory for the VRFB view
        pMem = VirtualAlloc( NULL, 
                             VRFB_VIEW_SIZE, 
                             MEM_RESERVE, 
                             PAGE_NOACCESS );
                                                    
        if( pMem == NULL )
        {
            DEBUGMSG(ZONE_ERROR, (L"ERROR: VRFB_AllocateView: "
                 L"Failed reserve view memory\r\n"
                ));
            goto cleanUp;
        }

        //  Associate to the rotation angle 0 context by default
        bResult = VirtualCopy( pMem, 
                               (VOID*)(g_VrfbContextList[index].dwPhysicalAddr/256), 
                               VRFB_VIEW_SIZE, 
                               PAGE_READWRITE | PAGE_PHYSICAL | PAGE_NOCACHE );

        if( bResult == FALSE )
        {
            DEBUGMSG(ZONE_ERROR, (L"ERROR: VRFB_AllocateView: "
                 L"Failed map context memory\r\n"
                ));
            goto cleanUp;
        }

        //  Change the attributes of the buffer for cache write thru
        bResult = CeSetMemoryAttributes( pMem, (VOID*)(g_VrfbContextList[index].dwPhysicalAddr/256), VRFB_VIEW_SIZE, PAGE_WRITECOMBINE );
        if( bResult == FALSE )
        {
            DEBUGMSG(ZONE_ERROR, (L"ERROR: VRFB_AllocateView: "
                 L"Failed CeSetMemoryAttributes for context # %d\r\n", index
                ));
            goto cleanUp;
        }

        DEBUGMSG(ZONE_INFO, (L"INFO: VRFB_AllocateView: "
             L"Context # %d: va = 0x%08X, pa = 0x%08X, buffer va = 0x%08X, buffer pa = 0x%08X", 
             index, pMem, g_VrfbContextList[index].dwPhysicalAddr, dwBufferVirtAddr, dwBufferPhysAddr
            ));
        
        //  Update the context table
        g_VrfbContextList[index].bInUse = TRUE;                                               
        g_VrfbContextList[index].dwWidth = dwWidth;                                               
        g_VrfbContextList[index].dwHeight = dwHeight;                                               
        g_VrfbContextList[index].dwRotationAngle = VRFB_ROTATE_ANGLE_0;                                               
        g_VrfbContextList[index].dwVirtualAddr = (DWORD) pMem;
        g_VrfbContextList[index].dwBufferPhysAddr = (DWORD) dwBufferPhysAddr;
        g_VrfbContextList[index].dwBufferVirtAddr = (DWORD) dwBufferVirtAddr;
        
        //  Program the VRFB registers
        OUTREG32(&pDevice->pVRFBRegs->aVRFB_SMS_ROT_CTRL[index].VRFB_SMS_ROT_CONTROL, dwPixelSize|SMS_ROT_CONTROL_PAGEWIDTH(DEFAULT_PAGE_WIDTH)|SMS_ROT_CONTROL_PAGEHEIGHT(DEFAULT_PAGE_HEIGHT));
        OUTREG32(&pDevice->pVRFBRegs->aVRFB_SMS_ROT_CTRL[index].VRFB_SMS_ROT_SIZE, SMS_ROT_SIZE_WIDTH(dwImageWidth)|SMS_ROT_SIZE_HEIGHT(dwImageHeight));
        OUTREG32(&pDevice->pVRFBRegs->aVRFB_SMS_ROT_CTRL[index].VRFB_SMS_ROT_PHYSICAL_BA, dwBufferPhysAddr);
        
        DebugPrintRegs(pDevice->pVRFBRegs, index);

        // mark VRFB registers dirty, to update backup copy in OAL
        HalContextUpdateDirtyRegister(HAL_CONTEXTSAVE_VRFB);

        //  A view handle is just the index of the VRFB context
        hView = (HANDLE)(VRFB_VIEW_HANDLE_BASE + index);  
    }
    else
    {
        DEBUGMSG(ZONE_WARNING, (L"ERROR: VRFB_AllocateView: "
            L"No available views\r\n"
            ));
        goto cleanUp;
    }
    
cleanUp:    
    //  Clean up failure conditions
    if( hView == NULL && dwBufferVirtAddr != 0 )
        FreePhysMem( (VOID*)dwBufferVirtAddr );

    if( hView == NULL && pMem != NULL )
        VirtualFree( pMem, 0, MEM_RELEASE );
        
    //  Return result
    DEBUGMSG(ZONE_FUNCTION, (L"-VRFB_AllocateView(hView = 0x%08X)\r\n", hView));
    return hView;
}

//------------------------------------------------------------------------------
//
//  Function:  VRFB_ReleaseView
//
//  This releases a VRFB view back to the pool.
//
BOOL    VRFB_ReleaseView(HANDLE hContext, HANDLE hView)
{
    BOOL        bResult = FALSE;
    Instance_t *pInstance = (Instance_t*)hContext;
    Device_t   *pDevice;
    DWORD       index;


    DEBUGMSG(ZONE_FUNCTION, (
        L"+VRFB_ReleaseView(0x%08x, 0x%08x)\r\n", hContext, hView
        ));

    // Check if we get correct context
    if ((pInstance == NULL) || (pInstance->cookie != VRFB_INSTANCE_COOKIE))
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: VRFB_ReleaseView: "
            L"Incorrect context param\r\n"
            ));
        goto cleanUp;
        }

    //  Get Device
    pDevice = pInstance->pDevice;

    //  Validate view handle
    index = (DWORD)hView - VRFB_VIEW_HANDLE_BASE;
    
    if( index < pDevice->dwNumDisplayContexts )
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: VRFB_ReleaseView: "
            L"Can't release display view # %d\r\n", index
            ));
        goto cleanUp;
    }

    
    //  Release the virtual memory associated with this view and free the context
    if( index < VRFB_ROTATION_CONTEXTS && g_VrfbContextList[index].bInUse )
    {
        //  Release the mapping to the VRFB context
        VirtualFree( (VOID*)g_VrfbContextList[index].dwVirtualAddr, VRFB_VIEW_SIZE, MEM_DECOMMIT );
        VirtualFree( (VOID*)g_VrfbContextList[index].dwVirtualAddr, 0, MEM_RELEASE );

        //  Release any allocated physical memory for the VRFB view buffer
        if( g_VrfbContextList[index].dwBufferVirtAddr != 0 )
        {
            FreePhysMem( (VOID*)g_VrfbContextList[index].dwBufferVirtAddr );
        }
        
        //  Clean up the context information
        g_VrfbContextList[index].bInUse = FALSE;
        g_VrfbContextList[index].dwWidth = 0;
        g_VrfbContextList[index].dwHeight = 0;
        g_VrfbContextList[index].dwRotationAngle = 0;
        g_VrfbContextList[index].dwVirtualAddr = 0;
        g_VrfbContextList[index].dwBufferPhysAddr = 0;
        g_VrfbContextList[index].dwBufferVirtAddr = 0;
        
        bResult = TRUE;
    }
    else
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: VRFB_ReleaseView: "
            L"Invalid view handle 0x%08X\r\n", hView
            ));
        goto cleanUp;
    }
    
cleanUp:    
    //  Return result
    DEBUGMSG(ZONE_FUNCTION, (L"-VRFB_ReleaseView(bResult = %d)\r\n", bResult));
    return bResult;
}
    
//------------------------------------------------------------------------------
//
//  Function:  VRFB_GetViewInfo
//
//  This returns VRFB View Info for the given view handle
//
BOOL    VRFB_GetViewInfo(HANDLE hContext, HANDLE hView, VRFB_VIEW_INFO *pInfo)
{
    BOOL        bResult = FALSE;
    Instance_t *pInstance = (Instance_t*)hContext;
    Device_t   *pDevice;
    DWORD       index;

    // Check if we get correct context
    if ((pInstance == NULL) || (pInstance->cookie != VRFB_INSTANCE_COOKIE))
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: VRFB_GetViewInfo: "
            L"Incorrect context param\r\n"
            ));
        goto cleanUp;
        }

    //  Get Device
    pDevice = pInstance->pDevice;

    //  Validate view handle
    index = (DWORD)hView - VRFB_VIEW_HANDLE_BASE;
    
    if( index < VRFB_ROTATION_CONTEXTS && g_VrfbContextList[index].bInUse )
    {
        DWORD   offset = 0;
        
        //  Get VRFB view rotation offset
        switch( g_VrfbContextList[index].dwRotationAngle )
        {
            //  Deliberate fall through on the case statements below
            case VRFB_ROTATE_ANGLE_270:
                offset += VRFB_VIEW_SIZE;
                
            case VRFB_ROTATE_ANGLE_180:
                offset += VRFB_VIEW_SIZE;

            case VRFB_ROTATE_ANGLE_90:
                offset += VRFB_VIEW_SIZE;

            case VRFB_ROTATE_ANGLE_0:
            default:
                break;
        }

        //  Fill in the view info structure
        memset( pInfo, 0, sizeof(VRFB_VIEW_INFO));
        
        pInfo->hView = hView;
        pInfo->dwPixelSize = INREG32(&pDevice->pVRFBRegs->aVRFB_SMS_ROT_CTRL[index].VRFB_SMS_ROT_CONTROL) & 0x00000003;
        pInfo->dwPixelSizeBytes = PIXEL_SIZE_TO_BYTES(pInfo->dwPixelSize);
        pInfo->dwWidth = g_VrfbContextList[index].dwWidth;
        pInfo->dwHeight = g_VrfbContextList[index].dwHeight;
        pInfo->dwPageWidth = DEFAULT_PAGE_WIDTH;
        pInfo->dwPageHeight = DEFAULT_PAGE_HEIGHT;
        pInfo->dwImageWidth = INREG32(&pDevice->pVRFBRegs->aVRFB_SMS_ROT_CTRL[index].VRFB_SMS_ROT_SIZE) & 0x000007FF;
        pInfo->dwImageHeight = (INREG32(&pDevice->pVRFBRegs->aVRFB_SMS_ROT_CTRL[index].VRFB_SMS_ROT_SIZE)>>16) & 0x000007FF;
        pInfo->dwImageStride = PIXEL_SIZE_TO_BYTES(pInfo->dwPixelSize) * VRFB_IMAGE_WIDTH_MAX;
        pInfo->dwOriginOffset = 0;
        pInfo->dwRotationAngle = g_VrfbContextList[index].dwRotationAngle;
        pInfo->dwVirtualAddr = g_VrfbContextList[index].dwVirtualAddr;
        pInfo->dwPhysicalAddrInput = g_VrfbContextList[index].dwPhysicalAddr + offset;
        pInfo->dwPhysicalAddrOutput = g_VrfbContextList[index].dwPhysicalAddr;
        pInfo->dwPhysicalBufferAddr = g_VrfbContextList[index].dwBufferPhysAddr;
        
        //  Adjust for VRFB limit on image width and height (0 indicates max of 2K pixels)
        pInfo->dwImageWidth = (pInfo->dwImageWidth == 0) ? VRFB_IMAGE_WIDTH_MAX : pInfo->dwImageWidth;
        pInfo->dwImageHeight = (pInfo->dwImageHeight == 0) ? VRFB_IMAGE_HEIGHT_MAX : pInfo->dwImageHeight;
        
        bResult = TRUE;
    }    
    else
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: VRFB_GetViewInfo: "
            L"Invalid view handle 0x%08X\r\n", hView
            ));
        goto cleanUp;
    }

cleanUp:    
    //  Return result
    return bResult;
}

//------------------------------------------------------------------------------
//
//  Function:  VRFB_RotateView
//
//  This rotates a VRFB view.
//
BOOL    VRFB_RotateView(HANDLE hContext, HANDLE hView, DWORD dwRotateAngle)
{
    BOOL        bResult = FALSE;
    Instance_t *pInstance = (Instance_t*)hContext;
    Device_t   *pDevice;
    DWORD       index;
    DWORD       offset = 0;

 
    DEBUGMSG(ZONE_FUNCTION, (
        L"+VRFB_RotateView(0x%08x, 0x%08x, %d)\r\n", hContext, hView, dwRotateAngle
        ));

    // Check if we get correct context
    if ((pInstance == NULL) || (pInstance->cookie != VRFB_INSTANCE_COOKIE))
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: VRFB_RotateView: "
            L"Incorrect context param\r\n"
            ));
        goto cleanUp;
        }

    //  Get Device
    pDevice = pInstance->pDevice;


    //  Validate view handle
    index = (DWORD)hView - VRFB_VIEW_HANDLE_BASE;
    
    if( index < VRFB_ROTATION_CONTEXTS && g_VrfbContextList[index].bInUse )
    {
        //  If the rotation angle is the same as currently set, do nothing
        if( dwRotateAngle == g_VrfbContextList[index].dwRotationAngle )
        {
            bResult = TRUE;
            goto cleanUp;
        }
        
        //  Rotate the view by remapping underlying VRFB physical address into the virtual address for the view
        switch( dwRotateAngle )
        {
            //  Deliberate fall through on the case statements below
            case VRFB_ROTATE_ANGLE_270:
                offset += VRFB_VIEW_SIZE;
                
            case VRFB_ROTATE_ANGLE_180:
                offset += VRFB_VIEW_SIZE;

            case VRFB_ROTATE_ANGLE_90:
                offset += VRFB_VIEW_SIZE;

            case VRFB_ROTATE_ANGLE_0:
            default:
                break;
        }
        
        //  Decommit the existing mapping to VRFB 
        VirtualFree( (VOID*)g_VrfbContextList[index].dwVirtualAddr, VRFB_VIEW_SIZE, MEM_DECOMMIT );

        //  Associate to the selected rotation angle offset
        bResult = VirtualCopy( (VOID*)g_VrfbContextList[index].dwVirtualAddr, 
                               (VOID*)((g_VrfbContextList[index].dwPhysicalAddr + offset)/256), 
                               VRFB_VIEW_SIZE, 
                               PAGE_READWRITE | PAGE_PHYSICAL | PAGE_NOCACHE );

        if( bResult == FALSE )
        {
            DEBUGMSG(ZONE_ERROR, (L"ERROR: VRFB_RotateView: "
                 L"Failed map context memory for context # %d\r\n", index
                ));
            goto cleanUp;
        }

        //  Change the attributes of the buffer for cache write thru
        bResult = CeSetMemoryAttributes( (VOID*)g_VrfbContextList[index].dwVirtualAddr, (VOID*)((g_VrfbContextList[index].dwPhysicalAddr + offset)/256), VRFB_VIEW_SIZE, PAGE_WRITECOMBINE );
        if( bResult == FALSE )
        {
            DEBUGMSG(ZONE_ERROR, (L"ERROR: VRFB_RotateView: "
                 L"Failed CeSetMemoryAttributes for context # %d\r\n", index
                ));
            goto cleanUp;
        }
        
        //  Set new rotation angle
        g_VrfbContextList[index].dwRotationAngle = dwRotateAngle;
    }
    else
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: VRFB_RotateView: "
            L"Invalid view handle 0x%08X\r\n", hView
            ));
        goto cleanUp;
    }
    
cleanUp:    
    //  Return result
    DEBUGMSG(ZONE_FUNCTION, (L"-VRFB_RotateView(bResult = %d)\r\n", bResult));
    return bResult;
}

//------------------------------------------------------------------------------
//
//  Function:  VRFB_UpdateView
//
//  This updates a VRFB view parameters.
//
BOOL    VRFB_UpdateView(HANDLE hContext, HANDLE hView, DWORD dwPixelSize, DWORD dwWidth, DWORD dwHeight, DWORD dwBufferPhysAddr)
{
    BOOL        bResult = FALSE;
    Instance_t *pInstance = (Instance_t*)hContext;
    Device_t   *pDevice;
    DWORD       index;
    DWORD       dwImageWidth;
    DWORD       dwImageHeight;

 
    DEBUGMSG(ZONE_FUNCTION, (
        L"+VRFB_UpdateView(0x%08x, 0x%08x, %d, %d, %d, 0x%08x )\r\n", hContext, hView, dwPixelSize, dwWidth, dwHeight, dwBufferPhysAddr
        ));

    // Check if we get correct context
    if ((pInstance == NULL) || (pInstance->cookie != VRFB_INSTANCE_COOKIE))
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: VRFB_UpdateView: "
            L"Incorrect context param\r\n"
            ));
        goto cleanUp;
        }

    //  Get Device
    pDevice = pInstance->pDevice;

    //  Validate parameters
    if( (dwPixelSize != VRFB_PIXELSIZE_1B) && (dwPixelSize != VRFB_PIXELSIZE_2B) && (dwPixelSize != VRFB_PIXELSIZE_4B) )
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: VRFB_UpdateView: "
            L"Invalid pixel size\r\n"
            ));
        goto cleanUp;
        }

    //  Ensure image width and height are multiple of page size
    dwImageWidth  = IMAGE_SIZE_ROUNDING(dwWidth, DEFAULT_PAGE_WIDTH);
    dwImageHeight = IMAGE_SIZE_ROUNDING(dwHeight, DEFAULT_PAGE_HEIGHT);


    //  Validate view handle
    index = (DWORD)hView - VRFB_VIEW_HANDLE_BASE;
    
    if( index < VRFB_ROTATION_CONTEXTS && g_VrfbContextList[index].bInUse )
    {
        //  Can't update parameters for views with internally allocated buffer memory
        if( g_VrfbContextList[index].dwBufferVirtAddr != 0 )
        {
            DEBUGMSG(ZONE_ERROR, (L"ERROR: VRFB_UpdateView: "
                 L"Can't update view\r\n"
                ));
            goto cleanUp;
        }
        
        //  Re-use existing buffer address if given one is 0, otherwise cache the value
        if( dwBufferPhysAddr == 0 )
        {
            dwBufferPhysAddr = g_VrfbContextList[index].dwBufferPhysAddr;
        }
        else
        {
            g_VrfbContextList[index].dwBufferPhysAddr = dwBufferPhysAddr;
        }

        //  Update real width and heigth
        g_VrfbContextList[index].dwWidth = dwWidth;
        g_VrfbContextList[index].dwHeight = dwHeight;


        //  Program the VRFB registers
        OUTREG32(&pDevice->pVRFBRegs->aVRFB_SMS_ROT_CTRL[index].VRFB_SMS_ROT_CONTROL, dwPixelSize|SMS_ROT_CONTROL_PAGEWIDTH(DEFAULT_PAGE_WIDTH)|SMS_ROT_CONTROL_PAGEHEIGHT(DEFAULT_PAGE_HEIGHT));
        OUTREG32(&pDevice->pVRFBRegs->aVRFB_SMS_ROT_CTRL[index].VRFB_SMS_ROT_SIZE, SMS_ROT_SIZE_WIDTH(dwImageWidth)|SMS_ROT_SIZE_HEIGHT(dwImageHeight));
        OUTREG32(&pDevice->pVRFBRegs->aVRFB_SMS_ROT_CTRL[index].VRFB_SMS_ROT_PHYSICAL_BA, dwBufferPhysAddr);
        
        DebugPrintRegs(pDevice->pVRFBRegs, index);

        // mark VRFB registers dirty, to update backup copy in OAL
        HalContextUpdateDirtyRegister(HAL_CONTEXTSAVE_VRFB);
        
        bResult = TRUE;
    }
    else
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: VRFB_UpdateView: "
            L"Invalid view handle 0x%08X\r\n", hView
            ));
        goto cleanUp;
    }
    
cleanUp:    
    //  Return result
    DEBUGMSG(ZONE_FUNCTION, (L"-VRFB_UpdateView(bResult = %d)\r\n", bResult));
    return bResult;
}

//------------------------------------------------------------------------------
//
//  Function:  VRFB_NumDisplayViews
//
//  Returns the number of pre-allocated display views
//
DWORD   VRFB_NumDisplayViews(HANDLE hContext)
{
    DWORD       dwResult = 0;
    Instance_t *pInstance = (Instance_t*)hContext;
    Device_t   *pDevice;

    // Check if we get correct context
    if ((pInstance == NULL) || (pInstance->cookie != VRFB_INSTANCE_COOKIE))
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: VRFB_NumDisplayViews: "
            L"Incorrect context param\r\n"
            ));
        goto cleanUp;
        }

    //  Get Device
    pDevice = pInstance->pDevice;

    //  Get value
    dwResult = pDevice->dwNumDisplayContexts;
    
cleanUp:    
    //  Return result
    return dwResult;
}
    
//------------------------------------------------------------------------------
//
//  Function:  VRFB_GetDisplayView
//
//  This returns a handle to the request display view by index.
//
HANDLE  VRFB_GetDisplayView(HANDLE hContext, DWORD dwIndex)
{
    HANDLE      hView = NULL;
    Instance_t *pInstance = (Instance_t*)hContext;
    Device_t   *pDevice;

    // Check if we get correct context
    if ((pInstance == NULL) || (pInstance->cookie != VRFB_INSTANCE_COOKIE))
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: VRFB_GetDisplayView: "
            L"Incorrect context param\r\n"
            ));
        goto cleanUp;
        }

    //  Get Device
    pDevice = pInstance->pDevice;

    //  Check the index value
    if( dwIndex < pDevice->dwNumDisplayContexts )
    {
        //  A view handle is just the index of the VRFB context
        hView = (HANDLE)(VRFB_VIEW_HANDLE_BASE + dwIndex);  
    }
    
cleanUp:    
    //  Return result
    return hView;
}
    
//------------------------------------------------------------------------------
//
//  Function:  VRFB_GetDisplayViewInfo
//
//  This returns VRFB View Info for the given display view index and rotation angle
//
BOOL    VRFB_GetDisplayViewInfo(HANDLE hContext, DWORD dwIndex, DWORD dwRotateAngle, BOOL bMirror, VRFB_VIEW_INFO *pInfo)
{
    BOOL        bResult = FALSE;
    Instance_t *pInstance = (Instance_t*)hContext;
    Device_t   *pDevice;
 

    // Check if we get correct context
    if ((pInstance == NULL) || (pInstance->cookie != VRFB_INSTANCE_COOKIE))
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: VRFB_GetDisplayViewInfo: "
            L"Incorrect context param\r\n"
            ));
        goto cleanUp;
        }

    //  Get Device
    pDevice = pInstance->pDevice;

    //  Check the index value
    if( dwIndex < pDevice->dwNumDisplayContexts )
    {
        DWORD   dwRotationOffset;
        
        //  Get VRFB view rotation offset for requested output angle
        switch( dwRotateAngle )
        {
            case VRFB_ROTATE_ANGLE_270:
                dwRotationOffset = VRFB_VIEW_SIZE;
                break;
                
            case VRFB_ROTATE_ANGLE_180:
                dwRotationOffset = (bMirror) ? 0 : 2*VRFB_VIEW_SIZE;
                break;

            case VRFB_ROTATE_ANGLE_90:
                dwRotationOffset = 3*VRFB_VIEW_SIZE;
                break;

            case VRFB_ROTATE_ANGLE_0:
            default:
                dwRotationOffset = (bMirror) ? 2*VRFB_VIEW_SIZE : 0;
                break;
        }


        //  Fill in the view info structure
        memset( pInfo, 0, sizeof(VRFB_VIEW_INFO));
        
        pInfo->dwPixelSize = INREG32(&pDevice->pVRFBRegs->aVRFB_SMS_ROT_CTRL[dwIndex].VRFB_SMS_ROT_CONTROL) & 0x00000003;
        pInfo->dwPixelSizeBytes = PIXEL_SIZE_TO_BYTES(pInfo->dwPixelSize);
        pInfo->dwWidth = g_VrfbContextList[dwIndex].dwWidth;
        pInfo->dwHeight = g_VrfbContextList[dwIndex].dwHeight;
        pInfo->dwPageWidth = DEFAULT_PAGE_WIDTH;
        pInfo->dwPageHeight = DEFAULT_PAGE_HEIGHT;
        pInfo->dwImageWidth = INREG32(&pDevice->pVRFBRegs->aVRFB_SMS_ROT_CTRL[dwIndex].VRFB_SMS_ROT_SIZE) & 0x000007FF;
        pInfo->dwImageHeight = (INREG32(&pDevice->pVRFBRegs->aVRFB_SMS_ROT_CTRL[dwIndex].VRFB_SMS_ROT_SIZE)>>16) & 0x000007FF;
        pInfo->dwImageStride = PIXEL_SIZE_TO_BYTES(pInfo->dwPixelSize) * VRFB_IMAGE_WIDTH_MAX;
        pInfo->dwRotationAngle = dwRotateAngle;
        pInfo->dwVirtualAddr = g_VrfbContextList[dwIndex].dwVirtualAddr;
        pInfo->dwPhysicalAddrInput = g_VrfbContextList[dwIndex].dwPhysicalAddr;
        pInfo->dwPhysicalAddrOutput = g_VrfbContextList[dwIndex].dwPhysicalAddr + dwRotationOffset;
        pInfo->dwPhysicalBufferAddr = g_VrfbContextList[dwIndex].dwBufferPhysAddr;
        
        //  Adjust for VRFB limit on image width and height (0 indicates max of 2K pixels)
        pInfo->dwImageWidth = (pInfo->dwImageWidth == 0) ? VRFB_IMAGE_WIDTH_MAX : pInfo->dwImageWidth;
        pInfo->dwImageHeight = (pInfo->dwImageHeight == 0) ? VRFB_IMAGE_HEIGHT_MAX : pInfo->dwImageHeight;

        //  Compute pixel origin offset
        if( bMirror )
        {
            //  Compute pixel origin offset (mirrored)
            switch( dwRotateAngle )
            {
                case VRFB_ROTATE_ANGLE_270:
                    pInfo->dwOriginOffset  = VRFB_IMAGE_WIDTH_MAX * (pInfo->dwWidth - 1) * pInfo->dwPixelSizeBytes;
                    pInfo->dwOriginOffset += (pInfo->dwImageHeight - pInfo->dwHeight) * pInfo->dwPixelSizeBytes;
                    break;
                    
                case VRFB_ROTATE_ANGLE_180:
                    pInfo->dwOriginOffset  = VRFB_IMAGE_WIDTH_MAX * (pInfo->dwHeight - 1) * pInfo->dwPixelSizeBytes;
                    break;

                case VRFB_ROTATE_ANGLE_90:
                    pInfo->dwOriginOffset = VRFB_IMAGE_HEIGHT_MAX * (pInfo->dwImageWidth - 1) * pInfo->dwPixelSizeBytes;
                    break;

                case VRFB_ROTATE_ANGLE_0:
                default:
                    pInfo->dwOriginOffset  = VRFB_IMAGE_WIDTH_MAX * (pInfo->dwImageHeight - 1) * pInfo->dwPixelSizeBytes;
                    pInfo->dwOriginOffset += (pInfo->dwImageWidth - pInfo->dwWidth) * pInfo->dwPixelSizeBytes;
                    break;
            }
        }
        else
        {
            //  Compute pixel origin offset (non-mirrored)
            switch( dwRotateAngle )
            {
                case VRFB_ROTATE_ANGLE_270:
                    pInfo->dwOriginOffset = (pInfo->dwImageHeight - pInfo->dwHeight) * pInfo->dwPixelSizeBytes;
                    break;
                    
                case VRFB_ROTATE_ANGLE_180:
                    pInfo->dwOriginOffset  = VRFB_IMAGE_WIDTH_MAX * (pInfo->dwImageHeight - pInfo->dwHeight) * pInfo->dwPixelSizeBytes;
                    pInfo->dwOriginOffset += (pInfo->dwImageWidth - pInfo->dwWidth) * pInfo->dwPixelSizeBytes;
                    break;

                case VRFB_ROTATE_ANGLE_90:
                    pInfo->dwOriginOffset = VRFB_IMAGE_HEIGHT_MAX * (pInfo->dwImageWidth - pInfo->dwWidth) * pInfo->dwPixelSizeBytes;
                    break;

                case VRFB_ROTATE_ANGLE_0:
                default:
                    pInfo->dwOriginOffset = 0;
                    break;
            }
        }
        
        bResult = TRUE;
    }    
    else
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: VRFB_GetDisplayViewInfo: "
            L"Invalid view index %d\r\n", dwIndex
            ));
        goto cleanUp;
    }

cleanUp:    
    //  Return result
    return bResult;
}

//------------------------------------------------------------------------------
//
//  Function:  DebugPrintRegs
//
//  Dumps out register values
//
VOID    DebugPrintRegs(OMAP_VRFB_REGS *pVrfbRegs, DWORD dwIndex)
{
    DWORD   dwControl,
            dwSize,
            dwBuffer;
            
    dwControl = INREG32(&pVrfbRegs->aVRFB_SMS_ROT_CTRL[dwIndex].VRFB_SMS_ROT_CONTROL);
    dwSize =    INREG32(&pVrfbRegs->aVRFB_SMS_ROT_CTRL[dwIndex].VRFB_SMS_ROT_SIZE);
    dwBuffer =  INREG32(&pVrfbRegs->aVRFB_SMS_ROT_CTRL[dwIndex].VRFB_SMS_ROT_PHYSICAL_BA);

    DEBUGMSG(ZONE_INFO, (L"DebugPrintRegs: VRFB Context #           = %d\r\n", dwIndex));
    DEBUGMSG(ZONE_INFO, (L"DebugPrintRegs: VRFB_SMS_ROT_CONTROL     = 0x%08x\r\n", dwControl));
    DEBUGMSG(ZONE_INFO, (L"DebugPrintRegs: VRFB_SMS_ROT_SIZE        = 0x%08x\r\n", dwSize));
    DEBUGMSG(ZONE_INFO, (L"DebugPrintRegs: VRFB_SMS_ROT_PHYSICAL_BA = 0x%08x\r\n", dwBuffer));
}


//------------------------------------------------------------------------------
//
//  Function:  DllMain
//
//  Standard Windows DLL entry point.
//
BOOL
__stdcall
DllMain(
    HANDLE hDLL,
    DWORD reason,
    VOID *pReserved
    )
{
    UNREFERENCED_PARAMETER(pReserved);
    switch (reason)
        {
        case DLL_PROCESS_ATTACH:
            RETAILREGISTERZONES((HMODULE)hDLL);
            DisableThreadLibraryCalls((HMODULE)hDLL);
            break;
        }
    return TRUE;
}

//------------------------------------------------------------------------------


