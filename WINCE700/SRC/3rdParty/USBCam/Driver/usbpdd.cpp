//-------------------------------------------------------------------------
// <copyright file="usbpdd.cpp" company="Microsoft">
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//    The use and distribution terms for this software are covered by the
//    Microsoft Limited Permissive License (Ms-LPL) 
//    which can be found in the file MS-LPL.txt at the root of this distribution.
//    By using this software in any fashion, you are agreeing to be bound by
//    the terms of this license.
//
//    THE SOFTWARE IS LICENSED "AS-IS" WITH NO WARRANTIES OR INDEMNITIES. 
//
//    You must not remove this notice, or any other, from this software.
// </copyright>
// 
// <summary>
//    USB camera driver for Windows Embedded CE 6.0
// </summary>
//-------------------------------------------------------------------------
//======================================================================
// USB camera driver for Windows Embedded CE 6.0
//======================================================================


#include <windows.h>
#include <pkfuncs.h>
#include <pm.h>

#include "Cs.h"
#include "Csmedia.h"

#include "CameraPDDProps.h"
#include "dstruct.h"
#include "dbgsettings.h"
#include <camera.h>
#include "CameraDriver.h"
#include "SensorFormats.h"
#include "PinDriver.h"

#include "CameraPDD.h"
#include <USBdi.h>                  // USB includes
#include <usb100.h>                 // USB includes
#include <usbclient.h>              // USB client driver helper code
#include <usbfnioctl.h>

#include "USBCode.h"
#include "USBPdd.h"
#include "jpegheader.h"


// Work around DHT / HDT typo mmreg.h, version 6.0 and earlier
#define MJPGHDTSEG_STORAGE 
#define MJPGDHTSEG_STORAGE
#include "mmreg.h"

PDDFUNCTBL FuncTbl = {
    sizeof(PDDFUNCTBL),
    PDD_Init,
    PDD_DeInit,
    PDD_GetAdapterInfo,
    PDD_HandleVidProcAmpChanges,
    PDD_HandleCamControlChanges,
    PDD_HandleVideoControlCapsChanges,
    PDD_SetPowerState,
    PDD_HandleAdapterCustomProperties,
    PDD_InitSensorMode,
    PDD_DeInitSensorMode,
    PDD_SetSensorState,
    PDD_TakeStillPicture,
    PDD_GetSensorModeInfo,
    PDD_SetSensorModeFormat,
    PDD_AllocateBuffer,
    PDD_DeAllocateBuffer,
    PDD_RegisterClientBuffer,
    PDD_UnRegisterClientBuffer,
    PDD_FillBuffer,
    PDD_HandleModeCustomProperties
};

// ----------------------------------------------------------------------------
// Default Video Control Caps 
// ----------------------------------------------------------------------------
ULONG DefaultVideoControlCaps[] = {
    0x0                               /*CAPTURE*/,
    CS_VideoControlFlag_ExternalTriggerEnable | CS_VideoControlFlag_Trigger /*STILL*/,
    0x0                              /*PREVIEW*/
    };


//PDD constructor
CUSBPdd::CUSBPdd()
{
    DEBUGMSG (ZONE_FUNCTION, (DTAG TEXT("CUSBPdd\r\n")));

    // By default, support both MJPEG and uncompressed images
    // Can override this in the registry
    m_bMJPEGSupported        = 1;
    m_bUncompressedSupported = 1;

    // The driver supports STILL and CAPTURE pins, but not PREVIEW
    m_ulCTypes = 2;

    // Initializing the pdd context structure
    m_PddContext.dwSize = sizeof(PDDCONTEXT);
    m_PddContext.pipeStream.hPipe = 0;
    m_PddContext.pipeStream.wPacketSize = 0;
    m_pDrv = NULL;
    memset( &m_CsState, 0x0, sizeof(m_CsState));
    memset( &m_SensorModeInfo, 0x0, sizeof(m_SensorModeInfo));
    memset( &PowerCaps, 0x0, sizeof(PowerCaps));
    m_PowerState = D4 ; // OFF state
    memset( &m_SensorProps, 0x0, sizeof(m_SensorProps));

    m_pPinVideoFormats = NULL;
    m_pUSBVideoModes = NULL;
    m_pVideoCaps = NULL;
    m_ppModeContext = NULL;

    m_pCurrentFormat = NULL;
    m_pCurrentStreamFormat = NULL;
    m_dwPreferredHeight = 0;
    m_dwPreferredWidth  = 0;

    // Enable DMA for Isoch transfers by default, otherwise memcpy will be very expensive
    EnableDMASupport();
    
    m_hCaptureThread     = NULL;
    m_hCaptureThreadExit = NULL;

    m_ulBufSize            = 0;
    m_pBufVirtualCached    = NULL;
    m_pBufVirtualUncached  = NULL;
    m_BufPhysical.QuadPart = 0;

    m_hFillThread     = NULL;
    m_hFillThreadRun  = NULL;
    m_hFillThreadExit = NULL;
    m_pFillBuffer     = NULL;
    m_dwFillBytes     = 0;
}


CUSBPdd::~CUSBPdd()
{
    DEBUGMSG (ZONE_FUNCTION, (DTAG TEXT("~CUSBPdd++\r\n")));

    if (NULL != m_SensorModeInfo[CAPTURE].pVideoFormat)
    {
        delete m_SensorModeInfo[CAPTURE].pVideoFormat;
        m_SensorModeInfo[CAPTURE].pVideoFormat = NULL;        
    }
    
    if (NULL != m_SensorModeInfo[STILL].pVideoFormat)
    {
        delete m_SensorModeInfo[STILL].pVideoFormat;
        m_SensorModeInfo[STILL].pVideoFormat = NULL;        
    }

    if( NULL != m_pVideoCaps )
    {
        delete [] m_pVideoCaps;
        m_pVideoCaps = NULL;
    }

    if( NULL != m_pCurrentFormat )
    {
        delete [] m_pCurrentFormat;
        m_pCurrentFormat = NULL;
    }

    if( NULL != m_pPinVideoFormats )
    {
        for (int i = 0; i < (int)m_pPinVideoFormats[CAPTURE].ulAvailFormats; i++)
        {
            delete m_pPinVideoFormats[CAPTURE].pCsDataRangeVideo[ i ];
            m_pPinVideoFormats[CAPTURE].pCsDataRangeVideo[ i ] = NULL;
        }

        for (int i = 0; i < (int)m_pPinVideoFormats[STILL].ulAvailFormats; i++)
        {
            delete m_pPinVideoFormats[STILL].pCsDataRangeVideo[ i ];
            m_pPinVideoFormats[STILL].pCsDataRangeVideo[ i ] = NULL;
        }

        delete [] m_pPinVideoFormats[CAPTURE].pCsDataRangeVideo;
        delete [] m_pPinVideoFormats[STILL].pCsDataRangeVideo;
        m_pPinVideoFormats[CAPTURE].pCsDataRangeVideo = NULL;
        m_pPinVideoFormats[STILL].pCsDataRangeVideo = NULL;

        delete [] m_pPinVideoFormats;
        m_pPinVideoFormats = NULL;
    }

    if( NULL != m_ppModeContext )
    {
        delete [] m_ppModeContext;
        m_ppModeContext = NULL;
    }
    
    if( NULL != m_pCurrentStreamFormat )
    {
        LocalFree( m_pCurrentStreamFormat );
        m_pCurrentStreamFormat = NULL;
    }

    if( NULL != m_pUSBVideoModes )
    {
        delete [] m_pUSBVideoModes;
        m_pUSBVideoModes = NULL;
    }
}


