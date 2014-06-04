// All rights reserved ADENEO EMBEDDED 2010
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
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//

#ifndef _CAMERA_DRV_PDD_H
#define _CAMERA_DRV_PDD_H


#ifdef __cplusplus
extern "C" {
#endif

typedef MMRESULT (WINAPI *FNTIMEKILLEVENT)(UINT);
typedef MMRESULT (WINAPI *FNTIMESETEVENT)(UINT, UINT, LPTIMECALLBACK, DWORD, UINT );

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// YUV specific buffer filler 

UINT YUVBufferFill( PUCHAR pImage, PCS_VIDEOINFOHEADER pCsVideoInfoHdr, bool FlipHorizontal, LPVOID lpParam );

// YUY2 specific defines
#define MACROPIXEL_RED 0xf0525a52
#define MACROPIXEL_GRN 0x22913691
#define MACROPIXEL_WHITE 0x80EB80EB
#define MACROPIXEL_BLACK 0x80108010

// YV12 specific defines
#define Y_WHITE 235
#define Y_BLACK 16
#define VU_BOTH 128

// Misc YUV filler flags
#define BOXWIDTHDIVIDER 8
#define BOXHEIGHTDIVIDER 8
#define LOCATIONWIDTHMASK 0xFF
#define LOCATIONHEIGHTMASK 0xFF
#define LOCATIONSHIFT 8
// divide the initial tick count by four to make the box move 2x faster.
#define SPEEDSHIFT 4

// Default number of Sensor Modes is 3
#define DEFAULT_PINS 3

#include "ISPctrl.h"
#include "TvpCtrl.h"

// The IAMCameraControl::Set() & IAMCameraControl::Get() methods have been 
// overloaded to support setting and retrieveing custom properties of the 
// OMAP3530/DM3730 camera subsytem and the TVP5146 decoder chip
// NOTE: The 3rd argument to these methods is ignored when setting
// one of the custome properties.

// CameraControlProperty_t
// This enum defines values that can be passed as the first arg (Property) to 
// IAMCameraControl::Set() and IAMCameraControl::Get()
typedef enum {
    CameraControl_InputPort = 0x100,  // Select input port -  use one of the
                                          // values specified by the enum
                                          // type CameraInput
} CameraControlProperty_t;

// CameraInput_t
// This enum defines values that can be passed as the second arg (lValue) to 
// IAMCameraControl::Set() and IAMCameraControl::Get()
typedef enum {
    CameraInput_YPbPr = 0,
    CameraInput_AV,
    CameraInput_SVideo
} CameraInputPort_t;

typedef class CCameraPdd
{
public:
    
    friend class CCameraDevice;

    CCameraPdd();

    ~CCameraPdd();

    DWORD PDDInit( 
        PVOID MDDContext,
        PPDDFUNCTBL pPDDFuncTbl
        );

    DWORD Open(
        PVOID MDDOpenContext );

    DWORD Close(
        PVOID MDDOpenContext );

    DWORD PDDDeinit( 
        );

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

    DWORD GetSensorModeInfo( 
        ULONG ulModeType, 
        PSENSORMODEINFO pSensorModeInfo 
        );

    DWORD SetSensorModeFormat( 
        ULONG ulModeType, 
        PCS_DATARANGE_VIDEO pCsDataRangeVideo 
        );

    PVOID AllocateBuffer(
        ULONG ulModeType 
        );

    DWORD DeAllocateBuffer( 
        ULONG ulModeType, 
        PVOID pBuffer
        );

    DWORD RegisterClientBuffer(
        ULONG ulModeType, 
        PVOID pBuffer 
        );

    DWORD UnRegisterClientBuffer( 
        ULONG ulModeType, 
        PVOID pBuffer 
        );

    DWORD FillBuffer( 
        ULONG ulModeType, 
        PUCHAR pImage );

    DWORD GetMetadata(
        DWORD dwPropId,
        PUCHAR pOutBuf,
        DWORD OutBufLen,
        PDWORD pdwBytesTransferred);

    DWORD HandleSensorModeCustomProperties( 
        ULONG ulModeType, 
        PUCHAR pInBuf, 
        DWORD  InBufLen, 
        PUCHAR pOutBuf, 
        DWORD  OutBufLen, 
        PDWORD pdwBytesTransferred 
        );

    DWORD Run(ULONG lModeType);
    DWORD Pause(ULONG lModeType);
    DWORD Stop(ULONG lModeType);   
    
    static
    void
    CaptureTimerCallBack(
        UINT uTimerID,
        UINT uMsg,
        DWORD_PTR dwUser,
        DWORD_PTR dw1,
        DWORD_PTR dw2
        ); 

    static
    void
    StillTimerCallBack(
        UINT uTimerID,
        UINT uMsg,
        DWORD_PTR dwUser,
        DWORD_PTR dw1,
        DWORD_PTR dw2
        ); 

private:

    bool
    CreateTimer( ULONG ulModeType );

    void 
    HandleCaptureInterrupt( UINT uTimerID );

    void 
    HandleStillInterrupt( UINT uTimerID );

    DWORD 
    YUVBufferFill( ULONG ulModeType, PUCHAR pImage );

    bool 
    ReadMemoryModelFromRegistry();

private:

    bool    m_bStillCapInProgress;
    HANDLE  m_hContext;
	HANDLE  m_hOpenContext;
    
    CSSTATE                 m_CsState[MAX_SUPPORTED_PINS];
    SENSORMODEINFO          m_SensorModeInfo[MAX_SUPPORTED_PINS];

    // Total number of pins implemented by this camera
    ULONG m_ulCTypes;

    // Power Capabilities
    POWER_CAPABILITIES m_PowerCaps; 

    // All ProcAmp and CameraControl props
    SENSOR_PROPERTY     m_SensorProps[NUM_PROPERTY_ITEMS];

    // All the Video Formats supported by all the pins
    PPINVIDEOFORMAT   m_pModeVideoFormat;

    // VideoControl Caps corresponding to all the pins
    VIDCONTROLCAPS   *m_pModeVideoCaps;

    // Pointer to the MDD Pin Object corresponding to all the pins.
    // HandlePinIO() method of this object is then called whenever 
    // an image is ready. HandlePinIO() internally calls 
    // FillPinBuffer() of PDD interface.
    LPVOID       *m_ppModeContext;
    
    // Currently selected video format for each pin
    PCS_DATARANGE_VIDEO m_pCurrentFormat;

    // Timer specific
    FNTIMESETEVENT     m_pfnTimeSetEvent; 
    FNTIMEKILLEVENT    m_pfnTimeKillEvent;
    HMODULE            m_hTimerDll;
    MMRESULT           m_TimerIdentifier[MAX_SUPPORTED_PINS];
    CEDEVICE_POWER_STATE m_CurrentPowerState;

private:
    DWORD          m_CAMISPIntr;
    HANDLE         m_CAMISPEvent;
    HANDLE         m_hCCDCInterruptThread;
    HANDLE         m_hParentBus;
    BOOL           m_bInterruptThreadExit;
    CIspCtrl*      m_pIspCtrl;
    CTvpCtrl*      m_pTvpCtrl;

	CRITICAL_SECTION	m_csSensorState;
	HANDLE				m_hEndOfFrameEvent;
    CameraInputPort_t   m_inputPort;
    
	// Metadata
    PCS_PROPERTYITEM  m_pMetadata;

public:
    DWORD CameraInterruptThreadImpl();

} CAMERAPDD, * PCAMERAPDD;

#ifdef __cplusplus
}
#endif

#endif
