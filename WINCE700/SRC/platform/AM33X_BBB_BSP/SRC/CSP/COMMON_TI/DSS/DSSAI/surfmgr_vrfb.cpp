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

#include "omap.h"
#include "omap_dss_regs.h"
#include "dssai.h"
#include "heap.h"
#include "lcd.h"
#include "ceddkex.h"
#include "omap_vrfb_regs.h"

#include <initguid.h>
#include "vrfb.h"

//
//  Defines
//

// Uncomment following lines to get debugmsgs for this file alone in RETAIL build.
//#undef DEBUGMSG
//#define DEBUGMSG(x,y) RETAILMSG(TRUE,y)

#define MIN_NUM_VIEWS   4

//
//  Structs
//



//
//  Globals
//

BOOL    g_bVRFBViewReserved[VRFB_ROTATION_CONTEXTS] = 
{
    FALSE,  FALSE,  FALSE,  FALSE,
    FALSE,  FALSE,  FALSE,  FALSE,
    FALSE,  FALSE,  FALSE,  FALSE
};



//------------------------------------------------------------------------------
OMAPVrfbSurface::OMAPVrfbSurface()
{
    m_hHeap = NULL;
    m_hVRFB = NULL;
    m_hVRFBView = NULL;
    m_dwViewIndex = (DWORD) -1;
    m_dwWidthFactor = 1;
    m_dwHorizScale = 1;
    m_dwVertScale = 1;
    m_bUseResizer = FALSE;
    m_pAssocSurface = NULL;
    m_eSurfaceType = OMAP_SURFACE_NORMAL;
}

//------------------------------------------------------------------------------
OMAPVrfbSurface::~OMAPVrfbSurface()
{
    Heap    *pHeap = (Heap*) m_hHeap;
    
    //  Free the underlying surface memory
    if( pHeap )
        pHeap->Free();
        
    //  Free the VRFB view
    if( m_dwViewIndex != -1 && m_dwViewIndex < VRFB_ROTATION_CONTEXTS)
    {
        g_bVRFBViewReserved[m_dwViewIndex] = FALSE;        

        DEBUGMSG(ZONE_WARNING, (L"INFO: OMAPVrfbSurface::~OMAPVrfbSurface() - Freeing VRFB View # %d\r\n", m_dwViewIndex));
    }
}