//
// This function is called when the camera driver is loaded and it 
// initializes all the structures by querying the camera
//
DWORD CUSBPdd::PDDInit( PVOID MDDContext, PPDDFUNCTBL pPDDFuncTbl )
{
    DEBUGMSG (ZONE_FUNCTION, (DTAG TEXT("PddInit++\r\n")));

    // Pass the function table to MDD
//    if( pPDDFuncTbl->dwSize > sizeof(PDDFUNCTBL))
    if( pPDDFuncTbl->dwSize < sizeof(PDDFUNCTBL))
    {
         DEBUGMSG(ZONE_ERROR, (DTAG TEXT("ERROR: copying the function table. Insufficient memory\r\n"))); 
         return ERROR_INSUFFICIENT_BUFFER;
    }
    memcpy( pPDDFuncTbl, &FuncTbl, sizeof( PDDFUNCTBL ) );


    m_hCaptureThreadExit = CreateEvent( NULL, TRUE, FALSE, NULL );
    m_hFillThreadRun     = CreateEvent( NULL, TRUE, FALSE, NULL );  // remains signalled while thread is in Run state
    m_hFillThreadExit    = CreateEvent( NULL, TRUE, FALSE, NULL );
    m_hFrameReady        = CreateEvent( NULL, TRUE, FALSE, NULL );

    if (!m_hCaptureThreadExit || !m_hFillThreadRun || !m_hFillThreadExit || !m_hFrameReady) {
        DEBUGMSG(ZONE_ERROR, (DTAG TEXT("ERROR: CreateEvent() returned 0x%x\r\n"), GetLastError()));
        return ERROR_OUTOFMEMORY;
    }
    
    wcsncpy_s(m_szActiveRegPath, (LPCTSTR)MDDContext, wcslen((LPCTSTR)MDDContext));
    PDRVCONTEXT pDrvT = GetRegData(MDDContext);
    if(pDrvT == NULL)
    {
        DEBUGMSG(ZONE_ERROR, (DTAG TEXT("ERROR: read device context from registry\r\n")));
        return ERROR_INVALID_PARAMETER;
    }
    m_pDrv = pDrvT;
    

    // Allow user to override some hard-coded defaults
    ReadConfigFromRegistry();

    // Set the Sensor Power State (turn the camera ON)
    SetPowerState(D1);                   

    // Sensor only supports ON / OFF states
    PowerCaps.DeviceDx = DX_MASK(D0)|DX_MASK(D4);

    // See which properties the camera supports, then initialize them
    CheckCameraProperties();
    InitCameraProperties();

    // Hardware bug: camera doesn't always start up with
    // default settings (especially after soft reset or driver load/unload).
    // Reset them to be sure.
    ResetCameraProperties();

    QueryFormats();

    // Allocate Video Control Caps specific array.
    m_pVideoCaps = new VIDCONTROLCAPS[m_ulCTypes];
    if( NULL == m_pVideoCaps )
    {
         DEBUGMSG(ZONE_ERROR, (DTAG TEXT("Insufficient memory\r\n"))); 
         return ERROR_INSUFFICIENT_BUFFER;
    }

    // Video Control Caps -- not used by the PDD or MDD (backward compatibilty)
    m_pVideoCaps[CAPTURE].DefaultVideoControlCaps     = DefaultVideoControlCaps[CAPTURE];
    m_pVideoCaps[STILL].DefaultVideoControlCaps       = DefaultVideoControlCaps[STILL];
    if( 3 == m_ulCTypes )
    {
        //Note PREVIEW control caps are the same, so we don't differentiate
        m_pVideoCaps[PREVIEW].DefaultVideoControlCaps     = DefaultVideoControlCaps[PREVIEW];
    }

    m_SensorModeInfo[CAPTURE].MemoryModel = CSPROPERTY_BUFFER_CLIENT_UNLIMITED;
    m_SensorModeInfo[CAPTURE].MaxNumOfBuffers = MAXCAPTUREBUFFS;
    m_SensorModeInfo[CAPTURE].PossibleCount = 1;                                    // Instances of the PIN
    m_SensorModeInfo[CAPTURE].VideoCaps.CurrentVideoControlCaps = DefaultVideoControlCaps[CAPTURE];
    m_SensorModeInfo[CAPTURE].VideoCaps.DefaultVideoControlCaps = DefaultVideoControlCaps[CAPTURE];
    m_SensorModeInfo[CAPTURE].pVideoFormat = new PINVIDEOFORMAT;
    m_SensorModeInfo[CAPTURE].pVideoFormat->categoryGUID = m_pPinVideoFormats[CAPTURE].categoryGUID;
    m_SensorModeInfo[CAPTURE].pVideoFormat->ulAvailFormats = m_pPinVideoFormats[CAPTURE].ulAvailFormats;       //from QueryFormats 
    m_SensorModeInfo[CAPTURE].pVideoFormat->pCsDataRangeVideo = m_pPinVideoFormats[CAPTURE].pCsDataRangeVideo; //from QueryFormats
    
    m_SensorModeInfo[STILL].MemoryModel = CSPROPERTY_BUFFER_CLIENT_UNLIMITED;
    m_SensorModeInfo[STILL].MaxNumOfBuffers = MAXSTILLBUFFS;
    m_SensorModeInfo[STILL].PossibleCount = 1;                                      // Instances of the PIN
    m_SensorModeInfo[STILL].VideoCaps.CurrentVideoControlCaps = DefaultVideoControlCaps[STILL];
    m_SensorModeInfo[STILL].VideoCaps.DefaultVideoControlCaps = DefaultVideoControlCaps[STILL];
    m_SensorModeInfo[STILL].pVideoFormat = new PINVIDEOFORMAT;
    m_SensorModeInfo[STILL].pVideoFormat->categoryGUID = m_pPinVideoFormats[STILL].categoryGUID;
    m_SensorModeInfo[STILL].pVideoFormat->ulAvailFormats = m_pPinVideoFormats[STILL].ulAvailFormats;        //from QueryFormats  
    m_SensorModeInfo[STILL].pVideoFormat->pCsDataRangeVideo = m_pPinVideoFormats[STILL].pCsDataRangeVideo;  //from QueryFormats


    // Holds the call back function pointer    
    m_ppModeContext = new LPVOID[m_ulCTypes];
    if ( NULL == m_ppModeContext )
    {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    // Initialised by MDD on InitSensorState 
    m_ppModeContext[CAPTURE] = NULL;
    m_ppModeContext[STILL] = NULL;                                  
    
    // Hold the current format set on the sensor. Set to undefined format, when no format is set on the sensor. 
    m_pCurrentFormat = new CS_DATARANGE_VIDEO[m_ulCTypes];
    if( NULL == m_pCurrentFormat )
    {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    memset(m_pCurrentFormat, 0x0, (sizeof(CS_DATARANGE_VIDEO)*m_ulCTypes));

    // Structure to hold current format in USB video class specific format
    m_pCurrentStreamFormat = (PUSBVIDEOMODESTRUCT)LocalAlloc (LPTR, sizeof (USBVIDEOMODESTRUCT));
    if( NULL == m_pCurrentStreamFormat )
    {
        return ERROR_INSUFFICIENT_BUFFER;
    }
    m_pCurrentStreamFormat->wFormatType = 0;   // Undefined format type
  

    return ERROR_SUCCESS;
}


DWORD CUSBPdd::PDDDeInit()
{
    DWORD rc = ERROR_SUCCESS;

    // Terminate capture thread, if running 
    // Close USB pipe if open 
    if (FAILED( DeInitSensorMode(CAPTURE) )){
        rc = E_FAIL;
    }

    if (FAILED( DeInitSensorMode(STILL) )){
        rc = E_FAIL;
    }

    if (m_hCaptureThreadExit)
    {
        CloseHandle(m_hCaptureThreadExit);
    }

    if (m_hFillThreadRun)
    {
        CloseHandle(m_hFillThreadRun);
    }

    if (m_hFillThreadExit)
    {
        CloseHandle(m_hCaptureThreadExit);
    }

    if (m_hFrameReady)
    {
        CloseHandle(m_hFrameReady);
    }

    if (m_pBufVirtualCached && m_pBufVirtualUncached && m_ulBufSize)
    {
        FreeContiguousCachedBuffer(
            m_ulBufSize,
            &m_pBufVirtualCached,
            &m_pBufVirtualUncached,
            &m_BufPhysical);
    }
    

    return rc;
}


DWORD CUSBPdd::GetAdapterInfo( PADAPTERINFO pAdapterInfo )
{
    DEBUGMSG (ZONE_FUNCTION, (DTAG TEXT("GetAdaptorInfo++\r\n")));
    
    pAdapterInfo->ulCTypes = m_ulCTypes;
    pAdapterInfo->PowerCaps = PowerCaps;
    pAdapterInfo->ulVersionID = DRIVER_VERSION_2; //Camera MDD and DShow support DRIVER_VERSION and DRIVER_VERSION_2. Defined in camera.h
    memcpy( &pAdapterInfo->SensorProps, &m_SensorProps, sizeof(m_SensorProps));
    
    return ERROR_SUCCESS;
}


//
// These functions will later change when all the properties are individually handled - 
// the present code gives a generic functionality to set the property for the given value. 
// It is assumed that the MDD passes a value in the specified range acceptable to the sensor
//
DWORD CUSBPdd::HandleVidProcAmpChanges( DWORD dwPropId, LONG lFlags, LONG lValue )
{
    DEBUGMSG (ZONE_FUNCTION, (DTAG TEXT("HandleVidProcAmpChanges++\r\n")));
    DWORD rc;
    
    if( PropSupported( dwPropId ) )
    {
        // Auto mode
        if ((lFlags & CSPROPERTY_CAMERACONTROL_FLAGS_AUTO) &&
            (m_SensorProps[dwPropId].ulCapabilities & CSPROPERTY_CAMERACONTROL_FLAGS_AUTO))
        {
            // Switch to auto mode if we were in manual
            if (m_SensorProps[dwPropId].ulFlags & CSPROPERTY_CAMERACONTROL_FLAGS_MANUAL)
            {
                if ( PropSupported( dwPropId|ENUM_AUTO ) )
                {
                    rc = PropSetValue( dwPropId|ENUM_AUTO, TRUE );

                    if (rc != ERROR_SUCCESS)
                        return rc;

                    m_SensorProps[dwPropId].ulFlags &= ~CSPROPERTY_CAMERACONTROL_FLAGS_MANUAL;
                    m_SensorProps[dwPropId].ulFlags |= CSPROPERTY_CAMERACONTROL_FLAGS_AUTO;
                }
            }

            return ERROR_SUCCESS;
        }
        else if ((lFlags & CSPROPERTY_CAMERACONTROL_FLAGS_MANUAL) &&
                 (m_SensorProps[dwPropId].ulCapabilities & CSPROPERTY_CAMERACONTROL_FLAGS_MANUAL))
        {
            // Switch to manual mode if we were in auto
            if (m_SensorProps[dwPropId].ulFlags & CSPROPERTY_CAMERACONTROL_FLAGS_AUTO)
            {
                if ( PropSupported( dwPropId|ENUM_AUTO ) )
                {
                    rc = PropSetValue( dwPropId|ENUM_AUTO, FALSE );

                    if (rc != ERROR_SUCCESS)
                        return rc;

                    m_SensorProps[dwPropId].ulFlags &= ~CSPROPERTY_CAMERACONTROL_FLAGS_AUTO;
                    m_SensorProps[dwPropId].ulFlags |= CSPROPERTY_CAMERACONTROL_FLAGS_MANUAL;
                }
            }

            // Set the property to given value
            rc = PropSetValue( dwPropId, lValue );

            if (rc == ERROR_SUCCESS)
                m_SensorProps[dwPropId].ulCurrentValue = lValue;

            return rc;
        }
    }

    return ERROR_NOT_SUPPORTED;
}


DWORD CUSBPdd::HandleCamControlChanges( DWORD dwPropId, LONG lFlags, LONG lValue )
{
    DEBUGMSG (ZONE_FUNCTION, (DTAG TEXT("HandleCamControlChanges++\r\n")));
    DWORD rc;
    
    if( PropSupported( dwPropId ) )
    {
        // Auto mode
        if ((lFlags & CSPROPERTY_CAMERACONTROL_FLAGS_AUTO) &&
            (m_SensorProps[dwPropId].ulCapabilities & CSPROPERTY_CAMERACONTROL_FLAGS_AUTO))
        {
            // Switch to auto mode if we were in manual
            if (m_SensorProps[dwPropId].ulFlags & CSPROPERTY_CAMERACONTROL_FLAGS_MANUAL)
            {
                if ( PropSupported( dwPropId|ENUM_AUTO ) )
                {
                    rc = PropSetValue( dwPropId|ENUM_AUTO, TRUE );

                    if (rc != ERROR_SUCCESS)
                        return rc;

                    m_SensorProps[dwPropId].ulFlags &= ~CSPROPERTY_CAMERACONTROL_FLAGS_MANUAL;
                    m_SensorProps[dwPropId].ulFlags |= CSPROPERTY_CAMERACONTROL_FLAGS_AUTO;
                }
            }

            return ERROR_SUCCESS;
        }
        else if ((lFlags & CSPROPERTY_CAMERACONTROL_FLAGS_MANUAL) &&
                 (m_SensorProps[dwPropId].ulCapabilities & CSPROPERTY_CAMERACONTROL_FLAGS_MANUAL))
        {
            // Switch to manual mode if we were in auto
            if (m_SensorProps[dwPropId].ulFlags & CSPROPERTY_CAMERACONTROL_FLAGS_AUTO)
            {
                if ( PropSupported( dwPropId|ENUM_AUTO ) )
                {
                    rc = PropSetValue( dwPropId|ENUM_AUTO, FALSE );

                    if (rc != ERROR_SUCCESS)
                        return rc;

                    m_SensorProps[dwPropId].ulFlags &= ~CSPROPERTY_CAMERACONTROL_FLAGS_AUTO;
                    m_SensorProps[dwPropId].ulFlags |= CSPROPERTY_CAMERACONTROL_FLAGS_MANUAL;
                }
            }

            // Set the property to given value
            rc = PropSetValue( dwPropId, lValue );

            if (rc == ERROR_SUCCESS)
                m_SensorProps[dwPropId].ulCurrentValue = lValue;

            return rc;
        }
    }

    return ERROR_NOT_SUPPORTED;
}


//
// This function is deprecated but we implement it anyway
//
DWORD CUSBPdd::HandleVideoControlCapsChanges( LONG lModeType, ULONG ulCaps )
{
    DEBUGMSG (ZONE_FUNCTION, (DTAG TEXT("HandleVideoControlChanges++\r\n")));
    
    m_pVideoCaps[lModeType].CurrentVideoControlCaps = ulCaps;
    return ERROR_SUCCESS;
}


//
// Let PDD check and handle certain properties
//
DWORD CUSBPdd::HandleAdapterCustomProperties( PUCHAR pInBuf, DWORD  InBufLen, PUCHAR pOutBuf, DWORD  OutBufLen, PDWORD pdwBytesTransferred )
{
    DEBUGMSG (ZONE_FUNCTION, (DTAG TEXT("HandleAdapterCustomProperties++\r\n")));

    return ERROR_NOT_SUPPORTED;
}


//
// This function saves a handle to call back function to interrupt the MDD on data ready
// for capture mode
//
DWORD CUSBPdd::InitSensorMode( ULONG ulModeType, LPVOID ModeContext )
{
    DEBUGMSG (ZONE_FUNCTION, (DTAG TEXT("InitSensorMode++\r\n")));
    
    // Save the pointer to the call back function
    ASSERT( ModeContext );
    m_ppModeContext[ulModeType] = ModeContext;

    return ERROR_SUCCESS;
}


//
// Here the PDD terminates the CaptureThread and sets the PIN in STOP mode
// Called by the MDD at the end of streaming
//
DWORD CUSBPdd::DeInitSensorMode( ULONG ulModeType )
{
    DEBUGMSG (ZONE_FUNCTION, (DTAG TEXT("DeInitSensorMode++\r\n")));
    
    // Put the pin in STOP mode if needed
    if(m_CsState[ulModeType] != CSSTATE_STOP)
        SetSensorState(ulModeType, CSSTATE_STOP);

    m_ppModeContext[ulModeType] = NULL;

    return ERROR_SUCCESS;
}


DWORD WINAPI CUSBPdd::FillThread(LPVOID lpVoid)
{
    // This is the PDD context passed to the thread
    PUSBPDD pPDD = reinterpret_cast<PUSBPDD>(lpVoid);
    DWORD dwMSBetweenFrames;
    DWORD dwRval = ERROR_SUCCESS;
    
    DEBUGMSG (ZONE_FUNCTION, (DTAG TEXT("FillThread created\r\n")));

    if (pPDD->m_pDrv->usbDeviceInfo.Descriptor.bcdUSB < 0x200) 
    {
        // USB 1.x host controller
        dwMSBetweenFrames = (pPDD->m_pCurrentStreamFormat->dwCurFrameInterval/1000)/4;
        DEBUGMSG (ZONE_INIT, (DTAG TEXT("Detected USB 1.x host controller (bcdUSB = 0x%x)\r\n"),
            pPDD->m_pDrv->usbDeviceInfo.Descriptor.bcdUSB));
    } 
    else 
    {
        // USB 2.x host controller
        dwMSBetweenFrames = (pPDD->m_pCurrentStreamFormat->dwCurFrameInterval/10000)/4;
        DEBUGMSG (ZONE_INIT, (DTAG TEXT("Detected USB 2.x host controller (bcdUSB = 0x%x)\r\n"),
            pPDD->m_pDrv->usbDeviceInfo.Descriptor.bcdUSB));
    }

    if(lpVoid == NULL)
        return ERROR_INVALID_PARAMETER;

    // elevate the thread priority to better service the Isoch transfers
    CeSetThreadPriority(GetCurrentThread(), FILL_THREAD_PRI);

    HANDLE phEvents[] = { pPDD->m_hFillThreadExit, pPDD->m_hFillThreadRun };
    
    while(TRUE)
    {
        switch( WaitForMultipleObjects(2, phEvents, FALSE, INFINITE) )
        {
            case WAIT_OBJECT_0:
                // Thread asked to terminate
                DEBUGMSG (ZONE_FUNCTION, (DTAG TEXT("FillThread asked to exit.\r\n")));
                goto Exit;
                break;
            case WAIT_OBJECT_0 + 1:
                // Thread is in Run state; proceed.
                // SetSensorState() will cause thread to BLOCK on m_hFillThreadRun
                // when in Paused or Stopped state.
                break;
            default:   
                DEBUGMSG (ZONE_ERROR, (DTAG TEXT("ERROR: FillThread: WaitForMultipleObjects() returned 0x%x.\r\n"),
                    GetLastError()));
                goto Exit;
                break;
        }

        // Grab a frame from the camera and push it to MDD
        if(pPDD->m_CsState[CAPTURE] == CSSTATE_RUN)
        {   
            if(pPDD->m_ppModeContext[CAPTURE])
            {
                DEBUGMSG (ZONE_DEVICE, (DTAG TEXT("MDD_HandleIO()\r\n")));
                
                // Tell MDD to read image data from buffer; will block until a frame is ready
                // See: FillBuffer()
                dwRval = MDD_HandleIO(pPDD->m_ppModeContext[CAPTURE], CAPTURE);

                if (dwRval == ERROR_OUTOFMEMORY) {
                    // MDD does not have a buffer available so just sleep a bit and wait
                    // Sleep based on the frame interval reported by the camera (e.g. 30fps)
                    Sleep(dwMSBetweenFrames);
                }
                else if (dwRval != ERROR_SUCCESS)
                {
                    DEBUGMSG (ZONE_ERROR, (DTAG TEXT("ERROR: MDD_HandleIO() returned 0x%x\r\n"), dwRval));
                }
            }

        }
        else
        {
            DEBUGMSG (ZONE_ERROR, (DTAG TEXT("WARNING: FillThread: resumed but state != CAPTURE\r\n")));
            goto Exit;
        }
    }

Exit:    
    DEBUGMSG (ZONE_FUNCTION, (DTAG TEXT("FillThread exiting\r\n")));

    return dwRval;
}


//
// This thread reads frames from the camera via USB.
// 
// Frame data streams into a DMA buffer, and is copied
// to the MDD buffer until a complete frame is ready.  
// 
// If no MDD buffer is available (DShow is still using it)
// we copy the frame data to a backup buffer.
//
// If no MDD buffer is available, and the backup buffer is
// full, we drop the frame.
//
// FillThread is the top half: it passes frames to DShow.
//
DWORD WINAPI CUSBPdd::CaptureThread(LPVOID lpVoid)
{
    // This is the PDD context passed to the thread
    PUSBPDD      pPDD                   = reinterpret_cast<PUSBPDD>(lpVoid);
    HANDLE       phEvents[2]            = {pPDD->m_hCaptureThreadExit, NULL};
    DWORD        dwPacketLen            = pPDD->m_PddContext.pipeStream.wPacketSize;
    DWORD        dwFrameState, dwLength;
    DWORD        dwMSBetweenFrames;
    PBYTE        pBackupBuffer;
    DWORD        dwRet                  = ERROR_SUCCESS;
    int          i,j;
    static DWORD dwFrameCount           = 0;

    DEBUGMSG (ZONE_FUNCTION, (DTAG TEXT("CaptureThread created\r\n")));
        

    if (pPDD->m_pDrv->usbDeviceInfo.Descriptor.bcdUSB < 0x200) 
    {
        // USB 1.x host controller
        dwMSBetweenFrames = (pPDD->m_pCurrentStreamFormat->dwCurFrameInterval/1000);
        DEBUGMSG (ZONE_INIT, (DTAG TEXT("Detected USB 1.x host controller (bcdUSB = 0x%x)\r\n"),
            pPDD->m_pDrv->usbDeviceInfo.Descriptor.bcdUSB));
    } 
    else 
    {
        // USB 2.x host controller
        dwMSBetweenFrames = (pPDD->m_pCurrentStreamFormat->dwCurFrameInterval/10000);
        DEBUGMSG (ZONE_INIT, (DTAG TEXT("Detected USB 2.x host controller (bcdUSB = 0x%x)\r\n"),
            pPDD->m_pDrv->usbDeviceInfo.Descriptor.bcdUSB));
    }


    // elevate the thread priority to better service the Isoch transfers
    CeSetThreadPriority(GetCurrentThread(), CAPTURE_THREAD_PRI);

    // Allocate backup frame buffer
    // If MDD doesn't provide buffer in time (m_pFillBuffer) we read frames into the backup
    // buffer, overwriting w/ each new frame, until MDD provides a buffer again.
    pBackupBuffer = (PBYTE)LocalAlloc(LMEM_FIXED, pPDD->m_pCurrentStreamFormat->dwMaxVideoFrameBufferSize);
    if (!pBackupBuffer)
    {
        DEBUGMSG (ZONE_ERROR, (DTAG TEXT("Error: could not allocate backup capture buffer\r\n")));
        dwRet = ERROR_OUTOFMEMORY;
        goto Finish;
    }

    // 
    // Now ensure we have DMA capture buffer for actual USB reads (partial video frames)
    //
    if(pPDD->m_pBufVirtualCached == NULL || pPDD->m_pBufVirtualUncached == NULL)
    {
        DEBUGMSG (ZONE_ERROR, (DTAG TEXT("Error: DMA capture buffers not available\r\n")));
        dwRet = ERROR_OUTOFMEMORY;
        goto Finish;
    }
    
    DWORD            dwUsbRc[TRANSFER_SIZE];
    DWORD            dwFrameLen[SIZE_TRANSFER_QUEUE][TRANSFER_SIZE];
    HANDLE           hEvent[SIZE_TRANSFER_QUEUE] = {0,};
    PBYTE            pBufferVirtual[SIZE_TRANSFER_QUEUE] = {0,};
    PHYSICAL_ADDRESS BufferPhysical[SIZE_TRANSFER_QUEUE] = {0,};
    USB_TRANSFER     hTransfer[SIZE_TRANSFER_QUEUE] = {0,};

    // Transfer queue - we first initialize SIZE_TRANSFER_QUEUE transfers and then in the next loop wait on these 
    // transfers to complete and immediately queue a new transfer when a given transfer is complete
    for (i = 0; i < SIZE_TRANSFER_QUEUE; i++)
    {
        pBufferVirtual[i] = pPDD->m_pBufVirtualCached + (pPDD->m_PddContext.pipeStream.wPacketSize * TRANSFER_SIZE * i);
        BufferPhysical[i].LowPart = pPDD->m_BufPhysical.LowPart + (pPDD->m_PddContext.pipeStream.wPacketSize * TRANSFER_SIZE * i);


        if (pPDD->m_fDMASupported)
            CacheRangeFlush( pBufferVirtual[i], (pPDD->m_PddContext.pipeStream.wPacketSize)*TRANSFER_SIZE, CACHE_SYNC_DISCARD);
        
        hEvent[i] = CreateEvent (NULL, FALSE, FALSE, NULL);
        if(hEvent[i] == NULL)
        {
            DEBUGMSG (ZONE_ERROR, (DTAG TEXT("Error: could not create event\r\n")));
            dwRet = ERROR_OUTOFMEMORY;
            goto Finish;
        }

        for(j = 0; j < TRANSFER_SIZE; j++)
            dwFrameLen[i][j] = pPDD->m_PddContext.pipeStream.wPacketSize;

        hTransfer[i]= pPDD->m_pDrv->lpUsbFuncs->lpIssueIsochTransfer (
            pPDD->m_PddContext.pipeStream.hPipe, 
            DefaultTransferComplete,
            hEvent[i],
            USB_IN_TRANSFER | USB_START_ISOCH_ASAP | USB_SHORT_TRANSFER_OK, 
            0,                  // Using USB_START_ISOCH_ASAP precludes the need for a start frame
            TRANSFER_SIZE,      // Number of frames to read
            dwFrameLen[i],      // Array that receives the length of each frame
            pBufferVirtual[i],  // Pointer to transfer buffer
            BufferPhysical[i].LowPart // Physical address of buffer, or NULL if DMA unsupported on this platform
            );
        
        if(hTransfer[i] == NULL)
        {
            DEBUGMSG (ZONE_ERROR, (DTAG TEXT("Error: IssueIsochTransfer failed\r\n")));
            dwRet = ERROR_GEN_FAILURE;
            goto Finish;
        }
    }


    // set up the buffer pointers
    PBYTE pStart    = pBackupBuffer;
    PBYTE pCur      = pStart;
    PBYTE pEnd      = pStart + pPDD->m_pCurrentStreamFormat->dwMaxVideoFrameBufferSize;

    
    // we start getting data from somewhere in the middle of a frame and wait 
    // for that frame to finish before starting to read the "first" frame.
    dwFrameState = RE_SYNC;
    i = 0;
    
    // Capture and process frames until asked to stop
    while(TRUE)
    {
        // Wait for first Isoch transfer to complete or CaptureThread exit
        phEvents[1] = hEvent[i];
        switch(WaitForMultipleObjects(2, phEvents, FALSE, dwMSBetweenFrames * 2))
        {
            case WAIT_OBJECT_0:
                // Thread asked to terminate
                DEBUGMSG (ZONE_FUNCTION, (DTAG TEXT("CaptureThread asked to exit.\r\n")));
                goto Finish;
                break;

            case WAIT_OBJECT_0 + 1:
                // Isoch transfer done
                break;

            case WAIT_TIMEOUT:
                DEBUGMSG (ZONE_ERROR, (DTAG TEXT("Error: IsochTransfer timed out\r\n")));
                dwRet = ERROR_TIMEOUT;
                goto RestartTrans;

            default:   
                DEBUGMSG (ZONE_ERROR, (DTAG TEXT("ERROR: CaptureThread: WaitForMultipleObjects() returned 0x%x.\r\n"),
                    GetLastError()));
                goto Finish;
        }

        // get the transfer results and make sure there was no error in reading any frame of the complete transfer
        pPDD->m_pDrv->lpUsbFuncs->lpGetIsochResults (hTransfer[i], TRANSFER_SIZE, dwFrameLen[i], dwUsbRc);
        for(j = 0; j < TRANSFER_SIZE; j++)
        {
            if(dwUsbRc[j] && (dwUsbRc[j] != USB_DATA_TOGGLE_MISMATCH_ERROR) && (dwUsbRc[j] != USB_DATA_UNDERRUN_ERROR)) 
            {
                DEBUGMSG (ZONE_ERROR, (DTAG TEXT("IsochTransfer error 0x%x, microframe %d\r\n"), dwUsbRc[j], j));
                dwRet = ERROR_GEN_FAILURE;
                goto RestartTrans;
            }
        }

        __try 
        {
            // process the read data based on the format
            PBYTE pDataBuff = pBufferVirtual[i];
            for(j = 0; j < TRANSFER_SIZE; j++)
            {
                PUSBVIDPAYLOADHDR pPktHdr;
                BYTE bPktFlags;
                int n;
                static BOOL bFID;
    
                dwLength = dwFrameLen[i][j];
                pPktHdr = (PUSBVIDPAYLOADHDR)pDataBuff;
                bPktFlags = pPktHdr->bFlags;
    
                // skip packets which are not at least the size of the payload header
                if(dwLength < sizeof(USBVIDPAYLOADHDR) || pPktHdr->bLen != sizeof(USBVIDPAYLOADHDR))
                {
                    pDataBuff += dwPacketLen;
                    continue;
                }
    
                // if there is an error indicated in the header, skip and re-sync
                if(bPktFlags & USBVID_PAYLOADHDR_ERR)
                {
                    // Video packet errors may occur at the beginning of a capture.  This is normal
                    // since the camera may have overflowed its hardware FIFO buffer before we
                    // started FillThread to empty it.  This error should go away once FillThread is running.
                    // If the problem persists, it usually means FillThread is not getting enough CPU cycles
                    // to keep the Isoch transfers going per microframe without idle gaps
                    DEBUGMSG (ZONE_ERROR, (DTAG TEXT("Error: video packet error; RE_SYNC\r\n")));
    
                    // discard all packets from this frame and wait for a new frame to start
                    dwFrameState = RE_SYNC;
                    pDataBuff += dwPacketLen;
                    continue;
                }

    
                // see if MDD buffer is now available so that we can switch to it
                if (pStart == pBackupBuffer && pPDD->m_pFillBuffer)
                {
                    // switch buffers by copying all of the data from backup buffer to MDD buffer
                    if (pCur - pStart) {
                        DEBUGMSG (ZONE_READDATA, (DTAG TEXT("Copying frame %d (0x%x bytes) from backup buff (0x%x) to client buff (0x%x)\r\n"),
                            dwFrameCount,
                            pCur - pStart, 
                            pBackupBuffer, 
                            pPDD->m_pFillBuffer));
                        memcpy(pPDD->m_pFillBuffer, pBackupBuffer, pCur - pStart);
                    }
    
                    pStart = pPDD->m_pFillBuffer;
                    pCur   = pStart + (pCur - pBackupBuffer);
                    pEnd   = pStart + pPDD->m_pCurrentStreamFormat->dwMaxVideoFrameBufferSize;
                }

    
                // capture the frame data
                switch(dwFrameState)
                {
                    // first packet in the new frame
                    case START_CAP:
                        DEBUGMSG (ZONE_READDATA, (DTAG TEXT("Capture State: frame %d START_CAP, pCur = 0x%x\r\n"), dwFrameCount, pCur));

                        bFID = bPktFlags & 0x1;
    
                        // if this packet contains data, then strip off the USB header
                        // and copy payload to frame buffer
                        if(dwLength > sizeof(USBVIDPAYLOADHDR))
                        {
                            dwLength = dwLength - sizeof(USBVIDPAYLOADHDR);
    
                            if (pPDD->m_pCurrentStreamFormat->wFormatType == USB_VIDEO_VS_FORMAT_UNCOMPRESSED)
                            {
                                if(dwLength > (DWORD)(pEnd - pCur))
                                    dwLength = pEnd - pCur;
                                    
                                memcpy(pCur, pDataBuff + sizeof(USBVIDPAYLOADHDR), dwLength);
                                pCur += dwLength;
                                dwFrameState = CAPTURING;
                            }
                            else if (pPDD->m_pCurrentStreamFormat->wFormatType == USB_VIDEO_VS_FORMAT_MJPEG)
                            {
                                if(dwLength <= 4 || *(pDataBuff+sizeof(USBVIDPAYLOADHDR)) != 0xff || *(pDataBuff+sizeof(USBVIDPAYLOADHDR)+1) != 0xd8)
                                {
                                    // bad packet, discard all packets from this frame and wait for a new frame to start
                                    DEBUGMSG (ZONE_ERROR, (DTAG TEXT("Error: video packet error; RE_SYNC\r\n")));
    
                                    dwFrameState = RE_SYNC;
                                    pDataBuff += dwPacketLen;
                                    continue;
                                }
                            
                                // Copy the JPEG headers at the beginning of the frame if the sensor is set in MJPEG format
                                memcpy(pCur, JFIFHdr, sizeof(JFIFHdr));
                                pCur = pCur + sizeof(JFIFHdr);
                                memcpy(pCur, MJPGDHTSeg, sizeof(MJPGDHTSeg));
                                pCur = pCur + sizeof(MJPGDHTSeg);
    
                                // Remove the camera's frame header
                                n = *(pDataBuff + sizeof(USBVIDPAYLOADHDR) + 4);
                                n = n << 8;
                                n += *(pDataBuff+sizeof(USBVIDPAYLOADHDR) + 5) + 4;
                                dwLength = dwLength - n;
                                memcpy(pCur, pDataBuff + sizeof(USBVIDPAYLOADHDR) + n, dwLength);
                                pCur += dwLength;
                                dwFrameState = CAPTURING;
                            }
                        }
                        break;
    
                    case CAPTURING: // in CAPTURE state simply add all incoming data (without the camera header) to the buffer
                        if (pCur == pStart)
                            DEBUGMSG (ZONE_READDATA, (DTAG TEXT("Capture State: CAPTURING\r\n")));
                        
                        if (bPktFlags &  USBVID_PAYLOADHDR_FRAMEEND)
                        {
                            DEBUGMSG (ZONE_READDATA, (DTAG TEXT("Capture State: frame %d COMPLETE\r\n"), dwFrameCount));
    
                            if (bFID != (bPktFlags & 0x1))
                            {
                                DEBUGMSG(ZONE_ERROR, (TEXT("Error: FID mismatch, frame %d dropped\r\n"), dwFrameCount));
    
                                dwFrameState = RE_SYNC;
                                pDataBuff += dwPacketLen;
                                dwFrameCount++;
                                continue;
                            }
                            
                            dwFrameState = COMPLETE;
                        }
    
                        if(dwLength > sizeof(USBVIDPAYLOADHDR))
                        {
                            dwLength = dwLength - sizeof(USBVIDPAYLOADHDR);
                            if(dwLength > (DWORD)(pEnd - pCur))
                                dwLength = pEnd - pCur;
                            memcpy(pCur, pDataBuff + sizeof(USBVIDPAYLOADHDR), dwLength);
                            pCur += dwLength;
                        }
    
                        // process the completed frame
                        if (dwFrameState == COMPLETE)
                        {
                            // Tell FillBuffer that a frame is ready for MDD
                            if (pStart == pPDD->m_pFillBuffer)
                            {
                                // deliver completed frame buffer to MDD
                                DEBUGMSG (ZONE_READDATA, (DTAG TEXT("Complete frame %d delivered to MDD\r\n"), dwFrameCount));
                                pPDD->m_dwCapturedFrames++;
                                pPDD->m_pFillBuffer = NULL;
//                                pPDD->m_pFillBuffer = pBackupBuffer;
                                pPDD->m_dwFillBytes = pCur - pStart;
                                SetEvent(pPDD->m_hFrameReady);
                            }
                            else
                            {
                                // MDD buffer was not available so the frame will be dropped.
                                // This problem usually occurs when Dshow is not processing the frames
                                // fast enough to return the frame buffers to the MDD in time for the next frame.
                                // This usually happens when there is not enough CPU cycles to go around
                                pPDD->m_dwDroppedFrames++;
                                DEBUGMSG (ZONE_READDATA, (DTAG TEXT("WARNING: MDD did not provide buffer in time for capture, dropped frame %d\r\n"), dwFrameCount));
                            }

                            // Reset the capture buffer
//                            pStart = pPDD->m_pFillBuffer;
                            pStart = pBackupBuffer;
                            pCur   = pStart;
                            pEnd   = pStart + pPDD->m_pCurrentStreamFormat->dwMaxVideoFrameBufferSize;
                            dwFrameCount++;
                            dwFrameState = START_CAP;
                        }
                        break;
    
                    case RE_SYNC:
                        DEBUGMSG (ZONE_READDATA, (DTAG TEXT("Capture State: RE_SYNC\r\n")));

                        // Point to beginning of frame buffer
                        pCur = pStart;

                        // if we got end of frame, next packet is going to be for the new frame
                        if(bPktFlags &  USBVID_PAYLOADHDR_FRAMEEND)
                        {
                            dwFrameState = START_CAP;
                        }
                        break;
        
                    default:
                        DEBUGMSG (ZONE_ERROR, (DTAG TEXT("Unknown Capture State: %x\r\n"), dwFrameState));
                        break;
                }
    
                pDataBuff += dwPacketLen;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            // Exception occurred, most likely due to bad buffer passed in by FillBuffer
            DEBUGMSG (ZONE_ERROR, (DTAG TEXT("Error: Exception in CaptureThread, most likely due to bad buffer from FillBuffer\r\n")));
            pPDD->m_pFillBuffer = NULL;
            pPDD->m_dwFillBytes = 0;
            SetEvent(pPDD->m_hFrameReady);

            // Reset the buffer pointers and frame state
//            pStart = pPDD->m_pFillBuffer;
            pStart = pBackupBuffer;
            pCur   = pStart;
            pEnd   = pStart + pPDD->m_pCurrentStreamFormat->dwMaxVideoFrameBufferSize;
            dwFrameState = RE_SYNC;
        }

        // Queue up another transfer
RestartTrans:
        CloseTransferHandle(pPDD->m_pDrv->lpUsbFuncs, hTransfer[i]);

        if (pPDD->m_fDMASupported)
            CacheRangeFlush( pBufferVirtual[i], (pPDD->m_PddContext.pipeStream.wPacketSize)*TRANSFER_SIZE, CACHE_SYNC_DISCARD);

        for(j = 0; j < TRANSFER_SIZE; j++)
            dwFrameLen[i][j] = pPDD->m_PddContext.pipeStream.wPacketSize;
    
        hTransfer[i]= pPDD->m_pDrv->lpUsbFuncs->lpIssueIsochTransfer (
            pPDD->m_PddContext.pipeStream.hPipe, 
            DefaultTransferComplete, 
            hEvent[i],
            USB_IN_TRANSFER | USB_START_ISOCH_ASAP | USB_SHORT_TRANSFER_OK,
            0,                        // Using USB_START_ISOCH_ASAP precludes the need for a start frame
            TRANSFER_SIZE,            // Number of frames to read
            dwFrameLen[i],            // Array that receives the length of each frame
            pBufferVirtual[i],        // Pointer to transfer buffer
            BufferPhysical[i].LowPart // Physical address of buffer, or NULL if DMA unsupported on this platform
        );

        // goto the next transfer
        if (hTransfer[i])
        {
            i = (i + 1) % SIZE_TRANSFER_QUEUE;
        }
        else
        {
            DEBUGMSG (ZONE_ERROR, (DTAG TEXT("Error: IssueIsochTransfer failed\r\n")));
            dwRet = ERROR_GEN_FAILURE;
            goto Finish;
        }
    } // END capture and process frames

    
Finish:

    for(i = 0; i < SIZE_TRANSFER_QUEUE; i++)
    {
        if(hTransfer[i] != NULL)
            CloseTransferHandle(pPDD->m_pDrv->lpUsbFuncs, hTransfer[i]);
            
        if (hEvent[i] != NULL)
            CloseHandle(hEvent[i]);
    }
    
    if (pBackupBuffer)
        LocalFree( pBackupBuffer );

    DEBUGMSG (ZONE_FUNCTION, (DTAG TEXT("CaptureThread exiting\r\n")));

    return dwRet;
}


DWORD CUSBPdd::SetSensorState( ULONG lModeType, CSSTATE csState )
{
    DWORD dwError = ERROR_SUCCESS;

    DEBUGMSG (ZONE_FUNCTION, (DTAG TEXT("SetSensorState++\r\n")));


    switch ( csState )
    {
        case CSSTATE_STOP:
            DEBUGMSG (ZONE_THSTATE, (DTAG TEXT("CSSTATE_STOP\r\n")));

            // Pause the FillThread immediately
            if(lModeType == CAPTURE) {
                // Block the FillThread from running
                ResetEvent(m_hFillThreadRun);
            }

            // Stop CaptureThread if it's no longer needed
            if((lModeType == CAPTURE && m_CsState[STILL] == CSSTATE_STOP) ||
               (lModeType == STILL && m_CsState[CAPTURE] == CSSTATE_STOP))
            {
                // Terminate the CaptureThread
                if(m_hCaptureThread != NULL) 
                {
                    SetEvent(m_hCaptureThreadExit);
                    WaitForSingleObject(m_hCaptureThread, 2000);
                    CloseHandle(m_hCaptureThread);
                    m_hCaptureThread = NULL;
                }
                // Close the USB pipe opened for streaming
                // If this is not done the camera will not be detected on reboot.
                if (m_PddContext.pipeStream.hPipe != 0)
                {
                     m_pDrv->lpUsbFuncs->lpClosePipe (m_PddContext.pipeStream.hPipe);
                     m_PddContext.pipeStream.hPipe = 0;
                }
            }

            // Stop FillThread if it's no longer needed
            if(lModeType == CAPTURE)
            {
                // Terminate the FillThread
                if(m_hFillThread != NULL) 
                {
                    SetEvent(m_hFillThreadExit);
                    SetEvent(m_hFrameReady);
                    WaitForSingleObject(m_hFillThread, 2000);
                    CloseHandle(m_hFillThread);
                    m_hFillThread = NULL;
                }
            }

            if(lModeType == CAPTURE) {
                m_dwStopTick = GetTickCount();
                DWORD dwSeconds = (m_dwStopTick - m_dwStartTick)/1000;
                if (dwSeconds) {
                    RETAILMSG(TRUE, (L"Ran for %d seconds [ %d minute(s) ]\r\n", dwSeconds, dwSeconds/60));
                    RETAILMSG(TRUE, (L"Frames  per second  = %d\r\n", m_dwCapturedFrames   / dwSeconds));
                    RETAILMSG(TRUE, (L"Dropped per second  = %d\r\n", m_dwDroppedFrames / dwSeconds));
                    m_dwStopTick = m_dwStartTick = 0;
                }
            }

            m_CsState[lModeType] = CSSTATE_STOP;
            break;

        case CSSTATE_PAUSE:
            DEBUGMSG (ZONE_THSTATE, (DTAG TEXT("CSSTATE_PAUSE\r\n")));

            if(lModeType == CAPTURE) {
                // Block the FillThread from running
                ResetEvent(m_hFillThreadRun);
            }

            m_CsState[lModeType] = CSSTATE_PAUSE;
            break;

        case CSSTATE_RUN:
            DEBUGMSG (ZONE_THSTATE, (DTAG TEXT("CSSTATE_RUN\r\n")));

            // Already running?
            if (m_CsState[lModeType] != CSSTATE_RUN)
            {
                BYTE  bFormatIndex, bFrameIndex;
                DWORD dwInterval, dwThreadID;
                int   FoundStream;

                // Map the formats from MDD CS_DATARANGE_VIDEO structure to USB Video Class structure
                if(FALSE == FindVideoInfo(&m_pCurrentFormat[lModeType], lModeType, &bFormatIndex, &bFrameIndex, &dwInterval, &FoundStream))
                {
                    DEBUGMSG(ZONE_ERROR, (DTAG TEXT("Error: Could not find the given format")));
                    return ERROR_INVALID_PARAMETER;
                }

                // Is the current format the same as the format that we need to start running?
                if (m_pCurrentStreamFormat->wFormatType == m_pUSBVideoModes[FoundStream].wFormatType &&
                    m_pCurrentStreamFormat->wFormatIndex == m_pUSBVideoModes[FoundStream].wFormatIndex &&
                    m_pCurrentStreamFormat->wFrameIndex == m_pUSBVideoModes[FoundStream].wFrameIndex &&
                    m_pCurrentStreamFormat->wWidth == m_pUSBVideoModes[FoundStream].wWidth &&
                    m_pCurrentStreamFormat->wHeight == m_pUSBVideoModes[FoundStream].wHeight &&
                    m_pCurrentStreamFormat->bBitsPerPixel == m_pUSBVideoModes[FoundStream].bBitsPerPixel &&
                    m_pCurrentStreamFormat->dwMaxVideoFrameBufferSize == m_pUSBVideoModes[FoundStream].dwMaxVideoFrameBufferSize)
                {
                    // If FillThread is already running, then we can just try to resume.
                    // Otherwise this code will bail out and both FillThread and CaptureThread will be restarted.
                    if(m_hFillThread != NULL)
                    {
                        if(m_ppModeContext[lModeType] != NULL)
                        {
                            // For STILL mode, read one frame from camera and then pause.  Use the current interval setting.
                            if(lModeType == STILL)
                            {
                                m_CsState[lModeType] = CSSTATE_RUN;
                                MDD_HandleIO(m_ppModeContext[lModeType], lModeType);
                                m_CsState[lModeType] = CSSTATE_PAUSE;
                                return ERROR_SUCCESS;
                            }
                            else if (lModeType == CAPTURE && m_pCurrentStreamFormat->dwCurFrameInterval == dwInterval)
                            {
                                // If the Capture interval is the same, then we can just resume.
                                // Otherwise this code will bail out and both FillThread and CaptureThread will be restarted.
                                if(m_hCaptureThread == NULL)
                                {
                                    ResetEvent(m_hCaptureThreadExit);
                                    m_hCaptureThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CaptureThread, (LPVOID)this, 0, &dwThreadID);
                                    if(NULL == m_hCaptureThread)
                                    {
                                        DEBUGMSG(ZONE_ERROR,(DTAG TEXT("CreateThread : Could not create CaptureThread")));
                                        return GetLastError();
                                    }
                                }

                                // Wake up the FillThread
                                m_CsState[lModeType] = CSSTATE_RUN;
                                SetEvent(m_hFillThreadRun);
                                m_dwCapturedFrames = m_dwDroppedFrames = 0;
                                m_dwStartTick = GetTickCount();

                                return ERROR_SUCCESS;
                            }
                        }
                        else
                        {
                            DEBUGMSG(ZONE_ERROR, (DTAG TEXT("Error: m_ppModeContext is NULL when in run state\r\n")));
                            return ERROR_BAD_ENVIRONMENT;
                        }
                    }
                }
                else if((lModeType == CAPTURE && m_CsState[STILL] == CSSTATE_RUN) ||
                        (lModeType == STILL && m_CsState[CAPTURE] == CSSTATE_RUN))
                {
                    // Current format is not the same as the format that we need to start running and someone is running already
                    DEBUGMSG(ZONE_ERROR, (DTAG TEXT("Error: Camera already running in a different format\r\n")));
                    return ERROR_BAD_ENVIRONMENT;
                }

                // If we get here then both Capture and Still are stopped or paused.
                // However, we will need to start/re-start in a different format so stop both FillThread and CaptureThread
                if(m_ppModeContext[lModeType])
                {
                    // Terminate the CaptureThread if it's currently running
                    if(lModeType == CAPTURE)
                    {
                        if(m_hCaptureThread != NULL) 
                        {
                            SetEvent(m_hCaptureThreadExit);
                            WaitForSingleObject(m_hCaptureThread, 2000);
                            CloseHandle(m_hCaptureThread);
                            m_hCaptureThread = NULL;
                        }
                    }

                    // Close the USB pipe opened for streaming
                    // If this is not done the camera will not be detected on reboot.
                    if (m_PddContext.pipeStream.hPipe != 0)
                    {
                        m_pDrv->lpUsbFuncs->lpClosePipe (m_PddContext.pipeStream.hPipe);
                        m_PddContext.pipeStream.hPipe = 0;
                    }

                    // Terminate the FillThread if it's currently running
                    if(m_hFillThread != NULL)
                    {
                        SetEvent(m_hFillThreadExit);
                        SetEvent(m_hFrameReady);
                        WaitForSingleObject(m_hFillThread, 2000);
                        CloseHandle(m_hFillThread);
                        m_hFillThread = NULL;
                    }


                    // Set the new/current format
                    dwError = SetVideoFormat(bFormatIndex, bFrameIndex, dwInterval);
                    if (dwError != ERROR_SUCCESS)
                    {
                        DEBUGMSG(ZONE_ERROR, (DTAG TEXT("Error %d setting the video format for the camera"), dwError));
                        return dwError;
                    }

                    // Copy the set formats to current formats structures
                    memcpy(m_pCurrentStreamFormat, &m_pUSBVideoModes[FoundStream], sizeof(USBVIDEOMODESTRUCT));
                    m_pCurrentStreamFormat->dwCurFrameInterval = dwInterval;

                    // Allocate DMA capture buffers.  We'll attempt to use the previously allocated buffer as much as possible
                    // to reduce memory fragmentation.  What this means is that this buffer will get bigger when a higher
                    // resolution or higer frame rate is selected.  It will not grow smaller.  The buffer will be freed when
                    // the USB camera is unplugged.
                    if ((!m_pBufVirtualCached && !m_pBufVirtualUncached) ||
                        m_ulBufSize < (ULONG)(m_PddContext.pipeStream.wPacketSize * TRANSFER_SIZE * SIZE_TRANSFER_QUEUE))
                    {
                        if (m_pBufVirtualCached && m_pBufVirtualUncached && m_ulBufSize)
                        {
                            FreeContiguousCachedBuffer(
                                m_ulBufSize,
                                &m_pBufVirtualCached,
                                &m_pBufVirtualUncached,
                                &m_BufPhysical);
                        }

                        m_ulBufSize = (m_PddContext.pipeStream.wPacketSize * TRANSFER_SIZE * SIZE_TRANSFER_QUEUE);

                        AllocateContiguousCachedBuffer(
                            m_ulBufSize,
                            &m_pBufVirtualCached,
                            &m_pBufVirtualUncached,
                            &m_BufPhysical);

                        if(m_pBufVirtualCached == NULL || m_pBufVirtualUncached == NULL)
                        {
                            DEBUGMSG (ZONE_ERROR, (DTAG TEXT("Error: could not allocate DMA capture buffers\r\n")));
                            return ERROR_OUTOFMEMORY;
                        }
                    }

                    // Activate CaptureThread
                    ResetEvent(m_hCaptureThreadExit);
                    m_hCaptureThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CaptureThread, (LPVOID)this, 0, &dwThreadID);
                    if(NULL == m_hCaptureThread)
                    {
                        DEBUGMSG(ZONE_ERROR,(DTAG TEXT("CreateThread : Could not create CaptureThread")));
                        return GetLastError();
                    }

                    // For STILL mode read one frame from camera and then pause
                    if(lModeType == STILL)
                    {
                        m_CsState[lModeType] = CSSTATE_RUN;
                        MDD_HandleIO( m_ppModeContext[lModeType], lModeType);
                        m_CsState[lModeType] = CSSTATE_PAUSE;
                        return ERROR_SUCCESS;
                    }
                    else
                    {
                        // if FillThread is not already active
                        if(m_hFillThread == NULL)
                        {
                            ResetEvent(m_hFillThreadExit);
                            m_hFillThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)FillThread, (LPVOID)this, 0, &dwThreadID);
                            if(NULL == m_hFillThread)
                            {
                                DEBUGMSG(ZONE_ERROR,(DTAG TEXT("CreateThread : Could not create FillThread")));
                                return GetLastError();
                            }
                        }

                        // Wake up the FillThread
                        m_CsState[lModeType] = CSSTATE_RUN;
                        SetEvent(m_hFillThreadRun);
                        m_dwCapturedFrames = m_dwDroppedFrames = 0;
                        m_dwStartTick = GetTickCount();
                    }
                }
                else
                {
                    DEBUGMSG(ZONE_ERROR,(DTAG TEXT("Error: m_ppModeContext is NULL when in run state\r\n")));
                    return ERROR_BAD_ENVIRONMENT;
                }
            }
            break;

        default:
            DEBUGMSG( ZONE_IOCTL|ZONE_ERROR, ( _T("SetSensorState(0x%x, 0x%x): Incorrect State\r\n"), lModeType, csState ) );
            dwError = ERROR_INVALID_PARAMETER;
            break;
    }

    return dwError;
}


