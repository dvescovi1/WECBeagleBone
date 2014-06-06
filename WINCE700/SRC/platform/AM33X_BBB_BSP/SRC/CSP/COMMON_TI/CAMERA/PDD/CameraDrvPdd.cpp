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

#pragma warning(push)
#pragma warning(disable : 4201 6244)
#include <windows.h>
#include <pm.h>

#include "Cs.h"
#include "Csmedia.h"
#pragma warning(pop)

#include "sdk_padcfg.h"
#include "bsp_cfg.h"

#include "CameraPDDProps.h"
#include "dstruct.h"
#include "dbgsettings.h"
#include <camera.h>
#include "CameraDriver.h"
#include "SensorFormats.h"
#include "SensorProperties.h"
#include "PinDriver.h"

#include "oal_io.h"
#include "oalex.h"
#include <pkfuncs.h>
#include "Ispreg.h"
#include "CameraPDD.h"
#include "CameraDrvPdd.h"
#include "wchar.h"
#include "util.h"
#include "params.h"

void* UYVYtoYUY2(WORD* pInput, BYTE* pOutput, DWORD dwNumByte);
static DWORD CameraInterruptThread(LPVOID lpParameter);

PDDFUNCTBL2 FuncTbl = {
    sizeof(PDDFUNCTBL2),
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
    PDD_HandleModeCustomProperties,
    PDD_Open,
    PDD_Close,
    PDD_GetMetadata
};


CCameraPdd::CCameraPdd()
{
    InitializeCriticalSection(&m_csSensorState);
    m_ulCTypes = DEFAULT_PINS;
    m_bStillCapInProgress = false;
    m_hContext = NULL;
    m_pModeVideoFormat = NULL;
    m_pModeVideoCaps = NULL;
    m_ppModeContext = NULL;
    m_pCurrentFormat = NULL;
    m_pfnTimeSetEvent = NULL;
    m_pfnTimeKillEvent = NULL;
    m_hTimerDll = NULL;
    m_hParentBus = NULL;
    m_inputPort = CameraInput_AV;
    m_pIspCtrl = new CIspCtrl;
    m_pTvpCtrl = new CTvpCtrl;
    m_bInterruptThreadExit = FALSE;
	m_pMetadata = NULL;
	m_hEndOfFrameEvent = NULL;
    memset( &m_TimerIdentifier, 0x0, sizeof(m_TimerIdentifier));    
    memset( &m_CsState, 0x0, sizeof(m_CsState));
    memset( &m_SensorModeInfo, 0x0, sizeof(m_SensorModeInfo));
    memset( &m_SensorProps, 0x0, sizeof(m_SensorProps));
    memset( &m_PowerCaps, 0x0, sizeof(m_PowerCaps));
}

CCameraPdd::~CCameraPdd()
{
    if (m_CAMISPEvent != NULL)
    {
        m_bInterruptThreadExit = TRUE;
        SetEvent(m_CAMISPEvent);       
        WaitForSingleObject(m_hCCDCInterruptThread, 2000);
        CloseHandle(m_hCCDCInterruptThread);
    }

    if( m_pfnTimeKillEvent )
    {
        for( int i=0; i < MAX_SUPPORTED_PINS; i++ )
        {
            if ( NULL != m_TimerIdentifier[i] )
            {
                m_pfnTimeKillEvent( m_TimerIdentifier[i] );
                m_TimerIdentifier[i] = NULL;
            }
        }
    }

    if ( NULL != m_hTimerDll )
    {
        FreeLibrary( m_hTimerDll );
        m_hTimerDll = NULL;
    }

    if( NULL != m_pModeVideoCaps )
    {
        delete [] m_pModeVideoCaps;
        m_pModeVideoCaps = NULL;
    }

    if( NULL != m_pCurrentFormat )
    {
        delete [] m_pCurrentFormat;
        m_pCurrentFormat = NULL;
    }

    if( NULL != m_pModeVideoFormat )
    {
        delete [] m_pModeVideoFormat;
        m_pModeVideoFormat = NULL;
    }

    if( NULL != m_ppModeContext )
    {
        delete [] m_ppModeContext;
        m_ppModeContext = NULL;
    }

    if(NULL != m_pIspCtrl)
    {
		m_pIspCtrl->DeAllocBuffer();
        delete m_pIspCtrl;
        m_pIspCtrl = NULL;
    }

    DeleteCriticalSection(&m_csSensorState);
}