//------------------------------------------------------------------------------
BOOL    
OMAPVrfbSurface::Allocate(
    OMAP_DSS_PIXELFORMAT    ePixelFormat,
    DWORD                   dwWidth,
    DWORD                   dwHeight,
    HANDLE                  hHeap,
    HANDLE                  hVRFB
    )
{
    BOOL            bResult;
    Heap            *pMainHeap = (Heap*) hHeap,
                    *pSurfHeap = NULL;
    DWORD           dwVrfbPixelSize;
    DWORD           dwWidthFactor = 1;
    DWORD           i;
    HANDLE          hVRFBView;
    VRFB_VIEW_INFO  info;
    VOID            *ptr;       
    DWORD           dwStride;         
            
            
    //  Set surface properties based on pixel format
    switch( ePixelFormat )
    {
        case OMAP_DSS_PIXELFORMAT_RGB16:
        case OMAP_DSS_PIXELFORMAT_ARGB16:
            //  2 bytes per pixel
            dwVrfbPixelSize = VRFB_PIXELSIZE_2B;
            break;

        case OMAP_DSS_PIXELFORMAT_RGB32:
        case OMAP_DSS_PIXELFORMAT_ARGB32:
        case OMAP_DSS_PIXELFORMAT_RGBA32:
            //  4 bytes per pixel
            dwVrfbPixelSize = VRFB_PIXELSIZE_4B;
            break;

        case OMAP_DSS_PIXELFORMAT_YUV2:
        case OMAP_DSS_PIXELFORMAT_UYVY:
            //  2 bytes per pixel, but treated as 4 bytes and half the width
            //  to support VRFB rotation for YUV formats
            dwVrfbPixelSize = VRFB_PIXELSIZE_4B;
            dwWidthFactor = 2;
            break;

        default:
            ASSERT(0);
            return FALSE;
    }


    //  Find a free VRFB view to use
    for( i = 0; i < VRFBNumDisplayViews(hVRFB); i++ )
    {
        if( g_bVRFBViewReserved[i] == FALSE )
            break;
    }

    //  Check result
    if( i == VRFBNumDisplayViews(hVRFB) )
    {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: All VRFB views are allocated\n"));
        goto cleanUp;
    }


    //  Get the free display view
    hVRFBView = VRFBGetDisplayView( hVRFB, i );
    
    //  Update the view with the surface parameters         
    bResult = VRFBUpdateView( hVRFB, hVRFBView, dwVrfbPixelSize, dwWidth/dwWidthFactor, dwHeight, 0 );
    if(  bResult == FALSE )
    {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: Unable to update VRFB view parameters\n"));
        goto cleanUp;
    }

    //  Get the view information to allocate the underlying memory
    bResult = VRFBGetViewInfo( hVRFB, hVRFBView, &info );
    if(  bResult == FALSE )
    {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: Unable to get VRFB view info\n"));
        goto cleanUp;
    }
        


    //  Allocate the VRFB memory from the given video memory heap using the VRFB adjusted
    //  image width and height which are a multiple of the VRFB page size
    pSurfHeap = pMainHeap->Allocate( info.dwPixelSizeBytes * info.dwImageWidth * info.dwImageHeight );
    if( pSurfHeap == NULL )
    {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: Unable to allocate heap memory\n"));
        goto cleanUp;
    }

    //  Update the VRFB view with the surface parameters and allocated display memory         
    VRFBUpdateView( hVRFB, hVRFBView, dwVrfbPixelSize, dwWidth/dwWidthFactor, dwHeight, pSurfHeap->Address() );


    //  Initialize the surface properties for this surface type
    m_ePixelFormat   = ePixelFormat;
    m_dwPixelSize    = info.dwPixelSizeBytes;
    m_eOrientation   = OMAP_SURF_ORIENTATION_STANDARD;
    m_dwWidth        = dwWidth;
    m_dwHeight       = dwHeight;

    //  Set clipping region to be entire surface
    SetClipping( NULL );

    //  Set surface specific properties
    m_hHeap     = (HANDLE) pSurfHeap;
    m_hVRFB     = hVRFB;
    m_hVRFBView = hVRFBView;
    
    g_bVRFBViewReserved[i] = TRUE;
    m_dwViewIndex          = i;
    m_dwWidthFactor        = dwWidthFactor;

    //  Clear out the memory
    ptr = VirtualAddr();
    dwStride = Stride();
    
    for( i = 0; i < m_dwHeight; i++ )
    {
        memset( ptr, 0, m_dwPixelSize*m_dwWidth);
        ptr = (VOID*)((DWORD)ptr + dwStride);
    }

    // Initialize variables
    m_hRSZHandle = NULL;
    memset(&m_sRSZParams,0,sizeof(RSZParams_t));      

    DEBUGMSG(ZONE_WARNING, (L"INFO: OMAPVrfbSurface::Allocate() - Allocating VRFB View # %d\r\n", m_dwViewIndex));
    
cleanUp:        
    //  Return
    return (pSurfHeap != NULL);
}


//------------------------------------------------------------------------------
VOID*   
OMAPVrfbSurface::VirtualAddr()
{
    VRFB_VIEW_INFO  info;      

    if ((m_bUseResizer) && (m_eSurfaceType == OMAP_SURFACE_NORMAL) && (m_pAssocSurface))
        return m_pAssocSurface->VirtualAddr();
    else
    {
        //  Get virtual address of VRFB view
        VRFBGetViewInfo( m_hVRFB, m_hVRFBView, &info );
        return (VOID*) info.dwVirtualAddr;
    }
}


//------------------------------------------------------------------------------
DWORD   
OMAPVrfbSurface::Width(
    OMAP_DSS_ROTATION   eRotation
    )
{
    DWORD   dwWidth;
    
    //  Return the surface width depending on the rotation angle
    switch( eRotation )
    {
        case OMAP_DSS_ROTATION_0:
        case OMAP_DSS_ROTATION_180:
            //  Normal
            dwWidth = m_dwWidth/m_dwHorizScale;
            break;
            
        case OMAP_DSS_ROTATION_90:
        case OMAP_DSS_ROTATION_270:
            //  Rotated
            dwWidth = m_dwHeight/m_dwVertScale;
            break;

        default:
            ASSERT(0);
            return 0;
    }
    
    //  Return value
    return dwWidth;
}

