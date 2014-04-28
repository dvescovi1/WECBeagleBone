//-------------------------------------------------------------------------
// <copyright file="CamProperties.cpp" company="Microsoft">
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
#include "SensorProperties.h"
#include "PinDriver.h"

#include "CameraPDD.h"
#include <USBdi.h>                  // USB includes
#include <usb100.h>                 // USB includes
#include <usbclient.h>              // USB client driver helper code
#include <usbfnioctl.h>

#include "USBCode.h"
#include "USBPdd.h"


//-----------------------------------------------------------------------------
// Camera Properties (a.k.a. USB Video Class Controls)
//
// Lookup table to map the MDD camera properties onto USB Video Class Controls.
// There isn't a 1:1 mapping; there are more Video Class controls than the 
// MDD currently has properties for.
//
// Initially each camera control/property is "not supported." After parsing camera 
// controls, we mark if a control/property is supported.
//
// NOTE: According to the USB spec GET_CUR is mandatory, but SET_CUR is optional 
// for many controls. For now, both are hard-coded as TRUE for all controls.
//
// TODO: We could probe for SET_CUR support by trying to set each control when
// the camera is idle, and remember which ones fail.  Failure to SET_CUR should
// cause a USB stall error.
//-----------------------------------------------------------------------------

static PROPERTYCONTROLSTRUCT g_CameraControls[] = {

    // Features supported by the "camera" (Input Terminal)
    // NOTE: order is defined by the Video Class standard 3.7.2.3: Camera Terminal Descriptor
    // DON'T ADD OR REMOVE

    // MDD Property ID              // USB Unit/Terminal           // USB Video Class Control Selector             //num bytes  // Supported   // Get & Set support
    {ENUM_PROP_UNSUPPORTED,         USB_VIDEO_VC_INPUT_TERMINAL,   USB_VIDEO_CT_CS_SCANNING_MODE_CTL,                  1,       FALSE,         0},
    {(ENUM_EXPOSURE|ENUM_AUTO),     USB_VIDEO_VC_INPUT_TERMINAL,   USB_VIDEO_CT_CS_AE_MODE_CTL,                        1,       FALSE,         GET_SUPPORTED | SET_SUPPORTED},
    {ENUM_PROP_UNSUPPORTED,         USB_VIDEO_VC_INPUT_TERMINAL,   USB_VIDEO_CT_CS_AE_PRIORITY_CTL,                    1,       FALSE,         0},
    {ENUM_EXPOSURE,                 USB_VIDEO_VC_INPUT_TERMINAL,   USB_VIDEO_CT_CS_EXPOSURE_TIME_ABSOLUTE_CTL,         4,       FALSE,         GET_SUPPORTED | SET_SUPPORTED},
    {ENUM_PROP_UNSUPPORTED,         USB_VIDEO_VC_INPUT_TERMINAL,   USB_VIDEO_CT_CS_EXPOSURE_TIME_RELATIVE_CTL,         1,       FALSE,         0},
    {ENUM_FOCUS,                    USB_VIDEO_VC_INPUT_TERMINAL,   USB_VIDEO_CT_CS_FOCUS_ABSOLUTE_CTL,                 2,       FALSE,         GET_SUPPORTED | SET_SUPPORTED},
    {ENUM_PROP_UNSUPPORTED,         USB_VIDEO_VC_INPUT_TERMINAL,   USB_VIDEO_CT_CS_FOCUS_RELATIVE_CTL,                 1,       FALSE,         0},
    {ENUM_IRIS,                     USB_VIDEO_VC_INPUT_TERMINAL,   USB_VIDEO_CT_CS_IRIS_ABSOLUTE_CTL,                  2,       FALSE,         GET_SUPPORTED | SET_SUPPORTED},
    {ENUM_PROP_UNSUPPORTED,         USB_VIDEO_VC_INPUT_TERMINAL,   USB_VIDEO_CT_CS_IRIS_RELATIVE_CTL,                  1,       FALSE,         0},
    {ENUM_ZOOM,                     USB_VIDEO_VC_INPUT_TERMINAL,   USB_VIDEO_CT_CS_ZOOM_ABSOLUTE_CTL,                  2,       FALSE,         GET_SUPPORTED | SET_SUPPORTED},
    {ENUM_PROP_UNSUPPORTED,         USB_VIDEO_VC_INPUT_TERMINAL,   USB_VIDEO_CT_CS_ZOOM_RELATIVE_CTL,                  3,       FALSE,         0},
    {ENUM_PAN,                      USB_VIDEO_VC_INPUT_TERMINAL,   USB_VIDEO_CT_CS_PANTILT_ABSOLUTE_CTL,               8,       FALSE,         GET_SUPPORTED | SET_SUPPORTED},
    {ENUM_PROP_UNSUPPORTED,         USB_VIDEO_VC_INPUT_TERMINAL,   USB_VIDEO_CT_CS_PANTILT_RELATIVE_CTL,               4,       FALSE,         0},
    {ENUM_ROLL,                     USB_VIDEO_VC_INPUT_TERMINAL,   USB_VIDEO_CT_CS_ROLL_ABSOLUTE_CTL,                  2,       FALSE,         GET_SUPPORTED | SET_SUPPORTED},
    {ENUM_PROP_UNSUPPORTED,         USB_VIDEO_VC_INPUT_TERMINAL,   USB_VIDEO_CT_CS_ROLL_RELATIVE_CTL,                  1,       FALSE,         0},
    {ENUM_TILT,                     USB_VIDEO_VC_INPUT_TERMINAL,   USB_VIDEO_CT_CS_PANTILT_ABSOLUTE_CTL,               8,       FALSE,         GET_SUPPORTED | SET_SUPPORTED},
    {ENUM_PROP_UNSUPPORTED,         USB_VIDEO_VC_INPUT_TERMINAL,   USB_VIDEO_CT_CS_PANTILT_RELATIVE_CTL,               4,       FALSE,         0},
    {(ENUM_FOCUS|ENUM_AUTO),        USB_VIDEO_VC_INPUT_TERMINAL,   USB_VIDEO_CT_CS_FOCUS_AUTO_CTL,                     1,       FALSE,         GET_SUPPORTED | SET_SUPPORTED},
    {ENUM_PROP_UNSUPPORTED,         USB_VIDEO_VC_INPUT_TERMINAL,   USB_VIDEO_CT_CS_PRIVACY_CTL,                        1,       FALSE,         0},
                                    

    // Features supported by the "sensor" (Processing Unit)
    // NOTE: order is defined by the Video Class standard 3.7.2.5: Processing Unit Descriptor
    // DON'T ADD OR REMOVE
                                    
    // MDD Property ID              // USB Unit/Terminal            // USB Video Class Control Selector            //num bytes  // Supported   // Get & Set support
    {ENUM_BRIGHTNESS,               USB_VIDEO_VC_PROCESSING_UNIT,  USB_VIDEO_PU_CS_BRIGHTNESS_CTL,                     2,       FALSE,         GET_SUPPORTED | SET_SUPPORTED},
    {ENUM_CONTRAST,                 USB_VIDEO_VC_PROCESSING_UNIT,  USB_VIDEO_PU_CS_CONTRAST_CTL,                       2,       FALSE,         GET_SUPPORTED | SET_SUPPORTED},
    {ENUM_HUE,                      USB_VIDEO_VC_PROCESSING_UNIT,  USB_VIDEO_PU_CS_HUE_CTL,                            2,       FALSE,         GET_SUPPORTED | SET_SUPPORTED},
    {ENUM_SATURATION,               USB_VIDEO_VC_PROCESSING_UNIT,  USB_VIDEO_PU_CS_SATURATION_CTL,                     2,       FALSE,         GET_SUPPORTED | SET_SUPPORTED},
    {ENUM_SHARPNESS,                USB_VIDEO_VC_PROCESSING_UNIT,  USB_VIDEO_PU_CS_SHARPNESS_CTL,                      2,       FALSE,         GET_SUPPORTED | SET_SUPPORTED},
    {ENUM_GAMMA,                    USB_VIDEO_VC_PROCESSING_UNIT,  USB_VIDEO_PU_CS_GAMMA_CTL,                          2,       FALSE,         GET_SUPPORTED | SET_SUPPORTED},
    {ENUM_WHITEBALANCE,             USB_VIDEO_VC_PROCESSING_UNIT,  USB_VIDEO_PU_CS_WHITE_BALANCE_TEMP_CTL,             2,       FALSE,         GET_SUPPORTED | SET_SUPPORTED},
    {ENUM_PROP_UNSUPPORTED,         USB_VIDEO_VC_PROCESSING_UNIT,  USB_VIDEO_PU_CS_WHITE_BALANCE_COMPONENT_CTL,        4,       FALSE,         0},
    {ENUM_BACKLIGHT_COMPENSATION,   USB_VIDEO_VC_PROCESSING_UNIT,  USB_VIDEO_PU_CS_BACKLIGHT_COMPENSATION_CTL,         2,       FALSE,         GET_SUPPORTED | SET_SUPPORTED},
    {ENUM_GAIN,                     USB_VIDEO_VC_PROCESSING_UNIT,  USB_VIDEO_PU_CS_GAIN_CTL,                           2,       FALSE,         GET_SUPPORTED | SET_SUPPORTED},
    {ENUM_PROP_UNSUPPORTED,         USB_VIDEO_VC_PROCESSING_UNIT,  USB_VIDEO_PU_CS_POWER_LINE_FREQUENCY_CTL,           1,       FALSE,         0},
    {(ENUM_HUE|ENUM_AUTO),          USB_VIDEO_VC_PROCESSING_UNIT,  USB_VIDEO_PU_CS_HUE_AUTO_CTL,                       1,       FALSE,         GET_SUPPORTED | SET_SUPPORTED},
    {(ENUM_WHITEBALANCE|ENUM_AUTO), USB_VIDEO_VC_PROCESSING_UNIT,  USB_VIDEO_PU_CS_WHITE_BALANCE_TEMP_AUTO_CTL,        1,       FALSE,         GET_SUPPORTED | SET_SUPPORTED},
    {ENUM_PROP_UNSUPPORTED,         USB_VIDEO_VC_PROCESSING_UNIT,  USB_VIDEO_PU_CS_WHITE_BALANCE_COMPONENT_AUTO_CTL,   1,       FALSE,         0},
    {ENUM_PROP_UNSUPPORTED,         USB_VIDEO_VC_PROCESSING_UNIT,  USB_VIDEO_PU_CS_DIGITAL_MULTIPLIER_CTL,             2,       FALSE,         0},
    {ENUM_PROP_UNSUPPORTED,         USB_VIDEO_VC_PROCESSING_UNIT,  USB_VIDEO_PU_CS_DIGITAL_MULTIPLIER_LIMIT_CTL,       2,       FALSE,         0},
    {ENUM_PROP_UNSUPPORTED,         USB_VIDEO_VC_PROCESSING_UNIT,  USB_VIDEO_PU_CS_ANALOG_VIDEO_STANDARD_CTL,          1,       FALSE,         0},
    {ENUM_PROP_UNSUPPORTED,         USB_VIDEO_VC_PROCESSING_UNIT,  USB_VIDEO_PU_CS_ANALOG_LOCK_STATUS_CTL,             1,       FALSE,         0}
};


