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

// Uncomment following lines to get debugmsgs for this file alone in RETAIL build.
//#undef DEBUGMSG
//#define DEBUGMSG(x,y) RETAILMSG(TRUE,y)

//
//  Defines
//

#define CACHE_WRITETHRU         0x08
#define MIN_STRIDE_RGB          32
#define MIN_STRIDE_YUV          32

//
//  Structs
//



//
//  Globals
//



//------------------------------------------------------------------------------
OMAPFlatSurface::OMAPFlatSurface()
{
    m_hHeap = NULL;    
    m_dwHorizScale = 1;
    m_dwVertScale = 1;
    m_dwWidthFactor = 1;
    m_bUseResizer = FALSE;
    m_pAssocSurface = NULL;
    m_eSurfaceType = OMAP_SURFACE_NORMAL;
}

//------------------------------------------------------------------------------
OMAPFlatSurface::~OMAPFlatSurface()
{
    Heap    *pHeap = (Heap*) m_hHeap;
    
    //  Free the underlying surface memory
    if( pHeap )
        pHeap->Free();

    DEBUGMSG(ZONE_WARNING, (L"INFO: OMAPFlatSurface::~OMAPFlatSurface - FreeSurface\r\n"));
}

//------------------------------------------------------------------------------
BOOL    
OMAPFlatSurface::Allocate(
    OMAP_DSS_PIXELFORMAT    ePixelFormat,
    DWORD                   dwWidth,
    DWORD                   dwHeight,
    HANDLE                  hHeap,
    DWORD                   dwBasePhysicalAddr
    )
{
    Heap    *pMainHeap = (Heap*) hHeap,
            *pSurfHeap = NULL;
    DWORD   dwMinStride;
    DWORD    allocateSize=0;
    
    //  Determine min stride for surface based on pixel type
    if( (ePixelFormat == OMAP_DSS_PIXELFORMAT_YUV2) || (ePixelFormat == OMAP_DSS_PIXELFORMAT_UYVY) )
    {
        dwMinStride = MIN_STRIDE_YUV;
        m_dwWidthFactor = 2;
    }
    else
    {
        dwMinStride = MIN_STRIDE_RGB;
        m_dwWidthFactor = 1;
    }
                
    //  Initialize the surface properties for this surface type
    m_ePixelFormat   = ePixelFormat;
    m_dwPixelSize    = OMAPDisplayController::PixelFormatToPixelSize(m_ePixelFormat);
    m_eOrientation   = OMAP_SURF_ORIENTATION_STANDARD;
    m_dwWidth        = dwWidth;
    m_dwHeight       = dwHeight;
    m_dwActualWidth  = ((dwWidth  + dwMinStride -1)/dwMinStride) * dwMinStride;
    m_dwActualHeight = ((dwHeight + dwMinStride -1)/dwMinStride) * dwMinStride;
    
    //  Set clipping region to be entire surface
    SetClipping( NULL );
    
    // Aligning the size to PAGE_SIZE since some applications (ex: graphics) require a page aligned buffer    
    allocateSize = m_dwPixelSize * m_dwActualWidth * m_dwActualHeight;
    allocateSize = (allocateSize + PAGE_SIZE -1) & ~(PAGE_SIZE -1);
    //  Allocate the surface memory from the given video memory heap
    pSurfHeap = pMainHeap->Allocate( allocateSize);    
    if( pSurfHeap == NULL )
    {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: Unable to allocate heap memory\n"));
        goto cleanUp;
    }

    //  Clear out the surface memory
    memset( (VOID*) pSurfHeap->Address(), 0, pSurfHeap->NodeSize() );

    //  Set surface specific properties
    m_hHeap = (HANDLE) pSurfHeap;
    m_dwPhysicalAddr = dwBasePhysicalAddr +(DWORD)(pSurfHeap->Address() - pMainHeap->Address());

    DEBUGMSG(ZONE_WARNING, (L"INFO: OMAPFlatSurface::Allocate() - SurfacePhyaddr 0x%x\r\n", m_dwPhysicalAddr));

    
    // Initialize variables
    m_hRSZHandle = NULL;
    memset(&m_sRSZParams,0,sizeof(RSZParams_t));      
    
cleanUp:        
    //  Return
    return (pSurfHeap != NULL);
}

