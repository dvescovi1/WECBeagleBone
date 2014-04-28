//-------------------------------------------------------------------------
// <copyright file="usbpdd.h" company="Microsoft">
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

#ifndef _USBPDD_H
#define _USBPDD_H


#define CAPTURE_THREAD_PRI  120
#define FILL_THREAD_PRI     110

#define SIZE_TRANSFER_QUEUE 4
#define TRANSFER_SIZE 32

#define MAXCAPTUREBUFFS 2
#define MAXSTILLBUFFS   1

#define CAPTURING    0
#define RE_SYNC      1
#define START_CAP    2
#define COMPLETE     3

#define FEAT_UNSUPPORTED                0
#define FEAT_SCANNING_MODE              1
#define FEAT_AUTO_EXPOSURE_MODE         2 
#define FEAT_AUTO_EXPOSURE_PRI          3
#define FEAT_EXPOSURE_TIME_ABS          4
#define FEAT_EXPOSURE_TIME_REL          5
#define FEAT_FOCUS_ABS                  6
#define FEAT_FOCUS_REL                  7
#define FEAT_IRIS_ABS                   8
#define FEAT_IRIS_REL                   9
#define FEAT_ZOOM_ABS                  10
#define FEAT_ZOOM_REL                  11 
#define FEAT_PAN_ABS                   12
#define FEAT_PAN_REL                   13
#define FEAT_ROLL_ABS                  14
#define FEAT_ROLL_REL                  15
#define FEAT_TILT_ABS                  16
#define FEAT_TILT_REL                  17
#define FEAT_FOCUS_AUTO                18
#define FEAT_PRIVACY                   19
#define FEAT_BRIGHTNESS                20
#define FEAT_CONTRAST                  21
#define FEAT_HUE                       22
#define FEAT_SATURATION                23
#define FEAT_SHARPNESS                 24
#define FEAT_GAMMA                     25
#define FEAT_WHITE_BAL_TEMP            26
#define FEAT_WHITE_BAL_COMPONENT       27
#define FEAT_BACKLIGHT_COMPENSATION    28
#define FEAT_GAIN                      29
#define FEAT_POWER_LINE_FREQ           30
#define FEAT_AUTO_HUE                  31
#define FEAT_AUTO_WHITE_BAL_TEMP       32
#define FEAT_AUTO_WHITE_BAL_COMPONENT  33
#define FEAT_DIGITAL_MULTIPLIER        34
#define FEAT_DIGITAL_MULTIPLIER_LIMIT  35
#define FEAT_ANALOG_VIDEO_STANDARD     36
#define FEAT_ANALOG_VIDEO_LOCK_STATUS  37


// USB pipe
typedef struct {
        USB_PIPE hPipe;
        UCHAR ucAddr;
        WORD wPacketSize;
} PIPE, *LPPIPE;


// Max number of intervals (frames-per-second) for each image format
#define  MAXINTERVALS  10


// USB Video Mode.  We define this structure to be essentially a union
// of the interesting fields from:
//  USB Video Format Descriptor: MJPEG|Uncompressed, interlacing, aspect ratio
//  USB Video Frame Descriptor:  width, height, frames-per-second, etc.
typedef struct {
    DWORD cbSize;               // Size of the structure
    WORD  wFormatType;          // Video format type (MJPEG, Uncompressed, etc.) 
    WORD  wFormatIndex;         // Video format index
    WORD  wFrameIndex;          // Video frame index   
    WORD  wWidth;
    WORD  wHeight;
    BYTE  bBitsPerPixel;
    DWORD dwMaxVideoFrameBufferSize;
    DWORD dwDefaultFrameInterval;

    int   nNumInterval;         // Number of frame intervals supported
                                // If zero, frame interval values are
                                // not discrete but are continuous. If 0, 
                                // dwInterval[0] is min value and 
                                // dwInterval[1] is max value and 
                                // dwInterval[2] is step value
    DWORD dwInterval[MAXINTERVALS];
    DWORD dwCurFrameInterval;
                                
} USBVIDEOMODESTRUCT, *PUSBVIDEOMODESTRUCT;



typedef struct {
    DWORD   dwSize;
    PIPE    pipeStream;
} PDDCONTEXT, *PPDDCONTEXT;


#define ENUM_AUTO 0x80
#define ENUM_PROP_UNSUPPORTED -1
#define GET_SUPPORTED   1
#define SET_SUPPORTED   2


// USB Camera Control (a.k.a property, e.g. brightness, focus, hue, ... )
typedef struct {
    DWORD dwMDDPropertyID;
    BYTE  bUnit;
    BYTE  bControlSelector;
    WORD  wLength;
    BYTE  bSupported;
    BYTE  fGetSetSupport;
} PROPERTYCONTROLSTRUCT, *PPROPERTYCONTROLSTRUCT;