//------------------------------------------------------------------------------
DWORD   
OMAPVrfbSurface::Height(
    OMAP_DSS_ROTATION   eRotation
    )
{
    DWORD   dwHeight;
    
    //  Return the surface height depending on the rotation angle
    switch( eRotation )
    {
        case OMAP_DSS_ROTATION_0:
        case OMAP_DSS_ROTATION_180:
            //  Normal
            dwHeight = m_dwHeight/m_dwVertScale;
            break;
            
        case OMAP_DSS_ROTATION_90:
        case OMAP_DSS_ROTATION_270:
            //  Rotated
            dwHeight = m_dwWidth/m_dwHorizScale;
            break;

        default:
            ASSERT(0);
            return 0;
    }
    
    //  Return value
    return dwHeight;
}

//------------------------------------------------------------------------------
DWORD   
OMAPVrfbSurface::Stride(
    OMAP_DSS_ROTATION   eRotation
    )
{
    DWORD           dwRotation;
    VRFB_VIEW_INFO  info;        
    DWORD           dwScaleFactor;        

    //  Translate DSS rotation enum to VRFB enum
    switch( eRotation )
    {   
        case OMAP_DSS_ROTATION_0:
            dwRotation = VRFB_ROTATE_ANGLE_0;
            dwScaleFactor = m_dwHorizScale;
            break;

        case OMAP_DSS_ROTATION_90:
            dwRotation = VRFB_ROTATE_ANGLE_90;
            dwScaleFactor = m_dwVertScale;
            break;

        case OMAP_DSS_ROTATION_180:
            dwRotation = VRFB_ROTATE_ANGLE_180;
            dwScaleFactor = m_dwHorizScale;
            break;
            
        case OMAP_DSS_ROTATION_270:
            dwRotation = VRFB_ROTATE_ANGLE_270;
            dwScaleFactor = m_dwVertScale;
            break;

        default:
            ASSERT(0);
            return 0;
    }
    
    //  Get stride of VRFB display view
    VRFBGetDisplayViewInfo( m_hVRFB, m_dwViewIndex, dwRotation, FALSE, &info );
    return (info.dwImageStride * dwScaleFactor);
}

//------------------------------------------------------------------------------
DWORD   
OMAPVrfbSurface::PhysicalAddr(
    OMAP_DSS_ROTATION       eRotation,
    BOOL                    bMirror,
    OMAP_ASSOC_SURF_USAGE   eUseAssocSurface
    )
{
    DWORD           dwRotation;
    VRFB_VIEW_INFO  info;                
    INT             iClipOffsetX = 0,
                    iClipOffsetY = 0;

     if ((((eUseAssocSurface==OMAP_ASSOC_SURF_DEFAULT) && (m_bUseResizer)) ||
          ( eUseAssocSurface==OMAP_ASSOC_SURF_FORCE_ON )                     ) &&
        (m_pAssocSurface))
        
        return m_pAssocSurface->PhysicalAddr(eRotation,bMirror,OMAP_ASSOC_SURF_FORCE_OFF);        


    //  Translate DSS rotation enum to VRFB enum
    switch( eRotation )
    {   
        case OMAP_DSS_ROTATION_0:
            dwRotation = VRFB_ROTATE_ANGLE_0;
            break;

        case OMAP_DSS_ROTATION_90:
            dwRotation = VRFB_ROTATE_ANGLE_90;
            break;

        case OMAP_DSS_ROTATION_180:
            dwRotation = VRFB_ROTATE_ANGLE_180;
            break;
            
        case OMAP_DSS_ROTATION_270:
            dwRotation = VRFB_ROTATE_ANGLE_270;
            break;

        default:
            ASSERT(0);
            return 0;
    }
    
    //  Get virtual address of VRFB display view
    VRFBGetDisplayViewInfo( m_hVRFB, m_dwViewIndex, dwRotation, bMirror, &info );


    //  Compute clip offset
    switch( eRotation )
    {   
        case OMAP_DSS_ROTATION_0:
            if( bMirror )
            {
                iClipOffsetX = ((m_dwWidth - m_rcClip.right) / m_dwWidthFactor) * info.dwPixelSizeBytes;
                iClipOffsetY = -1 * (m_rcClip.top * info.dwImageStride);
            }
            else
            {
                iClipOffsetX = (m_rcClip.left / m_dwWidthFactor) * info.dwPixelSizeBytes;
                iClipOffsetY = m_rcClip.top * info.dwImageStride;
            }
            break;

        case OMAP_DSS_ROTATION_90:
            if( bMirror )
            {
                iClipOffsetX = -1 * (m_rcClip.left / m_dwWidthFactor) * info.dwImageStride;
                iClipOffsetY = m_rcClip.top * info.dwPixelSizeBytes;
            }
            else
            {
                iClipOffsetX = ((m_dwWidth - m_rcClip.right) / m_dwWidthFactor) * info.dwImageStride;
                iClipOffsetY = m_rcClip.top * info.dwPixelSizeBytes;
            }
            break;

        case OMAP_DSS_ROTATION_180:
            if( bMirror )
            {
                iClipOffsetX = (m_rcClip.left / m_dwWidthFactor) * info.dwPixelSizeBytes;
                iClipOffsetY = -1 * ((m_dwHeight - m_rcClip.bottom) * info.dwImageStride);
            }
            else
            {
                iClipOffsetX = ((m_dwWidth - m_rcClip.right) / m_dwWidthFactor) * info.dwPixelSizeBytes;
                iClipOffsetY = (m_dwHeight - m_rcClip.bottom) * info.dwImageStride;
            }
            break;
            
        case OMAP_DSS_ROTATION_270:
            if( bMirror )
            {
                iClipOffsetX = -1 * ((m_dwWidth - m_rcClip.right) / m_dwWidthFactor) * info.dwImageStride;
                iClipOffsetY = (m_dwHeight - m_rcClip.bottom) * info.dwPixelSizeBytes;
            }
            else
            {
                iClipOffsetX = (m_rcClip.left / m_dwWidthFactor) * info.dwImageStride;
                iClipOffsetY = (m_dwHeight - m_rcClip.bottom) * info.dwPixelSizeBytes;
            }
            break;
    }

    return info.dwPhysicalAddrOutput + info.dwOriginOffset + iClipOffsetX + iClipOffsetY;
}