DWORD CUSBPdd::TakeStillPicture( LPVOID pBurstModeInfo )
{
    DEBUGMSG (ZONE_FUNCTION, (DTAG TEXT("TakeStillPicture++\r\n")));

    // Setting Still to run will automatically take a single frame
    return SetSensorState(STILL, CSSTATE_RUN);
}


//
// Sensor Mode info passes back the formats supported on each pin 
//
DWORD CUSBPdd::GetSensorModeInfo( ULONG ulModeType, PSENSORMODEINFO pSensorModeInfo )
{
    DEBUGMSG (ZONE_FUNCTION, (DTAG TEXT("GetSensorModeInfo++\r\n")));
    
    pSensorModeInfo->MemoryModel     = m_SensorModeInfo[ulModeType].MemoryModel;
    pSensorModeInfo->MaxNumOfBuffers = m_SensorModeInfo[ulModeType].MaxNumOfBuffers;
    pSensorModeInfo->PossibleCount   = m_SensorModeInfo[ulModeType].PossibleCount;
    
    pSensorModeInfo->VideoCaps.DefaultVideoControlCaps = DefaultVideoControlCaps[ulModeType];
    pSensorModeInfo->VideoCaps.CurrentVideoControlCaps = m_pVideoCaps[ulModeType].CurrentVideoControlCaps;
    pSensorModeInfo->pVideoFormat                      = &m_pPinVideoFormats[ulModeType];
    
    return ERROR_SUCCESS;
}