DWORD CCameraPdd::PDDInit( PVOID MDDContext, PPDDFUNCTBL pPDDFuncTbl )
{
    DWORD dwIrq = BSPGetCameraIrq();

    m_hContext = (HANDLE)MDDContext;
    // Real drivers may want to create their context

    m_ulCTypes = MAX_SUPPORTED_PINS; // Capture & Preview is used for Video driver  
    
    // Read registry to override the default number of Sensor Modes.
    ReadMemoryModelFromRegistry();  

    if (pPDDFuncTbl->dwSize  < sizeof(PDDFUNCTBL))
    {
        return ERROR_INSUFFICIENT_BUFFER;
    }
    else if (pPDDFuncTbl->dwSize  < sizeof(PDDFUNCTBL2))
    {
        memcpy(pPDDFuncTbl, &FuncTbl, sizeof(PDDFUNCTBL));
        pPDDFuncTbl->dwSize = sizeof(PDDFUNCTBL);
    }
    else if (pPDDFuncTbl->dwSize  >= sizeof(PDDFUNCTBL2))
    {
        memcpy(pPDDFuncTbl, &FuncTbl, sizeof(PDDFUNCTBL2));
    }

    memset( m_SensorProps, 0x0, sizeof(m_SensorProps) );

    // driver supports D0 and D4
    m_PowerCaps.DeviceDx = 0x11;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Set all VideoProcAmp and CameraControl properties.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //VideoProcAmp
    m_SensorProps[ENUM_BRIGHTNESS].ulCurrentValue     = BrightnessDefault;
    m_SensorProps[ENUM_BRIGHTNESS].ulDefaultValue     = BrightnessDefault;
    m_SensorProps[ENUM_BRIGHTNESS].pRangeNStep        = &BrightnessRangeAndStep[0];
    m_SensorProps[ENUM_BRIGHTNESS].ulFlags            = CSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL;
    m_SensorProps[ENUM_BRIGHTNESS].ulCapabilities     = CSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL|CSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO;
    m_SensorProps[ENUM_BRIGHTNESS].fSetSupported      = VideoProcAmpProperties[ENUM_BRIGHTNESS].SetSupported;
    m_SensorProps[ENUM_BRIGHTNESS].fGetSupported      = VideoProcAmpProperties[ENUM_BRIGHTNESS].GetSupported;
    m_SensorProps[ENUM_BRIGHTNESS].pCsPropValues      = &BrightnessValues;

    m_SensorProps[ENUM_CONTRAST].ulCurrentValue       = ContrastDefault;
    m_SensorProps[ENUM_CONTRAST].ulDefaultValue       = ContrastDefault;
    m_SensorProps[ENUM_CONTRAST].pRangeNStep          = &ContrastRangeAndStep[0];
    m_SensorProps[ENUM_CONTRAST].ulFlags              = CSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL;
    m_SensorProps[ENUM_CONTRAST].ulCapabilities       = CSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL|CSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO;
    m_SensorProps[ENUM_CONTRAST].fSetSupported        = VideoProcAmpProperties[ENUM_CONTRAST].SetSupported;
    m_SensorProps[ENUM_CONTRAST].fGetSupported        = VideoProcAmpProperties[ENUM_CONTRAST].GetSupported;
    m_SensorProps[ENUM_CONTRAST].pCsPropValues        = &ContrastValues;

    m_SensorProps[ENUM_HUE].ulCurrentValue            = HueDefault;
    m_SensorProps[ENUM_HUE].ulDefaultValue            = HueDefault;
    m_SensorProps[ENUM_HUE].pRangeNStep               = &HueRangeAndStep[0];
    m_SensorProps[ENUM_HUE].ulFlags                   = CSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO;
    m_SensorProps[ENUM_HUE].ulCapabilities            = CSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO;
    m_SensorProps[ENUM_HUE].fSetSupported             = VideoProcAmpProperties[ENUM_HUE].SetSupported;
    m_SensorProps[ENUM_HUE].fGetSupported             = VideoProcAmpProperties[ENUM_HUE].GetSupported;
    m_SensorProps[ENUM_HUE].pCsPropValues             = &HueValues;

    m_SensorProps[ENUM_SATURATION].ulCurrentValue     = SaturationDefault;
    m_SensorProps[ENUM_SATURATION].ulDefaultValue     = SaturationDefault;
    m_SensorProps[ENUM_SATURATION].pRangeNStep        = &SaturationRangeAndStep[0];
    m_SensorProps[ENUM_SATURATION].ulFlags            = CSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO;
    m_SensorProps[ENUM_SATURATION].ulCapabilities     = CSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL|CSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO;
    m_SensorProps[ENUM_SATURATION].fSetSupported      = VideoProcAmpProperties[ENUM_SATURATION].SetSupported;
    m_SensorProps[ENUM_SATURATION].fGetSupported      = VideoProcAmpProperties[ENUM_SATURATION].GetSupported;
    m_SensorProps[ENUM_SATURATION].pCsPropValues      = &SaturationValues;

    m_SensorProps[ENUM_SHARPNESS].ulCurrentValue      = SharpnessDefault;
    m_SensorProps[ENUM_SHARPNESS].ulDefaultValue      = SharpnessDefault;
    m_SensorProps[ENUM_SHARPNESS].pRangeNStep         = &SharpnessRangeAndStep[0];
    m_SensorProps[ENUM_SHARPNESS].ulFlags             = CSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO;
    m_SensorProps[ENUM_SHARPNESS].ulCapabilities      = CSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO;
    m_SensorProps[ENUM_SHARPNESS].fSetSupported       = VideoProcAmpProperties[ENUM_SHARPNESS].SetSupported;
    m_SensorProps[ENUM_SHARPNESS].fGetSupported       = VideoProcAmpProperties[ENUM_SHARPNESS].GetSupported;
    m_SensorProps[ENUM_SHARPNESS].pCsPropValues       = &SharpnessValues;

    m_SensorProps[ENUM_GAMMA].ulCurrentValue          = GammaDefault;
    m_SensorProps[ENUM_GAMMA].ulDefaultValue          = GammaDefault;
    m_SensorProps[ENUM_GAMMA].pRangeNStep             = &GammaRangeAndStep[0];
    m_SensorProps[ENUM_GAMMA].ulFlags                 = CSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO;
    m_SensorProps[ENUM_GAMMA].ulCapabilities          = CSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO;
    m_SensorProps[ENUM_GAMMA].fSetSupported           = VideoProcAmpProperties[ENUM_GAMMA].SetSupported;
    m_SensorProps[ENUM_GAMMA].fGetSupported           = VideoProcAmpProperties[ENUM_GAMMA].GetSupported;
    m_SensorProps[ENUM_GAMMA].pCsPropValues           = &GammaValues;

    m_SensorProps[ENUM_COLORENABLE].ulCurrentValue    = ColorEnableDefault;
    m_SensorProps[ENUM_COLORENABLE].ulDefaultValue    = ColorEnableDefault;
    m_SensorProps[ENUM_COLORENABLE].pRangeNStep       = &ColorEnableRangeAndStep[0];
    m_SensorProps[ENUM_COLORENABLE].ulFlags           = CSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO;
    m_SensorProps[ENUM_COLORENABLE].ulCapabilities    = CSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL|CSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO;
    m_SensorProps[ENUM_COLORENABLE].fSetSupported     = VideoProcAmpProperties[ENUM_COLORENABLE].SetSupported;
    m_SensorProps[ENUM_COLORENABLE].fGetSupported     = VideoProcAmpProperties[ENUM_COLORENABLE].GetSupported;
    m_SensorProps[ENUM_COLORENABLE].pCsPropValues     = &ColorEnableValues;

    m_SensorProps[ENUM_WHITEBALANCE].ulCurrentValue   = WhiteBalanceDefault;
    m_SensorProps[ENUM_WHITEBALANCE].ulDefaultValue   = WhiteBalanceDefault;
    m_SensorProps[ENUM_WHITEBALANCE].pRangeNStep      = &WhiteBalanceRangeAndStep[0];
    m_SensorProps[ENUM_WHITEBALANCE].ulFlags          = CSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO;
    m_SensorProps[ENUM_WHITEBALANCE].ulCapabilities   = CSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL|CSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO;
    m_SensorProps[ENUM_WHITEBALANCE].fSetSupported    = VideoProcAmpProperties[ENUM_WHITEBALANCE].SetSupported;
    m_SensorProps[ENUM_WHITEBALANCE].fGetSupported    = VideoProcAmpProperties[ENUM_WHITEBALANCE].GetSupported;
    m_SensorProps[ENUM_WHITEBALANCE].pCsPropValues    = &WhiteBalanceValues;

    m_SensorProps[ENUM_BACKLIGHT_COMPENSATION].ulCurrentValue = BackLightCompensationDefault;
    m_SensorProps[ENUM_BACKLIGHT_COMPENSATION].ulDefaultValue = BackLightCompensationDefault;
    m_SensorProps[ENUM_BACKLIGHT_COMPENSATION].pRangeNStep    = &BackLightCompensationRangeAndStep[0];
    m_SensorProps[ENUM_BACKLIGHT_COMPENSATION].ulFlags        = CSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO;
    m_SensorProps[ENUM_BACKLIGHT_COMPENSATION].ulCapabilities = CSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO;
    m_SensorProps[ENUM_BACKLIGHT_COMPENSATION].fSetSupported  = VideoProcAmpProperties[ENUM_BACKLIGHT_COMPENSATION].SetSupported;
    m_SensorProps[ENUM_BACKLIGHT_COMPENSATION].fGetSupported  = VideoProcAmpProperties[ENUM_BACKLIGHT_COMPENSATION].GetSupported;
    m_SensorProps[ENUM_BACKLIGHT_COMPENSATION].pCsPropValues  = &BackLightCompensationValues;

    m_SensorProps[ENUM_GAIN].ulCurrentValue           = GainDefault;
    m_SensorProps[ENUM_GAIN].ulDefaultValue           = GainDefault;
    m_SensorProps[ENUM_GAIN].pRangeNStep              = &GainRangeAndStep[0];
    m_SensorProps[ENUM_GAIN].ulFlags                  = CSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO;
    m_SensorProps[ENUM_GAIN].ulCapabilities           = CSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO;
    m_SensorProps[ENUM_GAIN].fSetSupported            = VideoProcAmpProperties[ENUM_GAIN].SetSupported;
    m_SensorProps[ENUM_GAIN].fGetSupported            = VideoProcAmpProperties[ENUM_GAIN].GetSupported;
    m_SensorProps[ENUM_GAIN].pCsPropValues            = &GainValues;

    //CameraControl
    m_SensorProps[ENUM_PAN].ulCurrentValue            = PanDefault;
    m_SensorProps[ENUM_PAN].ulDefaultValue            = PanDefault;
    m_SensorProps[ENUM_PAN].pRangeNStep               = &PanRangeAndStep[0];
    m_SensorProps[ENUM_PAN].ulFlags                   = CSPROPERTY_CAMERACONTROL_FLAGS_AUTO;
    m_SensorProps[ENUM_PAN].ulCapabilities            = CSPROPERTY_CAMERACONTROL_FLAGS_MANUAL|CSPROPERTY_CAMERACONTROL_FLAGS_AUTO;
    m_SensorProps[ENUM_PAN].fSetSupported             = VideoProcAmpProperties[ENUM_PAN-NUM_VIDEOPROCAMP_ITEMS].SetSupported;
    m_SensorProps[ENUM_PAN].fGetSupported             = VideoProcAmpProperties[ENUM_PAN-NUM_VIDEOPROCAMP_ITEMS].GetSupported;
    m_SensorProps[ENUM_PAN].pCsPropValues             = &PanValues;

    m_SensorProps[ENUM_TILT].ulCurrentValue           = TiltDefault;
    m_SensorProps[ENUM_TILT].ulDefaultValue           = TiltDefault;
    m_SensorProps[ENUM_TILT].pRangeNStep              = &TiltRangeAndStep[0];
    m_SensorProps[ENUM_TILT].ulFlags                  = CSPROPERTY_CAMERACONTROL_FLAGS_AUTO;
    m_SensorProps[ENUM_TILT].ulCapabilities           = CSPROPERTY_CAMERACONTROL_FLAGS_MANUAL|CSPROPERTY_CAMERACONTROL_FLAGS_AUTO;
    m_SensorProps[ENUM_TILT].fSetSupported            = VideoProcAmpProperties[ENUM_TILT-NUM_VIDEOPROCAMP_ITEMS].SetSupported;
    m_SensorProps[ENUM_TILT].fGetSupported            = VideoProcAmpProperties[ENUM_TILT-NUM_VIDEOPROCAMP_ITEMS].GetSupported;
    m_SensorProps[ENUM_TILT].pCsPropValues            = &TiltValues;

    m_SensorProps[ENUM_ROLL].ulCurrentValue           = RollDefault;
    m_SensorProps[ENUM_ROLL].ulDefaultValue           = RollDefault;
    m_SensorProps[ENUM_ROLL].pRangeNStep              = &RollRangeAndStep[0];
    m_SensorProps[ENUM_ROLL].ulFlags                  = CSPROPERTY_CAMERACONTROL_FLAGS_AUTO;
    m_SensorProps[ENUM_ROLL].ulCapabilities           = CSPROPERTY_CAMERACONTROL_FLAGS_MANUAL|CSPROPERTY_CAMERACONTROL_FLAGS_AUTO;
    m_SensorProps[ENUM_ROLL].fSetSupported            = VideoProcAmpProperties[ENUM_ROLL-NUM_VIDEOPROCAMP_ITEMS].SetSupported;
    m_SensorProps[ENUM_ROLL].fGetSupported            = VideoProcAmpProperties[ENUM_ROLL-NUM_VIDEOPROCAMP_ITEMS].GetSupported;
    m_SensorProps[ENUM_ROLL].pCsPropValues            = &RollValues;

    m_SensorProps[ENUM_ZOOM].ulCurrentValue           = ZoomDefault;
    m_SensorProps[ENUM_ZOOM].ulDefaultValue           = ZoomDefault;
    m_SensorProps[ENUM_ZOOM].pRangeNStep              = &ZoomRangeAndStep[0];
    m_SensorProps[ENUM_ZOOM].ulFlags                  = CSPROPERTY_CAMERACONTROL_FLAGS_AUTO;
    m_SensorProps[ENUM_ZOOM].ulCapabilities           = CSPROPERTY_CAMERACONTROL_FLAGS_MANUAL|CSPROPERTY_CAMERACONTROL_FLAGS_AUTO;
    m_SensorProps[ENUM_ZOOM].fSetSupported            = VideoProcAmpProperties[ENUM_ZOOM-NUM_VIDEOPROCAMP_ITEMS].SetSupported;
    m_SensorProps[ENUM_ZOOM].fGetSupported            = VideoProcAmpProperties[ENUM_ZOOM-NUM_VIDEOPROCAMP_ITEMS].GetSupported;
    m_SensorProps[ENUM_ZOOM].pCsPropValues            = &ZoomValues;

    m_SensorProps[ENUM_IRIS].ulCurrentValue           = IrisDefault;
    m_SensorProps[ENUM_IRIS].ulDefaultValue           = IrisDefault;
    m_SensorProps[ENUM_IRIS].pRangeNStep              = &IrisRangeAndStep[0];
    m_SensorProps[ENUM_IRIS].ulFlags                  = CSPROPERTY_CAMERACONTROL_FLAGS_AUTO;
    m_SensorProps[ENUM_IRIS].ulCapabilities           = CSPROPERTY_CAMERACONTROL_FLAGS_MANUAL|CSPROPERTY_CAMERACONTROL_FLAGS_AUTO;
    m_SensorProps[ENUM_IRIS].fSetSupported            = VideoProcAmpProperties[ENUM_IRIS-NUM_VIDEOPROCAMP_ITEMS].SetSupported;
    m_SensorProps[ENUM_IRIS].fGetSupported            = VideoProcAmpProperties[ENUM_IRIS-NUM_VIDEOPROCAMP_ITEMS].GetSupported;
    m_SensorProps[ENUM_IRIS].pCsPropValues            = &IrisValues;

    m_SensorProps[ENUM_EXPOSURE].ulCurrentValue       = (ULONG)ExposureDefault;
    m_SensorProps[ENUM_EXPOSURE].ulDefaultValue       = (ULONG)ExposureDefault;
    m_SensorProps[ENUM_EXPOSURE].pRangeNStep          = &ExposureRangeAndStep[0];
    m_SensorProps[ENUM_EXPOSURE].ulFlags              = CSPROPERTY_CAMERACONTROL_FLAGS_AUTO;
    m_SensorProps[ENUM_EXPOSURE].ulCapabilities       = CSPROPERTY_CAMERACONTROL_FLAGS_MANUAL|CSPROPERTY_CAMERACONTROL_FLAGS_AUTO;
    m_SensorProps[ENUM_EXPOSURE].fSetSupported        = VideoProcAmpProperties[ENUM_EXPOSURE-NUM_VIDEOPROCAMP_ITEMS].SetSupported;
    m_SensorProps[ENUM_EXPOSURE].fGetSupported        = VideoProcAmpProperties[ENUM_EXPOSURE-NUM_VIDEOPROCAMP_ITEMS].GetSupported;
    m_SensorProps[ENUM_EXPOSURE].pCsPropValues        = &ExposureValues;

    m_SensorProps[ENUM_FOCUS].ulCurrentValue          = FocusDefault;
    m_SensorProps[ENUM_FOCUS].ulDefaultValue          = FocusDefault;
    m_SensorProps[ENUM_FOCUS].pRangeNStep             = &FocusRangeAndStep[0];
    m_SensorProps[ENUM_FOCUS].ulFlags                 = CSPROPERTY_CAMERACONTROL_FLAGS_MANUAL;
    m_SensorProps[ENUM_FOCUS].ulCapabilities          = CSPROPERTY_CAMERACONTROL_FLAGS_MANUAL;
    m_SensorProps[ENUM_FOCUS].fSetSupported           = VideoProcAmpProperties[ENUM_FOCUS-NUM_VIDEOPROCAMP_ITEMS].SetSupported;
    m_SensorProps[ENUM_FOCUS].fGetSupported           = VideoProcAmpProperties[ENUM_FOCUS-NUM_VIDEOPROCAMP_ITEMS].GetSupported;
    m_SensorProps[ENUM_FOCUS].pCsPropValues           = &FocusValues;

    m_SensorProps[ENUM_FLASH].ulCurrentValue          = FlashDefault;
    m_SensorProps[ENUM_FLASH].ulDefaultValue          = FlashDefault;
    m_SensorProps[ENUM_FLASH].pRangeNStep             = &FlashRangeAndStep[0];
    m_SensorProps[ENUM_FLASH].ulFlags                 = CSPROPERTY_CAMERACONTROL_FLAGS_MANUAL;
    m_SensorProps[ENUM_FLASH].ulCapabilities          = CSPROPERTY_CAMERACONTROL_FLAGS_MANUAL;
    m_SensorProps[ENUM_FLASH].fSetSupported           = VideoProcAmpProperties[ENUM_FLASH-NUM_VIDEOPROCAMP_ITEMS].SetSupported;
    m_SensorProps[ENUM_FLASH].fGetSupported           = VideoProcAmpProperties[ENUM_FLASH-NUM_VIDEOPROCAMP_ITEMS].GetSupported;
    m_SensorProps[ENUM_FLASH].pCsPropValues           = &FlashValues;
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    
    m_pModeVideoFormat = NULL;
    // Allocate Video Format specific array.
    m_pModeVideoFormat = new PINVIDEOFORMAT[m_ulCTypes];
    if( NULL == m_pModeVideoFormat )
    {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    static PCS_DATARANGE_VIDEO pcsDataRangeVideo[] = 
    {
        &DCAM_StreamMode_DVD_NTSC_UYVY,
        &DCAM_StreamMode_TV_D1_NTSC_UYVY,
        &DCAM_StreamMode_CIF_NTSC_UYVY,
        &DCAM_StreamMode_QCIF_NTSC_UYVY,
#ifdef ENABLE_YUY2
        &DCAM_StreamMode_DVD_NTSC_YUY2,
        &DCAM_StreamMode_TV_D1_NTSC_YUY2,
        &DCAM_StreamMode_CIF_NTSC_YUY2,
        &DCAM_StreamMode_QCIF_NTSC_YUY2,
#endif
#ifdef ENABLE_PAL
        &DCAM_StreamMode_DVD_PAL_UYVY,
        &DCAM_StreamMode_TV_D1_PAL_UYVY,
        &DCAM_StreamMode_CIF_PAL_UYVY,
        &DCAM_StreamMode_QCIF_PAL_UYVY,
#ifdef ENABLE_YUY2
        &DCAM_StreamMode_DVD_PAL_YUY2,
        &DCAM_StreamMode_TV_D1_PAL_YUY2,
        &DCAM_StreamMode_CIF_PAL_YUY2,
        &DCAM_StreamMode_QCIF_PAL_YUY2,
#endif
#endif
        &DCAM_StreamMode_VGA_UYVY_30, 
        &DCAM_StreamMode_QVGA_UYVY_30,
#ifdef ENABLE_YUY2
        &DCAM_StreamMode_VGA_YUY2_30, 
        &DCAM_StreamMode_QVGA_YUY2_30,
#endif
    };

    int count = sizeof(pcsDataRangeVideo)/sizeof(PCS_DATARANGE_VIDEO);

    // Video Format initialization
    m_pModeVideoFormat[CAPTURE].categoryGUID         = PINNAME_VIDEO_CAPTURE;
    m_pModeVideoFormat[CAPTURE].ulAvailFormats       = count;
    m_pModeVideoFormat[CAPTURE].pCsDataRangeVideo = new PCS_DATARANGE_VIDEO[m_pModeVideoFormat[CAPTURE].ulAvailFormats];

    if( NULL == m_pModeVideoFormat[CAPTURE].pCsDataRangeVideo )
    {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    for (int i = 0; i < count; i++) 
    {
         m_pModeVideoFormat[CAPTURE].pCsDataRangeVideo[i] = pcsDataRangeVideo[i];
    }

    m_pModeVideoFormat[STILL].categoryGUID         = PINNAME_VIDEO_STILL;
    m_pModeVideoFormat[STILL].ulAvailFormats       = count;
    m_pModeVideoFormat[STILL].pCsDataRangeVideo    = new PCS_DATARANGE_VIDEO[m_pModeVideoFormat[STILL].ulAvailFormats];

    if( NULL == m_pModeVideoFormat[STILL].pCsDataRangeVideo )
    {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    for (int i = 0; i < count; i++) 
    {
         m_pModeVideoFormat[STILL].pCsDataRangeVideo[i] = pcsDataRangeVideo[i];
    }

    if( MAX_SUPPORTED_PINS == m_ulCTypes )
    {
        m_pModeVideoFormat[PREVIEW].categoryGUID         = PINNAME_VIDEO_PREVIEW;
        m_pModeVideoFormat[PREVIEW].ulAvailFormats       = count;
        m_pModeVideoFormat[PREVIEW].pCsDataRangeVideo = new PCS_DATARANGE_VIDEO[m_pModeVideoFormat[PREVIEW].ulAvailFormats];

        if( NULL == m_pModeVideoFormat[PREVIEW].pCsDataRangeVideo )
        {
            return ERROR_INSUFFICIENT_BUFFER;
        }

        for (int i = 0; i < count; i++) 
        {
             m_pModeVideoFormat[PREVIEW].pCsDataRangeVideo[i] = pcsDataRangeVideo[i];
        }
    }
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    m_pModeVideoCaps = NULL;
    // Allocate Video Control Caps specific array.
    m_pModeVideoCaps = new VIDCONTROLCAPS[m_ulCTypes];
    if( NULL == m_pModeVideoCaps )
    {
        return ERROR_INSUFFICIENT_BUFFER;
    }
    // Video Control Caps

    m_pModeVideoCaps[CAPTURE].DefaultVideoControlCaps     = DefaultVideoControlCaps[CAPTURE];
    m_pModeVideoCaps[CAPTURE].CurrentVideoControlCaps     = DefaultVideoControlCaps[CAPTURE];
    m_pModeVideoCaps[STILL].DefaultVideoControlCaps     = DefaultVideoControlCaps[STILL];
    m_pModeVideoCaps[STILL].CurrentVideoControlCaps     = DefaultVideoControlCaps[STILL];

    if( MAX_SUPPORTED_PINS == m_ulCTypes )
    {
        // Note PREVIEW control caps are the same, so we don't differentiate
        m_pModeVideoCaps[PREVIEW].DefaultVideoControlCaps     = DefaultVideoControlCaps[PREVIEW];
        m_pModeVideoCaps[PREVIEW].CurrentVideoControlCaps     = DefaultVideoControlCaps[PREVIEW];;
    }

    // Timer specific variables. Only to be used in this NULL PDD
    m_hTimerDll                 = NULL;
    m_pfnTimeSetEvent           = NULL;
    m_pfnTimeKillEvent          = NULL;
    memset( &m_TimerIdentifier, 0, MAX_SUPPORTED_PINS * sizeof(MMRESULT));

    m_SensorModeInfo[STILL].MemoryModel = CSPROPERTY_BUFFER_CLIENT_UNLIMITED;
    m_SensorModeInfo[STILL].MaxNumOfBuffers = 3;
    m_SensorModeInfo[STILL].PossibleCount = 1;

    m_SensorModeInfo[CAPTURE].MemoryModel = CSPROPERTY_BUFFER_CLIENT_UNLIMITED;
    m_SensorModeInfo[CAPTURE].MaxNumOfBuffers = 3;
    m_SensorModeInfo[CAPTURE].PossibleCount = 1;

    if( MAX_SUPPORTED_PINS == m_ulCTypes )
    {
        m_SensorModeInfo[PREVIEW].MemoryModel = CSPROPERTY_BUFFER_CLIENT_UNLIMITED;
        m_SensorModeInfo[PREVIEW].MaxNumOfBuffers = 3;
        m_SensorModeInfo[PREVIEW].PossibleCount = 1;
    }

    m_ppModeContext = new LPVOID[m_ulCTypes];
    if ( NULL == m_ppModeContext )
    {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    m_pCurrentFormat = new CS_DATARANGE_VIDEO[m_ulCTypes];
    if( NULL == m_pCurrentFormat )
    {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    //////    Initial interrupt thread    //////

    // Camera ISP Interrupt Event - IRQ_CAM_0 signaled.
    m_CAMISPEvent = CreateEvent( NULL, FALSE, FALSE, NULL);

    if (!m_CAMISPEvent) {
        ERRORMSG(ZONE_ERROR, (_T("Failed to CreateEvent(m_CAMISPEvent) \r\n")));
        goto clean;
    }

	m_hEndOfFrameEvent = CreateEvent( NULL, FALSE, FALSE, NULL);
	if (m_hEndOfFrameEvent == NULL)
	{
	    ERRORMSG(ZONE_ERROR, (_T("Failed to CreateEvent(m_hEndOfFrameEvent) \r\n")));
        goto clean;
	}

    if(!KernelIoControl(IOCTL_HAL_REQUEST_SYSINTR,
        &dwIrq, sizeof(dwIrq), &m_CAMISPIntr, sizeof(DWORD), NULL)) {
        ERRORMSG(ZONE_ERROR, (_T("Failed to KernelIoControl(IOCTL_HAL_REQUEST_SYSINTR)\r\n")));
        m_CAMISPIntr = 0;
    }

    if (m_CAMISPIntr == 0) {
        ERRORMSG(ZONE_ERROR, (_T(" Failed to allocate camera system interrupt event, m_CAMISPIntr\r\n") ));
        goto clean;
    }

    if (!InterruptInitialize(m_CAMISPIntr, m_CAMISPEvent, NULL, 0)) {
        ERRORMSG(ZONE_ERROR, (_T(" Failed to InterruptInitialize(m_CAMISPIntr) \r\n")));
        goto clean;
    }

    InterruptDone( m_CAMISPIntr );

    m_hParentBus = CreateBusAccessHandle((LPCWSTR)m_hContext);
    if (m_hParentBus == NULL)
    {
        ERRORMSG(ZONE_ERROR, (_T(" Failed to retrieve parent bus handle\r\n")));
        goto clean;
    }

    if (!RequestDevicePads(BSPGetCameraDevice()))
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: CAM_Init: Failed request pads\r\n"));
        goto clean;
    }

    m_hCCDCInterruptThread = CreateThread( NULL,
                                              0,
                      (LPTHREAD_START_ROUTINE)CameraInterruptThread,
                                              this,
                                              0,
                                              NULL); //CREATE_SUSPENDED
    m_pIspCtrl->InitializeCamera(); //Init. ISP 
	m_pIspCtrl->AllocBuffer();
    m_pTvpCtrl->Init(MDDContext);// Init. tvp5146 video decoder.

    m_CurrentPowerState = D4;
    SetPowerState(D0);

    m_pMetadata = (PCS_PROPERTYITEM)Metadata;

    return ERROR_SUCCESS;

clean:

    return ERROR_INVALID_FUNCTION;
}

DWORD CCameraPdd::Open(PVOID MDDOpenContext)
{
    if (MDDOpenContext == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    m_hOpenContext = (HANDLE)MDDOpenContext;
    return ERROR_SUCCESS;
}

DWORD CCameraPdd::Close(PVOID MDDOpenContext)
{
    if (m_hOpenContext != MDDOpenContext)
    {
        return ERROR_INVALID_PARAMETER;
    }

    m_hOpenContext = NULL;
    return ERROR_SUCCESS;
}

DWORD CCameraPdd::PDDDeinit( )
{
    if (!ReleaseDevicePads(BSPGetCameraDevice()))
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: CAM_PDDDeinit: Failed to release pads\r\n"));
    }    

    SetPowerState(D4);

	m_pIspCtrl->DeInitializeCamera();

    return ERROR_SUCCESS;
}

//------------------------------------------------------------------------------
//
//  CameraInterruptThread
//
//  Description: Camera event interrupt handler function
//
//

static DWORD
CameraInterruptThread(LPVOID lpParameter )
{
    CCameraPdd *pThis = reinterpret_cast<CCameraPdd *>(lpParameter);

    pThis->CameraInterruptThreadImpl();

    return ERROR_SUCCESS;
}

//-----------------------------------------------------------------------------
//
//  CameraInterruptThreadImpl
//
//  Description : This is a thread function for handling the camera data events
//                On receiving camera event, the thread pass the buffer to MDD
//                and notifies the MDD to send it to the Dshow middleware.
//
    
DWORD CCameraPdd::CameraInterruptThreadImpl()
{
    DWORD dwEventStatus = 0;
    UINT32 setting      = 0;

    for(;;)
    {       
        /* wait for start */
        dwEventStatus = WaitForSingleObject(m_CAMISPEvent, INFINITE);

        if (m_bInterruptThreadExit)
        {
            break;
        }

        if (dwEventStatus == WAIT_OBJECT_0)
        {               
            BOOL oddField = FALSE;

            setting = INREG32(&m_pIspCtrl->GetCCDCRegs()->CCDC_SYN_MODE);

            if (0 == (setting&ISPCCDC_SYN_MODE_FLDSTAT)) 
			{
                oddField = TRUE;
            }

			setting = m_pIspCtrl->GetInterruptStatus();

#ifdef ENABLE_DEINTERLACED_OUTPUT
            if ((setting & 0x80000000)&& oddField) //VD detected & odd field
#else
            if (setting & 0x80000000) //VD detected
#endif
            {
				SetEvent(m_hEndOfFrameEvent);
			}

			m_pIspCtrl->InterruptAck(setting);
     
            InterruptDone(m_CAMISPIntr);
        }
		else
		{
			DEBUGMSG(ZONE_VERBOSE,(_T("CameraInterruptThreadImpl:Camera thread timed out\r\n")));
		}
    }

    return ERROR_SUCCESS;
}

DWORD CCameraPdd::GetAdapterInfo( PADAPTERINFO pAdapterInfo )
{
    pAdapterInfo->ulCTypes = m_ulCTypes;
    pAdapterInfo->PowerCaps = m_PowerCaps;
    pAdapterInfo->ulVersionID = DRIVER_VERSION_2; //Camera MDD and DShow support DRIVER_VERSION and DRIVER_VERSION_2. Defined in camera.h
    memcpy( &pAdapterInfo->SensorProps, &m_SensorProps, sizeof(m_SensorProps));

    return ERROR_SUCCESS;

}

DWORD CCameraPdd::HandleVidProcAmpChanges( DWORD dwPropId, LONG lFlags, LONG lValue )
{
    PSENSOR_PROPERTY pDevProp = NULL;

    EnterCriticalSection(&m_csSensorState);

    pDevProp = m_SensorProps + dwPropId;
    
    if( CSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL == lFlags )
    {
        pDevProp->ulCurrentValue = lValue;
    }

    pDevProp->ulFlags = lFlags;

    LeaveCriticalSection(&m_csSensorState);

    return ERROR_SUCCESS;
}

DWORD CCameraPdd::HandleCamControlChanges( DWORD dwPropId, LONG lFlags, LONG lValue )
{
    PSENSOR_PROPERTY pDevProp = NULL;
    
    EnterCriticalSection(&m_csSensorState);

    pDevProp = m_SensorProps + dwPropId;
    
    if( CSPROPERTY_CAMERACONTROL_FLAGS_MANUAL == lFlags )
    {
        pDevProp->ulCurrentValue = lValue;
    }

    pDevProp->ulFlags = lFlags;

    LeaveCriticalSection(&m_csSensorState);

    return ERROR_SUCCESS;
}

DWORD CCameraPdd::HandleVideoControlCapsChanges( LONG lModeType ,ULONG ulCaps )
{
    m_pModeVideoCaps[lModeType].CurrentVideoControlCaps = ulCaps;
    return ERROR_SUCCESS;
}

DWORD CCameraPdd :: SetPowerState( CEDEVICE_POWER_STATE PowerState )
{
    DEBUGMSG(ZONE_FUNCTION,( _T("CCameraPdd::SetPowerState - State=%d\r\n"), PowerState));

    if (PowerState <= D3 && m_CurrentPowerState >= D3)
    {
        // Set the camera Power state to D0
        m_pTvpCtrl->SetPowerState(TRUE);
        SetDevicePowerState(m_hParentBus, D0, NULL);
	}
	else if (PowerState >= D3 && m_CurrentPowerState <= D2)
    {
        // Set the camera Power state to D4
        SetDevicePowerState(m_hParentBus, D4, NULL);
        m_pTvpCtrl->SetPowerState(FALSE);
	}

    m_CurrentPowerState = PowerState;
		
    return ERROR_SUCCESS;
}

DWORD CCameraPdd::HandleAdapterCustomProperties( PUCHAR pInBuf, DWORD  InBufLen, PUCHAR pOutBuf, DWORD  OutBufLen, PDWORD pdwBytesTransferred )
{
    DEBUGMSG( ZONE_IOCTL, ( _T("IOControl Adapter PDD: HandleAdapterCustomProperties Request\r\n")) );

    DWORD dwRet = ERROR_SUCCESS;
 
    UNREFERENCED_PARAMETER(pdwBytesTransferred);
    
    if(InBufLen == sizeof(CSPROPERTY_CAMERACONTROL_S))
    {
        DEBUGMSG(ZONE_VERBOSE, (L"InBufLen == sizeof(CameraControl)()\r\n"));
        CSPROPERTY_CAMERACONTROL_S  CameraControl;
        memcpy(&CameraControl, pInBuf, InBufLen);
       
        switch( CameraControl.Property.Flags )
        {
            case CSPROPERTY_TYPE_GET:
            {
            
                CameraControlProperty_t prop = (CameraControlProperty_t) CameraControl.Property.Id;
                if (prop == CameraControl_InputPort)
                {
                    DEBUGMSG(ZONE_VERBOSE, (L"CSPROPERTY_TYPE_GET(), m_inputPort=%d\r\n", m_inputPort));

                    CSPROPERTY_CAMERACONTROL_S  CameraControlOut;
                    CameraControlOut.Value = (LONG) m_inputPort;
                    memcpy(pOutBuf, &CameraControlOut, OutBufLen);
                    
                    dwRet = ERROR_SUCCESS;
                    break;
                }
            }
            dwRet = ERROR_NOT_SUPPORTED;
            break;
            
            case CSPROPERTY_TYPE_SET:
            {
        
                DEBUGMSG(ZONE_ACTIVITY, (L"CSPROPERTY_TYPE_SET()\r\n"));

                CameraControlProperty_t prop = (CameraControlProperty_t) CameraControl.Property.Id;
        
                switch (prop)
                {

                    case CameraControl_InputPort:
                    {
                        DEBUGMSG(ZONE_ACTIVITY, (L"CameraControl_InputPort CameraControl.Value(%d)\r\n", CameraControl.Value));
                        m_inputPort = (CameraInputPort_t) CameraControl.Value;
                        switch(m_inputPort)
                        {
                            case CameraInput_YPbPr:
                                m_pTvpCtrl->SelectComponent();
                                break;
                            case CameraInput_AV:
                                m_pTvpCtrl->SelectComposite();
                                break;
                            case CameraInput_SVideo:
                                m_pTvpCtrl->SelectSVideo();
                                break;
                            default:
                                dwRet = ERROR_INVALID_DATA;
                                break;
                        }
                    }
                    break;

                    default:
                    {
                        dwRet = ERROR_NOT_SUPPORTED;
                    }
                    break;
                }
            }
            break;
                
            default :
            {
                dwRet = ERROR_NOT_SUPPORTED;
            }
            break;
        }

        DEBUGMSG(ZONE_VERBOSE, (L"CameraControl.Property.Flags(%d)\r\n", CameraControl.Property.Flags));
        DEBUGMSG(ZONE_VERBOSE, (L"CameraControl.Property.Id(%d)\r\n", CameraControl.Property.Id));
        DEBUGMSG(ZONE_VERBOSE, (L"CameraControl.Flags(%d)\r\n", CameraControl.Flags));
        DEBUGMSG(ZONE_VERBOSE, (L"CameraControl.Value(%d)\r\n", CameraControl.Value));
        DEBUGMSG(ZONE_VERBOSE, (L"CameraControl.Capabilities(%d)\r\n", CameraControl.Capabilities));
        DEBUGMSG(ZONE_VERBOSE, (L"CSPROPERTY_TYPE_SET(%d)\r\n", CSPROPERTY_TYPE_SET));
        
        return dwRet;
        
    }
    return ERROR_NOT_SUPPORTED;
}

DWORD CCameraPdd::InitSensorMode( ULONG ulModeType, LPVOID ModeContext )
{
    ASSERT( ModeContext );
    DEBUGMSG(ZONE_ERROR,(_T("InitSensorMode: lModeType=%d, ModeContext=0x%08X\r\n"),
        ulModeType,ModeContext));
    m_ppModeContext[ulModeType] = ModeContext;
    return ERROR_SUCCESS;
}

DWORD CCameraPdd::DeInitSensorMode( ULONG ulModeType )
{
    UNREFERENCED_PARAMETER(ulModeType);

    return ERROR_SUCCESS;
}

DWORD CCameraPdd::Run(ULONG lModeType)
{
	DWORD dwError = ERROR_SUCCESS;

    DEBUGMSG(ZONE_FUNCTION, (_T("+SetSensorState: CSSTATE_RUN: lModeType=%d\r\n"),lModeType));

    m_CsState[lModeType] = CSSTATE_RUN;  

    if ((lModeType==CAPTURE) || (lModeType==PREVIEW))
    {
		if (NULL == m_TimerIdentifier[lModeType])
		{
			dwError = CreateTimer(lModeType) ? ERROR_SUCCESS : ERROR_NOT_READY;
		}

		ResetEvent(m_hEndOfFrameEvent);

        if(!m_pIspCtrl->EnableCamera(&m_pCurrentFormat[lModeType]))
		{
            return ERROR_INVALID_PARAMETER;         
		}
	}

    return  dwError;            
}

DWORD CCameraPdd::Pause(ULONG lModeType)
{   
    DEBUGMSG(ZONE_FUNCTION, (_T("+SetSensorState: CSSTATE_PAUSE: lModeType=%d\r\n"),lModeType));

    m_CsState[lModeType] = CSSTATE_PAUSE;

	if ((CAPTURE == lModeType) || (PREVIEW == lModeType))
	{
        if(!m_pIspCtrl->DisableCamera())
		{
			DEBUGMSG(ZONE_ERROR,(_T("SetSensorState: Failed to disable Camera\r\n")));
            return ERROR_INVALID_PARAMETER;                     
		}

		ResetEvent(m_hEndOfFrameEvent);
    }        

    return  ERROR_SUCCESS;
}

DWORD CCameraPdd::Stop(ULONG lModeType)
{
    DEBUGMSG(ZONE_FUNCTION, (_T("+SetSensorState: CSSTATE_STOP: lModeType=%d\r\n"),lModeType));
    
    m_CsState[lModeType] = CSSTATE_STOP;

    // Kill the timer to conserve resources
    if ( NULL != m_TimerIdentifier[lModeType] )            
    {
        ASSERT( m_pfnTimeKillEvent );
        m_pfnTimeKillEvent(m_TimerIdentifier[lModeType]);
        m_TimerIdentifier[lModeType] = NULL;
    }

    if (STILL == lModeType)
    {
        m_bStillCapInProgress = false;
    }
	else
	{
        if(!m_pIspCtrl->DisableCamera())
		{
            return ERROR_INVALID_PARAMETER;                     
		}
    } 

    return  ERROR_SUCCESS;
}
    
DWORD CCameraPdd::SetSensorState( ULONG lModeType, CSSTATE csState )
{
    DWORD dwError = ERROR_SUCCESS;

    EnterCriticalSection(&m_csSensorState);

    switch ( csState )
    {
        case CSSTATE_STOP:
            dwError = Stop(lModeType);
            break;

        case CSSTATE_PAUSE:
            dwError = Pause(lModeType);
            break;

        case CSSTATE_RUN:
            dwError = Run(lModeType);                      
            break;

        default:
            ERRORMSG( ZONE_IOCTL|ZONE_ERROR, ( _T("IOControl(%08x): Incorrect State\r\n"), this ) );
            dwError = ERROR_INVALID_PARAMETER;
    }

    LeaveCriticalSection(&m_csSensorState);
    return dwError;
}

DWORD CCameraPdd::TakeStillPicture(LPVOID /*pBurstModeInfo*/)
{
    DWORD dwError = ERROR_SUCCESS;

	DEBUGMSG(ZONE_FUNCTION, (_T("TakeStillPicture\r\n")));

    EnterCriticalSection(&m_csSensorState);
    m_bStillCapInProgress = true;

    //Ignore pBurstModeInfo
    m_CsState[STILL] = CSSTATE_RUN;

    if (NULL == m_TimerIdentifier[STILL])
    {
        LeaveCriticalSection(&m_csSensorState);
        return CreateTimer(STILL) ? ERROR_SUCCESS : ERROR_NOT_READY;
    }

    LeaveCriticalSection(&m_csSensorState);
    return dwError;
}

DWORD CCameraPdd::GetSensorModeInfo( ULONG ulModeType, PSENSORMODEINFO pSensorModeInfo )
{
    pSensorModeInfo->MemoryModel = m_SensorModeInfo[ulModeType].MemoryModel;
    pSensorModeInfo->MaxNumOfBuffers = m_SensorModeInfo[ulModeType].MaxNumOfBuffers;
    pSensorModeInfo->PossibleCount = m_SensorModeInfo[ulModeType].PossibleCount;
    pSensorModeInfo->VideoCaps.DefaultVideoControlCaps = DefaultVideoControlCaps[ulModeType];
    pSensorModeInfo->VideoCaps.CurrentVideoControlCaps = m_pModeVideoCaps[ulModeType].CurrentVideoControlCaps;
    pSensorModeInfo->pVideoFormat = &m_pModeVideoFormat[ulModeType];
    
    return ERROR_SUCCESS;
}

DWORD CCameraPdd::SetSensorModeFormat( ULONG ulModeType, PCS_DATARANGE_VIDEO pCsDataRangeVideo )
{
    PCS_VIDEOINFOHEADER pCsVideoInfoHdr = &(pCsDataRangeVideo->VideoInfoHeader);

#ifndef SHIP_BUILD
    UINT biWidth        = pCsVideoInfoHdr->bmiHeader.biWidth;
    UINT biHeight       = pCsVideoInfoHdr->bmiHeader.biHeight;
    UINT biSizeImage    = pCsVideoInfoHdr->bmiHeader.biSizeImage;
    UINT biWidthBytes   = CS_DIBWIDTHBYTES (pCsVideoInfoHdr->bmiHeader);
#endif 

    DWORD biCompression = pCsVideoInfoHdr->bmiHeader.biCompression & ~BI_SRCPREROTATE;

    DEBUGMSG(ZONE_IOCTL,
        (_T("CCameraPdd::SetSensorModeFormat: biWidth=%d, biHeight=%d biSizeImage=%d biWidthBytes=%d\r\n"),
        biWidth, biHeight, biSizeImage, biWidthBytes));

    if (biCompression == FOURCC_UYVY) {
        DEBUGMSG(ZONE_IOCTL,
            (_T("CCameraPdd::SetSensorModeFormat: biCompression=UYVY\r\n")));
    } 
    else if (biCompression == FOURCC_YUY2) {
            DEBUGMSG(ZONE_IOCTL,
                (_T("CCameraPdd::SetSensorModeFormat: biCompression=YUY2\r\n")));
    } 
    else {
        DEBUGMSG(ZONE_IOCTL,
            (_T("CCameraPdd::SetSensorModeFormat: ERROR! Unsupported biCompression format\r\n")));
        return ERROR_NOT_SUPPORTED;
    }
    memcpy( &m_pCurrentFormat[ulModeType], pCsDataRangeVideo, sizeof ( CS_DATARANGE_VIDEO ) );

    return ERROR_SUCCESS;
}

PVOID CCameraPdd::AllocateBuffer( ULONG ulModeType )
{  
    // Real PDD may want to save off this allocated pointer
    // in an array.
    ULONG ulFrameSize = CS__DIBSIZE (m_pCurrentFormat[ulModeType].VideoInfoHeader.bmiHeader);
    return RemoteLocalAlloc( LPTR, ulFrameSize );
}

DWORD CCameraPdd::DeAllocateBuffer( ULONG ulModeType, PVOID pBuffer )
{
    UNREFERENCED_PARAMETER(ulModeType);

    RemoteLocalFree( pBuffer );
    return ERROR_SUCCESS;
}

DWORD CCameraPdd::RegisterClientBuffer( ULONG ulModeType, PVOID pBuffer )
{
    UNREFERENCED_PARAMETER(ulModeType);
    UNREFERENCED_PARAMETER(pBuffer);

    // Real PDD may want to save pBuffer which is a pointer to buffer that DShow created.   
    return ERROR_SUCCESS;
}

DWORD CCameraPdd::UnRegisterClientBuffer( ULONG ulModeType, PVOID pBuffer )
{
    UNREFERENCED_PARAMETER(ulModeType);
    UNREFERENCED_PARAMETER(pBuffer);

    // DShow is not going to use pBuffer (which was originally allocated by DShow) anymore. If the PDD
    // is keeping a cached pBuffer pointer (in RegisterClientBuffer()) then this is the right place to
    // stop using it and maybe set the cached pointer to NULL. 
    // Note: PDD must not delete this pointer as it will be deleted by DShow itself
    return ERROR_SUCCESS;
}

DWORD CCameraPdd::GetMetadata(DWORD dwPropId, PUCHAR pOutBuf, DWORD OutBufLen, PDWORD pdwBytesTransferred)
{
    if (CSPROPERTY_METADATA_ALL != dwPropId)
    {
        return ERROR_NOT_SUPPORTED;
    }

    if (NULL == m_pMetadata)
    {
        return ERROR_INTERNAL_ERROR;
    }

    DWORD dwBufSize = sizeof(CS_PROPERTYITEM);
    dwBufSize *= NUMBER_OF_PROPERTYITEMS;
    dwBufSize += sizeof(ULONG);

    for (int i = 0; i < NUMBER_OF_PROPERTYITEMS; ++i)
    {
        dwBufSize += m_pMetadata[i].ulLength;
    }

    if (NULL == pOutBuf)
    {
        if (NULL == pdwBytesTransferred)
        {
            return ERROR_INVALID_PARAMETER;
        }

        *pdwBytesTransferred = dwBufSize;
        return ERROR_SUCCESS;
    }

    if (OutBufLen < dwBufSize)
    {
        return ERROR_INVALID_PARAMETER;
    }

    PCSMETADATA_S pMetadata = (PCSMETADATA_S)pOutBuf;
    pMetadata->ulCount = NUMBER_OF_PROPERTYITEMS;
    PBYTE pCurData = (PBYTE)&pMetadata->rgitemMetadata[NUMBER_OF_PROPERTYITEMS];

    for (int i = 0; i < NUMBER_OF_PROPERTYITEMS; ++i)
    {
        pMetadata->rgitemMetadata[i] = m_pMetadata[i];
        pMetadata->rgitemMetadata[i].ulDataOffset = pCurData - (PBYTE)pMetadata;
        memcpy(pCurData, (PBYTE)m_pMetadata[i].ulDataOffset, m_pMetadata[i].ulLength);
        pCurData += m_pMetadata[i].ulLength;
    }

    if (NULL != pdwBytesTransferred)
    {
        *pdwBytesTransferred = dwBufSize;
    }

    return ERROR_SUCCESS;
}

DWORD CCameraPdd::HandleSensorModeCustomProperties( ULONG ulModeType, PUCHAR pInBuf, DWORD  InBufLen, PUCHAR pOutBuf, DWORD  OutBufLen, PDWORD pdwBytesTransferred )
{
    UNREFERENCED_PARAMETER(ulModeType);
    UNREFERENCED_PARAMETER(pInBuf);
    UNREFERENCED_PARAMETER(InBufLen);
    UNREFERENCED_PARAMETER(pOutBuf);
    UNREFERENCED_PARAMETER(OutBufLen);
    UNREFERENCED_PARAMETER(pdwBytesTransferred);

    DEBUGMSG( ZONE_IOCTL, ( _T("IOControl: Unsupported PropertySet Request\r\n")) );
    return ERROR_NOT_SUPPORTED;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The following code is only meant for this sample pdd
// The real PDD should not contain any of the code below.
// Instead the real PDD should implement its own FillPinBuffer() 
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

extern "C" { WINGDIAPI HBITMAP WINAPI CreateBitmapFromPointer( CONST BITMAPINFO *pbmi, int iStride, PVOID pvBits); }

#ifndef ENABLE_PACK8
LPVOID UYVYtoYUY2(WORD* pInput, BYTE* pOutput, DWORD dwNumByte)
{
    ULONG* pIn= (ULONG*) pInput;
    PBYTE pOut=pOutput;
    
    if(dwNumByte/2==0)
        return NULL;

    DWORD i=0;
    for(i; i<dwNumByte/2; i++)
    {
        pOut[i*2]=(BYTE) (pIn[i]>>18);
        pOut[i*2+1]=(BYTE) (pIn[i] >>2);
    }
    
    return pOutput;
}
#else

LPVOID UYVYtoYUY2(WORD* pInput, BYTE* pOutput, DWORD dwNumByte)
{
    WORD* pIn= (WORD*) pInput;
    PBYTE pOut=pOutput;
    
    if(dwNumByte/2==0)
        return NULL;

    DWORD i=0;
    for(i; i<dwNumByte/2; i++)
    {
        pOut[i*2]=(BYTE) (pIn[i]>>8);
        pOut[i*2+1]=(BYTE) (pIn[i]);
    }
    
    return pOutput;
}
#endif //ENABLE_PACK8

DWORD CCameraPdd::FillBuffer( ULONG ulModeType, PUCHAR pImage )
{
    PCS_VIDEOINFOHEADER pCsVideoInfoHdr = &m_pCurrentFormat[ulModeType].VideoInfoHeader;
	DWORD dwEventStatus = 0;

#ifndef SHIP_BUILD
    LONG biWidth         = pCsVideoInfoHdr->bmiHeader.biWidth;
    LONG biHeight        = pCsVideoInfoHdr->bmiHeader.biHeight;
#endif

    DWORD biSizeImage    = pCsVideoInfoHdr->bmiHeader.biSizeImage;
    DWORD biCompression  = pCsVideoInfoHdr->bmiHeader.biCompression & ~BI_SRCPREROTATE;

    DEBUGMSG(ZONE_FUNCTION,(_T("CCameraPdd::FillBuffer: biWidth=%d, biHeight=%d, biSizeImage=%d, pImage=0x%08X \r\n"),
            biWidth, biHeight, biSizeImage, pImage));

    if ((biCompression != FOURCC_UYVY) && (biCompression != FOURCC_YUY2))
	{
        ERRORMSG(ZONE_ERROR,(_T("CCameraPdd::FillBuffer: Unsupported format \r\n")));
        return 0;
    }

	for (;;)
	{
		dwEventStatus = WaitForSingleObject(m_hEndOfFrameEvent, 1000);

		if (dwEventStatus == WAIT_TIMEOUT)
		{
			DEBUGMSG(ZONE_WARN,(_T("FillBuffer:Camera thread timed out\r\n")));  
			return 0;
		} 
		else
		{      
			CeSafeCopyMemory(pImage, m_pIspCtrl->GetFrameBuffer(), biSizeImage);
			CacheRangeFlush(m_pIspCtrl->GetFrameBuffer(), biSizeImage, TI_CACHE_SYNC_INVALIDATE);
			break;
		}
	}
        
    return biSizeImage;
}

bool CCameraPdd :: CreateTimer( ULONG ulModeType )
{
	DEBUGMSG(ZONE_FUNCTION, (_T("CreateTimer(%08x): ulModeType = %d\r\n"), this, ulModeType));

    if ( NULL == m_hTimerDll )
    {
        m_hTimerDll        = LoadLibrary( L"MMTimer.dll" );
        m_pfnTimeSetEvent  = (FNTIMESETEVENT)GetProcAddress( m_hTimerDll, L"timeSetEvent" );
        m_pfnTimeKillEvent = (FNTIMEKILLEVENT)GetProcAddress( m_hTimerDll, L"timeKillEvent" );

        if ( NULL == m_pfnTimeSetEvent || NULL == m_pfnTimeKillEvent )
        {
            ERRORMSG(ZONE_IOCTL|ZONE_ERROR, (_T("IOControl(%08x): GetProcAddress Returned Null.\r\n"), this));
            return false;
        }
    }
    
    if ( NULL == m_hTimerDll )
    {
            ERRORMSG(ZONE_IOCTL|ZONE_ERROR, (_T("IOControl(%08x): LoadLibrary failed.\r\n"), this));
            return false;
    }

    if (ulModeType >= MAX_SUPPORTED_PINS)
        return FALSE;

    ASSERT( m_pfnTimeSetEvent );

    if ( NULL == m_TimerIdentifier[ulModeType] )
    {
        DEBUGMSG(ZONE_IOCTL|ZONE_ERROR, (_T("IOControl(%08x): Creating new timer.\r\n"), this));
        
		if( STILL == ulModeType )
		{
			m_TimerIdentifier[ulModeType] = m_pfnTimeSetEvent((ULONG)m_pCurrentFormat[ulModeType].VideoInfoHeader.AvgTimePerFrame/10000, 10, CCameraPdd::StillTimerCallBack, reinterpret_cast<DWORD>(this), TIME_PERIODIC|TIME_CALLBACK_FUNCTION);
		}
		else
        {
            m_TimerIdentifier[ulModeType] = m_pfnTimeSetEvent((ULONG)m_pCurrentFormat[ulModeType].VideoInfoHeader.AvgTimePerFrame/10000, 10, CCameraPdd::CaptureTimerCallBack, reinterpret_cast<DWORD>(this), TIME_PERIODIC|TIME_CALLBACK_FUNCTION);
        }

        if ( NULL == m_TimerIdentifier[ulModeType] )
        {
            DEBUGMSG(ZONE_IOCTL|ZONE_ERROR, (_T("IOControl(%08x): Timer could not be created.\r\n"), this));
            return false;
        }
    }
    else
    {
        DEBUGMSG(ZONE_IOCTL|ZONE_ERROR, (_T("IOControl(%08x): Timer already created.\r\n"), this));
    }

    return true;
}

void CCameraPdd :: CaptureTimerCallBack( UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2 )
{
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(dw1);
    UNREFERENCED_PARAMETER(dw2);

    CCameraPdd *pCameraPdd= reinterpret_cast<CCameraPdd *>(dwUser);
    if( NULL == pCameraPdd )
    {
        DEBUGMSG(ZONE_IOCTL|ZONE_ERROR, (_T("IOControl: TimerCallBack pCameraPdd is NULL.\r\n"))) ;
    }
    else
    {
        __try
        {
            pCameraPdd->HandleCaptureInterrupt( uTimerID );
        }
        __except(GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
        {
            DEBUGMSG(ZONE_IOCTL|ZONE_ERROR, (_T("IOControl: TimerCallBack, Access violation.\r\n"))) ;
        }
    }
}

void CCameraPdd :: StillTimerCallBack(UINT uTimerID, UINT /*uMsg*/, DWORD_PTR dwUser, DWORD_PTR /*dw1*/, DWORD_PTR /*dw2*/)
{
    CCameraPdd *pCameraPdd= reinterpret_cast<CCameraPdd *>(dwUser);
    if (NULL == pCameraPdd)
    {
        DEBUGMSG(ZONE_IOCTL|ZONE_ERROR, (_T("IOControl: TimerCallBack pCameraPdd is NULL.\r\n"))) ;
    }
    else
    {
        __try
        {
            pCameraPdd->HandleStillInterrupt(uTimerID);
        }
        __except(GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
        {
            DEBUGMSG(ZONE_IOCTL|ZONE_ERROR, (_T("IOControl: TimerCallBack, Access violation.\r\n"))) ;
        }
    }
}

void CCameraPdd :: HandleCaptureInterrupt( UINT uTimerID )
{
    ULONG ulModeType;

    EnterCriticalSection(&m_csSensorState);

    if (m_bStillCapInProgress)
    {
        LeaveCriticalSection(&m_csSensorState);
        return;
    }

    if (m_TimerIdentifier[CAPTURE] == uTimerID)
    {
        ulModeType = CAPTURE;
    }
    else if (m_ulCTypes == 3 && m_TimerIdentifier[PREVIEW] == uTimerID)
    {
        ulModeType = PREVIEW;
    }
    else
    {
        LeaveCriticalSection(&m_csSensorState);
		ERRORMSG(TRUE, (_T("HandleCaptureInterrupt: Wrong Timer ID = %d\r\n"), uTimerID));
        ASSERT(false);
        return;
    }

	DEBUGMSG(ZONE_FUNCTION, (_T("HandleCaptureInterrupt: ulModeType = %d\r\n"), ulModeType));

    // If we are not currently running, just kill the timer and return
    if (CSSTATE_RUN != m_CsState[ulModeType])
    {
        ASSERT(m_pfnTimeKillEvent);

        if (m_pfnTimeKillEvent)
        {
            m_pfnTimeKillEvent(m_TimerIdentifier[ulModeType]);
            m_TimerIdentifier[ulModeType] = NULL;
        }

        LeaveCriticalSection(&m_csSensorState);
        return;
    }

    LeaveCriticalSection(&m_csSensorState);
    MDD_HandleIO(m_ppModeContext[ulModeType], ulModeType);
}

void CCameraPdd::HandleStillInterrupt(UINT /*uTimerID*/)
{
    EnterCriticalSection(&m_csSensorState);

    // If we are not currently running, just kill the timer and return
    if (CSSTATE_RUN != m_CsState[STILL])
    {
        ASSERT(m_pfnTimeKillEvent);

        if (m_pfnTimeKillEvent)
        {
            m_pfnTimeKillEvent(m_TimerIdentifier[STILL]);
            m_TimerIdentifier[STILL] = NULL;
        }

        LeaveCriticalSection(&m_csSensorState);
        return;
    }

    LeaveCriticalSection(&m_csSensorState);
    MDD_HandleIO(m_ppModeContext[STILL], STILL);
    m_bStillCapInProgress = false;
}

bool CCameraPdd::ReadMemoryModelFromRegistry()
{
    HKEY  hKey = 0;
    DWORD dwType  = 0;
    DWORD dwSize  = sizeof ( DWORD );
    DWORD dwValue = (DWORD)-1;

    if( ERROR_SUCCESS != RegOpenKeyEx( HKEY_LOCAL_MACHINE, L"Drivers\\Capture\\SampleCam", 0, 0, &hKey ))
    {
        false;
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
    // valid options is 1 (capture)
    if ( ERROR_SUCCESS == RegQueryValueEx( hKey, L"PinCount", 0, &dwType, (BYTE *)&dwValue, &dwSize ) )
    {
        if ( REG_DWORD == dwType
             && sizeof ( DWORD ) == dwSize
             && 1 == dwValue )
        {
            m_ulCTypes = 1;
        }
    }

    RegCloseKey( hKey );
    return true;
}