//------------------------------------------------------------------------------
DWORD   
OMAPVrfbSurface::PixelIncr(
    OMAP_DSS_ROTATION   eRotation,
    BOOL                bMirror
    )
{
    DWORD   dwIncr = 1;
    
    UNREFERENCED_PARAMETER(bMirror);
    //  Pixel increment is always 1 for VRFB surfaces (mirror setting has no impact)
    
    //  Scale factor is dependent on rotation angle
    switch( eRotation )
    {
        case OMAP_DSS_ROTATION_0:
        case OMAP_DSS_ROTATION_180:
            //  (+ horizontal_scale_factor*pixel_size)
            dwIncr += ((m_dwHorizScale - 1) * m_dwPixelSize);
            break;

        case OMAP_DSS_ROTATION_90:
        case OMAP_DSS_ROTATION_270:
            //  (+ horizontal_scale_factor*pixel_size)
            dwIncr += ((m_dwVertScale - 1) * m_dwPixelSize);
            break;

        default:
            ASSERT(0);
            return 0;
    }            
    
    //  Return pixel increment
    return dwIncr;
}
                            
//------------------------------------------------------------------------------
DWORD   
OMAPVrfbSurface::RowIncr(
    OMAP_DSS_ROTATION   eRotation,
    BOOL                bMirror
    )
{
    VRFB_VIEW_INFO  info;
    DWORD           dwRowIncr;     
    DWORD           dwClipOffsetX,
                    dwClipOffsetY;

    if ((m_bUseResizer) && (m_eSurfaceType == OMAP_SURFACE_NORMAL) && (m_pAssocSurface))
        return m_pAssocSurface->RowIncr(eRotation,bMirror);

    //  Compute the VRFB row increment
    VRFBGetViewInfo( m_hVRFB, m_hVRFBView, &info );

    //  Compute clipping offsets
    dwClipOffsetX = m_dwWidth - (m_rcClip.right - m_rcClip.left);
    dwClipOffsetY = m_dwHeight - (m_rcClip.bottom - m_rcClip.top);
    
    //  Row increment depends on angle, mirror and clipping region
    switch( eRotation )
    {
        case OMAP_DSS_ROTATION_0:
        case OMAP_DSS_ROTATION_180:
            //  Computation changes depending on mirror setting
            if( !bMirror )
            {
                dwRowIncr = (VRFB_IMAGE_WIDTH_MAX - (m_dwWidth - dwClipOffsetX)/m_dwWidthFactor) * info.dwPixelSizeBytes + 1;

                //  (+ vertical_scale_factor*stride + horizontal_scale_factor*pixel_size) 
                dwRowIncr += ((m_dwVertScale - 1) * VRFB_IMAGE_WIDTH_MAX * info.dwPixelSizeBytes);
                dwRowIncr += ((m_dwHorizScale - 1) * info.dwPixelSizeBytes);
            }
            else
            {
                dwRowIncr = (VRFB_IMAGE_WIDTH_MAX + (m_dwWidth - dwClipOffsetX)/m_dwWidthFactor) * info.dwPixelSizeBytes;
                dwRowIncr = 1 - dwRowIncr;

                //  (- vertical_scale_factor*stride + horizontal_scale_factor*pixel_size) 
                dwRowIncr -= ((m_dwVertScale - 1) * VRFB_IMAGE_WIDTH_MAX * info.dwPixelSizeBytes);
                dwRowIncr += ((m_dwHorizScale - 1) * info.dwPixelSizeBytes);
            }
            break;

        case OMAP_DSS_ROTATION_90:
        case OMAP_DSS_ROTATION_270:
            //  Computation changes depending on mirror setting
            if( !bMirror )
            {
                dwRowIncr = (VRFB_IMAGE_HEIGHT_MAX - (m_dwHeight - dwClipOffsetY)) * info.dwPixelSizeBytes + 1;

                //  (+ horizontal_scale_factor*stride + vertical_scale_factor*pixel_size) 
                dwRowIncr += ((m_dwHorizScale - 1) * VRFB_IMAGE_HEIGHT_MAX * info.dwPixelSizeBytes);
                dwRowIncr += ((m_dwVertScale - 1) * info.dwPixelSizeBytes);
            }
            else
            {
                dwRowIncr = (VRFB_IMAGE_HEIGHT_MAX + (m_dwHeight - dwClipOffsetY)) * info.dwPixelSizeBytes;
                dwRowIncr = 1 - dwRowIncr;

                //  (- horizontal_scale_factor*stride + vertical_scale_factor*pixel_size) 
                dwRowIncr -= ((m_dwHorizScale - 1) * VRFB_IMAGE_HEIGHT_MAX * info.dwPixelSizeBytes);
                dwRowIncr += ((m_dwVertScale - 1) * info.dwPixelSizeBytes);
            }
            break;

        default:
            ASSERT(0);
            return 0;
    }
    
    return dwRowIncr;
}