//
// Here if CAPTURE is RUN state then STILL will not be able to set the sensor mode.
// The sensor mode for still will also be set to the current CAPTURE mode.
// Also if STILL is running -- which is not likely the capture will simply return with an error.
// Also if capture is already running -- will return with an error.
//
DWORD CUSBPdd::SetSensorModeFormat( ULONG ulModeType, PCS_DATARANGE_VIDEO pCsDataRangeVideo )
{
    DEBUGMSG (ZONE_FUNCTION, (DTAG TEXT("SetSensorModeFormat++\r\n")));
    

    if(pCsDataRangeVideo == NULL)
        return ERROR_INVALID_PARAMETER;

    // Copy the set formats to current formats structures
    memcpy(&m_pCurrentFormat[ulModeType], pCsDataRangeVideo, sizeof (CS_DATARANGE_VIDEO));

    DEBUGMSG(ZONE_DEVICE, (DTAG TEXT("Set sensor format (%s) to %d x %d x %dbpp @ %d FPS\r\n"),
        ulModeType == CAPTURE ? L"CAPTURE" : L"STILL",
        pCsDataRangeVideo->VideoInfoHeader.bmiHeader.biWidth,
        pCsDataRangeVideo->VideoInfoHeader.bmiHeader.biHeight,
        pCsDataRangeVideo->VideoInfoHeader.bmiHeader.biBitCount,
        1000/(pCsDataRangeVideo->VideoInfoHeader.AvgTimePerFrame/10000) ));

    switch (pCsDataRangeVideo->VideoInfoHeader.bmiHeader.biCompression)
    {
        case FOURCC_UYVY: 
            DEBUGMSG( ZONE_DEVICE, (DTAG TEXT("Sensor format = UYVY\r\n")));
            break;    
        case FOURCC_YUY2: 
            DEBUGMSG( ZONE_DEVICE, (DTAG TEXT("Sensor format = YUY2\r\n")));
            break;    
        case FOURCC_YV12: 
            DEBUGMSG( ZONE_DEVICE, (DTAG TEXT("Sensor format = YV12\r\n")));
            break;    
        case FOURCC_JPEG: 
            DEBUGMSG( ZONE_DEVICE, (DTAG TEXT("Sensor format = JPEG\r\n")));
            break;    
        case FOURCC_MJPG: 
            DEBUGMSG( ZONE_DEVICE, (DTAG TEXT("Sensor format = MJPEG\r\n")));
            break;    
    }
    
    return ERROR_SUCCESS;
}