static TCHAR* g_PropIDToString[] = {
    // VideoProcAmp
/*    ENUM_BRIGHTNESS              */ TEXT("ENUM_BRIGHTNESS"), 
/*    ENUM_CONTRAST                */ TEXT("ENUM_CONTRAST"), 
/*    ENUM_HUE                     */ TEXT("ENUM_HUE"), 
/*    ENUM_SATURATION              */ TEXT("ENUM_SATURATION"), 
/*    ENUM_SHARPNESS               */ TEXT("ENUM_SHARPNESS"), 
/*    ENUM_GAMMA                   */ TEXT("ENUM_GAMMA"), 
/*    ENUM_COLORENABLE             */ TEXT("ENUM_COLORENABLE"), 
/*    ENUM_WHITEBALANCE            */ TEXT("ENUM_WHITEBALANCE"), 
/*    ENUM_BACKLIGHT_COMPENSATION  */ TEXT("ENUM_BACKLIGHT_COMPENSATION"), 
/*    ENUM_GAIN                    */ TEXT("ENUM_GAIN"), 

    // CameraControl
/*    ENUM_PAN                     */ TEXT("ENUM_PAN"), 
/*    ENUM_TILT                    */ TEXT("ENUM_TILT"), 
/*    ENUM_ROLL                    */ TEXT("ENUM_ROLL"), 
/*    ENUM_ZOOM                    */ TEXT("ENUM_ZOOM"), 
/*    ENUM_IRIS                    */ TEXT("ENUM_IRIS"), 
/*    ENUM_EXPOSURE                */ TEXT("ENUM_EXPOSURE"), 
/*    ENUM_FOCUS                   */ TEXT("ENUM_FOCUS"), 
/*    ENUM_FLASH                   */ TEXT("ENUM_FLASH")
};


//
// This function executes all the get commands on a property -- MIN, MAX, Default, current 
// -- depending on the Cmd argument passed to the function
//
DWORD CUSBPdd::PropGetValue(DWORD MDDPropertyID, BYTE bCmd)
{
    DWORD dwRes[2] = {0,};
    int   rc;

    // If MDDPropertyID is not in the list, return error
    PROPERTYCONTROLSTRUCT *pControl = MDDPropertyToUSBControl( MDDPropertyID );
    if(!pControl)
    {
        DEBUGMSG (ZONE_ERROR, (DTAG TEXT("ERROR: PropGetValue: illegal Property ID %d\r\n"), MDDPropertyID));
        return -1;
    }        
   
    rc = DoVendorTransfer(
            bCmd, 
            pControl->bControlSelector, 
            0,  
            pControl->bUnit, 
            (PBYTE)dwRes, 
            pControl->wLength
         );
    
    if(rc) 
    {
        DEBUGMSG (ZONE_ERROR, (DTAG TEXT("ERROR: DoVendorTransfer() returned 0x%x\r\n"), rc));
        DEBUGMSG (ZONE_ERROR, (DTAG TEXT("Camera may not support getting property %d\r\n"), MDDPropertyID));
        return -1;
    }

    // special handling for tilt
    if (MDDPropertyID == ENUM_TILT)
        dwRes[0] = dwRes[1];

    TCHAR buf[32];
    switch (bCmd) {
        case USBVID_GET_CUR:
            swprintf(buf, TEXT("CUR"));
            break;

        case USBVID_GET_MIN:
            swprintf(buf, TEXT("MIN"));
            break;

        case USBVID_GET_MAX:
            swprintf(buf, TEXT("MAX"));
            break;

        case USBVID_GET_RES:
            swprintf(buf, TEXT("RES"));
            break;

        case USBVID_GET_DEF:
            swprintf(buf, TEXT("DEF"));
            break;

        case USBVID_GET_LEN:
            swprintf(buf, TEXT("LEN"));
            break;

        case USBVID_GET_INFO:
            swprintf(buf, TEXT("INFO"));
            break;
    }

    DEBUGMSG(ZONE_PROPERTY, (DTAG TEXT("%s (%d) GET %s = %d\r\n"),
        MDDPropertyToString(MDDPropertyID), MDDPropertyID, buf, dwRes[0]));
    

    return dwRes[0];
}