//------------------------------------------------------------------------------
BOOL
OMAPVrfbSurface::SetOrientation(
    OMAP_SURF_ORIENTATION       eOrientation
    )
{
    VRFB_VIEW_INFO  info;                

    //  Do nothing if orientation is the same
    if( eOrientation == m_eOrientation )
        return TRUE;


    //  Get the current configuration
    VRFBGetViewInfo( m_hVRFB, m_hVRFBView, &info );
    
    //  Reconfigure the VRFB view for the new orientation
    //  by swapping width and height values
    VRFBUpdateView( m_hVRFB, m_hVRFBView, info.dwPixelSize, info.dwHeight, info.dwWidth, 0 );

    //  Get the new VRFB configuration
    VRFBGetViewInfo( m_hVRFB, m_hVRFBView, &info );
    
        
    //  Update surface parameters
    m_eOrientation   = eOrientation;
    m_dwWidth        = info.dwWidth;
    m_dwHeight       = info.dwHeight;

    //  Reset clipping rect
    SetClipping( NULL );

    if ((m_pAssocSurface) && (m_eSurfaceType==OMAP_SURFACE_NORMAL))
        m_pAssocSurface->SetOrientation(eOrientation);
    
    return TRUE;
}

//------------------------------------------------------------------------------
OMAPVrfbSurfaceManager::OMAPVrfbSurfaceManager()
{
    //  Initialize properties
    m_hHeap = NULL;
    m_dwDisplayBufferSize = 0;
    m_dwPhysicalDisplayAddr = 0;
    
    m_hVRFB = NULL;

    m_hOffscreenHeapPA = NULL;
    m_hOffscreenHeapVA = NULL;
    m_pOffscreenBuffer = NULL;
    m_dwOffscreenPhysical = 0;
}