DWORD CUSBPdd::SetVideoFormat(BYTE bFormatIndex, BYTE bFrameIndex, DWORD dwInterval)
{
    DEBUGMSG (ZONE_FUNCTION, (DTAG TEXT("SetVideoFormat++\r\n")));

    DWORD rc; 
    WORD  wMinQuality, wMaxQuality, wInc; 
    DWORD dwPacketSize;
    
    rc = GetQualityParameters(bFormatIndex, bFrameIndex, dwInterval, &wMinQuality, &wMaxQuality, &wInc);

    // Ensure wInc, wMinQuality, and wMaxQuality are good values
    if (!wInc || wMinQuality == wMaxQuality)
    {
        // check if we can commit to this quality parameter
        if((rc = ProbeCommit(bFormatIndex, bFrameIndex, dwInterval, wMaxQuality, &dwPacketSize)) == 0)
        {
            // we could commit to the above quality
            if((rc = SetStreamInterface(1, dwPacketSize)) == 0)
            {
                // opened the stream interface pipe for above committed format
                DEBUGMSG(ZONE_DEVICE, (DTAG TEXT("SetVideoFormat: bFormatIndex %d, bFrameIndex %d, dwInterval %d (%d fps), wQuality %d (max %d), dwPacketSize %d\r\n"),
                    bFormatIndex,
                    bFrameIndex,
                    dwInterval,
                    1000/(dwInterval/10000),
                    wMaxQuality,
                    wMaxQuality,
                    dwPacketSize
                    ));

                return ERROR_SUCCESS;
            }
        }
    }
    else
    {
        for(WORD wQuality = wMaxQuality; wQuality >= wMinQuality; wQuality = wQuality-wInc)
        {
            // check if we can commit to this quality parameter
            if((rc = ProbeCommit(bFormatIndex, bFrameIndex, dwInterval, wQuality, &dwPacketSize)) == 0)
            {
                // we could commit to the above quality
                if((rc = SetStreamInterface(1, dwPacketSize)) == 0)
                {
                    // opened the stream interface pipe for above committed format
                    DEBUGMSG(ZONE_DEVICE, (DTAG TEXT("SetVideoFormat: bFormatIndex %d, bFrameIndex %d, dwInterval %d (%d fps), wQuality %d (max %d), dwPacketSize %d\r\n"),
                        bFormatIndex,
                        bFrameIndex,
                        dwInterval,
                        1000/(dwInterval/10000),
                        wQuality,
                        wMaxQuality,
                        dwPacketSize
                        ));

                    return ERROR_SUCCESS;
                }
            }
        }
    }

    return rc;
}


//
// Choose the communication endpoint (interface) and set it on the camera
//
BOOL CUSBPdd::SetStreamInterface(BYTE nInterface, DWORD dwPacketSize) 
{
    DEBUGMSG (ZONE_FUNCTION, (DTAG TEXT("SetStreamInterface++\r\n")));
    
    int rc = 0;
    USB_TRANSFER hTransfer;
    PUSBSTRMIF pInterface;

    // Close the pipe if its already open
    if (m_PddContext.pipeStream.hPipe != 0)
    {
         m_pDrv->lpUsbFuncs->lpClosePipe (m_PddContext.pipeStream.hPipe);
    }

    pInterface = FindStreamInterface(nInterface, (WORD)dwPacketSize);

    if(pInterface == 0)
        return ERROR_INVALID_PARAMETER;

    // Setting the chosen interface
    hTransfer = m_pDrv->lpUsbFuncs->lpSetInterface (m_pDrv->hDevice, NULL, NULL, 0, nInterface, pInterface->ifDesc.bAlternateSetting);
    if(hTransfer == NULL)
        return GetLastError();

    rc = CloseTransferHandle (m_pDrv->lpUsbFuncs, hTransfer);
    if(!rc)
        return GetLastError();

    // Open pipe
    m_PddContext.pipeStream.hPipe = m_pDrv->lpUsbFuncs->lpOpenPipe (m_pDrv->hDevice, &pInterface->epDesc);
    if (m_PddContext.pipeStream.hPipe)
    {
        // Copy pipe attributes
        m_PddContext.pipeStream.ucAddr = pInterface->epDesc.bEndpointAddress;
        m_PddContext.pipeStream.wPacketSize = (WORD)dwPacketSize;
    }
    else
    {
        return GetLastError();
    }

    return ERROR_SUCCESS;
}



// The stream interface is chosen based on the size of the endpoint
PUSBSTRMIF CUSBPdd::FindStreamInterface (BYTE nInterface, WORD wPacketSize)
{
    DEBUGMSG (ZONE_FUNCTION, (DTAG TEXT("FindStreamInterface++\r\n")));
    
    int i;
    PUSBSTRMIF lpFound = 0;
    LONG lTranSize;

    // Find the interface
    for (i = 0; i < m_pDrv->pStreamInfo->nStreamInterfaces; i++)
    {
        if (m_pDrv->pStreamInfo->usbstrmIF[i].ifDesc.bInterfaceNumber == nInterface) 
        {
            // Does the interface even have an endpoint?
            if (m_pDrv->pStreamInfo->usbstrmIF[i].fEndpoint)
            {
                // Find the IF with packet size requested -- Always choose the end point with immediate larger packet size
                // Calculate the max number of bytes transfer per microframe
                lTranSize = (m_pDrv->pStreamInfo->usbstrmIF[i].epDesc.wMaxPacketSize & 0x7ff) * 
                            (1 + ((m_pDrv->pStreamInfo->usbstrmIF[i].epDesc.wMaxPacketSize >> 11) & 0x3));
                if (lTranSize >= wPacketSize) 
                {
                    if (lpFound == 0) lpFound = &m_pDrv->pStreamInfo->usbstrmIF[i];
                    if (lTranSize < ((lpFound->epDesc.wMaxPacketSize & 0x7ff) * (1 + ((lpFound->epDesc.wMaxPacketSize >> 11) & 0x3))))
                    {
                        lpFound = &m_pDrv->pStreamInfo->usbstrmIF[i];
                    }
                }
            }
            // See if we want a pipe with no bandwidth
            else if (wPacketSize == 0)
            {
                return &m_pDrv->pStreamInfo->usbstrmIF[i];
            }
        }
    }
    
    return lpFound;
}


int CUSBPdd::ProbeCommit (BYTE bFormatIndex, BYTE bFrameIndex, 
                          DWORD dwFrameInterval, WORD wCompression, PDWORD pdwMaxBandwidth) 
{
    DEBUGMSG (ZONE_FUNCTION, (DTAG TEXT("ProbeCommit++\r\n")));
    
    int rc = 0;
    BYTE bInterface = VID_IF_STREAM;
    BYTE bUnit = 0;
   
    STREAM_PROBE_CONTROLSTRUCT StrProbeCtl;

    // Set probe values
    memset (&StrProbeCtl, 0, sizeof (STREAM_PROBE_CONTROLSTRUCT));
    StrProbeCtl.bFormatIndex = bFormatIndex;
    StrProbeCtl.bFrameIndex = bFrameIndex;
    StrProbeCtl.dwFrameInterval = dwFrameInterval;
    StrProbeCtl.wCompQuality = wCompression;
    StrProbeCtl.bmHint = USB_VIDEO_PROBE_HINT_FRAMEINTERVAL;

    // Set these values
    rc = DoVendorTransfer (USBVID_SET_CUR, USB_VIDEO_VS_CS_PROBE_CTL, bInterface, bUnit, (PBYTE)&StrProbeCtl, 0x1A);
    if (rc)
    {
        DEBUGMSG (ZONE_ERROR, (DTAG TEXT("Error code 0x%x setting probe values.\r\n"), rc));
        return rc;
    }

    // Get probe values
    rc = DoVendorTransfer (USBVID_GET_CUR, USB_VIDEO_VS_CS_PROBE_CTL, bInterface, bUnit, (PBYTE)&StrProbeCtl, 0x1A);
    if (rc)
    {
        DEBUGMSG (ZONE_ERROR, (DTAG TEXT("Error code 0x%x getting final probe values.\r\n"), rc));
        return rc;
    }

    // Commit the probed values from the camera
    rc = DoVendorTransfer (USBVID_SET_CUR, USB_VIDEO_VS_CS_COMMIT_CTL, bInterface, bUnit, (PBYTE)&StrProbeCtl, 0x1A);
    if (rc)
    {
        DEBUGMSG (ZONE_ERROR, (DTAG TEXT("Error code 0x%x setting probe values.\r\n"), rc));
        return rc;
    }

    // Set the max packet size for each transfer from the given data
    *pdwMaxBandwidth = StrProbeCtl.dwMaxPayloadTransferSize;

     return rc;
}


int CUSBPdd::GetQualityParameters (BYTE bFormatIndex, BYTE bFrameIndex, DWORD dwFrameInterval,
                      WORD *pwMinQuality, WORD *pwMaxQuality, WORD *pwInc) 
{
    DEBUGMSG (ZONE_FUNCTION, (DTAG TEXT("GetQualityParameters++\r\n")));
    
    int rc = 0;
    BYTE bInterface = VID_IF_STREAM;
    BYTE bUnit = 0;
        
    STREAM_PROBE_CONTROLSTRUCT StrProbeCtl;

    // Minimum Quality Values
    memset(&StrProbeCtl, 0, sizeof(STREAM_PROBE_CONTROLSTRUCT));
    StrProbeCtl.bFormatIndex = bFormatIndex;
    StrProbeCtl.bFrameIndex = bFrameIndex;
    StrProbeCtl.dwFrameInterval = dwFrameInterval;

    rc = DoVendorTransfer(USBVID_GET_MIN, USB_VIDEO_VS_CS_PROBE_CTL, bInterface, bUnit, (PBYTE)&StrProbeCtl, 0x1A); 

    if(rc)
    {
        DEBUGMSG (ZONE_ERROR, (DTAG TEXT("Error 0x%x getting min values\r\n"), rc));
        DEBUGMSG (ZONE_ERROR, (DTAG TEXT("Camera may not support getting min values\r\n")));
        goto Error;
    }

    *pwMinQuality = StrProbeCtl.wCompQuality;

    // Max Quality Values
    memset(&StrProbeCtl, 0, sizeof(STREAM_PROBE_CONTROLSTRUCT));
    StrProbeCtl.bFormatIndex = bFormatIndex;
    StrProbeCtl.bFrameIndex = bFrameIndex;
    StrProbeCtl.dwFrameInterval = dwFrameInterval;

    rc = DoVendorTransfer(USBVID_GET_MAX, USB_VIDEO_VS_CS_PROBE_CTL, bInterface, bUnit, (PBYTE)&StrProbeCtl, 0x1A); 

    if(rc)
    {
        DEBUGMSG (ZONE_ERROR, (DTAG TEXT("Error 0x%x getting max values\r\n"), rc));
        DEBUGMSG (ZONE_ERROR, (DTAG TEXT("Camera may not support getting max values\r\n")));
        goto Error;
    }

    *pwMaxQuality = StrProbeCtl.wCompQuality;

   
    // Get Increments  
    memset(&StrProbeCtl, 0, sizeof(STREAM_PROBE_CONTROLSTRUCT));
    StrProbeCtl.bFormatIndex = bFormatIndex;
    StrProbeCtl.bFrameIndex = bFrameIndex;
    StrProbeCtl.dwFrameInterval = dwFrameInterval;

    rc = DoVendorTransfer(USBVID_GET_RES, USB_VIDEO_VS_CS_PROBE_CTL, bInterface, bUnit, (PBYTE)&StrProbeCtl, 0x1A); 

    if(rc)
    {
        DEBUGMSG (ZONE_ERROR, (DTAG TEXT("Error 0x%x getting res values\r\n"), rc));
        DEBUGMSG (ZONE_ERROR, (DTAG TEXT("Camera may not support getting res values\r\n")));
        goto Error;
    }

    *pwInc = StrProbeCtl.wCompQuality;

    DEBUGMSG(ZONE_DEVICE, (DTAG TEXT("Quality: bFormatIndex %d, bFrameIndex %d, min %d, max %d, increment %d\r\n"), 
        bFormatIndex, bFrameIndex, *pwMinQuality, *pwMaxQuality, *pwInc));
           
    return ERROR_SUCCESS;

Error:
    *pwInc = *pwMinQuality = *pwMaxQuality = 0;
    return ERROR_SUCCESS;
}