//------------------------------------------------------------------------------
VOID*   
OMAPFlatSurface::VirtualAddr()
{
    Heap*   pHeap;
    if ((m_bUseResizer) && (m_eSurfaceType == OMAP_SURFACE_NORMAL) && (m_pAssocSurface))
        return m_pAssocSurface->VirtualAddr();
    else
    {
        pHeap = (Heap*) m_hHeap;    
        //  For flat surfaces, always same virtual memory value        
        return (VOID*) pHeap->Address();
    }
}


//------------------------------------------------------------------------------
DWORD   
OMAPFlatSurface::Width(
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
OMAPFlatSurface::Height(
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
OMAPFlatSurface::Stride(
    OMAP_DSS_ROTATION   eRotation
    )
{
    DWORD   dwStride;
    
    //  Return the surface stride depending on the rotation angle
    switch( eRotation )
    {
        case OMAP_DSS_ROTATION_0:
        case OMAP_DSS_ROTATION_180:
            //  Normal
            dwStride = m_dwPixelSize * m_dwActualWidth * m_dwHorizScale;
            break;
            
        case OMAP_DSS_ROTATION_90:
        case OMAP_DSS_ROTATION_270:
            //  Rotated
            dwStride = m_dwPixelSize * m_dwActualHeight * m_dwVertScale;
            break;

        default:
            ASSERT(0);
            return 0;
    }

    //  Return the surface stride
    return dwStride;
}

//------------------------------------------------------------------------------
DWORD   
OMAPFlatSurface::PhysicalAddr(
    OMAP_DSS_ROTATION       eRotation,
    BOOL                    bMirror,
    OMAP_ASSOC_SURF_USAGE   eUseAssocSurface
    )
{
    DWORD   dwAddr;
    DWORD   dwClipOffsetX,
            dwClipOffsetY;

    if ((((eUseAssocSurface==OMAP_ASSOC_SURF_DEFAULT) && (m_bUseResizer)) ||
          ( eUseAssocSurface==OMAP_ASSOC_SURF_FORCE_ON )                     ) &&
        (m_pAssocSurface))
        
        return m_pAssocSurface->PhysicalAddr(eRotation,bMirror,OMAP_ASSOC_SURF_FORCE_OFF);                 
    
    
    if( bMirror )
    {
        //  Compute the base address for DMA based rotation (mirrored)    
        switch( eRotation )
        {
            case OMAP_DSS_ROTATION_0:
                //  Compute clip offset
                dwClipOffsetX = m_dwWidth - m_rcClip.right;
                dwClipOffsetY = m_rcClip.top * m_dwActualWidth;

                //  Offset to the upper right corner
                dwAddr = m_dwPhysicalAddr + (m_dwWidth - dwClipOffsetX - 1)*m_dwPixelSize + dwClipOffsetY*m_dwPixelSize;
                break;
                
            case OMAP_DSS_ROTATION_90:
                //  Compute clip offset
                dwClipOffsetX = m_rcClip.left;
                dwClipOffsetY = m_rcClip.top * m_dwActualWidth;

                //  Same as set physical address
                dwAddr = m_dwPhysicalAddr + dwClipOffsetX*m_dwPixelSize + dwClipOffsetY*m_dwPixelSize;
                break;

            case OMAP_DSS_ROTATION_180:
                //  Compute clip offset
                dwClipOffsetX = m_rcClip.left;
                dwClipOffsetY = m_dwHeight - m_rcClip.bottom;

                //  Offset to the lower left corner
                dwAddr = m_dwPhysicalAddr + (m_dwActualWidth * (m_dwHeight - dwClipOffsetY - 1))*m_dwPixelSize + dwClipOffsetX*m_dwPixelSize;
                break;

            case OMAP_DSS_ROTATION_270:
                //  Compute clip offset
                dwClipOffsetX = m_dwWidth - m_rcClip.right;
                dwClipOffsetY = (m_dwHeight - m_rcClip.bottom)*m_dwActualWidth;

                //  Offset to the lower right corner
                dwAddr = m_dwPhysicalAddr + (m_dwActualWidth * m_dwHeight - 1)*m_dwPixelSize - (m_dwActualWidth - m_dwWidth)*m_dwPixelSize;
                dwAddr = dwAddr - dwClipOffsetX*m_dwPixelSize - dwClipOffsetY*m_dwPixelSize;
                break;
    
            default:
                ASSERT(0);
                return 0;
        }
    }
    else
    {
        //  Compute the base address for DMA based rotation (non-mirrored)    
        switch( eRotation )
        {
            case OMAP_DSS_ROTATION_0:
                //  Compute clip offset
                dwClipOffsetX = m_rcClip.left;
                dwClipOffsetY = m_rcClip.top * m_dwActualWidth;

                //  Same as set physical address
                dwAddr = m_dwPhysicalAddr + dwClipOffsetX*m_dwPixelSize + dwClipOffsetY*m_dwPixelSize;
                break;
                
            case OMAP_DSS_ROTATION_90:
                //  Compute clip offset
                dwClipOffsetX = m_dwWidth - m_rcClip.right;
                dwClipOffsetY = m_rcClip.top * m_dwActualWidth;

                //  Offset to the upper right corner
                dwAddr = m_dwPhysicalAddr + (m_dwWidth - dwClipOffsetX - 1)*m_dwPixelSize + dwClipOffsetY*m_dwPixelSize;
                break;

            case OMAP_DSS_ROTATION_180:
                //  Compute clip offset
                dwClipOffsetX = m_dwWidth - m_rcClip.right;
                dwClipOffsetY = (m_dwHeight - m_rcClip.bottom)*m_dwActualWidth;

                //  Offset to the lower right corner
                dwAddr = m_dwPhysicalAddr + (m_dwActualWidth * m_dwHeight - 1)*m_dwPixelSize - (m_dwActualWidth - m_dwWidth)*m_dwPixelSize;
                dwAddr = dwAddr - dwClipOffsetX*m_dwPixelSize - dwClipOffsetY*m_dwPixelSize;
                break;

            case OMAP_DSS_ROTATION_270:
                //  Compute clip offset
                dwClipOffsetX = m_rcClip.left;
                dwClipOffsetY = m_dwHeight - m_rcClip.bottom;

                //  Offset to the lower left corner
                dwAddr = m_dwPhysicalAddr + (m_dwActualWidth * (m_dwHeight - dwClipOffsetY - 1))*m_dwPixelSize + dwClipOffsetX*m_dwPixelSize;
                break;

            default:
                ASSERT(0);
                return 0;
        }
    }
        
    //  Return address
    return dwAddr;
}

//------------------------------------------------------------------------------
DWORD   
OMAPFlatSurface::PixelIncr(
    OMAP_DSS_ROTATION   eRotation,
    BOOL                bMirror
    )
{
    DWORD   dwIncr;
   
    if( bMirror )
    {
        //  Compute the pixel increment for DMA based rotation (mirrored)
        switch( eRotation )
        {
            case OMAP_DSS_ROTATION_0:
                //  Backward 1 pixel (- horizontal_scale_factor*pixel_size)
                dwIncr = 1 - 2 * m_dwPixelSize;
                dwIncr -= ((m_dwHorizScale - 1) * m_dwPixelSize * m_dwWidthFactor);
                break;
                
            case OMAP_DSS_ROTATION_90:
                //  Forward 1 row - 1 pixel (+ vertical_scale_factor*stride)
                dwIncr = 1 + m_dwActualWidth * m_dwPixelSize - m_dwPixelSize;
                dwIncr += ((m_dwVertScale - 1) * m_dwActualWidth * m_dwPixelSize);
                break;

            case OMAP_DSS_ROTATION_180:
                //  Forward 1 pixel (+ horizontal_scale_factor*pixel_size)
                dwIncr = 1;
                dwIncr += ((m_dwHorizScale - 1) * m_dwPixelSize * m_dwWidthFactor);
                break;

            case OMAP_DSS_ROTATION_270:
                //  Backward 1 row and 1 pixel (- vertical_scale_factor*stride)
                dwIncr = 1 - m_dwActualWidth * m_dwPixelSize - m_dwPixelSize;
                dwIncr -= ((m_dwVertScale - 1) * m_dwActualWidth * m_dwPixelSize);
                break;

            default:
                ASSERT(0);
                return 0;
        }
    }
    else
    {
        //  Compute the pixel increment for DMA based rotation (non-mirrored)
        switch( eRotation )
        {
            case OMAP_DSS_ROTATION_0:
                //  Forward 1 pixel (+ horizontal_scale_factor*pixel_size)
                dwIncr = 1;
                dwIncr += ((m_dwHorizScale - 1) * m_dwPixelSize * m_dwWidthFactor);
                break;
                
            case OMAP_DSS_ROTATION_90:
                //  Forward 1 row - 1 pixel (+ vertical_scale_factor*stride)
                dwIncr = 1 + m_dwActualWidth * m_dwPixelSize - m_dwPixelSize;
                dwIncr += ((m_dwVertScale - 1) * m_dwActualWidth * m_dwPixelSize);
                break;

            case OMAP_DSS_ROTATION_180:
                //  Backward 1 pixel (- horizontal_scale_factor*pixel_size)
                dwIncr = 1 - 2 * m_dwPixelSize;
                dwIncr -= ((m_dwHorizScale - 1) * m_dwPixelSize * m_dwWidthFactor);
                break;

            case OMAP_DSS_ROTATION_270:
                //  Backward 1 row and 1 pixel (- vertical_scale_factor*stride)
                dwIncr = 1 - m_dwActualWidth * m_dwPixelSize - m_dwPixelSize;
                dwIncr -= ((m_dwVertScale - 1) * m_dwActualWidth * m_dwPixelSize);
                break;

            default:
                ASSERT(0);
                return 0;
        }
    }
        
    //  Return increment
    return dwIncr;
}
                            
//------------------------------------------------------------------------------
DWORD   
OMAPFlatSurface::RowIncr(
    OMAP_DSS_ROTATION   eRotation,
    BOOL                bMirror
    )
{
    DWORD   dwIncr;
    DWORD   dwClipOffsetX,
            dwClipOffsetY;

    if ((m_bUseResizer) && (m_eSurfaceType == OMAP_SURFACE_NORMAL) && (m_pAssocSurface))
        return m_pAssocSurface->RowIncr(eRotation,bMirror);
    
    if( bMirror )
    {
        //  Compute the row increment for DMA based rotation (mirrored)
        switch( eRotation )
        {
            case OMAP_DSS_ROTATION_0:
                //  Compute clip offset
                dwClipOffsetX = m_dwWidth - (m_rcClip.right - m_rcClip.left);

                //  Forward 2 rows - 1 pixel
                dwIncr = 1 + (m_dwActualWidth * m_dwPixelSize) + ((m_dwWidth - dwClipOffsetX) * m_dwPixelSize) - (2 * m_dwPixelSize);

                //  (+ vertical_scale_factor*stride - horizontal_scale_factor*pixel_size) 
                dwIncr += ((m_dwVertScale - 1) * m_dwActualWidth * m_dwPixelSize);
                dwIncr -= ((m_dwHorizScale - 1) * m_dwPixelSize * m_dwWidthFactor);
                break;
                
            case OMAP_DSS_ROTATION_90:
                //  Compute clip offset
                dwClipOffsetY = m_dwHeight - (m_rcClip.bottom - m_rcClip.top);

                //  Backward 1 frame + 1 row
                dwIncr = 1 - (m_dwActualWidth * m_dwHeight * m_dwPixelSize) + (m_dwActualWidth * m_dwPixelSize);
                dwIncr = dwIncr + (dwClipOffsetY * m_dwActualWidth * m_dwPixelSize);

                //  (+ vertical_scale_factor*stride + horizontal_scale_factor*pixel_size)
                dwIncr += ((m_dwVertScale - 1) * m_dwActualWidth * m_dwPixelSize);
                dwIncr += ((m_dwHorizScale - 1) * m_dwPixelSize * m_dwWidthFactor);
                break;

            case OMAP_DSS_ROTATION_180:
                //  Compute clip offset
                dwClipOffsetX = m_dwWidth - (m_rcClip.right - m_rcClip.left);

                //  Backward 2 rows
                dwIncr = 1 - (m_dwActualWidth * m_dwPixelSize) - ((m_dwWidth - dwClipOffsetX) * m_dwPixelSize);

                //  (- vertical_scale_factor*stride + horizontal_scale_factor*pixel_size)
                dwIncr -= ((m_dwVertScale - 1) * m_dwActualWidth * m_dwPixelSize);
                dwIncr += ((m_dwHorizScale - 1) * m_dwPixelSize * m_dwWidthFactor);
                break;

            case OMAP_DSS_ROTATION_270:
                //  Compute clip offset
                dwClipOffsetY = m_dwHeight - (m_rcClip.bottom - m_rcClip.top);

                //  Forward 1 frame - 1 row - 1 pixel
                dwIncr = 1 + (m_dwActualWidth * m_dwHeight * m_dwPixelSize) - (m_dwActualWidth * m_dwPixelSize) - (2 * m_dwPixelSize);
                dwIncr = dwIncr - (dwClipOffsetY * m_dwActualWidth * m_dwPixelSize);

                //  (- vertical_scale_factor*stride - horizontal_scale_factor*pixel_size)
                dwIncr -= ((m_dwVertScale - 1) * m_dwActualWidth * m_dwPixelSize);
                dwIncr -= ((m_dwHorizScale - 1) * m_dwPixelSize * m_dwWidthFactor);                
                break;

            default:
                ASSERT(0);
                return 0;
        }
    }
    else
    {
        //  Compute the row increment for DMA based rotation (non-mirrored)
        switch( eRotation )
        {
            case OMAP_DSS_ROTATION_0:
                //  Compute clip offset
                dwClipOffsetX = m_dwWidth - (m_rcClip.right - m_rcClip.left);

                //  Forward 1 pixel on next row
                dwIncr = 1 + (m_dwActualWidth - m_dwWidth + dwClipOffsetX) * m_dwPixelSize;
                
                //  (+ vertical_scale_factor*stride + horizontal_scale_factor*pixel_size) 
                dwIncr += ((m_dwVertScale - 1) * m_dwActualWidth * m_dwPixelSize);
                dwIncr += ((m_dwHorizScale - 1) * m_dwPixelSize * m_dwWidthFactor);
                break;
                
            case OMAP_DSS_ROTATION_90:
                //  Compute clip offset
                dwClipOffsetY = m_dwHeight - (m_rcClip.bottom - m_rcClip.top);

                //  Backward 1 frame + 1 row - 1 pixel
                dwIncr = 1 - (m_dwActualWidth * m_dwHeight * m_dwPixelSize) + (m_dwActualWidth * m_dwPixelSize) - (2 * m_dwPixelSize);
                dwIncr = dwIncr + (dwClipOffsetY * m_dwActualWidth * m_dwPixelSize);
                
                //  (+ vertical_scale_factor*stride - horizontal_scale_factor*pixel_size)
                dwIncr += ((m_dwVertScale - 1) * m_dwActualWidth * m_dwPixelSize);
                dwIncr -= ((m_dwHorizScale - 1) * m_dwPixelSize * m_dwWidthFactor);
                break;

            case OMAP_DSS_ROTATION_180:
                //  Compute clip offset
                dwClipOffsetX = m_dwWidth - (m_rcClip.right - m_rcClip.left);

                //  Backward 1 pixel on previous row
                dwIncr = 1 - 2 * m_dwPixelSize - (m_dwActualWidth - m_dwWidth + dwClipOffsetX) * m_dwPixelSize;

                //  (- vertical_scale_factor*stride - horizontal_scale_factor*pixel_size)
                dwIncr -= ((m_dwVertScale - 1) * m_dwActualWidth * m_dwPixelSize);
                dwIncr -= ((m_dwHorizScale - 1) * m_dwPixelSize * m_dwWidthFactor);
                break;

            case OMAP_DSS_ROTATION_270:
                //  Compute clip offset
                dwClipOffsetY = m_dwHeight - (m_rcClip.bottom - m_rcClip.top);

                //  Forward 1 frame - 1 row + 1 pixel
                dwIncr = 1 + (m_dwActualWidth * m_dwHeight * m_dwPixelSize) - (m_dwActualWidth * m_dwPixelSize);
                dwIncr = dwIncr - (dwClipOffsetY * m_dwActualWidth * m_dwPixelSize);

                //  (- vertical_scale_factor*stride + horizontal_scale_factor*pixel_size)
                dwIncr -= ((m_dwVertScale - 1) * m_dwActualWidth * m_dwPixelSize);
                dwIncr += ((m_dwHorizScale - 1) * m_dwPixelSize * m_dwWidthFactor);                
                break;

            default:
                ASSERT(0);
                return 0;
        }
    }
        
    //  Return increment
    return dwIncr;
}


//------------------------------------------------------------------------------
BOOL
OMAPFlatSurface::SetOrientation(
    OMAP_SURF_ORIENTATION       eOrientation
    )
{
    DWORD   oldWidth = m_dwWidth,
            oldHeight = m_dwHeight,
            oldActualWidth = m_dwActualWidth,
            oldActualHeight = m_dwActualHeight;
    
    //  Do nothing if orientation is the same
    if( eOrientation == m_eOrientation )
        return TRUE;

    //  Swap width and height parameters
    m_eOrientation   = eOrientation;
    m_dwWidth        = oldHeight;
    m_dwHeight       = oldWidth;
    m_dwActualWidth  = oldActualHeight;
    m_dwActualHeight = oldActualWidth;
    
    //  Reset clipping rect
    SetClipping( NULL );

    if ((m_pAssocSurface) && (m_eSurfaceType==OMAP_SURFACE_NORMAL))
        m_pAssocSurface->SetOrientation(eOrientation);
    
    return TRUE;
}


//------------------------------------------------------------------------------
OMAPFlatSurfaceManager::OMAPFlatSurfaceManager()
{
    //  Initialize properties
    m_pVirtualDisplayBuffer = NULL;
    m_hHeap = NULL;
    m_dwDisplayBufferSize = 0;
    m_dwPhysicalDisplayAddr = 0;

    m_hOffscreenHeap = NULL;
    m_pOffscreenBuffer = NULL;
    m_dwOffscreenPhysical = 0;
}

//------------------------------------------------------------------------------
OMAPFlatSurfaceManager::~OMAPFlatSurfaceManager()
{
    Heap*   pHeap = (Heap*) m_hHeap;
    Heap*   pOffscreenHeap = (Heap*) m_hOffscreenHeap;
    
    //  Free the heap manager
    if( pHeap )
        pHeap->Free();
        
    //  Free memory
    if( m_pVirtualDisplayBuffer ) 
        VirtualFree( m_pVirtualDisplayBuffer, 0, MEM_RELEASE );
        
    //  Free the offscreen heap manager
    if( pOffscreenHeap )
        pOffscreenHeap->Free();
        
    //  Free offscreen memory
    if( m_pOffscreenBuffer ) 
        VirtualFree( m_pOffscreenBuffer, 0, MEM_RELEASE );

    DEBUGMSG(ZONE_WARNING, (L"INFO: OMAPFlatSurfaceManager::~OMAPFlatSurfaceManager\r\n"));
}

//------------------------------------------------------------------------------
BOOL
OMAPFlatSurfaceManager::Initialize(
    DWORD   dwOffscreenMemory
    )
{
    BOOL    bResult;
    Heap*   pHeap;
    
    
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
        
        
    //  Map physical memory to VM
    m_pVirtualDisplayBuffer = VirtualAlloc(0, m_dwDisplayBufferSize, MEM_RESERVE, PAGE_NOACCESS);
    if( !m_pVirtualDisplayBuffer )
    {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: Unable to allocate display buffer\n"));
        goto cleanUp;
    }

    if( !VirtualCopy(m_pVirtualDisplayBuffer, (void *)(m_dwPhysicalDisplayAddr >> 8), m_dwDisplayBufferSize-dwOffscreenMemory, PAGE_READWRITE | PAGE_NOCACHE | PAGE_PHYSICAL))
    {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: Unable to map display buffer physical memory\n"));
        VirtualFree( m_pVirtualDisplayBuffer, 0, MEM_RELEASE );
        m_pVirtualDisplayBuffer = NULL;
        goto cleanUp;
    }

    //  Change the attributes of the buffer for cache write combine
    if( !CeSetMemoryAttributes(m_pVirtualDisplayBuffer, (void *)(m_dwPhysicalDisplayAddr >> 8), m_dwDisplayBufferSize-dwOffscreenMemory, PAGE_WRITECOMBINE))
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: Failed CeSetMemoryAttributes for display buffer\r\n"));
        VirtualFree( m_pVirtualDisplayBuffer, 0, MEM_RELEASE );
        m_pVirtualDisplayBuffer = NULL;
        goto cleanUp;
    }
    
    //  Initialize the heap manager for the display memory
    pHeap = new Heap(m_dwDisplayBufferSize-dwOffscreenMemory, (DWORD) m_pVirtualDisplayBuffer);
    if( pHeap == NULL )
    {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: Unable to create heap manager\n"));
        VirtualFree( m_pVirtualDisplayBuffer, 0, MEM_RELEASE );
        m_pVirtualDisplayBuffer = NULL;
        goto cleanUp;
    }
    
    m_hHeap = (HANDLE) pHeap;


    //  Allocate physical memory for offscreen surfaces
    if( dwOffscreenMemory > 0 )
    {
        BYTE*   pOffscreenPtr = (BYTE*) m_pVirtualDisplayBuffer;

        
        //  Offset to where offscreen buffer is
        pOffscreenPtr += m_dwDisplayBufferSize - dwOffscreenMemory;
        m_pOffscreenBuffer = (VOID*) pOffscreenPtr;
        m_dwOffscreenPhysical = m_dwPhysicalDisplayAddr + m_dwDisplayBufferSize - dwOffscreenMemory;
        
        //  VirtualCopy offscreen memory region to follow m_pVirtualDisplayBuffer
        if( !VirtualCopy(m_pOffscreenBuffer, (void *)(m_dwOffscreenPhysical >> 8), dwOffscreenMemory, PAGE_READWRITE | PAGE_NOCACHE | PAGE_PHYSICAL))
        {
            DEBUGMSG (ZONE_ERROR, (L"ERROR: Unable to map offscreen buffer physical memory\n"));
            m_pOffscreenBuffer = NULL;
            goto cleanUp;
        }

        //  Change the attributes of the buffer for cache write combine
        if( !CeSetMemoryAttributes(m_pOffscreenBuffer, (void *)(m_dwOffscreenPhysical >> 8), dwOffscreenMemory, PAGE_WRITECOMBINE))
        {
            DEBUGMSG(ZONE_ERROR, (L"ERROR: Failed CeSetMemoryAttributes for offscreen buffer\r\n"));
            VirtualFree( m_pOffscreenBuffer, 0, MEM_RELEASE );
            m_pOffscreenBuffer = NULL;
            goto cleanUp;
        }

        //  Initialize the heap manager for the offscreen memory
        pHeap = new Heap(dwOffscreenMemory, (DWORD) m_pOffscreenBuffer);
        if( pHeap == NULL )
        {
            DEBUGMSG (ZONE_ERROR, (L"ERROR: Unable to create offscreen heap manager\n"));
            VirtualFree( m_pOffscreenBuffer, 0, MEM_RELEASE );
            m_pOffscreenBuffer = NULL;
            goto cleanUp;
        }
        
        m_hOffscreenHeap = (HANDLE) pHeap;
    }

cleanUp:
    //  Retrun result
    return bResult;
}

//------------------------------------------------------------------------------
DWORD
OMAPFlatSurfaceManager::TotalMemorySize()
{
    //  Return total display memory size
    return m_dwDisplayBufferSize;
}

//------------------------------------------------------------------------------
DWORD
OMAPFlatSurfaceManager::FreeMemorySize()
{
    Heap*   pHeap = (Heap*) m_hHeap;

    //  Return free memory of heap
    return pHeap->TotalFree();
}

//------------------------------------------------------------------------------
VOID*
OMAPFlatSurfaceManager::VirtualBaseAddr()
{
    //  Return base address of display memory
    return m_pVirtualDisplayBuffer;
}

//------------------------------------------------------------------------------
DWORD
OMAPFlatSurfaceManager::NumPhysicalAddr()
{
    //  Flat memory manager has only a single physical memory segment
    return 1;
}

//------------------------------------------------------------------------------
DWORD
OMAPFlatSurfaceManager::PhysicalLen(DWORD dwIndex)
{
    DWORD   dwLen = 0;        

    //  Flat memory manager has only a single physical memory segment
    if( dwIndex == 0 )
        dwLen = m_dwDisplayBufferSize;
    
    //  Return length of segment
    return dwLen;
}

//------------------------------------------------------------------------------
DWORD
OMAPFlatSurfaceManager::PhysicalAddr(DWORD dwIndex)
{
    DWORD   dwAddr = 0;        

    //  Flat memory manager has only a single physical memory segment
    if( dwIndex == 0 )
        dwAddr = m_dwPhysicalDisplayAddr;
    
    //  Return physical address of segment
    return dwAddr;
}

//------------------------------------------------------------------------------
BOOL
OMAPFlatSurfaceManager::SupportsOffscreenSurfaces()
{
    //  If offscreen heap allocated, return TRUE
    return( m_hOffscreenHeap != NULL );
}

//------------------------------------------------------------------------------
BOOL
OMAPFlatSurfaceManager::Allocate(
    OMAP_DSS_PIXELFORMAT    ePixelFormat,
    DWORD                   dwWidth,
    DWORD                   dwHeight,
    OMAPSurface**           ppSurface
    )
{
    BOOL            bResult;
    OMAPFlatSurface *pFlatSurface;
    Heap*           pHeap = (Heap*) m_hHeap;
    
    //  Check return pointer
    if( ppSurface == NULL )
        goto cleanUp;    
    
    //  Initialize return pointer
    *ppSurface = NULL;

    //  Allocate a new flat surface object
    pFlatSurface = new OMAPFlatSurface;
    if( pFlatSurface == NULL )
    {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: Unable to create OMAPFlatSurface\n"));
        goto cleanUp;
    }

    //  Allocate the memory for the surface
    bResult = pFlatSurface->Allocate(
                                ePixelFormat,
                                dwWidth,
                                dwHeight,
                                pHeap,
                                m_dwPhysicalDisplayAddr );
    if( bResult == FALSE )
    {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: Unable to allocate OMAPFlatSurface memory\n"));
        delete pFlatSurface;
        goto cleanUp;
    }    

    DEBUGMSG(ZONE_WARNING, (L"INFO: OMAPFlatSurfaceManager::Allocate() - Allocating surface 0x%x\r\n", pFlatSurface));

    //  Return the new surface
    *ppSurface = pFlatSurface;            
    
cleanUp:    
    //  Return result
    return (ppSurface == NULL) ? FALSE : (*ppSurface != NULL);
}

//------------------------------------------------------------------------------
BOOL
OMAPFlatSurfaceManager::Allocate(
    OMAP_DSS_PIXELFORMAT    ePixelFormat,
    DWORD                   dwWidth,
    DWORD                   dwHeight,
    OMAPSurface**           ppAssocSurface,
    OMAPSurface*            pSurface
    )
{
    BOOL            bResult;
    OMAPFlatSurface *pFlatSurface;
    Heap*           pHeap = (Heap*) m_hHeap;


    //  Check return pointer
    if( ppAssocSurface == NULL )
        goto cleanUp;    
    
    //  Initialize return pointer
    *ppAssocSurface = NULL;

    //  Allocate a new flat surface object
    pFlatSurface = new OMAPFlatSurface;
    if( pFlatSurface == NULL )
    {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: Unable to create OMAPFlatSurface\n"));
        goto cleanUp;
    }

    //  Allocate the memory for the surface
    bResult = pFlatSurface->Allocate(
                                ePixelFormat,
                                dwWidth,
                                dwHeight,
                                pHeap,
                                m_dwPhysicalDisplayAddr );
    if( bResult == FALSE )
    {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: Unable to allocate OMAPFlatSurface memory\n"));
        delete pFlatSurface;
        goto cleanUp;
    }

    pFlatSurface->SetSurfaceType(OMAP_SURFACE_RESIZER);
    pFlatSurface->SetAssocSurface(pSurface);
    pSurface->SetAssocSurface(pFlatSurface);    
    
    DEBUGMSG(ZONE_WARNING, (L"INFO: OMAPFlatSurfaceManager::Allocate() - Allocating surface # 0x%x\r\n", pFlatSurface));

    //  Return the new surface
    *ppAssocSurface = pFlatSurface; 
    
cleanUp:        
    //  Return result
    return (ppAssocSurface == NULL) ? FALSE : (*ppAssocSurface != NULL);
}

//------------------------------------------------------------------------------
BOOL
OMAPFlatSurfaceManager::AllocateGDI(
    OMAP_DSS_PIXELFORMAT    ePixelFormat,
    DWORD                   dwWidth,
    DWORD                   dwHeight,
    OMAPSurface**           ppSurface
    )
{
//    return Allocate(ePixelFormat, dwWidth, dwHeight, ppSurface);

    BOOL            bResult;
    OMAPFlatSurface *pFlatSurface;
    Heap*           pHeap = (Heap*) m_hOffscreenHeap;
    
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

    DEBUGMSG(ZONE_WARNING, (L"INFO: OMAPFlatSurfaceManager::AllocateGDI() - Allocating surface # 0x%x\r\n", pFlatSurface));

    //  Return the new surface
    *ppSurface = pFlatSurface;            
    
cleanUp:    
    //  Return result
    return (ppSurface == NULL) ? FALSE : (*ppSurface != NULL);
}