//------------------------------------------------------------------------------
OMAPVrfbSurfaceManager::~OMAPVrfbSurfaceManager()
{
    Heap*   pHeap = (Heap*) m_hHeap;
    Heap*   pOffscreenHeapVA = (Heap*) m_hOffscreenHeapVA;
    Heap*   pOffscreenHeapPA = (Heap*) m_hOffscreenHeapPA;
    
    //  Free the VRFB manager
    if( m_hVRFB )
        VRFBClose( m_hVRFB );
        
    //  Free the heap manager
    if( pHeap )
        pHeap->Free();

    //  Free the offscreen virtual address heap manager
    if( pOffscreenHeapVA )
        pOffscreenHeapVA->Free();
        
    //  Free offscreen memory
    if( m_pOffscreenBuffer ) 
        VirtualFree( m_pOffscreenBuffer, 0, MEM_RELEASE );

    //  Free the offscreen physical address heap manager
    if( pOffscreenHeapPA )
        pOffscreenHeapPA->Free();
       
}

//------------------------------------------------------------------------------
BOOL
OMAPVrfbSurfaceManager::Initialize(
    DWORD   dwOffscreenMemory
    )
{
    BOOL    bResult = FALSE;
    Heap*   pHeap;
    DWORD   dwNumDisplayViews;
    
    
    //  Get video memory attributes from LCD PDD
    bResult = LcdPdd_GetMemory( &m_dwDisplayBufferSize, &m_dwPhysicalDisplayAddr );
    if( !bResult )
    {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: Unable to get video memory attributes\n"));
        goto cleanUp;
    }


    //  Check that offscreen reserve is not greater that all of display memory
    if( dwOffscreenMemory >= m_dwDisplayBufferSize )
        dwOffscreenMemory = 0;


    //  Initialize the heap manager for the display physical memory
    pHeap = new Heap(m_dwDisplayBufferSize, (DWORD) m_dwPhysicalDisplayAddr);
    if( pHeap == NULL )
    {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: Unable to create heap manager\n"));
        goto cleanUp;
    }
    
    m_hHeap = (HANDLE) pHeap;


    //  Open handle to VRFB manager
    m_hVRFB = VRFBOpen();
    if( m_hVRFB == NULL )
    {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: VRFBOpen() failed\r\n"));
        goto cleanUp;
    }

    
    //  Get number of display views
    dwNumDisplayViews = VRFBNumDisplayViews( m_hVRFB );
    if( dwNumDisplayViews < MIN_NUM_VIEWS )
    {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: Too few VRFB display views %d - should be %d\r\n", dwNumDisplayViews, MIN_NUM_VIEWS));
        goto cleanUp;
    }

    //  Success for main memory.  If the following offscreen surface allocation fails, the driver can still continue to operate
    //  without those surfaces
    bResult = TRUE;

    
    //  Allocate physical memory for offscreen surfaces
    if( dwOffscreenMemory > 0 )
    {
        Heap*   pHeapPA = (Heap *) m_hHeap;
        
        //  Carve out from the VRFB physical memory heap a block for offscreen surfaces 
        pHeap = pHeapPA->Allocate( dwOffscreenMemory );
        if( pHeap == NULL )
        {
            DEBUGMSG (ZONE_ERROR, (L"ERROR: Unable to allocate offscreen heap memory\n"));
            goto cleanUp;
        }

        //  Map physical memory to VM for offscreen buffers
        m_pOffscreenBuffer = VirtualAlloc(0, dwOffscreenMemory, MEM_RESERVE, PAGE_NOACCESS);
        if( !m_pOffscreenBuffer )
        {
            DEBUGMSG (ZONE_ERROR, (L"ERROR: Unable to allocate offscreen buffer\n"));
            delete pHeap;
            m_pOffscreenBuffer = NULL;
            goto cleanUp;
        }

        //  VirtualCopy offscreen memory region
        if( !VirtualCopy(m_pOffscreenBuffer, (void *)(pHeap->Address() >> 8), dwOffscreenMemory, PAGE_READWRITE | PAGE_NOCACHE | PAGE_PHYSICAL))
        {
            DEBUGMSG (ZONE_ERROR, (L"ERROR: Unable to map offscreen buffer physical memory\n"));
            delete pHeap;
            m_pOffscreenBuffer = NULL;
            goto cleanUp;
        }

        //  Change the attributes of the buffer for cache write combine
        if( !CeSetMemoryAttributes(m_pOffscreenBuffer, (void *)(pHeap->Address() >> 8), dwOffscreenMemory, PAGE_WRITECOMBINE))
        {
            DEBUGMSG(ZONE_ERROR, (L"ERROR: Failed CeSetMemoryAttributes for offscreen buffer\r\n"));
            VirtualFree( m_pOffscreenBuffer, 0, MEM_RELEASE );
            delete pHeap;
            m_pOffscreenBuffer = NULL;
            goto cleanUp;
        }

        m_hOffscreenHeapPA = (HANDLE) pHeap;
        m_dwOffscreenPhysical = pHeap->Address();

        //  Allocate new heap for the virtual memory for the offscreen surfaces
        pHeap = new Heap(dwOffscreenMemory, (DWORD) m_pOffscreenBuffer);
        if( pHeap == NULL )
        {
            DEBUGMSG (ZONE_ERROR, (L"ERROR: Unable to create offscreen heap manager\n"));
            VirtualFree( m_pOffscreenBuffer, 0, MEM_RELEASE );
            delete pHeap;
            m_pOffscreenBuffer = NULL;
            goto cleanUp;
        }

        m_hOffscreenHeapVA = (HANDLE) pHeap;
    }

    
cleanUp:
    //  Retrun result
    return bResult;
}