//
// Re-engineer the formatID and FrameID of the format based on the given CSDATARANGE STRUCTURE. 
// In future this computation can be changed
BOOL CUSBPdd::FindVideoInfo(const PCS_DATARANGE_VIDEO pCsDataRangeVideo, ULONG ulModeType, BYTE *pbFormatIndex, BYTE *pbFrameIndex, DWORD  *pdwInterval, int *pFoundStream)
{
    DEBUGMSG (ZONE_FUNCTION, (DTAG TEXT("FindVideoInfo++\r\n")));
    
    int i;
    const GUID YUY2[] = MEDIASUBTYPE_YUY2;
    const GUID MJPG[] = MEDIASUBTYPE_MJPG;
    const GUID IJPG[] = MEDIASUBTYPE_IJPG;
    WORD wFormatType;

    // Find out the format type
    if (memcmp(YUY2, &(pCsDataRangeVideo->DataRange.SubFormat), sizeof(GUID)) == 0)
    {
        wFormatType = USB_VIDEO_VS_FORMAT_UNCOMPRESSED;
    }
    else if (memcmp(MJPG, &(pCsDataRangeVideo->DataRange.SubFormat), sizeof(GUID)) == 0 ||
             memcmp(IJPG, &(pCsDataRangeVideo->DataRange.SubFormat), sizeof(GUID)) == 0)
    {
        wFormatType = USB_VIDEO_VS_FORMAT_MJPEG;
    }
    else
    {
        wFormatType = USB_VIDEO_VS_UNDEFINED;
    }

    for(i = 0; i < m_nUSBVideoModes; i++)
    {
        if((m_pUSBVideoModes[i].wFormatType   == wFormatType) &&
           (m_pUSBVideoModes[i].wWidth        == pCsDataRangeVideo->VideoInfoHeader.bmiHeader.biWidth) &&
           (m_pUSBVideoModes[i].wHeight       == pCsDataRangeVideo->VideoInfoHeader.bmiHeader.biHeight) &&
           (m_pUSBVideoModes[i].bBitsPerPixel == pCsDataRangeVideo->VideoInfoHeader.bmiHeader.biBitCount))
        {
            *pFoundStream  = i;
            *pbFormatIndex = (BYTE) m_pUSBVideoModes[i].wFormatIndex;
            *pbFrameIndex  = (BYTE) m_pUSBVideoModes[i].wFrameIndex;
            *pdwInterval   = (DWORD)pCsDataRangeVideo->VideoInfoHeader.AvgTimePerFrame;
    
            return TRUE;
        }
    }
    
    return FALSE;
}


//
// DirectShow may pass us buffers to hold video frames.
// This lets us capture directly to client memory (generally provided by the
// display driver).
//
DWORD CUSBPdd::RegisterClientBuffer( ULONG ulModeType, PVOID pBuffer )
{
    DEBUGMSG (ZONE_FUNCTION, (DTAG TEXT("RegisterClientBuffer++ (Not Implemented)\r\n")));

    return ERROR_SUCCESS;
}


//
// Function not used
//
DWORD CUSBPdd::UnRegisterClientBuffer( ULONG ulModeType, PVOID pBuffer )
{
    DEBUGMSG (ZONE_FUNCTION, (DTAG TEXT("UnregisterClientBuffer++ (Not Implemented)\r\n")));
    
    return ERROR_SUCCESS;
}


//
// This driver dos not support custom properties
//
DWORD CUSBPdd::HandleSensorModeCustomProperties( ULONG ulModeType, PUCHAR pInBuf, DWORD  InBufLen, PUCHAR pOutBuf, DWORD  OutBufLen, PDWORD pdwBytesTransferred )
{
    DEBUGMSG( ZONE_IOCTL, ( _T("IOControl: Unsupported PropertySet Request\r\n")) );
    
    return ERROR_NOT_SUPPORTED;
}


//
// This function is called by the MDD, when interrupted. The MDD expects the video frame in pImage
//
DWORD CUSBPdd::FillBuffer( ULONG ulModeType, PUCHAR pImage )
{
    DEBUGMSG (ZONE_FUNCTION, (DTAG TEXT("FillBuffer++\r\n")));
        

    // Check to see if the PIN is in RUN state
    if(m_CsState[ulModeType] != CSSTATE_RUN) {
        DEBUGMSG(ZONE_WARN, (DTAG TEXT("WARNING: FillBuffer() called, but camera not in RUN state.\r\n")));
        return 0;
    }

    // Tell CaptureThread to use this buffer
    m_dwFillBytes = 0;
    m_pFillBuffer = pImage;

    // Block until frame delivered by CaptureThread, or signaled to exit
    ResetEvent(m_hFrameReady);
    WaitForSingleObject(m_hFrameReady, INFINITE);
    DEBUGMSG (ZONE_FUNCTION, (DTAG TEXT("----------------------------------\r\n")));
    DEBUGMSG (ZONE_FUNCTION, (DTAG TEXT("FillBuffer() got frame from camera\r\n")));
    DEBUGMSG (ZONE_FUNCTION, (DTAG TEXT("----------------------------------\r\n")));

    return m_dwFillBytes;
}


//
// Returns the error code control from the camera
//
int CUSBPdd::GetCameraError(BYTE *pErr)
{
    int rc = 0;
    BYTE bInterface = VID_IF_CTL;
    BYTE bUnit = 0;

    DEBUGMSG (ZONE_FUNCTION, (DTAG TEXT("GetCameraError++\r\n")));

    *pErr = 0;
//    rc = DoVendorTransfer (USBVID_GET_CUR, USB_VIDEO_VS_CS_STREAM_ERROR_CODE_CTL,
//                           bInterface, bUnit, pErr, 1);
    rc = DoVendorTransfer (USBVID_GET_CUR, USB_VIDEO_VC_CS_REQUEST_ERROR_CODE_CONTROL,
                           bInterface, bUnit, pErr, 1);

    if (rc)
        DEBUGMSG (ZONE_ERROR, (DTAG TEXT("Error reading error code from camera rc %d\r\n"), rc));

    DEBUGMSG (ZONE_FUNCTION, (DTAG TEXT("GetCameraError-- rc %d error code %d\r\n"), rc, *pErr));
    return rc;
}


BOOL CUSBPdd::ReadConfigFromRegistry()
{
    DEBUGMSG (ZONE_FUNCTION, (DTAG TEXT("ReadConfigFromRegistry++\r\n")));
    
    HKEY  hKey = 0;
    DWORD dwType  = 0;
    DWORD dwSize  = sizeof ( DWORD );
    DWORD dwValue = -1;


    if( ERROR_SUCCESS != RegOpenKeyEx( HKEY_LOCAL_MACHINE, L"Drivers\\USB\\ClientDrivers\\Video_Class", 0, 0, &hKey ))
    {
        return false;
    }

    if( ERROR_SUCCESS == RegQueryValueEx( hKey, L"MemoryModel", 0, &dwType, (BYTE *)&dwValue, &dwSize ) )
    {
        if(   ( REG_DWORD == dwType ) 
           && ( sizeof( DWORD ) == dwSize ) 
           && (( dwValue == CSPROPERTY_BUFFER_DRIVER ) || ( dwValue == CSPROPERTY_BUFFER_CLIENT_LIMITED ) || ( dwValue == CSPROPERTY_BUFFER_CLIENT_UNLIMITED )))
        {
            for( int i=0; i<MAX_SUPPORTED_PINS ; i++ )
            {
                m_SensorModeInfo[i].MemoryModel = (CSPROPERTY_BUFFER_MODE) dwValue;
            }
        }
    }

    // Find out if we should be using some other number of supported modes. The only
    // valid options are 2 or 3. Default to 2.
    dwSize = sizeof(DWORD);
    if ( ERROR_SUCCESS == RegQueryValueEx( hKey, L"PinCount", 0, &dwType, (BYTE *)&dwValue, &dwSize ) )
    {
        if ( REG_DWORD == dwType
             && sizeof ( DWORD ) == dwSize
             && 3 == dwValue )
        {
            m_ulCTypes = 3;
        }
    }

    dwSize = sizeof(DWORD);
    if( ERROR_SUCCESS == RegQueryValueEx( hKey, L"PreferredWidth", 0, &dwType, (BYTE *)&dwValue, &dwSize ) )
    {
        if(  ( REG_DWORD == dwType       ) && 
             ( sizeof(DWORD) == dwSize   )
          )
        {
            m_dwPreferredWidth = dwValue;
            DEBUGMSG (ZONE_INIT, (DTAG TEXT("PreferredWidth = %d\r\n"), dwValue));
        }
    }
        
    dwSize = sizeof(DWORD);
    if( ERROR_SUCCESS == RegQueryValueEx( hKey, L"PreferredHeight", 0, &dwType, (BYTE *)&dwValue, &dwSize ) )
    {
        if(  ( REG_DWORD == dwType       ) && 
             ( sizeof(DWORD) == dwSize   )
          )
        {
            m_dwPreferredHeight = dwValue;
            DEBUGMSG (ZONE_INIT, (DTAG TEXT("PreferredHeight = %d\r\n"), dwValue));
        }
    }

    dwSize = sizeof(DWORD);
    if( ERROR_SUCCESS == RegQueryValueEx( hKey, L"MJPEGSupport", 0, &dwType, (BYTE *)&dwValue, &dwSize ) )
    {
        if(  ( REG_DWORD == dwType       ) && 
             ( sizeof(DWORD) == dwSize   )
          )
        {
            m_bMJPEGSupported = (dwValue ? TRUE : FALSE);
            DEBUGMSG (ZONE_INIT, (DTAG TEXT("MJPEG Support = %d\r\n"), dwValue));
        }
    }

    dwSize = sizeof(DWORD);
    if( ERROR_SUCCESS == RegQueryValueEx( hKey, L"UncompressedSupport", 0, &dwType, (BYTE *)&dwValue, &dwSize ) )
    {
        if(  ( REG_DWORD == dwType       ) && 
             ( sizeof(DWORD) == dwSize   )
          )
        {
            m_bUncompressedSupported = (dwValue ? TRUE : FALSE);
            DEBUGMSG (ZONE_INIT, (DTAG TEXT("Uncompressed Support = %d\r\n"), dwValue));
        }
    }

    RegCloseKey( hKey );
    
    return true;
}



//
// Reads the Drv Context passed by the DeviceAttach function to the stream driver form registry
//
PDRVCONTEXT GetRegData (PVOID dwContext)
{
    int nLen, rc;
    DWORD dwLen, dwType, dwSize = 0;
    HKEY hKey;
    PDRVCONTEXT pDrv = 0;

    nLen = 0;
    
    rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE,(LPTSTR)dwContext,0, 0, &hKey);

    if(rc)
    {
        DEBUGMSG (ZONE_ERROR, (DTAG TEXT("Could not open the registry\r\n")));
        return pDrv;
    }
        

    dwLen = sizeof(pDrv);
    rc = RegQueryValueEx (hKey, TEXT("ClientInfo"), NULL, &dwType, (PBYTE)&pDrv, &dwLen);
    RegCloseKey(hKey);

    if(rc || dwType != REG_DWORD)
    {
        DEBUGMSG (ZONE_ERROR, (DTAG TEXT("Error reading the registry\r\n")));
        return pDrv;
    }


    __try 
    {
        if (pDrv->dwSize != sizeof (DRVCONTEXT))
            pDrv = NULL;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) 
    {
        pDrv = NULL;
    }


    return pDrv;
}


//
// SetPower - Sets the camera power 
// 
DWORD CUSBPdd::SetPowerState (CEDEVICE_POWER_STATE PowerState) 
{
    DEBUGMSG (ZONE_FUNCTION, (DTAG TEXT("SetPower++\r\n")));
    
    int rc = 0;
    BYTE bDevicePowerMode;
    BYTE bInterface = VID_IF_CTL;
    BYTE bUnit = 0;

    if (m_PowerState == PowerState) // Its already ON
        return ERROR_SUCCESS;
    

    if(PowerState == D1)
        bDevicePowerMode = 0x00;     // Full Power Mode
    
    rc = DoVendorTransfer (USBVID_SET_CUR, USB_VIDEO_VC_CS_VIDEO_POWER_MODE_CONTROL, bInterface, bUnit, (PBYTE)&bDevicePowerMode, 1);

    if (rc)
    {
        DEBUGMSG (ZONE_ERROR, (DTAG TEXT("Error code 0x%x setting camera power\r\n"), rc));
        DEBUGMSG (ZONE_ERROR, (DTAG TEXT("Camera may not support setting power\r\n")));
    }

    m_PowerState = PowerState;
    return rc;
}