DWORD CUSBPdd::PropSetValue(DWORD MDDPropertyID, long lValue)
{
    DEBUGMSG (ZONE_PROPERTY, (DTAG TEXT("%s (%d) SET to %d \r\n"), 
        MDDPropertyToString(MDDPropertyID), MDDPropertyID, lValue));

    DWORD dwRes[2] = {0,};
    int rc;
    
    // If MDDPropertyID is not in the list, return error
    PROPERTYCONTROLSTRUCT *pControl = MDDPropertyToUSBControl( MDDPropertyID );
    if(!pControl)
    {
        DEBUGMSG (ZONE_ERROR, (DTAG TEXT("ERROR: PropSetValue: illegal Property ID %d\r\n"), MDDPropertyID));
        return -1;
    }        
        
    // special handling for pan and tilt
    if (MDDPropertyID == ENUM_PAN)
    {
        dwRes[0] = lValue;
        dwRes[1] = m_SensorProps[ENUM_TILT].ulCurrentValue;
    }
    else if (MDDPropertyID == ENUM_TILT)
    {
        dwRes[0] = m_SensorProps[ENUM_PAN].ulCurrentValue;
        dwRes[1] = lValue;
    }
    else
    {
        dwRes[0] = lValue;
    }
    
    
    // Send packet to the device to set the parameter
    rc = DoVendorTransfer(
            USBVID_SET_CUR, 
            pControl->bControlSelector, 
            0, 
            pControl->bUnit, 
            (PBYTE)dwRes, 
            pControl->wLength
         );

    if(rc) 
    {
        DEBUGMSG (ZONE_ERROR, (DTAG TEXT("ERROR: DoVendorTransfer() returned 0x%x\r\n"), rc));
        DEBUGMSG (ZONE_ERROR, (DTAG TEXT("Camera may not support setting property %d\r\n"), MDDPropertyID));
        return -1;
    }
    
    return rc;
}


//
// Check the camera properties list to find out if an MDD property 
// is supported.  Return whether there is GET / SET / both support.
//
BYTE CUSBPdd::PropSupported(DWORD MDDPropertyID)
{
    int i;
    BYTE bCapability = 0;
    
//    for(i = 0; i < dim(g_CameraControls); i++) 
    for(i = 0; i < _countof(g_CameraControls); i++) 
    {
        if(g_CameraControls[i].dwMDDPropertyID == MDDPropertyID)
        {
            if (g_CameraControls[i].bSupported) 
            {
                bCapability |= g_CameraControls[i].fGetSetSupport;
            }
            break;
        }
    }
    
    DEBUGMSG (ZONE_PROPERTY, (DTAG TEXT("%s %s supported, GET = %d SET = %d\r\n"), 
                MDDPropertyToString(MDDPropertyID), 
                bCapability ? L"is" : L"is NOT",
                (bCapability & GET_SUPPORTED) ? 1 : 0,
                (bCapability & SET_SUPPORTED) ? 1 : 0
             ));
    
    return bCapability;   
}


//
// Maps an MDD property to corresponding USB Video Class Control
//
PROPERTYCONTROLSTRUCT * CUSBPdd::MDDPropertyToUSBControl(DWORD MDDPropertyID)
{
//    for(int i = 0; i < dim(g_CameraControls); i++) 
    for(int i = 0; i < _countof(g_CameraControls); i++) 
    {
        if (g_CameraControls[i].dwMDDPropertyID == MDDPropertyID) {
//            DEBUGMSG( ZONE_PROPERTY, (DTAG TEXT("Mapped MDDProperty %d to USB Control %d on Unit %d\r\n"),
//                MDDPropertyID,
//                g_CameraControls[i].bControlSelector,
//                g_CameraControls[i].bUnit
//                ));
            return & g_CameraControls[i];
        }
    }
    
    return NULL;
}


TCHAR * CUSBPdd::MDDPropertyToString(DWORD MDDPropertyID)
{
    static TCHAR buf[64];
    BOOL bAuto = FALSE;
    
    if (MDDPropertyID & ENUM_AUTO) {
        bAuto = TRUE;    
        MDDPropertyID &= ~ENUM_AUTO;
    }

//    if (MDDPropertyID >= dim(g_PropIDToString)) 
    if (MDDPropertyID >= _countof(g_PropIDToString)) 
    {
        _stprintf(buf, _T("UNKNOWN PROPERTY"));
    } 
    else 
    {
        _stprintf(buf, _T("%s%s"), g_PropIDToString[MDDPropertyID], bAuto ? _T(" | AUTO") : _T(""));
    }

    return buf;
}