//------------------------------------------------------------------------------
DWORD
OMAPVrfbSurfaceManager::TotalMemorySize()
{
    DWORD   dwSize;
    
    //  Return total display virtual memory size
    //  This will be the number of VRFB views times the size of each view

    dwSize = VRFBNumDisplayViews(m_hVRFB) * VRFB_VIEW_SIZE; 
    
    return dwSize;
}

//------------------------------------------------------------------------------
DWORD
OMAPVrfbSurfaceManager::FreeMemorySize()
{
    Heap*   pHeap = (Heap*) m_hHeap;

    //  Return free memory of heap
    return pHeap->TotalFree();
}

//------------------------------------------------------------------------------
VOID*
OMAPVrfbSurfaceManager::VirtualBaseAddr()
{
    VOID*           pAddr = NULL;
    HANDLE          hView;
    BOOL            bResult;    
    VRFB_VIEW_INFO  info;


    //  Get the base address of the primary display view
    hView = VRFBGetDisplayView( m_hVRFB, 0 );
    if( hView == NULL )
    {
        goto cleanUp;
    }
    
    //  Get the view info
    bResult = VRFBGetViewInfo( m_hVRFB, hView, &info);
    if( bResult == FALSE )
    {
        goto cleanUp;
    }
    
    //  Get the virtual address of the primary view
    pAddr = (VOID*) info.dwVirtualAddr;
    
    
cleanUp:    
    //  Return base address of display memory
    return pAddr;
}

//------------------------------------------------------------------------------
DWORD
OMAPVrfbSurfaceManager::NumPhysicalAddr()
{
    //  Each view maps to a VRFB physical address
    return VRFBNumDisplayViews( m_hVRFB );
}

//------------------------------------------------------------------------------
DWORD
OMAPVrfbSurfaceManager::PhysicalLen(DWORD dwIndex)
{
    //  Check index
    if( dwIndex < VRFBNumDisplayViews(m_hVRFB))
    {
        //  All VRFB views are the same length in physical memory
        return VRFB_VIEW_SIZE;
    }
    
    //  Invalid index
    return 0;
}

//------------------------------------------------------------------------------
DWORD
OMAPVrfbSurfaceManager::PhysicalAddr(DWORD dwIndex)
{
    DWORD           dwAddr = 0;
    HANDLE          hView;
    BOOL            bResult;    
    VRFB_VIEW_INFO  info;


    //  Get the selected display view
    hView = VRFBGetDisplayView( m_hVRFB, dwIndex );
    if( hView == NULL )
    {
        goto cleanUp;
    }
    
    //  Get the view info
    bResult = VRFBGetViewInfo( m_hVRFB, hView, &info);
    if( bResult == FALSE )
    {
        goto cleanUp;
    }
    
    //  Get the physical address of the view
    dwAddr = info.dwPhysicalAddrInput;
    
    
cleanUp:    
    //  Return physical address of VRFB view
    return dwAddr;
}

//------------------------------------------------------------------------------
BOOL
OMAPVrfbSurfaceManager::SupportsOffscreenSurfaces()
{
    //  If offscreen heap allocated, return TRUE
    return( m_hOffscreenHeapVA != NULL );
}