//
// This function queries the Video Streaming (VS) Interface for supported 
// video CAPTURE formats (driver will report the same formats for STILL 
// mode as well)
//
// NOTE: 
//   If you want to understand this code, refer to the USB Device Class
//   Definition for Video Devices, Figure 3-1: Video Camera Descriptor Layout
//
int CUSBPdd::QueryFormats()
{
    DEBUGMSG (ZONE_FUNCTION, (DTAG TEXT("QueryFormats++\r\n")));


    //---------------------------------------------------------------------------
    // QUERY SUPPORTED CAMERA FORMATS
    //
    // (filter: we ignore MJPEG and/or Uncompressed if disabled in the registry)
    //---------------------------------------------------------------------------

    m_nUSBVideoFormatDesc                      = 0;
    m_nUSBVideoFrameDesc                       = 0;
    m_nUSBVideoModes                           = 0;

    // Read the Video Stream (VS) Interface Input Header Descriptor (USB Video Class 3.9.2.1)
    PUSBVIDSTREAMIFDESCRIPTOR pHdr = (PUSBVIDSTREAMIFDESCRIPTOR)m_pDrv->pStreamInfo->usbstrmIF->lpepExtDesc;

    __try 
    {
        // Sanity check the VS Interface Input Header
        if ((pHdr->bType != USB_VIDEO_CS_INTERFACE) || (pHdr->bSubtype != USB_VIDEO_VS_INPUT_HEADER)) 
        {
            DEBUGMSG (ZONE_ERROR, (DTAG TEXT("Bad Video Streaming Interface Input Header\r\n")));
            return ERROR_INVALID_PARAMETER;
        }
    
        // Point to first descriptor after Input Header, which will be a Format Descriptor
        PBYTE pData = (PBYTE)m_pDrv->pStreamInfo->usbstrmIF->lpepExtDesc + pHdr->bLen;
        PBYTE pEnd  = (PBYTE)m_pDrv->pStreamInfo->usbstrmIF->lpepExtDesc + pHdr->wTotalLen;
        PUSBVIDSTREAMIF_FORMATDESCRIPTOR pFmt = (PUSBVIDSTREAMIF_FORMATDESCRIPTOR)((PBYTE)pData);
    
   
        // Read each Format Descriptor (USB Video Class 3.9.2.3),
        // skipping over the Frame Descriptors that follow them (USB Video Class 3.9.2.4)
        while (pData + pFmt->bLen < pEnd)
        {
            if (pFmt->bType != USB_VIDEO_CS_INTERFACE)
                break;
    

            switch (pFmt->bSubtype) 
            {
                case USB_VIDEO_VS_FORMAT_MJPEG:         // 0x06
                    DEBUGMSG(ZONE_DEVICE, (DTAG TEXT("Camera supports USB_VIDEO_VS_FORMAT_MJPEG\r\n")));
                    if (m_bMJPEGSupported) 
                    {
                        m_nUSBVideoFormatDesc++;
                        m_nUSBVideoFrameDesc += pFmt->bNumFrameDescriptors;
                    }
                    break;
                    
                case USB_VIDEO_VS_FORMAT_UNCOMPRESSED:  // 0x04
                    DEBUGMSG(ZONE_DEVICE, (DTAG TEXT("Camera supports USB_VIDEO_VS_FORMAT_UNCOMPRESSED\r\n")));
                    if (m_bUncompressedSupported) 
                    {
                        m_nUSBVideoFormatDesc++;
                        m_nUSBVideoFrameDesc += pFmt->bNumFrameDescriptors;
                    }
                    break;

                default:
                    // Must be a Frame Descriptor; we'll look at them later.
                    break;
            }

            pData += pFmt->bLen;
            pFmt = (PUSBVIDSTREAMIF_FORMATDESCRIPTOR)((PBYTE)pData);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        DEBUGMSG (ZONE_ERROR, (TEXT("Exception scanning extended stream descriptor\r\n")));
        return -2;
    }


    //---------------------------------------------------------------------------
    // ALLOCATE USB MODE TABLES
    //---------------------------------------------------------------------------

    // Allocate memory only for ENABLED modes 
    // (MJPEG and/or Uncompressed, as filtered by registry setting)
    m_nUSBVideoModes = m_nUSBVideoFrameDesc;
    m_pUSBVideoModes = new USBVIDEOMODESTRUCT[m_nUSBVideoModes];
    if( NULL == m_pUSBVideoModes)
    {
        return ERROR_INSUFFICIENT_BUFFER;
    }


    m_pPinVideoFormats = new PINVIDEOFORMAT[m_ulCTypes];
    if( NULL == m_pPinVideoFormats )
    {
        DEBUGMSG(ZONE_ERROR, (DTAG TEXT("Insufficient memory\r\n"))); 
        return ERROR_INSUFFICIENT_BUFFER;
    }

    m_pPinVideoFormats[CAPTURE].categoryGUID    = PINNAME_VIDEO_CAPTURE;
    m_pPinVideoFormats[CAPTURE].ulAvailFormats  = 0;

    m_pPinVideoFormats[STILL  ].categoryGUID    = PINNAME_VIDEO_STILL;
    m_pPinVideoFormats[STILL  ].ulAvailFormats  = 0;



    //---------------------------------------------------------------------------
    // QUERY SUPPORTED CAMERA FRAME TYPES
    //
    // (filter: we ignore MJPEG and/or Uncompressed if disabled in the registry)
    //---------------------------------------------------------------------------

    PUSBVIDSTREAMIF_UNFORMATDESCRIPTOR      pUncompressedFormat;
    PUSBVIDSTREAMIF_UNFRAMEDESCRIPTOR       pUncompressedFrame;
    PUSBVIDSTREAMIF_MJPEGFORMATDESCRIPTOR   pMJPEGFormat;
    PUSBVIDSTREAMIF_MJPEGFRAMEDESCRIPTOR    pMJPEGFrame;


    // Re-read the Format Descriptors we just read (USB Video Class 3.9.2.3)
    // Only this time we grab the Frame Descriptors as well (USB Video Class 3.9.2.4)
    // Build up a table of all modes [Formats * Frames * Intervals]
    // Example: MJPEG 640x480 @ 5, 15, and 30fps == 3 modes

    // Point to first descriptor after Input Header, which will be a Format Descriptor
    PBYTE pData = (PBYTE)m_pDrv->pStreamInfo->usbstrmIF->lpepExtDesc + pHdr->bLen;
    PBYTE pEnd  = (PBYTE)m_pDrv->pStreamInfo->usbstrmIF->lpepExtDesc + pHdr->wTotalLen;
    PUSBVIDSTREAMIF_FORMATDESCRIPTOR pFmt = (PUSBVIDSTREAMIF_FORMATDESCRIPTOR)((PBYTE)pData);
    DWORD dwIndex = 0;

    while (pData + pFmt->bLen < pEnd)
    {
        if (pFmt->bType != USB_VIDEO_CS_INTERFACE)
            break;

        
        switch (pFmt->bSubtype) 
        {
            case USB_VIDEO_VS_FORMAT_MJPEG:         // 0x06
                if (!m_bMJPEGSupported)
                    break;   

                pMJPEGFormat = (PUSBVIDSTREAMIF_MJPEGFORMATDESCRIPTOR)pFmt;
                pMJPEGFrame  = (PUSBVIDSTREAMIF_MJPEGFRAMEDESCRIPTOR)((PBYTE)pFmt+pFmt->bLen);
  
                // Copy video format data into the driver's array.
                for (int i = 0; i < pMJPEGFormat->bNumFrameDescriptors; i++, dwIndex++)
                {
                    // Copy all the relevant information to a USBVIDEOMODESTRUCT
                    m_pUSBVideoModes[dwIndex].cbSize                    = sizeof (USBVIDEOMODESTRUCT);
                    m_pUSBVideoModes[dwIndex].wFormatType               = USB_VIDEO_VS_FORMAT_MJPEG;
                    m_pUSBVideoModes[dwIndex].wFormatIndex              = pMJPEGFormat->bFormatIndex;
                    m_pUSBVideoModes[dwIndex].wFrameIndex               = pMJPEGFrame->bFrameIndex;
                    m_pUSBVideoModes[dwIndex].wHeight                   = pMJPEGFrame->wHeight;
                    m_pUSBVideoModes[dwIndex].wWidth                    = pMJPEGFrame->wWidth;
                    m_pUSBVideoModes[dwIndex].bBitsPerPixel             = 16;
                    m_pUSBVideoModes[dwIndex].dwMaxVideoFrameBufferSize = pMJPEGFrame->dwMaxVideoFrameBufferSize;
                    m_pUSBVideoModes[dwIndex].dwDefaultFrameInterval    = pMJPEGFrame->dwDefaultFrameInterval;

                    DEBUGMSG (ZONE_DEVICE, (TEXT("\tMJPEG Format[%d] Width: %d, Height %d, Max Buf: 0x%x\r\n"),
                                dwIndex,
                                pMJPEGFrame->wWidth,
                                pMJPEGFrame->wHeight,
                                pMJPEGFrame->dwMaxVideoFrameBufferSize));
    
                    if (pMJPEGFrame->bFrameIntervalType != 0)
                    {
                        // There is a table of N discrete frame intervals to pick from
                        m_pUSBVideoModes[dwIndex].nNumInterval = pMJPEGFrame->bFrameIntervalType;
                        for (int k = 0; k < pMJPEGFrame->bFrameIntervalType; k++)
                        {
                            m_pPinVideoFormats[CAPTURE].ulAvailFormats++;
                            m_pUSBVideoModes[dwIndex].dwInterval[k] = pMJPEGFrame->Interval.dwDescrete[k];

                            DEBUGMSG (ZONE_DEVICE, (TEXT("\tdwInterval = %d (%d FPS)\r\n"), 
                                m_pUSBVideoModes[dwIndex].dwInterval[k],
                                1000/(m_pUSBVideoModes[dwIndex].dwInterval[k]/10000)));
                        }
                    }
                    else
                    {
                        // There is a continuous range of frame intervals from Min to Max at a given Stepping
                        m_pUSBVideoModes[dwIndex].nNumInterval = -1;
                        m_pUSBVideoModes[dwIndex].dwInterval[0] = pMJPEGFrame->Interval.strCont.dwMinFrameInterval;
                        m_pUSBVideoModes[dwIndex].dwInterval[1] = pMJPEGFrame->Interval.strCont.dwMaxFrameInterval;
                        m_pUSBVideoModes[dwIndex].dwInterval[2] = pMJPEGFrame->Interval.strCont.dwFrameIntervalStep;
                        m_pPinVideoFormats[CAPTURE].ulAvailFormats += ((m_pUSBVideoModes[dwIndex].dwInterval[1] - m_pUSBVideoModes[dwIndex].dwInterval[0])/m_pUSBVideoModes[dwIndex].dwInterval[2]);

                        DEBUGMSG (ZONE_DEVICE, (TEXT("\tdwInterval Min  = %d\r\n"), 
                            m_pUSBVideoModes[dwIndex].dwInterval[0]));
                        DEBUGMSG (ZONE_DEVICE, (TEXT("\tdwInterval Max  = %d\r\n"), 
                            m_pUSBVideoModes[dwIndex].dwInterval[1]));
                        DEBUGMSG (ZONE_DEVICE, (TEXT("\tdwInterval Step = %d\r\n"), 
                            m_pUSBVideoModes[dwIndex].dwInterval[2]));
                    }

                    DEBUGMSG (ZONE_DEVICE, (TEXT("\tdefault    = %d\r\n"), 
                        m_pUSBVideoModes[dwIndex].dwDefaultFrameInterval));

                    // Does this match the preferred format specified in registry?
                    // If so, make it the default (mode index 0).
                    if (pMJPEGFrame->wWidth == m_dwPreferredWidth && pMJPEGFrame->wHeight== m_dwPreferredHeight)
                    {
                        memcpy( &m_pUSBVideoModes[0], &m_pUSBVideoModes[dwIndex], sizeof(USBVIDEOMODESTRUCT));
                    }

                    pMJPEGFrame = (PUSBVIDSTREAMIF_MJPEGFRAMEDESCRIPTOR)((PBYTE)pMJPEGFrame + pMJPEGFrame->bLen);
                }
                break;

            // Copy everything for the uncompressed format if the camera supports streaming uncompressed data
            case USB_VIDEO_VS_FORMAT_UNCOMPRESSED:
                if (!m_bUncompressedSupported)
                    break;

                pUncompressedFormat = (PUSBVIDSTREAMIF_UNFORMATDESCRIPTOR)pFmt;
                pUncompressedFrame  = (PUSBVIDSTREAMIF_UNFRAMEDESCRIPTOR)((PBYTE)pFmt+pFmt->bLen);
            

                for (int i = 0; i < pUncompressedFormat->bNumFrameDescriptors; i++, dwIndex++)
                {
                    // Copy all the relevant information to a USBVIDEOMODESTRUCT
                    m_pUSBVideoModes[dwIndex].cbSize                    = sizeof (USBVIDEOMODESTRUCT);
                    m_pUSBVideoModes[dwIndex].wFormatType               = USB_VIDEO_VS_FORMAT_UNCOMPRESSED;
                    m_pUSBVideoModes[dwIndex].wFormatIndex              = pUncompressedFormat->bFormatIndex;
                    m_pUSBVideoModes[dwIndex].wFrameIndex               = pUncompressedFrame->bFrameIndex;
                    m_pUSBVideoModes[dwIndex].wHeight                   = pUncompressedFrame->wHeight;
                    m_pUSBVideoModes[dwIndex].wWidth                    = pUncompressedFrame->wWidth;
                    m_pUSBVideoModes[dwIndex].bBitsPerPixel             = pUncompressedFormat->bBitsPerPixel;
                    m_pUSBVideoModes[dwIndex].dwMaxVideoFrameBufferSize = pUncompressedFrame->dwMaxVideoFrameBufferSize;
                    m_pUSBVideoModes[dwIndex].dwDefaultFrameInterval    = pUncompressedFrame->dwDefaultFrameInterval;

                    DEBUGMSG (ZONE_DEVICE, (TEXT("\tUncompressed Format[%d] Width: %d, Height %d, Max Buf: 0x%x\r\n"),
                                dwIndex,
                                pUncompressedFrame->wWidth,
                                pUncompressedFrame->wHeight,
                                pUncompressedFrame->dwMaxVideoFrameBufferSize));

                    if (pUncompressedFrame->bFrameIntervalType != 0)
                    {
                        // There is a table of N discrete frame intervals to pick from
                        m_pUSBVideoModes[dwIndex].nNumInterval = pUncompressedFrame->bFrameIntervalType;
                        for (int k = 0; k < pUncompressedFrame->bFrameIntervalType; k++)
                        {
                            m_pPinVideoFormats[CAPTURE].ulAvailFormats++;
                            m_pUSBVideoModes[dwIndex].dwInterval[k] = pUncompressedFrame->Interval.dwDescrete[k];

                            DEBUGMSG (ZONE_DEVICE, (TEXT("\tdwInterval = %d\r\n"), 
                                m_pUSBVideoModes[dwIndex].dwInterval[k]));
                        }
                    }
                    else
                    {
                        // Continous range of intervals: given by min, max, and stepping
                        m_pUSBVideoModes[dwIndex].nNumInterval = -1;
                        m_pUSBVideoModes[dwIndex].dwInterval[0] = pUncompressedFrame->Interval.strCont.dwMinFrameInterval;
                        m_pUSBVideoModes[dwIndex].dwInterval[1] = pUncompressedFrame->Interval.strCont.dwMaxFrameInterval;
                        m_pUSBVideoModes[dwIndex].dwInterval[2] = pUncompressedFrame->Interval.strCont.dwFrameIntervalStep;
                        m_pPinVideoFormats[CAPTURE].ulAvailFormats += ((m_pUSBVideoModes[dwIndex].dwInterval[1] - m_pUSBVideoModes[dwIndex].dwInterval[0])/m_pUSBVideoModes[dwIndex].dwInterval[2]);

                        DEBUGMSG (ZONE_DEVICE, (TEXT("\tdwInterval Min  = %d\r\n"), 
                            m_pUSBVideoModes[dwIndex].dwInterval[0]));
                        DEBUGMSG (ZONE_DEVICE, (TEXT("\tdwInterval Max  = %d\r\n"), 
                            m_pUSBVideoModes[dwIndex].dwInterval[1]));
                        DEBUGMSG (ZONE_DEVICE, (TEXT("\tdwInterval Step = %d\r\n"), 
                            m_pUSBVideoModes[dwIndex].dwInterval[2]));
                    }

                    DEBUGMSG (ZONE_DEVICE, (TEXT("\tdefault    = %d\r\n"), 
                        m_pUSBVideoModes[dwIndex].dwDefaultFrameInterval));
        
                    // Does this match the preferred format specified in registry?
                    // If so, make it the default (mode index 0).
                    if (pUncompressedFrame->wWidth == m_dwPreferredWidth && pUncompressedFrame->wHeight== m_dwPreferredHeight)
                    {
                        memcpy( &m_pUSBVideoModes[0], &m_pUSBVideoModes[dwIndex], sizeof(USBVIDEOMODESTRUCT));
                    }

                    pUncompressedFrame = (PUSBVIDSTREAMIF_UNFRAMEDESCRIPTOR)((PBYTE)pUncompressedFrame + pUncompressedFrame->bLen);
                }
                break;

            default:
                break;
        }

        pData += pFmt->bLen;
        pFmt = (PUSBVIDSTREAMIF_FORMATDESCRIPTOR)((PBYTE)pData);
    }
    

    //--------------------------------------------------------------------------------
    // ALLOCATE PIN MODE TABLES (1:1 mapping of USB Video Mode:PIN (DShow) Video mode)
    //--------------------------------------------------------------------------------

    // Allocated memory for video formats == num formats * (num frame types for each format) * (num frame rates supported by each frame)
    m_pPinVideoFormats[CAPTURE].pCsDataRangeVideo = new PCS_DATARANGE_VIDEO[m_pPinVideoFormats[CAPTURE].ulAvailFormats];
    if( NULL == m_pPinVideoFormats[CAPTURE].pCsDataRangeVideo)
    {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    // Formats for still == num format types * num frame types inside each format type
    m_pPinVideoFormats[STILL].ulAvailFormats    = m_nUSBVideoFormatDesc * m_nUSBVideoFrameDesc;
    m_pPinVideoFormats[STILL].pCsDataRangeVideo = new PCS_DATARANGE_VIDEO[m_pPinVideoFormats[STILL].ulAvailFormats];
    if( NULL == m_pPinVideoFormats[STILL].pCsDataRangeVideo)
    {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    DEBUGMSG(ZONE_DEVICE, (DTAG TEXT("%02d CAPTURE formats supported\r\n"), m_pPinVideoFormats[CAPTURE].ulAvailFormats));
    DEBUGMSG(ZONE_DEVICE, (DTAG TEXT("%02d STILL   formats supported\r\n"), m_pPinVideoFormats[STILL].ulAvailFormats));


    //-----------------------------------------------------------------------
    // POPULATE PIN (DirectShow) MODE TABLES
    //-----------------------------------------------------------------------

    DWORD dwFormatIndex = 0, dwStillIndex = 0;
    for(int i = 0 ; i < m_nUSBVideoModes; i++)
    {
        if (m_pUSBVideoModes[i].nNumInterval != -1)
        {

            // This is a CAPTURE (video) format, not a STILL format
            for (int k = 0; k < m_pUSBVideoModes[i].nNumInterval; k++)
            {
                // Hardware bug - 640x480x16 @ 5fps locks up the Logitech Pro 5000 USB Webcam, hardware issue.  Force it to 1st entry, 30fps
                if (m_pUSBVideoModes[i].wWidth == 640 &&
                    m_pUSBVideoModes[i].wHeight == 480 &&
                    m_pUSBVideoModes[i].bBitsPerPixel == 16 &&
                    m_pUSBVideoModes[i].dwInterval[k] == 0x1E8480)
                {
                    m_pUSBVideoModes[i].dwInterval[k] = m_pUSBVideoModes[i].dwInterval[0];
                }


                if(m_pUSBVideoModes[i].wFormatType == USB_VIDEO_VS_FORMAT_UNCOMPRESSED)
                {
                    // use the macro to populate the CSDATARANGE_VIDEO structure. 
                    MAKE_STREAM_MODE_YUY2(FormatNameT, m_pUSBVideoModes[i].wWidth, m_pUSBVideoModes[i].wHeight, m_pUSBVideoModes[i].dwMaxVideoFrameBufferSize, m_pUSBVideoModes[i].dwInterval[k], m_pUSBVideoModes[i].bBitsPerPixel, CS_VIDEOSTREAM_CAPTURE)
                    m_pPinVideoFormats[CAPTURE].pCsDataRangeVideo[dwFormatIndex] = new CS_DATARANGE_VIDEO;
    
                    if( NULL == m_pPinVideoFormats[CAPTURE].pCsDataRangeVideo[dwFormatIndex])
                        return ERROR_INSUFFICIENT_BUFFER;
                    
                    memcpy(m_pPinVideoFormats[CAPTURE].pCsDataRangeVideo[dwFormatIndex],  &FormatNameT, sizeof(CS_DATARANGE_VIDEO));
                    dwFormatIndex++;
    
                    // use the first frame rate to expose a still format
                    if(k == 0)
                    {
                        m_pPinVideoFormats[STILL].pCsDataRangeVideo[dwStillIndex] = new CS_DATARANGE_VIDEO;
                        if(m_pPinVideoFormats[STILL].pCsDataRangeVideo[dwStillIndex] == NULL)
                            return ERROR_INSUFFICIENT_BUFFER;
    
                        MAKE_STREAM_MODE_YUY2(FormatNameT, m_pUSBVideoModes[i].wWidth, m_pUSBVideoModes[i].wHeight, m_pUSBVideoModes[i].dwMaxVideoFrameBufferSize, m_pUSBVideoModes[i].dwInterval[k], m_pUSBVideoModes[i].bBitsPerPixel, CS_VIDEOSTREAM_CAPTURE)
                        memcpy(m_pPinVideoFormats[STILL].pCsDataRangeVideo[dwStillIndex],  &FormatNameT, sizeof(CS_DATARANGE_VIDEO));
                        dwStillIndex++;
                    }
                }
                 
                if(m_pUSBVideoModes[i].wFormatType == USB_VIDEO_VS_FORMAT_MJPEG)
                {
                    // use the macro to populate the CSDATARANGE_VIDEO structure. -- exposes MJPEG for Capture 
                    MAKE_STREAM_MODE_MJPG(FormatNameT, m_pUSBVideoModes[i].wWidth, m_pUSBVideoModes[i].wHeight, m_pUSBVideoModes[i].dwMaxVideoFrameBufferSize, m_pUSBVideoModes[i].dwInterval[k], m_pUSBVideoModes[i].bBitsPerPixel,  CS_VIDEOSTREAM_CAPTURE)
                    m_pPinVideoFormats[CAPTURE].pCsDataRangeVideo[dwFormatIndex] = new CS_DATARANGE_VIDEO;
    
                    if( NULL == m_pPinVideoFormats[CAPTURE].pCsDataRangeVideo[dwFormatIndex])
                             return ERROR_INSUFFICIENT_BUFFER;
                    
                    memcpy(m_pPinVideoFormats[CAPTURE].pCsDataRangeVideo[dwFormatIndex],  &FormatNameT, sizeof(CS_DATARANGE_VIDEO));
                    dwFormatIndex++;
    
                    // use the first frame rate to expose a still format - JPEG for STILL
                    if(k == 0)
                    {
                        m_pPinVideoFormats[STILL].pCsDataRangeVideo[dwStillIndex] = new CS_DATARANGE_VIDEO;
                        if(m_pPinVideoFormats[STILL].pCsDataRangeVideo[dwStillIndex] == NULL)
                            return ERROR_INSUFFICIENT_BUFFER;
    
                        MAKE_STREAM_MODE_JPEG(FormatNameT, m_pUSBVideoModes[i].wWidth, m_pUSBVideoModes[i].wHeight, m_pUSBVideoModes[i].dwMaxVideoFrameBufferSize, m_pUSBVideoModes[i].dwInterval[k], m_pUSBVideoModes[i].bBitsPerPixel, CS_VIDEOSTREAM_CAPTURE)
                        memcpy(m_pPinVideoFormats[STILL].pCsDataRangeVideo[dwStillIndex],  &FormatNameT, sizeof(CS_DATARANGE_VIDEO));
                        dwStillIndex++;
                    }
                }
            }
        }
    }

    DEBUGMSG (ZONE_FUNCTION, (DTAG TEXT("QueryFormats--\r\n")));
    
    return ERROR_SUCCESS;
}


//
// DoVendorTransfer - Called to communicate with the camera.  Since I've
// noticed that the camera sometimes overruns the output buffer, this 
// function pipes the data into a fairly big buffer and then copies the
// data to the exact size buffer passed.
// 
int CUSBPdd::DoVendorTransfer (BYTE bRequest, BYTE bCmd, BYTE bInterface, 
                               BYTE bUnit, PBYTE pVal, WORD wLen)
{
    USB_DEVICE_REQUEST req;
    DWORD dwBytes;
    DWORD dw, dwUsbErr;
    BOOL fSet, bRetry = 1;
    static BYTE bTBuff[512];

Retry:
    if (bRequest == USBVID_SET_CUR)
        fSet = TRUE;
    else
        fSet = FALSE;


    //
    // Set transfer flags depending on read or write
    //
    if (fSet)
        req.bmRequestType = USB_REQUEST_HOST_TO_DEVICE | USB_REQUEST_CLASS | USB_REQUEST_FOR_INTERFACE;
    else
        req.bmRequestType = USB_REQUEST_DEVICE_TO_HOST | USB_REQUEST_CLASS | USB_REQUEST_FOR_INTERFACE;

    req.bRequest = bRequest;
    req.wValue   = MAKEWORD (0, bCmd);
    req.wIndex   = MAKEWORD (bInterface, bUnit);
    req.wLength  = wLen;

    dwBytes = 0;
    dwUsbErr = USB_NO_ERROR;
    dw = IssueVendorTransfer (m_pDrv->lpUsbFuncs, m_pDrv->hDevice, 
                 NULL, NULL, // this will be a synchronous call
                 (fSet ? USB_OUT_TRANSFER : USB_IN_TRANSFER) | USB_SHORT_TRANSFER_OK,
                 &req, fSet && (req.wLength < sizeof (bTBuff)) ? pVal : bTBuff, 
                 NULL, &dwBytes, 2000, &dwUsbErr);

    if ((ERROR_SUCCESS != dw || USB_NO_ERROR != dwUsbErr) && !bRetry)
        DEBUGMSG (ZONE_ERROR, 
                  (DTAG TEXT("Error calling IssueVendorTransfer rc: %d USB Err: %d  ExtErr: %d\r\n"), 
                  dw, dwUsbErr, GetLastError()));

    if (USB_STALL_ERROR == dwUsbErr) {
            ResetDefaultEndpoint(m_pDrv->lpUsbFuncs, m_pDrv->hDevice);

            if (bRetry)
            {
                bRetry = 0;
                goto Retry;
            }

            return dw;
    }

    // If no data transferred, stuff in an error
    if (!dw && !dwBytes)
        dw = ERROR_BAD_LENGTH;

    // Copy data to output buffer
    if (!dw && !fSet && (dwBytes <= req.wLength))
        memcpy (pVal, bTBuff, dwBytes);

    return dw;
}


//
// Use fast DMA into contiguous cached virtual memory, otherwise the performance will be poor
//
BOOL
CUSBPdd::EnableDMASupport()
{
    // Allow EHCI driver to DMA directly to the camera driver buffers
    m_fDMASupported            = TRUE;
    m_dmaAdapter.ObjectSize    = sizeof(DMA_ADAPTER_OBJECT);
    m_dmaAdapter.InterfaceType = Internal;
    m_dmaAdapter.BusNumber     = 0;

    return TRUE;    
}


//
//  When DMA capable USB controller is used, contiguous physical memory is 
//  requested via this function.  This improves DMA throughput.
//
DWORD
CUSBPdd::AllocateContiguousCachedBuffer(
    IN  SIZE_T              NumberOfBytes,
    IN  PBYTE               *ppVirtualAddressCached,
    IN  PBYTE               *ppVirtualAddressUncached,
    OUT PHYSICAL_ADDRESS    *pPhysicalAddress
    )
{
    DWORD dwRet;

    // Check the parameters
    if (!NumberOfBytes || !ppVirtualAddressCached || !ppVirtualAddressUncached || !pPhysicalAddress)
        return ERROR_INVALID_PARAMETER;

    // Is DMA supported?
    if (!m_fDMASupported)
        return ERROR_NOT_SUPPORTED;

    *ppVirtualAddressCached    = NULL;
    *ppVirtualAddressUncached  = NULL;
    pPhysicalAddress->QuadPart = 0;

    // Allocate the memory
    *ppVirtualAddressUncached = (PBYTE)HalAllocateCommonBuffer(&m_dmaAdapter, NumberOfBytes, pPhysicalAddress, TRUE);

    if (!(*ppVirtualAddressUncached))
    {
        DEBUGMSG (ZONE_ERROR, (DTAG TEXT("Error: HalAllocateCommonBuffer failed in AllocateContiguousCachedBuffer\r\n")));
        return ERROR_OUTOFMEMORY;
    }

    *ppVirtualAddressCached = (PBYTE)VirtualAlloc(NULL, NumberOfBytes, MEM_RESERVE, PAGE_NOACCESS);

    if (!(*ppVirtualAddressCached))
    {
        DEBUGMSG (ZONE_ERROR, (DTAG TEXT("Error: OOM VirtualAlloc() in AllocateContiguousCachedBuffer().\r\n")));
        dwRet = ERROR_OUTOFMEMORY;
        goto Error;
    }

    if (!VirtualCopy((LPVOID)*ppVirtualAddressCached, (LPVOID)*ppVirtualAddressUncached, NumberOfBytes, PAGE_READWRITE))
    {
        DEBUGMSG (ZONE_ERROR,
            (DTAG TEXT("Error: Failed VirtualCopy VA-Uncahced[0x%x] size[%d] Error[%d]\r\n"),
            *ppVirtualAddressUncached,
            NumberOfBytes,
            GetLastError()));
        dwRet = ERROR_OUTOFMEMORY;
        goto Error;
    }

    DEBUGMSG (ZONE_DEVICE,
        (DTAG TEXT("Allocated frame buffer: VA Uncached [0x%x] : VA Cached [0x%x]  : Size [%d]\r\n"),
        *ppVirtualAddressUncached,
        *ppVirtualAddressCached,
        NumberOfBytes));

    return ERROR_SUCCESS;


Error:

    if (*ppVirtualAddressUncached)
        HalFreeCommonBuffer(&m_dmaAdapter, NumberOfBytes, *pPhysicalAddress, *ppVirtualAddressUncached, TRUE);

    if (*ppVirtualAddressCached)
        VirtualFree((PVOID)*ppVirtualAddressCached, 0, MEM_RELEASE);

    *ppVirtualAddressCached = NULL;
    *ppVirtualAddressUncached = NULL;
    pPhysicalAddress->QuadPart = 0;

    return dwRet;
}


//
//  Free the DMA buffer.
//
DWORD
CUSBPdd::FreeContiguousCachedBuffer(
    IN  SIZE_T              NumberOfBytes,
    IN  PBYTE               *ppVirtualAddressCached,
    IN  PBYTE               *ppVirtualAddressUncached,
    OUT PHYSICAL_ADDRESS    *pPhysicalAddress
    )
{
    // Check the parameters
    if (!NumberOfBytes || !ppVirtualAddressCached || !ppVirtualAddressUncached || !pPhysicalAddress)
        return ERROR_INVALID_PARAMETER;

    // Is DMA supported?
    if (!m_fDMASupported)
        return ERROR_NOT_SUPPORTED;

    if (*ppVirtualAddressUncached)
        HalFreeCommonBuffer(&m_dmaAdapter, NumberOfBytes, *pPhysicalAddress, *ppVirtualAddressUncached, TRUE);

    if (*ppVirtualAddressCached)
        VirtualFree((PVOID)*ppVirtualAddressCached, 0, MEM_RELEASE);

    *ppVirtualAddressCached    = NULL;
    *ppVirtualAddressUncached  = NULL;
    pPhysicalAddress->QuadPart = 0;

    return ERROR_SUCCESS;
}