PDRVCONTEXT GetRegData (PVOID dwContext);

typedef class CUSBPdd
{
public:
    
    friend class CCameraDevice;

    CUSBPdd();

    ~CUSBPdd();

    DWORD PDDInit( 
        PVOID MDDContext,
        PPDDFUNCTBL pPDDFuncTbl
            );
    
    DWORD PDDDeInit();

    DWORD GetAdapterInfo( 
        PADAPTERINFO pAdapterInfo 
        );

    DWORD HandleVidProcAmpChanges(
        DWORD dwPropId, 
        LONG lFlags, 
        LONG lValue
        );
    
    DWORD HandleCamControlChanges( 
        DWORD dwPropId, 
        LONG lFlags, 
        LONG lValue 
        );

    DWORD HandleVideoControlCapsChanges(
        LONG lModeType ,
        ULONG ulCaps 
        );

    DWORD SetPowerState(
        CEDEVICE_POWER_STATE PowerState 
        );
    
    DWORD HandleAdapterCustomProperties(
        PUCHAR pInBuf, 
        DWORD  InBufLen, 
        PUCHAR pOutBuf, 
        DWORD  OutBufLen, 
        PDWORD pdwBytesTransferred 
        );

    DWORD InitSensorMode(
        ULONG ulModeType, 
        LPVOID ModeContext
        );
    
    DWORD DeInitSensorMode( 
        ULONG ulModeType 
        );

    DWORD SetSensorState( 
        ULONG lPinId, 
        CSSTATE csState 
        );

    DWORD TakeStillPicture(
        LPVOID pBurstModeInfo );

    int QueryFormats();

    DWORD GetSensorModeInfo( 
        ULONG ulModeType, 
        PSENSORMODEINFO pSensorModeInfo 
        );

    DWORD SetSensorModeFormat( 
        ULONG ulModeType, 
        PCS_DATARANGE_VIDEO pCsDataRangeVideo 
        );
 
    DWORD RegisterClientBuffer(
        ULONG ulModeType, 
        PVOID pBuffer 
        );

    DWORD UnRegisterClientBuffer( 
        ULONG ulModeType, 
        PVOID pBuffer 
        );

    DWORD HandleSensorModeCustomProperties( 
        ULONG ulModeType, 
        PUCHAR pInBuf, 
        DWORD  InBufLen, 
        PUCHAR pOutBuf, 
        DWORD  OutBufLen, 
        PDWORD pdwBytesTransferred 
        );
    
    DWORD FillBuffer(
        ULONG ulModeType,  
        PUCHAR pImage );


    BOOL CheckCameraProperties();
    BOOL InitCameraProperties();
    BOOL ResetCameraProperties();

    void MarkSupportedProperties (
        PBYTE pCtlBytes, 
        int   nCtlBytes, 
        PPROPERTYCONTROLSTRUCT pPropertiesArray, 
        int   nArraySize, 
        BYTE  bUnit);


    BYTE PropSupported(DWORD MDDPropertyID);

    DWORD PropGetValue(
        DWORD MDDPropertyID, 
        BYTE  bCmd);

    DWORD PropSetValue(
        DWORD MDDPropertyID, 
        long  lValue);
    
    PROPERTYCONTROLSTRUCT * MDDPropertyToUSBControl(
        DWORD MDDPropertyID);

    TCHAR * MDDPropertyToString(DWORD MDDPropertyID);

    int DoVendorTransfer (
        BYTE bRequest, 
        BYTE bCmd, 
        BYTE bInterface, 
        BYTE bUnit, 
        PBYTE pVal, 
        WORD wLen);
    
    BOOL FindVideoInfo(
        PCS_DATARANGE_VIDEO pCsDataRangeVideo, 
        ULONG               ulModeType,  
        BYTE *              pbFormatIndex, 
        BYTE *              pbFrameIndex, 
        DWORD *             pdwInterval, 
        int *               pFoundStream);

    BOOL SetStreamInterface (
        BYTE nInterface, 
        DWORD dwBandwidth);

    PUSBSTRMIF FindStreamInterface (
        BYTE nInterface, 
        WORD wPacketSize);

    int ProbeCommit(
        BYTE bFormatIndex, 
        BYTE bFrameIndex, 
        DWORD dwFrameInterval, 
        WORD wCompression, 
        PDWORD pdwMaxBandwidth);

    int GetQualityParameters (
        BYTE bFormatIndex, 
        BYTE bFrameIndex, 
        DWORD dwFrameInterval,
        WORD *pwCompMin, 
        WORD *pwCompMax, 
        WORD *pwCompInc); 

    DWORD SetVideoFormat(
        BYTE bFormatIndex, 
        BYTE bFrameIndex, 
        DWORD dwInterval);

    int GetCameraError(
        BYTE *pErr);
    
    static DWORD WINAPI CaptureThread(
        LPVOID lpVoid);

    static DWORD WINAPI FillThread(
        LPVOID lpVoid);

private:
    PDDCONTEXT  m_PddContext;       // Communication context
    PDRVCONTEXT m_pDrv;             // USB device context
    TCHAR       m_szActiveRegPath[MAX_PATH];
   
    CSSTATE     m_CsState[MAX_SUPPORTED_PINS];
    
    // Info about each of the sub modes - no. of buffers, formats supported etc
    SENSORMODEINFO m_SensorModeInfo[MAX_SUPPORTED_PINS];

    // Total number of pins implemented by this camera
    // [ always 2 for now: CAPTURE and STILL (no PREVIEW) ]
    ULONG m_ulCTypes;

    // Power Capabilities - is the structure that can be used to define what are the different power modes supported by the camera
    POWER_CAPABILITIES PowerCaps;

    CEDEVICE_POWER_STATE m_PowerState;      

    // All ProcAmp and CameraControl properties (contrast, brightness, etc.)
    SENSOR_PROPERTY    m_SensorProps[NUM_PROPERTY_ITEMS];

    // All the Video Modes supported (stored as DShow structures)
    PPINVIDEOFORMAT   m_pPinVideoFormats;

    // All the Video Modes suported (stored as USB structures)
    // corresponds to the PCS_DATARANGE_VIDEO array in PPINVIDEOFORMAT
    PUSBVIDEOMODESTRUCT m_pUSBVideoModes;      
    BYTE                m_nUSBVideoFormatDesc;     // format: aspect ratio and interlacing
    BYTE                m_nUSBVideoFrameDesc;      // frame:  width, height, max buffer size, frames per second
    BYTE                m_nUSBVideoModes;          // format types * frame types

    // VideoControl Caps corresponding to all the pins
    VIDCONTROLCAPS   *m_pVideoCaps;

    // Pointer to the MDD Pin Object corresponding to all the pins.
    // HandlePinIO() method of this object is then called whenever 
    // an image is ready. HandlePinIO() internally calls 
    // FillPinBuffer() of PDD interface.
    LPVOID       *m_ppModeContext;
    
    // Currently selected video format for each pin    
    PCS_DATARANGE_VIDEO m_pCurrentFormat;

    PUSBVIDEOMODESTRUCT  m_pCurrentStreamFormat;  //EXACTLY LIKE ABOVE FOR CURRENT FORMAT

    BOOL    ReadConfigFromRegistry();
    DWORD   m_dwPreferredWidth;
    DWORD   m_dwPreferredHeight;
    

    BOOL m_fDMASupported;
    DMA_ADAPTER_OBJECT m_dmaAdapter;

    BOOL  EnableDMASupport();
    DWORD AllocateContiguousCachedBuffer(
        IN  SIZE_T              NumberOfBytes,
        IN  PBYTE               *pVirtualAddressCached,
        IN  PBYTE               *pVirtualAddressUncached,
        OUT PHYSICAL_ADDRESS    *pPhysicalAddress);
        
    DWORD FreeContiguousCachedBuffer(
        IN  SIZE_T              NumberOfBytes,
        IN  PBYTE               *pVirtualAddressCached,
        IN  PBYTE               *pVirtualAddressUncached,
        OUT PHYSICAL_ADDRESS    *pPhysicalAddress);
      
    HANDLE m_hCaptureThread;
    HANDLE m_hCaptureThreadExit;

    ULONG  m_ulBufSize;
    PBYTE  m_pBufVirtualCached;
    PBYTE  m_pBufVirtualUncached;
    PHYSICAL_ADDRESS m_BufPhysical;

    HANDLE m_hFillThread;
    HANDLE m_hFillThreadExit;
    HANDLE m_hFillThreadRun;
    HANDLE m_hFrameReady;
    PBYTE  m_pFillBuffer;
    DWORD  m_dwFillBytes;

    DWORD   m_dwCapturedFrames;
    DWORD   m_dwDroppedFrames;
    DWORD   m_dwStartTick;
    DWORD   m_dwStopTick;
    
    BOOL    m_bMJPEGSupported;
    BOOL    m_bUncompressedSupported;
} USBPDD, *PUSBPDD;


#endif