//------------------------------------------------------------------------------
BOOL
OMAPVrfbSurfaceManager::Allocate(
    OMAP_DSS_PIXELFORMAT    ePixelFormat,
    DWORD                   dwWidth,
    DWORD                   dwHeight,
    OMAPSurface**           ppSurface
    )
{
    BOOL            bResult;
    OMAPVrfbSurface *pVrfbSurface;
    Heap*           pHeap = (Heap*) m_hHeap;
    
    //  Check return pointer
    if( ppSurface == NULL )
        goto cleanUp;    
    
    //  Initialize return pointer
    *ppSurface = NULL;

    //  Allocate a new flat surface object
    pVrfbSurface = new OMAPVrfbSurface;
    if( pVrfbSurface == NULL )
    {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: Unable to create OMAPVrfbSurface\n"));
        goto cleanUp;
    }

    //  Allocate the memory for the surface
    bResult = pVrfbSurface->Allocate(
                                ePixelFormat,
                                dwWidth,
                                dwHeight,
                                pHeap,
                                m_hVRFB );
    if( bResult == FALSE )
    {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: Unable to allocate OMAPVrfbSurface memory\n"));
        delete pVrfbSurface;
        goto cleanUp;
    }

    //  Return the new surface
    *ppSurface = pVrfbSurface;            
    
cleanUp:    
    //  Return result
    return (ppSurface == NULL) ? FALSE : (*ppSurface != NULL);
}

BOOL
OMAPVrfbSurfaceManager::Allocate(
    OMAP_DSS_PIXELFORMAT    ePixelFormat,
    DWORD                   dwWidth,
    DWORD                   dwHeight,
    OMAPSurface**           ppAssocSurface,
    OMAPSurface*            pSurface
    )
{
	BOOL            bResult;
    OMAPVrfbSurface *pVrfbSurface;
    Heap*           pHeap = (Heap*) m_hHeap;
    
    //  Check return pointer
    if( ppAssocSurface == NULL )
        goto cleanUp;    
    
    //  Initialize return pointer
    *ppAssocSurface = NULL;

    //  Allocate a new flat surface object
    pVrfbSurface = new OMAPVrfbSurface;
    if( pVrfbSurface == NULL )
    {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: Unable to create OMAPVrfbSurface\n"));
        goto cleanUp;
    }

    //  Allocate the memory for the surface
    bResult = pVrfbSurface->Allocate(
                                ePixelFormat,
                                dwWidth,
                                dwHeight,
                                pHeap,
                                m_hVRFB );
    if( bResult == FALSE )
    {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: Unable to allocate OMAPVrfbSurface memory\n"));
        delete pVrfbSurface;
        goto cleanUp;
    }

    pVrfbSurface->SetSurfaceType(OMAP_SURFACE_RESIZER);
    pVrfbSurface->SetAssocSurface(pSurface);
    pSurface->SetAssocSurface(pVrfbSurface);    

    //  Return the new surface
    *ppAssocSurface = pVrfbSurface;            
    
cleanUp:    
    //  Return result
    return (ppAssocSurface == NULL) ? FALSE : (*ppAssocSurface != NULL);
}

//------------------------------------------------------------------------------
BOOL
OMAPVrfbSurfaceManager::AllocateGDI(
    OMAP_DSS_PIXELFORMAT    ePixelFormat,
    DWORD                   dwWidth,
    DWORD                   dwHeight,
    OMAPSurface**           ppSurface
    )
{
    BOOL            bResult;
    OMAPFlatSurface *pFlatSurface;
    Heap*           pHeap = (Heap*) m_hOffscreenHeapVA;
    
    //  Check return pointer
    if( ppSurface == NULL )
        goto cleanUp;    
    
    //  Initialize return pointer
    *ppSurface = NULL;

    //  Check for offscreen heap
    if( pHeap == NULL )
        goto cleanUp;
        
        
    //  Allocate a new flat surface object
    pFlatSurface = new OMAPFlatSurface;
    if( pFlatSurface == NULL )
    {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: Unable to create offscreen OMAPFlatSurface\n"));
        goto cleanUp;
    }

    //  Allocate the memory for the offscreen surface
    bResult = pFlatSurface->Allocate(
                                ePixelFormat,
                                dwWidth,
                                dwHeight,
                                pHeap,
                                m_dwOffscreenPhysical );
    if( bResult == FALSE )
    {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: Unable to allocate offscreen OMAPFlatSurface memory\n"));
        delete pFlatSurface;
        goto cleanUp;
    }

    
    //  Return the new surface
    *ppSurface = pFlatSurface;            
    
cleanUp:    
    //  Return result
    return (ppSurface == NULL) ? FALSE : (*ppSurface != NULL);
}