BOOL CUSBPdd::InitCameraProperties()
{
    DEBUGMSG (ZONE_FUNCTION, (DTAG TEXT("InitCameraProperties++\r\n")));
    
    BYTE bSupport;
    ULONG ulValue;
    
    bSupport = PropSupported(ENUM_BRIGHTNESS);
    m_SensorProps[ENUM_BRIGHTNESS].fSetSupported  = (bSupport & SET_SUPPORTED) ? 1 : 0;;
    m_SensorProps[ENUM_BRIGHTNESS].fGetSupported  = (bSupport & SET_SUPPORTED) ? 1 : 0;;
    m_SensorProps[ENUM_BRIGHTNESS].pRangeNStep    = &BrightnessRangeAndStep[0];
    m_SensorProps[ENUM_BRIGHTNESS].pCsPropValues  = &BrightnessValues;    

    if(m_SensorProps[ENUM_BRIGHTNESS].fGetSupported)
    {
        m_SensorProps[ENUM_BRIGHTNESS].ulCurrentValue  = PropGetValue(ENUM_BRIGHTNESS, USBVID_GET_CUR);
        m_SensorProps[ENUM_BRIGHTNESS].ulDefaultValue  = BrightnessDefault = PropGetValue(ENUM_BRIGHTNESS, USBVID_GET_DEF);
        
        // Signed values
        BrightnessRangeAndStep[0].Bounds.SignedMinimum = PropGetValue(ENUM_BRIGHTNESS, USBVID_GET_MIN);
        BrightnessRangeAndStep[0].Bounds.SignedMaximum = PropGetValue(ENUM_BRIGHTNESS, USBVID_GET_MAX);
        BrightnessRangeAndStep[0].SteppingDelta        = PropGetValue(ENUM_BRIGHTNESS, USBVID_GET_RES);

        m_SensorProps[ENUM_BRIGHTNESS].ulFlags         = CSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL;
        m_SensorProps[ENUM_BRIGHTNESS].ulCapabilities  = CSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL;
    }
    else
    {
        BrightnessRangeAndStep[0].Bounds.SignedMinimum = 0;
        BrightnessRangeAndStep[0].Bounds.SignedMaximum = 0;
        BrightnessRangeAndStep[0].SteppingDelta        = 1;
    }


    bSupport = PropSupported(ENUM_CONTRAST);
    m_SensorProps[ENUM_CONTRAST].fSetSupported  = (bSupport & SET_SUPPORTED) ? 1 : 0; 
    m_SensorProps[ENUM_CONTRAST].fGetSupported  = (bSupport & GET_SUPPORTED) ? 1 : 0;
    m_SensorProps[ENUM_CONTRAST].pRangeNStep    = &ContrastRangeAndStep[0];
    m_SensorProps[ENUM_CONTRAST].pCsPropValues  = &ContrastValues;
    
    if(m_SensorProps[ENUM_CONTRAST].fGetSupported)  
    {
        m_SensorProps[ENUM_CONTRAST].ulCurrentValue  = PropGetValue(ENUM_CONTRAST, USBVID_GET_CUR);
        m_SensorProps[ENUM_CONTRAST].ulDefaultValue  = ContrastDefault = PropGetValue(ENUM_CONTRAST, USBVID_GET_DEF);
        
        // Signed values
        ContrastRangeAndStep[0].Bounds.SignedMinimum = PropGetValue(ENUM_CONTRAST, USBVID_GET_MIN);
        ContrastRangeAndStep[0].Bounds.SignedMaximum = PropGetValue(ENUM_CONTRAST, USBVID_GET_MAX);
        ContrastRangeAndStep[0].SteppingDelta        = PropGetValue(ENUM_CONTRAST, USBVID_GET_RES);

        m_SensorProps[ENUM_CONTRAST].ulFlags         = CSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL;
        m_SensorProps[ENUM_CONTRAST].ulCapabilities  = CSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL;
    }
    else
    {
        ContrastRangeAndStep[0].Bounds.SignedMinimum = 0;
        ContrastRangeAndStep[0].Bounds.SignedMaximum = 0;
        ContrastRangeAndStep[0].SteppingDelta        = 1;
    }


    bSupport = PropSupported(ENUM_HUE);
    m_SensorProps[ENUM_HUE].fSetSupported  = (bSupport & SET_SUPPORTED) ? 1 : 0; 
    m_SensorProps[ENUM_HUE].fGetSupported  = (bSupport & GET_SUPPORTED) ? 1 : 0;
    m_SensorProps[ENUM_HUE].pRangeNStep    = &HueRangeAndStep[0];
    m_SensorProps[ENUM_HUE].pCsPropValues  = &HueValues;
    
    if(m_SensorProps[ENUM_HUE].fGetSupported)
    {
        m_SensorProps[ENUM_HUE].ulCurrentValue  = PropGetValue(ENUM_HUE, USBVID_GET_CUR);
        m_SensorProps[ENUM_HUE].ulDefaultValue  = HueDefault = PropGetValue(ENUM_HUE, USBVID_GET_DEF);
        
        // Signed values
        HueRangeAndStep[0].Bounds.SignedMinimum = PropGetValue(ENUM_HUE, USBVID_GET_MIN);
        HueRangeAndStep[0].Bounds.SignedMaximum = PropGetValue(ENUM_HUE, USBVID_GET_MAX);
        HueRangeAndStep[0].SteppingDelta        = PropGetValue(ENUM_HUE, USBVID_GET_RES);
        
        m_SensorProps[ENUM_HUE].ulCapabilities  = CSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL;

        bSupport = PropSupported(ENUM_HUE|ENUM_AUTO);
        if (bSupport)
        {
            ulValue  = PropGetValue( ENUM_HUE|ENUM_AUTO, USBVID_GET_CUR );

            if (ulValue)
                m_SensorProps[ENUM_HUE].ulFlags     = CSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO;
            else
                m_SensorProps[ENUM_HUE].ulFlags     = CSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL;

            m_SensorProps[ENUM_HUE].ulCapabilities |= CSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO;
        }
    }
    else
    {
        HueRangeAndStep[0].Bounds.SignedMinimum = 0;
        HueRangeAndStep[0].Bounds.SignedMaximum = 0;
        HueRangeAndStep[0].SteppingDelta        = 1;
    }


    bSupport = PropSupported(ENUM_SATURATION);
    m_SensorProps[ENUM_SATURATION].fSetSupported  = (bSupport & SET_SUPPORTED) ? 1 : 0; 
    m_SensorProps[ENUM_SATURATION].fGetSupported  = (bSupport & GET_SUPPORTED) ? 1 : 0;
    m_SensorProps[ENUM_SATURATION].pRangeNStep    = &SaturationRangeAndStep[0];
    m_SensorProps[ENUM_SATURATION].pCsPropValues  = &SaturationValues;
    
    if(m_SensorProps[ENUM_SATURATION].fGetSupported)
    {
        m_SensorProps[ENUM_SATURATION].ulCurrentValue  = PropGetValue(ENUM_SATURATION, USBVID_GET_CUR);
        m_SensorProps[ENUM_SATURATION].ulDefaultValue  = SaturationDefault = PropGetValue(ENUM_SATURATION, USBVID_GET_DEF);
        
        // Signed values
        SaturationRangeAndStep[0].Bounds.SignedMinimum = PropGetValue(ENUM_SATURATION, USBVID_GET_MIN);
        SaturationRangeAndStep[0].Bounds.SignedMaximum = PropGetValue(ENUM_SATURATION, USBVID_GET_MAX);
        SaturationRangeAndStep[0].SteppingDelta        = PropGetValue(ENUM_SATURATION, USBVID_GET_RES);
        
        m_SensorProps[ENUM_SATURATION].ulFlags         = CSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL;
        m_SensorProps[ENUM_SATURATION].ulCapabilities  = CSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL;
    }   
    else
    {
        SaturationRangeAndStep[0].Bounds.SignedMinimum = 0;
        SaturationRangeAndStep[0].Bounds.SignedMaximum = 0;
        SaturationRangeAndStep[0].SteppingDelta        = 1;
    }


    bSupport = PropSupported(ENUM_SHARPNESS);
    m_SensorProps[ENUM_SHARPNESS].fSetSupported  = (bSupport & SET_SUPPORTED) ? 1 : 0; 
    m_SensorProps[ENUM_SHARPNESS].fGetSupported  = (bSupport & GET_SUPPORTED) ? 1 : 0;
    m_SensorProps[ENUM_SHARPNESS].pRangeNStep    = &SharpnessRangeAndStep[0];
    m_SensorProps[ENUM_SHARPNESS].pCsPropValues  = &SharpnessValues;
    
    if(m_SensorProps[ENUM_SHARPNESS].fGetSupported)
    {
        m_SensorProps[ENUM_SHARPNESS].ulCurrentValue  = PropGetValue(ENUM_SHARPNESS, USBVID_GET_CUR);
        m_SensorProps[ENUM_SHARPNESS].ulDefaultValue  = SharpnessDefault = PropGetValue(ENUM_SHARPNESS, USBVID_GET_DEF);
        
        // Signed values
        SharpnessRangeAndStep[0].Bounds.SignedMinimum = PropGetValue(ENUM_SHARPNESS, USBVID_GET_MIN);
        SharpnessRangeAndStep[0].Bounds.SignedMaximum = PropGetValue(ENUM_SHARPNESS, USBVID_GET_MAX);
        SharpnessRangeAndStep[0].SteppingDelta        = PropGetValue(ENUM_SHARPNESS, USBVID_GET_RES);
        
        m_SensorProps[ENUM_SHARPNESS].ulFlags         = CSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL;
        m_SensorProps[ENUM_SHARPNESS].ulCapabilities  = CSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL;
    }
    else
    {
        SharpnessRangeAndStep[0].Bounds.SignedMinimum = 0;
        SharpnessRangeAndStep[0].Bounds.SignedMaximum = 0;
        SharpnessRangeAndStep[0].SteppingDelta        = 1;
    }


    bSupport = PropSupported(ENUM_GAMMA);
    m_SensorProps[ENUM_GAMMA].fSetSupported  = (bSupport & SET_SUPPORTED) ? 1 : 0; 
    m_SensorProps[ENUM_GAMMA].fGetSupported  = (bSupport & GET_SUPPORTED) ? 1 : 0;
    m_SensorProps[ENUM_GAMMA].pRangeNStep    = &GammaRangeAndStep[0];
    m_SensorProps[ENUM_GAMMA].pCsPropValues  = &GammaValues;
    
    if(m_SensorProps[ENUM_GAMMA].fGetSupported) 
    {
        m_SensorProps[ENUM_GAMMA].ulCurrentValue  = PropGetValue(ENUM_GAMMA, USBVID_GET_CUR);
        m_SensorProps[ENUM_GAMMA].ulDefaultValue  = GammaDefault = PropGetValue(ENUM_GAMMA, USBVID_GET_DEF);
        
        // Signed values
        GammaRangeAndStep[0].Bounds.SignedMinimum = PropGetValue(ENUM_GAMMA, USBVID_GET_MIN);
        GammaRangeAndStep[0].Bounds.SignedMaximum = PropGetValue(ENUM_GAMMA, USBVID_GET_MAX);
        GammaRangeAndStep[0].SteppingDelta        = PropGetValue(ENUM_GAMMA, USBVID_GET_RES);
        
        m_SensorProps[ENUM_GAMMA].ulFlags         = CSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL;
        m_SensorProps[ENUM_GAMMA].ulCapabilities  = CSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL;
    }
    else
    {
        GammaRangeAndStep[0].Bounds.SignedMinimum = 0;
        GammaRangeAndStep[0].Bounds.SignedMaximum = 0;
        GammaRangeAndStep[0].SteppingDelta        = 1;
    }


    // return the white balance temperature, the white_bal_component is not used
    bSupport = PropSupported(ENUM_WHITEBALANCE);
    m_SensorProps[ENUM_WHITEBALANCE].fSetSupported  = (bSupport & SET_SUPPORTED) ? 1 : 0; 
    m_SensorProps[ENUM_WHITEBALANCE].fGetSupported  = (bSupport & GET_SUPPORTED) ? 1 : 0;
    m_SensorProps[ENUM_WHITEBALANCE].pRangeNStep    = &WhiteBalanceRangeAndStep[0];
    m_SensorProps[ENUM_WHITEBALANCE].pCsPropValues  = &WhiteBalanceValues;
    
    if(m_SensorProps[ENUM_WHITEBALANCE].fGetSupported)  
    {
        m_SensorProps[ENUM_WHITEBALANCE].ulCurrentValue  = PropGetValue(ENUM_WHITEBALANCE, USBVID_GET_CUR);
        m_SensorProps[ENUM_WHITEBALANCE].ulDefaultValue  = WhiteBalanceDefault = PropGetValue(ENUM_WHITEBALANCE, USBVID_GET_DEF);
        
        // Signed values
        WhiteBalanceRangeAndStep[0].Bounds.SignedMinimum = PropGetValue(ENUM_WHITEBALANCE, USBVID_GET_MIN);
        WhiteBalanceRangeAndStep[0].Bounds.SignedMaximum = PropGetValue(ENUM_WHITEBALANCE, USBVID_GET_MAX);
        WhiteBalanceRangeAndStep[0].SteppingDelta        = PropGetValue(ENUM_WHITEBALANCE, USBVID_GET_RES);
        
        m_SensorProps[ENUM_WHITEBALANCE].ulCapabilities  = CSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL;

        bSupport = PropSupported(ENUM_WHITEBALANCE|ENUM_AUTO);
        if (bSupport)
        {
            ulValue  = PropGetValue( ENUM_WHITEBALANCE|ENUM_AUTO, USBVID_GET_CUR );

            if (ulValue)
                m_SensorProps[ENUM_WHITEBALANCE].ulFlags     = CSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO;
            else
                m_SensorProps[ENUM_WHITEBALANCE].ulFlags     = CSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL;

            m_SensorProps[ENUM_WHITEBALANCE].ulCapabilities |= CSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO;
        }
    }
    else
    {
        WhiteBalanceRangeAndStep[0].Bounds.SignedMinimum = 0;
        WhiteBalanceRangeAndStep[0].Bounds.SignedMaximum = 0;
        WhiteBalanceRangeAndStep[0].SteppingDelta        = 1;
    }


    bSupport = PropSupported(ENUM_COLORENABLE);
    m_SensorProps[ENUM_COLORENABLE].fSetSupported  = (bSupport & SET_SUPPORTED) ? 1 : 0; 
    m_SensorProps[ENUM_COLORENABLE].fGetSupported  = (bSupport & GET_SUPPORTED) ? 1 : 0;
    m_SensorProps[ENUM_COLORENABLE].pRangeNStep    = &ColorEnableRangeAndStep[0];
    m_SensorProps[ENUM_COLORENABLE].pCsPropValues  = &ColorEnableValues;

    if(m_SensorProps[ENUM_COLORENABLE].fGetSupported)   
    {
        m_SensorProps[ENUM_COLORENABLE].ulCurrentValue  = PropGetValue(ENUM_COLORENABLE, USBVID_GET_CUR);
        m_SensorProps[ENUM_COLORENABLE].ulDefaultValue  = ColorEnableDefault = PropGetValue(ENUM_COLORENABLE, USBVID_GET_DEF);
        
        // Signed values
        ColorEnableRangeAndStep[0].Bounds.SignedMinimum = PropGetValue(ENUM_COLORENABLE, USBVID_GET_MIN);
        ColorEnableRangeAndStep[0].Bounds.SignedMaximum = PropGetValue(ENUM_COLORENABLE, USBVID_GET_MAX);
        ColorEnableRangeAndStep[0].SteppingDelta        = PropGetValue(ENUM_COLORENABLE, USBVID_GET_RES);
        
        m_SensorProps[ENUM_COLORENABLE].ulFlags         = CSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL;
        m_SensorProps[ENUM_COLORENABLE].ulCapabilities  = CSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL;
    }
    else
    {
        ColorEnableRangeAndStep[0].Bounds.SignedMinimum = 0;
        ColorEnableRangeAndStep[0].Bounds.SignedMaximum = 0;
        ColorEnableRangeAndStep[0].SteppingDelta        = 1;
    }    


    bSupport = PropSupported(ENUM_BACKLIGHT_COMPENSATION);
    m_SensorProps[ENUM_BACKLIGHT_COMPENSATION].fSetSupported  = (bSupport & SET_SUPPORTED) ? 1 : 0; 
    m_SensorProps[ENUM_BACKLIGHT_COMPENSATION].fGetSupported  = (bSupport & GET_SUPPORTED) ? 1 : 0;
    m_SensorProps[ENUM_BACKLIGHT_COMPENSATION].pRangeNStep    = &BackLightCompensationRangeAndStep[0];
    m_SensorProps[ENUM_BACKLIGHT_COMPENSATION].pCsPropValues  = &BackLightCompensationValues;

    if(m_SensorProps[ENUM_BACKLIGHT_COMPENSATION].fGetSupported)  
    {
        m_SensorProps[ENUM_BACKLIGHT_COMPENSATION].ulCurrentValue = PropGetValue(ENUM_BACKLIGHT_COMPENSATION, USBVID_GET_CUR);
        m_SensorProps[ENUM_BACKLIGHT_COMPENSATION].ulDefaultValue = BackLightCompensationDefault = PropGetValue(ENUM_BACKLIGHT_COMPENSATION, USBVID_GET_DEF);
        
        // Signed values
        BackLightCompensationRangeAndStep[0].Bounds.SignedMinimum = PropGetValue(ENUM_BACKLIGHT_COMPENSATION, USBVID_GET_MIN);
        BackLightCompensationRangeAndStep[0].Bounds.SignedMaximum = PropGetValue(ENUM_BACKLIGHT_COMPENSATION, USBVID_GET_MAX);
        BackLightCompensationRangeAndStep[0].SteppingDelta        = PropGetValue(ENUM_BACKLIGHT_COMPENSATION, USBVID_GET_RES);
        
        m_SensorProps[ENUM_BACKLIGHT_COMPENSATION].ulFlags        = CSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL;
        m_SensorProps[ENUM_BACKLIGHT_COMPENSATION].ulCapabilities = CSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL;
    }
    else
    {
        BackLightCompensationRangeAndStep[0].Bounds.SignedMinimum = 0;
        BackLightCompensationRangeAndStep[0].Bounds.SignedMaximum = 0;
        BackLightCompensationRangeAndStep[0].SteppingDelta        = 1;
    }


    bSupport = PropSupported(ENUM_GAIN);
    m_SensorProps[ENUM_GAIN].fSetSupported  = (bSupport & SET_SUPPORTED) ? 1 : 0; 
    m_SensorProps[ENUM_GAIN].fGetSupported  = (bSupport & GET_SUPPORTED) ? 1 : 0;
    m_SensorProps[ENUM_GAIN].pRangeNStep    = &GainRangeAndStep[0];
    m_SensorProps[ENUM_GAIN].pCsPropValues  = &GainValues;

    if(m_SensorProps[ENUM_GAIN].fGetSupported)  
    {
        m_SensorProps[ENUM_GAIN].ulCurrentValue  = PropGetValue(ENUM_GAIN, USBVID_GET_CUR);
        m_SensorProps[ENUM_GAIN].ulDefaultValue  = GainDefault = PropGetValue(ENUM_GAIN, USBVID_GET_DEF);
        
        // Signed values
        GainRangeAndStep[0].Bounds.SignedMinimum = PropGetValue(ENUM_GAIN, USBVID_GET_MIN);
        GainRangeAndStep[0].Bounds.SignedMaximum = PropGetValue(ENUM_GAIN, USBVID_GET_MAX);
        GainRangeAndStep[0].SteppingDelta        = PropGetValue(ENUM_GAIN, USBVID_GET_RES);
        
        m_SensorProps[ENUM_GAIN].ulFlags         = CSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL;
        m_SensorProps[ENUM_GAIN].ulCapabilities  = CSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL;
    }
    else
    {
        GainRangeAndStep[0].Bounds.SignedMinimum = 0;
        GainRangeAndStep[0].Bounds.SignedMaximum = 0;
        GainRangeAndStep[0].SteppingDelta        = 1;
    }    


    bSupport = PropSupported(ENUM_EXPOSURE);
    m_SensorProps[ENUM_EXPOSURE].fGetSupported = (bSupport & GET_SUPPORTED) ? 1 : 0;
    m_SensorProps[ENUM_EXPOSURE].fSetSupported = (bSupport & SET_SUPPORTED) ? 1 : 0;
    m_SensorProps[ENUM_EXPOSURE].pRangeNStep   = &ExposureRangeAndStep[0];
    m_SensorProps[ENUM_EXPOSURE].pCsPropValues = &ExposureValues;
        
    if(m_SensorProps[ENUM_EXPOSURE].fGetSupported)
    {
        m_SensorProps[ENUM_EXPOSURE].ulCurrentValue  = PropGetValue(ENUM_EXPOSURE, USBVID_GET_CUR);
        m_SensorProps[ENUM_EXPOSURE].ulDefaultValue  = ExposureDefault = PropGetValue(ENUM_EXPOSURE, USBVID_GET_DEF);
    
        // Signed values
        ExposureRangeAndStep[0].Bounds.SignedMinimum = PropGetValue(ENUM_EXPOSURE, USBVID_GET_MIN);
        ExposureRangeAndStep[0].Bounds.SignedMaximum = PropGetValue(ENUM_EXPOSURE, USBVID_GET_MAX);
        ExposureRangeAndStep[0].SteppingDelta        = PropGetValue(ENUM_EXPOSURE, USBVID_GET_RES);
    
        m_SensorProps[ENUM_EXPOSURE].ulFlags         = CSPROPERTY_CAMERACONTROL_FLAGS_MANUAL;
        m_SensorProps[ENUM_EXPOSURE].ulCapabilities  = CSPROPERTY_CAMERACONTROL_FLAGS_MANUAL;
    }
    else
    {
        ExposureRangeAndStep[0].Bounds.SignedMinimum = 0;
        ExposureRangeAndStep[0].Bounds.SignedMaximum = 0;
        ExposureRangeAndStep[0].SteppingDelta        = 1;
    }


    bSupport = PropSupported(ENUM_FOCUS);
    m_SensorProps[ENUM_FOCUS].fGetSupported = (bSupport & GET_SUPPORTED) ? 1 : 0;
    m_SensorProps[ENUM_FOCUS].fSetSupported = (bSupport & SET_SUPPORTED) ? 1 : 0;
    m_SensorProps[ENUM_FOCUS].pRangeNStep   = &FocusRangeAndStep[0];
    m_SensorProps[ENUM_FOCUS].pCsPropValues = &FocusValues;
        
    if(m_SensorProps[ENUM_FOCUS].fGetSupported)
    {
        m_SensorProps[ENUM_FOCUS].ulCurrentValue  = PropGetValue(ENUM_FOCUS, USBVID_GET_CUR);
        m_SensorProps[ENUM_FOCUS].ulDefaultValue  = FocusDefault = PropGetValue(ENUM_FOCUS, USBVID_GET_DEF);
    
        // Signed values
        FocusRangeAndStep[0].Bounds.SignedMinimum = PropGetValue(ENUM_FOCUS, USBVID_GET_MIN);
        FocusRangeAndStep[0].Bounds.SignedMaximum = PropGetValue(ENUM_FOCUS, USBVID_GET_MAX);
        FocusRangeAndStep[0].SteppingDelta        = PropGetValue(ENUM_FOCUS, USBVID_GET_RES);
    
        m_SensorProps[ENUM_FOCUS].ulCapabilities  = CSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL;

        bSupport = PropSupported(ENUM_FOCUS|ENUM_AUTO);

        if (bSupport)
        {
            ulValue  = PropGetValue( ENUM_FOCUS|ENUM_AUTO, USBVID_GET_CUR );

            if (ulValue)
                m_SensorProps[ENUM_FOCUS].ulFlags     = CSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO;
            else
                m_SensorProps[ENUM_FOCUS].ulFlags     = CSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL;

            m_SensorProps[ENUM_FOCUS].ulCapabilities |= CSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO;
        }
    }
    else
    {
        FocusRangeAndStep[0].Bounds.SignedMinimum = 0;
        FocusRangeAndStep[0].Bounds.SignedMaximum = 0;
        FocusRangeAndStep[0].SteppingDelta        = 1;
    }


    bSupport = PropSupported(ENUM_IRIS);
    m_SensorProps[ENUM_IRIS].fGetSupported = (bSupport & GET_SUPPORTED) ? 1 : 0;
    m_SensorProps[ENUM_IRIS].fSetSupported = (bSupport & SET_SUPPORTED) ? 1 : 0;
    m_SensorProps[ENUM_IRIS].pRangeNStep   = &IrisRangeAndStep[0];
    m_SensorProps[ENUM_IRIS].pCsPropValues = &IrisValues;

    if(m_SensorProps[ENUM_IRIS].fGetSupported)
    {
        m_SensorProps[ENUM_IRIS].ulCurrentValue  = PropGetValue(ENUM_IRIS, USBVID_GET_CUR);
        m_SensorProps[ENUM_IRIS].ulDefaultValue  = IrisDefault = PropGetValue(ENUM_IRIS, USBVID_GET_DEF);
    
        // Signed values
        IrisRangeAndStep[0].Bounds.SignedMinimum = PropGetValue(ENUM_IRIS, USBVID_GET_MIN);
        IrisRangeAndStep[0].Bounds.SignedMaximum = PropGetValue(ENUM_IRIS, USBVID_GET_MAX);
        IrisRangeAndStep[0].SteppingDelta        = PropGetValue(ENUM_IRIS, USBVID_GET_RES);
    
        m_SensorProps[ENUM_IRIS].ulFlags         = CSPROPERTY_CAMERACONTROL_FLAGS_MANUAL;
        m_SensorProps[ENUM_IRIS].ulCapabilities  = CSPROPERTY_CAMERACONTROL_FLAGS_MANUAL;
    }
    else
    {
        IrisRangeAndStep[0].Bounds.SignedMinimum = 0;
        IrisRangeAndStep[0].Bounds.SignedMaximum = 0;
        IrisRangeAndStep[0].SteppingDelta        = 1;
    }


    bSupport = PropSupported(ENUM_ZOOM);
    m_SensorProps[ENUM_ZOOM].fGetSupported  = (bSupport & GET_SUPPORTED) ? 1 : 0;
    m_SensorProps[ENUM_ZOOM].fSetSupported  = (bSupport & SET_SUPPORTED) ? 1 : 0;
    m_SensorProps[ENUM_ZOOM].pRangeNStep    = &ZoomRangeAndStep[0];
    m_SensorProps[ENUM_ZOOM].pCsPropValues  = &ZoomValues;
        
    if(m_SensorProps[ENUM_ZOOM].fGetSupported)
    {
        m_SensorProps[ENUM_ZOOM].ulCurrentValue  = PropGetValue(ENUM_ZOOM, USBVID_GET_CUR);
        m_SensorProps[ENUM_ZOOM].ulDefaultValue  = ZoomDefault = PropGetValue(ENUM_ZOOM, USBVID_GET_DEF);

        // Signed values
        ZoomRangeAndStep[0].Bounds.SignedMinimum = PropGetValue(ENUM_ZOOM, USBVID_GET_MIN);
        ZoomRangeAndStep[0].Bounds.SignedMaximum = PropGetValue(ENUM_ZOOM, USBVID_GET_MAX);
        ZoomRangeAndStep[0].SteppingDelta        = PropGetValue(ENUM_ZOOM, USBVID_GET_RES);

        m_SensorProps[ENUM_ZOOM].ulFlags         = CSPROPERTY_CAMERACONTROL_FLAGS_MANUAL;
        m_SensorProps[ENUM_ZOOM].ulCapabilities  = CSPROPERTY_CAMERACONTROL_FLAGS_MANUAL;
    }
    else
    {
        ZoomRangeAndStep[0].Bounds.SignedMinimum = 0;
        ZoomRangeAndStep[0].Bounds.SignedMaximum = 0;
        ZoomRangeAndStep[0].SteppingDelta        = 1;
    }


    bSupport = PropSupported(ENUM_PAN);
    m_SensorProps[ENUM_PAN].fGetSupported  = (bSupport & GET_SUPPORTED) ? 1 : 0;
    m_SensorProps[ENUM_PAN].fSetSupported  = (bSupport & SET_SUPPORTED) ? 1 : 0;
    m_SensorProps[ENUM_PAN].pRangeNStep    = &PanRangeAndStep[0];
    m_SensorProps[ENUM_PAN].pCsPropValues  = &PanValues;
        
    if(m_SensorProps[ENUM_PAN].fGetSupported)
    {
        m_SensorProps[ENUM_PAN].ulCurrentValue  = PropGetValue(ENUM_PAN, USBVID_GET_CUR);
        m_SensorProps[ENUM_PAN].ulDefaultValue  = PanDefault = PropGetValue(ENUM_PAN, USBVID_GET_DEF);
        
        // Signed values
        PanRangeAndStep[0].Bounds.SignedMinimum = PropGetValue(ENUM_PAN, USBVID_GET_MIN);
        PanRangeAndStep[0].Bounds.SignedMaximum = PropGetValue(ENUM_PAN, USBVID_GET_MAX);
        PanRangeAndStep[0].SteppingDelta        = PropGetValue(ENUM_PAN, USBVID_GET_RES);
        
        m_SensorProps[ENUM_PAN].ulFlags         = CSPROPERTY_CAMERACONTROL_FLAGS_MANUAL;
        m_SensorProps[ENUM_PAN].ulCapabilities  = CSPROPERTY_CAMERACONTROL_FLAGS_MANUAL;
    }
    else
    {
        PanRangeAndStep[0].Bounds.SignedMinimum = 0;
        PanRangeAndStep[0].Bounds.SignedMaximum = 0;
        PanRangeAndStep[0].SteppingDelta        = 1;
    }  


    bSupport = PropSupported(ENUM_ROLL);
    m_SensorProps[ENUM_ROLL].fGetSupported  = (bSupport & GET_SUPPORTED) ? 1 : 0;
    m_SensorProps[ENUM_ROLL].fSetSupported  = (bSupport & SET_SUPPORTED) ? 1 : 0;
    m_SensorProps[ENUM_ROLL].pRangeNStep    = &RollRangeAndStep[0];
    m_SensorProps[ENUM_ROLL].pCsPropValues  = &RollValues;
        
    if(m_SensorProps[ENUM_ROLL].fGetSupported)
    {
        m_SensorProps[ENUM_ROLL].ulCurrentValue  = PropGetValue(ENUM_ROLL, USBVID_GET_CUR);
        m_SensorProps[ENUM_ROLL].ulDefaultValue  = RollDefault = PropGetValue(ENUM_ROLL, USBVID_GET_DEF);
        
        // Signed values
        RollRangeAndStep[0].Bounds.SignedMinimum = PropGetValue(ENUM_ROLL, USBVID_GET_MIN);
        RollRangeAndStep[0].Bounds.SignedMaximum = PropGetValue(ENUM_ROLL, USBVID_GET_MAX);
        RollRangeAndStep[0].SteppingDelta        = PropGetValue(ENUM_ROLL, USBVID_GET_RES);
        
        m_SensorProps[ENUM_ROLL].ulFlags         = CSPROPERTY_CAMERACONTROL_FLAGS_MANUAL;
        m_SensorProps[ENUM_ROLL].ulCapabilities  = CSPROPERTY_CAMERACONTROL_FLAGS_MANUAL;
    }
    else
    {
        RollRangeAndStep[0].Bounds.SignedMinimum = 0;
        RollRangeAndStep[0].Bounds.SignedMaximum = 0;
        RollRangeAndStep[0].SteppingDelta        = 1;
    }  


    bSupport = PropSupported(ENUM_TILT);
    m_SensorProps[ENUM_TILT].fGetSupported  = (bSupport & GET_SUPPORTED) ? 1 : 0;
    m_SensorProps[ENUM_TILT].fSetSupported  = (bSupport & SET_SUPPORTED) ? 1 : 0;
    m_SensorProps[ENUM_TILT].pRangeNStep    = &TiltRangeAndStep[0];
    m_SensorProps[ENUM_TILT].pCsPropValues  = &TiltValues;
        
    if(m_SensorProps[ENUM_TILT].fGetSupported)
    {
        m_SensorProps[ENUM_TILT].ulCurrentValue  = PropGetValue(ENUM_TILT, USBVID_GET_CUR);
        m_SensorProps[ENUM_TILT].ulDefaultValue  = TiltDefault = PropGetValue(ENUM_TILT, USBVID_GET_DEF);
        
        // Signed values
        TiltRangeAndStep[0].Bounds.SignedMinimum = PropGetValue(ENUM_TILT, USBVID_GET_MIN);
        TiltRangeAndStep[0].Bounds.SignedMaximum = PropGetValue(ENUM_TILT, USBVID_GET_MAX);
        TiltRangeAndStep[0].SteppingDelta        = PropGetValue(ENUM_TILT, USBVID_GET_RES);
        
        m_SensorProps[ENUM_TILT].ulFlags         = CSPROPERTY_CAMERACONTROL_FLAGS_MANUAL;
        m_SensorProps[ENUM_TILT].ulCapabilities  = CSPROPERTY_CAMERACONTROL_FLAGS_MANUAL;
    }
    else
    {
        TiltRangeAndStep[0].Bounds.SignedMinimum = 0;
        TiltRangeAndStep[0].Bounds.SignedMaximum = 0;
        TiltRangeAndStep[0].SteppingDelta        = 1;
    }  


    bSupport = PropSupported(ENUM_FLASH);
    m_SensorProps[ENUM_FLASH].fGetSupported = (bSupport & GET_SUPPORTED) ? 1 : 0;
    m_SensorProps[ENUM_FLASH].fSetSupported = (bSupport & SET_SUPPORTED) ? 1 : 0;
    m_SensorProps[ENUM_FLASH].pRangeNStep   = &FlashRangeAndStep[0];
    m_SensorProps[ENUM_FLASH].pCsPropValues = &FlashValues;
        
    if(m_SensorProps[ENUM_FLASH].fGetSupported)
    {
        m_SensorProps[ENUM_FLASH].ulCurrentValue  = PropGetValue(ENUM_FLASH, USBVID_GET_CUR);
        m_SensorProps[ENUM_FLASH].ulDefaultValue  = FlashDefault = PropGetValue(ENUM_FLASH, USBVID_GET_DEF);
    
        // Signed values
        FlashRangeAndStep[0].Bounds.SignedMinimum = PropGetValue(ENUM_FLASH, USBVID_GET_MIN);
        FlashRangeAndStep[0].Bounds.SignedMaximum = PropGetValue(ENUM_FLASH, USBVID_GET_MAX);
        FlashRangeAndStep[0].SteppingDelta        = PropGetValue(ENUM_FLASH, USBVID_GET_RES);
    
        m_SensorProps[ENUM_FLASH].ulFlags         = CSPROPERTY_CAMERACONTROL_FLAGS_MANUAL;
        m_SensorProps[ENUM_FLASH].ulCapabilities  = CSPROPERTY_CAMERACONTROL_FLAGS_MANUAL;
    }
    else
    {
        FlashRangeAndStep[0].Bounds.SignedMinimum = 0;
        FlashRangeAndStep[0].Bounds.SignedMaximum = 0;
        FlashRangeAndStep[0].SteppingDelta        = 1;
    }


    DEBUGMSG (ZONE_FUNCTION, (DTAG TEXT("InitCameraProperties--\r\n")));
    
    return TRUE;
}



//
// Put the camera in a known good state by setting default properties.
// Defaults are queried from the camera itself, where GET DEFAULT is supported.
// Otherwise, property defaults are taken from SensorProperties.h.
//
BOOL
CUSBPdd::ResetCameraProperties()
{
    DEBUGMSG (ZONE_FUNCTION, (DTAG TEXT("ResetCameraProperties++\r\n")));
    
    ULONG ulDefault;
    
    for (int i = ENUM_BRIGHTNESS; i <= ENUM_FLASH; i++)
    {
        if ( m_SensorProps[i].fSetSupported ) 
        {
            ulDefault = m_SensorProps[i].ulDefaultValue;
            PropSetValue( i, ulDefault );
        }
    }

    
    DEBUGMSG (ZONE_FUNCTION, (DTAG TEXT("ResetCameraProperties--\r\n")));

    return TRUE;
}


BOOL CUSBPdd::CheckCameraProperties()
{
    DEBUGMSG (ZONE_FUNCTION, (DTAG TEXT("CheckCameraProperties++\r\n")));
    
    PUSBVIDCTLIFDESCRIPTOR pHdr = (PUSBVIDCTLIFDESCRIPTOR)(m_pDrv->pStreamInfo->usbctlIF.lpepExtDesc);
    __try 
    {
        // check the header type  
        if ((pHdr->bType != 0x24) || (pHdr->bSubtype != 1)) 
        {
            DEBUGMSG (ZONE_ERROR, (DTAG TEXT("Bad Extended Stream Descriptor\r\n")));
            return FALSE;
        }

        PBYTE const pEnd  = (PBYTE)pHdr + pHdr->wTotalLen;
        PBYTE pData = (PBYTE)pHdr;

        PUSBVIDSTDDESCHDR pStd = (PUSBVIDSTDDESCHDR)pHdr;
        // Loop through all the descriptors
        // We are checking for an Input Terminal Descriptor, type ITT_CAMERA.
        // In other words, a Camera Terminal Descriptor (USB Video Class Spec 3.7.2.3)
        // which should be the 2nd descriptor (after the VC Interface Header)
        while (pData + pStd->bLen < pEnd)
        {
            pData += pStd->bLen;
            pStd = (PUSBVIDSTDDESCHDR)pData;
    
            if (pStd->bType != USB_VIDEO_CS_INTERFACE)
            {
                break;
            }
            
            PUSBVIDCTLIF_INPUTTERMDESCRIPTOR pInputTerm;
            PUSBVIDCTLIF_CAMTERMDESCRIPTOR   pCameraTerm;
            PUSBVIDCTLIF_PROCUNITDESCRIPTOR  pProcUnit;

            switch (pStd->bSubtype) 
            {
                case USB_VIDEO_VC_INPUT_TERMINAL:       // 0x02
                    // Cast the std descriptor to an Input Terminal:
                    pInputTerm = (PUSBVIDCTLIF_INPUTTERMDESCRIPTOR)pStd;

                    // Verify that it's the camera
                    if (USB_VIDEO_ITT_CAMERA == pInputTerm->wTerminalType)
                    {
                        // Cast the Input Terminal to a Camera Terminal
                        pCameraTerm = (PUSBVIDCTLIF_CAMTERMDESCRIPTOR)pInputTerm;

                        // Verify structure length
                        if (pCameraTerm->bLen >= 0x11)
                        {
                            DEBUGMSG(ZONE_PROPERTY, (DTAG TEXT("Found Camera Input Terminal\r\n")));
                            MarkSupportedProperties( 
                                        pCameraTerm->bmaControls, 
                                        pCameraTerm->bControlSize,
                                        &g_CameraControls[0],
                                        19,
                                        pCameraTerm->bTerminalID
                                        );
                        }
                    }
                    break;
                case USB_VIDEO_VC_PROCESSING_UNIT:      // 0x05
                    // Cast the std descriptor to a Processing Unit:
                    pProcUnit = (PUSBVIDCTLIF_PROCUNITDESCRIPTOR)pStd;

                    // Verify structure length
                    if (pProcUnit->bLen >= 0x0b)
                    {
                        DEBUGMSG(ZONE_PROPERTY, (DTAG TEXT("Found Processing Unit\r\n")));
                        MarkSupportedProperties(
                                    pProcUnit->bmaControls,
                                    pProcUnit->bControlSize,
                                    &g_CameraControls[19],
//                                    dim(g_CameraControls) - 19,
                                    _countof(g_CameraControls) - 19,
                                    pProcUnit->bUnitID
                            );
                    }
                    break;
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        DEBUGMSG (ZONE_ERROR, (TEXT("Exception scanning extended control descriptor\r\n")));
        return FALSE;
    }

    DEBUGMSG (ZONE_FUNCTION, (DTAG TEXT("CheckCameraProperties--\r\n")));
    
    return TRUE;
}


//
// For each property the camera supports, mark it in g_CameraControls
//
void CUSBPdd::MarkSupportedProperties (PBYTE pCtlBytes, int nCtlBytes, PPROPERTYCONTROLSTRUCT pPropertiesArray, int nArraySize, BYTE bUnit) 
{
    DEBUGMSG (ZONE_FUNCTION, (DTAG TEXT("MarkSupportedProperties++\r\n")));
    
    BYTE x;

    // Check the bit for each property on this Unit/Terminal
    for (BYTE i = 0; i < nArraySize; i++)
    {
        if ((i % 8) == 0)
        {
            if (nCtlBytes == 0) 
                goto Exit;
                
            x = *pCtlBytes++;
            nCtlBytes--;
        }
        
        if (x & 0x01)       // feature is supported
        {
            pPropertiesArray[i].bUnit      = bUnit;  // mark which Unit or Terminal owns the property
            pPropertiesArray[i].bSupported = TRUE;
        }

        // Only log properties the MDD knows about.
        // E.G., we don't care if the camera supports USB_VIDEO_CT_CS_EXPOSURE_TIME_RELATIVE_CTL
        // because the MDD doesn't have any concept of relative exposure setting.
        if ( pPropertiesArray[i].dwMDDPropertyID != ENUM_PROP_UNSUPPORTED ) 
        {
            DEBUGMSG (ZONE_PROPERTY, (DTAG TEXT("%s %s supported\r\n"),
                        MDDPropertyToString( pPropertiesArray[i].dwMDDPropertyID ),
                        (x & 0x01) ? L"is" : L"is NOT"
                      ));
        }

        // Continue with next property bit
        x =  x >> 1;
    }
 
Exit:    
    DEBUGMSG (ZONE_FUNCTION, (DTAG TEXT("MarkSupportedProperties--\r\n")));
}


