//-------------------------------------------------------------------------
// <copyright file="usbcode.h" company="Microsoft">
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

#ifndef _USBCODE_H
#define _USBCODE_H


#define INF_REJECTED 0
#define INF_ACCEPTED 1


// USB video Specification declarations
#define USB_CAM_FEATURE_NUM         19
#define USB_PROC_FEATURE_NUM        18  


#define USB_MISC_INTERFACE_CLASS                 0xEF
#define USB_COMMON_INTERFACE_SUBCLASS            0x02

#define USB_DEVICE_CLASS_VIDEO                   0x0E

// Video interface subclasses
#define USB_VIDEO_SC_UNDEFINED                   0x00 
#define USB_VIDEO_SC_VIDEOCONTROL                0x01 
#define USB_VIDEO_SC_VIDEOSTREAMING              0x02
#define USB_VIDEO_SC_VIDEO_INTERFACE_COLLECTION  0x03


// Interface numbers for the two video interfaces
#define VID_IF_CTL       0
#define VID_IF_STREAM    1

//
// Video Class request types
//
#define USBVID_SET_CUR              0x01 
#define USBVID_GET_CUR              0x81 
#define USBVID_GET_MIN              0x82 
#define USBVID_GET_MAX              0x83 
#define USBVID_GET_RES              0x84 
#define USBVID_GET_DEF              0x87 
#define USBVID_GET_LEN              0x85 
#define USBVID_GET_INFO             0x86 

// Video interface protocols
#define USB_VIDEO_PC_PROTOCOL_UNDEFINED          0x00

// Video interface class specific descriptors
#define USB_VIDEO_CS_UNDEFINED                   0x20
#define USB_VIDEO_CS_DEVICE                      0x21
#define USB_VIDEO_CS_CONFIGURATION               0x22
#define USB_VIDEO_CS_STRING                      0x23
#define USB_VIDEO_CS_INTERFACE                   0x24
#define USB_VIDEO_CS_ENDPOINT                    0x25

// Video interface class Video Control descriptors
#define USB_VIDEO_VC_DESCRIPTOR_UNDEFINED        0x00
#define USB_VIDEO_VC_HEADER                      0x01
#define USB_VIDEO_VC_INPUT_TERMINAL              0x02
#define USB_VIDEO_VC_OUTPUT_TERMINAL             0x03
#define USB_VIDEO_VC_SELECTOR_UNIT               0x04
#define USB_VIDEO_VC_PROCESSING_UNIT             0x05
#define USB_VIDEO_VC_EXTENSION_UNIT              0x06

// Video interface class Video Stream descriptors
#define USB_VIDEO_VS_UNDEFINED                   0x00
#define USB_VIDEO_VS_INPUT_HEADER                0x01
#define USB_VIDEO_VS_OUTPUT_HEADER               0x02
#define USB_VIDEO_VS_STILL_IMAGE_FRAME           0x03
#define USB_VIDEO_VS_FORMAT_UNCOMPRESSED         0x04
#define USB_VIDEO_VS_FRAME_UNCOMPRESSED          0x05
#define USB_VIDEO_VS_FORMAT_MJPEG                0x06
#define USB_VIDEO_VS_FRAME_MJPEG                 0x07
#define USB_VIDEO_VS_FORMAT_MPEG2TS              0x0A
#define USB_VIDEO_VS_FORMAT_DV                   0x0C
#define USB_VIDEO_VS_COLORFORMAT                 0x0D
#define USB_VIDEO_VS_FORMAT_FRAME_BASED          0x10
#define USB_VIDEO_VS_FRAME_FRAME_BASED           0x11
#define USB_VIDEO_VS_FORMAT_STREAM_BASED         0x12

//
// Control Selector Codes
//

// Video Control Interface control selectors
#define USB_VIDEO_VC_CS_VIDEO_POWER_MODE_CONTROL    0x01
#define USB_VIDEO_VC_CS_REQUEST_ERROR_CODE_CONTROL  0x02

// Termainal Control Selectors
#define USB_VIDEO_TU_CS_UNDEFINED                   0x00

// Selector Unit Control Selectors
#define USB_VIDEO_SU_CS_INPUT_SELECT_CONTROL        0x01

// Camera Terminal Control Selectors
#define USB_VIDEO_CT_CS_CONTROL_UNDEFINED           0x00
#define USB_VIDEO_CT_CS_SCANNING_MODE_CTL           0x01
#define USB_VIDEO_CT_CS_AE_MODE_CTL                 0x02
#define USB_VIDEO_CT_CS_AE_PRIORITY_CTL             0x03
#define USB_VIDEO_CT_CS_EXPOSURE_TIME_ABSOLUTE_CTL  0x04
#define USB_VIDEO_CT_CS_EXPOSURE_TIME_RELATIVE_CTL  0x05
#define USB_VIDEO_CT_CS_FOCUS_ABSOLUTE_CTL          0x06
#define USB_VIDEO_CT_CS_FOCUS_RELATIVE_CTL          0x07
#define USB_VIDEO_CT_CS_FOCUS_AUTO_CTL              0x08
#define USB_VIDEO_CT_CS_IRIS_ABSOLUTE_CTL           0x09
#define USB_VIDEO_CT_CS_IRIS_RELATIVE_CTL           0x0A
#define USB_VIDEO_CT_CS_ZOOM_ABSOLUTE_CTL           0x0B
#define USB_VIDEO_CT_CS_ZOOM_RELATIVE_CTL           0x0C
#define USB_VIDEO_CT_CS_PANTILT_ABSOLUTE_CTL        0x0D
#define USB_VIDEO_CT_CS_PANTILT_RELATIVE_CTL        0x0E
#define USB_VIDEO_CT_CS_ROLL_ABSOLUTE_CTL           0x0F
#define USB_VIDEO_CT_CS_ROLL_RELATIVE_CTL           0x10
#define USB_VIDEO_CT_CS_PRIVACY_CTL                 0x11

// Auto-Exposure Priority control bitfields
#define USB_VIDEO_AE_MODE_MANUAL                    0x01
#define USB_VIDEO_AE_MODE_AUTO_FULL                 0x02
#define USB_VIDEO_AE_MODE_AUTO_SHUTTER_PRIO         0x04
#define USB_VIDEO_AE_MODE_AUTO_APERTURE_PRIO        0x08

// Processing Unit Control Selectors
#define USB_VIDEO_PU_CS_CONTROL_UNDEFINED                   0x00
#define USB_VIDEO_PU_CS_BACKLIGHT_COMPENSATION_CTL          0x01
#define USB_VIDEO_PU_CS_BRIGHTNESS_CTL                      0x02
#define USB_VIDEO_PU_CS_CONTRAST_CTL                        0x03
#define USB_VIDEO_PU_CS_GAIN_CTL                            0x04
#define USB_VIDEO_PU_CS_POWER_LINE_FREQUENCY_CTL            0x05
#define USB_VIDEO_PU_CS_HUE_CTL                             0x06
#define USB_VIDEO_PU_CS_SATURATION_CTL                      0x07
#define USB_VIDEO_PU_CS_SHARPNESS_CTL                       0x08
#define USB_VIDEO_PU_CS_GAMMA_CTL                           0x09
#define USB_VIDEO_PU_CS_WHITE_BALANCE_TEMP_CTL              0x0A
#define USB_VIDEO_PU_CS_WHITE_BALANCE_TEMP_AUTO_CTL         0x0B
#define USB_VIDEO_PU_CS_WHITE_BALANCE_COMPONENT_CTL         0x0C
#define USB_VIDEO_PU_CS_WHITE_BALANCE_COMPONENT_AUTO_CTL    0x0D
#define USB_VIDEO_PU_CS_DIGITAL_MULTIPLIER_CTL              0x0E
#define USB_VIDEO_PU_CS_DIGITAL_MULTIPLIER_LIMIT_CTL        0x0F
#define USB_VIDEO_PU_CS_HUE_AUTO_CTL                        0x10
#define USB_VIDEO_PU_CS_ANALOG_VIDEO_STANDARD_CTL           0x11
#define USB_VIDEO_PU_CS_ANALOG_LOCK_STATUS_CTL              0x12

// Extension Unit Control Selectors
#define USB_VIDEO_EU_CS_UNDEFINED                   0x00

// Video Streaming Interface Control Selectors
#define USB_VIDEO_VS_CS_CTL_UNDEFINED             0x00
#define USB_VIDEO_VS_CS_PROBE_CTL                 0x01
#define USB_VIDEO_VS_CS_COMMIT_CTL                0x02
#define USB_VIDEO_VS_CS_STILL_PROBE_CTL           0x03
#define USB_VIDEO_VS_CS_STILL_COMMIT_CTL          0x04
#define USB_VIDEO_VS_CS_STILL_IMAGE_TRIGGER_CTL   0x05
#define USB_VIDEO_VS_CS_STREAM_ERROR_CODE_CTL     0x06
#define USB_VIDEO_VS_CS_GENERATE_KEY_FRAME_CTL    0x07
#define USB_VIDEO_VS_CS_UPDATE_FRAME_SEGMENT_CTL  0x08
#define USB_VIDEO_VS_CS_SYNCH_DELAY_CTL           0x09


#define USB_VIDEO_PROBE_HINT_FRAMEINTERVAL        0x0001
#define USB_VIDEO_PROBE_HINT_KEYFRAMERATE         0x0002
#define USB_VIDEO_PROBE_HINT_PFRAMERATE           0x0004
#define USB_VIDEO_PROBE_HINT_COMPRESSQUAL         0x0008
#define USB_VIDEO_PROBE_HINT_COMPRESSWNDSIZE      0x0010

#define USB_VIDEO_VS_ERROR_NO_ERROR               0x0
#define USB_VIDEO_VS_ERROR_PROTECTED              0x1
#define USB_VIDEO_VS_ERROR_INPUT_UNDERRUN         0x2
#define USB_VIDEO_VS_ERROR_DISCONTINUITY          0x3
#define USB_VIDEO_VS_ERROR_OUTPUT_UNDERRUN        0x4
#define USB_VIDEO_VS_ERROR_OUTPUT_OVERRUN         0x5
#define USB_VIDEO_VS_ERROR_FORMAT_CHANGE          0x6
#define USB_VIDEO_VS_ERROR_STILL_CAPTURE          0x7
#define USB_VIDEO_VS_ERROR_UNKNOWN                0x8


//
// Video Terminal Types
//

// USB Terminal Types
#define USB_VIDEO_TT_VENDOR_SPECIFIC                0x0100
#define USB_VIDEO_TT_STREAMING                      0x0101

// Input Terminal Types
#define USB_VIDEO_ITT_VENDOR_SPECIFIC               0x0200
#define USB_VIDEO_ITT_CAMERA                        0x0201
#define USB_VIDEO_ITT_MEDIA_TRANSPORT_INPUT         0x0202

// Output Terminal Types
#define USB_VIDEO_OTT_VENDOR_SPECIFIC               0x0300
#define USB_VIDEO_OTT_DISPLAY                       0x0301
#define USB_VIDEO_OTT_MEDIA_TRANSPORT_OUTPUT        0x0302

#pragma pack(1)
typedef struct {
    WORD  bmHint;
    BYTE  bFormatIndex;
    BYTE  bFrameIndex;
    DWORD dwFrameInterval;
    WORD  wKeyFrameRate;
    WORD  wPFrameRate;
    WORD  wCompQuality;
    WORD  wCompWindowSize;
    WORD  wDelay;
    DWORD dwMaxVideoFrameSize;
    DWORD dwMaxPayloadTransferSize;
    DWORD dwClockFrequency;
    BYTE  bmFramingInfo;
    BYTE  bPreferedVersion;
    BYTE  bMinVersion;
    BYTE  bMaxVersion;
} STREAM_PROBE_CONTROLSTRUCT, *PSTREAM_PROBE_CONTROLSTRUCT;
#pragma pack ()


#pragma pack(1)
typedef struct {
    BYTE bLen;
    BYTE bType;
    BYTE bSubtype;
} USBVIDSTDDESCHDR, *PUSBVIDSTDDESCHDR;
#pragma pack ()

// Video Class Control Extended Interface Descriptor
#pragma pack(1)
typedef struct {
    BYTE  bLen;
    BYTE  bType;
    BYTE  bSubtype;
    WORD  wIFVersion;
    WORD  wTotalLen;
    DWORD dwClkFreq;
    BYTE  bInCollection;
    BYTE  bInterface[1];
} USBVIDCTLIFDESCRIPTOR, *PUSBVIDCTLIFDESCRIPTOR;
#pragma pack ()


// This is a generic Input Terminal Descriptor (USB Video Class Spec 3.7.2.1)
#pragma pack(1)
typedef struct {
    BYTE bLen;
    BYTE bType;
    BYTE bSubtype;
    BYTE bTerminalID;
    WORD wTerminalType;
    BYTE bAssocTerminal;
    BYTE iTerminal;
} USBVIDCTLIF_INPUTTERMDESCRIPTOR, *PUSBVIDCTLIF_INPUTTERMDESCRIPTOR;
#pragma pack ()

// This is a Camera Terminal Descriptor (USB Video Class Spec 3.7.2.3)
// It's a special case of the generic Input Terminal Descriptor above.
#pragma pack(1)
typedef struct {
    BYTE bLen;
    BYTE bType;
    BYTE bSubtype;
    BYTE bTerminalID;
    WORD wTerminalType;
    BYTE bAssocTerminal;
    BYTE iTerminal;
    WORD wObjectiveFocalLengthMin;
    WORD wObjectiveFocalLengthMax;
    WORD wOcularFocalLength;
    BYTE bControlSize;
    BYTE bmaControls[1];
} USBVIDCTLIF_CAMTERMDESCRIPTOR, *PUSBVIDCTLIF_CAMTERMDESCRIPTOR;
#pragma pack ()


#pragma pack(1)
typedef struct {
    BYTE bLen;
    BYTE bType;
    BYTE bSubtype;
    BYTE bUnitID;
    BYTE bSourceID;
    WORD wMaxMultiplier;
    BYTE bControlSize;
    BYTE bmaControls[1];
} USBVIDCTLIF_PROCUNITDESCRIPTOR, *PUSBVIDCTLIF_PROCUNITDESCRIPTOR;
#pragma pack ()


// Video Class Color Matching Interface Descriptor
#pragma pack(1)
typedef struct {
    BYTE bLen;
    BYTE bType;
    BYTE bSubtype;
    BYTE bColorPrimaries;
    BYTE bTransferCharacteristics;
    BYTE bMatrixCoefficients;
} USBVIDCOLORIFDESCRIPTOR, *PUSBVIDCOLORIFDESCRIPTOR;
#pragma pack ()

// Video Class Extended Stream Interface (Input or Output) Descriptor
#pragma pack(1)
typedef struct {
    BYTE bLen;
    BYTE bType;
    BYTE bSubtype;
    BYTE bNumFormats;
    WORD wTotalLen;
    BYTE bEndpointAddress;
    BYTE bmInfo;
    BYTE bTerminalLink;
    BYTE bStillCapMethod;
    BYTE bTriggerSupport;
    BYTE bTriggerUsage;
    BYTE bControlSize;
    BYTE bmaControls[1];
} USBVIDSTREAMIFDESCRIPTOR, *PUSBVIDSTREAMIFDESCRIPTOR;
#pragma pack ()

// Video Class Extended Stream Interface generic Format Descriptor
#pragma pack(1)
typedef struct {
    BYTE bLen;
    BYTE bType;
    BYTE bSubtype;
    BYTE bFormatIndex;
    BYTE bNumFrameDescriptors;
} USBVIDSTREAMIF_FORMATDESCRIPTOR, *PUSBVIDSTREAMIF_FORMATDESCRIPTOR;
#pragma pack ()

// Video Class Extended Stream Interface Uncompressed Format Descriptor
#pragma pack(1)
typedef struct {
    BYTE bLen;
    BYTE bType;
    BYTE bSubtype;
    BYTE bFormatIndex;
    BYTE bNumFrameDescriptors;
    BYTE bGUIDFormat[16];
    BYTE bBitsPerPixel;
    BYTE bDefaultFrameIndex;
    BYTE bAspectRatioX;
    BYTE bAspectRatioY;
    BYTE bmInterlaceFlags;
    BYTE bCopyProtect;
} USBVIDSTREAMIF_UNFORMATDESCRIPTOR, *PUSBVIDSTREAMIF_UNFORMATDESCRIPTOR;
#pragma pack ()

// Video Class Extended Stream Interface Uncompressed Frame Descriptor
#pragma pack(1)
typedef struct {
    BYTE bLen;
    BYTE bType;
    BYTE bSubtype;
    BYTE bFrameIndex;
    BYTE bmCapabilities;
    WORD wWidth;
    WORD wHeight;
    DWORD dwMinBitRate;
    DWORD dwMaxBitRate;
    DWORD dwMaxVideoFrameBufferSize;
    DWORD dwDefaultFrameInterval;
    BYTE bFrameIntervalType;    // 0 - Use ContInterval, else num of DescIntervals
    union 
    {
        struct {
            DWORD dwMinFrameInterval;
            DWORD dwMaxFrameInterval;
            DWORD dwFrameIntervalStep;
        } strCont;
        DWORD dwDescrete[1];
    } Interval;
} USBVIDSTREAMIF_UNFRAMEDESCRIPTOR, *PUSBVIDSTREAMIF_UNFRAMEDESCRIPTOR;
#pragma pack ()


// Video Class Extended Stream Interface MJPEG Format Descriptor
#pragma pack(1)
typedef struct {
    BYTE bLen;
    BYTE bType;
    BYTE bSubtype;
    BYTE bFormatIndex;
    BYTE bNumFrameDescriptors;
    BYTE bmFlags;
    BYTE bDefaultFrameIndex;
    BYTE bAspectRatioX;
    BYTE bAspectRatioY;
    BYTE bmInterlaceFlags;
    BYTE bCopyProtect;
} USBVIDSTREAMIF_MJEPGFORMATDESCRIPTOR, *PUSBVIDSTREAMIF_MJPEGFORMATDESCRIPTOR;
#pragma pack ()


// Video Class Extended Stream Interface MJPEG Frame Descriptor
#pragma pack(1)
typedef struct {
    BYTE  bLen;
    BYTE  bType;
    BYTE  bSubtype;
    BYTE  bFrameIndex;
    BYTE  bmCapabilities;
    WORD  wWidth;
    WORD  wHeight;
    DWORD dwMinBitRate;
    DWORD dwMaxBitRate;
    DWORD dwMaxVideoFrameBufferSize;
    DWORD dwDefaultFrameInterval;
    BYTE  bFrameIntervalType;    // 0 - Use ContInterval, else num of DescIntervals
    union 
    {
        struct {
            DWORD dwMinFrameInterval;
            DWORD dwMaxFrameInterval;
            DWORD dwFrameIntervalStep;
        } strCont;
        DWORD dwDescrete[1];
    } Interval;
} USBVIDSTREAMIF_MJPEGFRAMEDESCRIPTOR, *PUSBVIDSTREAMIF_MJPEGFRAMEDESCRIPTOR;
#pragma pack ()

// Video Class Extended Stream Interface Still Image Frame Descriptor
#pragma pack(1)
typedef struct {
    BYTE bLen;
    BYTE bType;
    BYTE bSubtype;
    BYTE bEndpointAddress;
    BYTE bNumImageSizePatterns;
    struct {
        WORD wWidth;
        WORD wHeight;
    } sStillFmt[1];
} USBVIDSTREAMIF_STILLIMGDESCRIPTOR, *PUSBVIDSTREAMIF_STILLIMGDESCRIPTOR;
#pragma pack ()


// Video Class Payload header
#pragma pack(1)
typedef struct {
    BYTE bLen;
    BYTE bFlags;
    BYTE extra[10];
} USBVIDPAYLOADHDR, *PUSBVIDPAYLOADHDR;
#pragma pack ()

#define USBVID_PAYLOADHDR_EOH         0x80
#define USBVID_PAYLOADHDR_ERR         0x40
#define USBVID_PAYLOADHDR_STILLFRAME  0x20
#define USBVID_PAYLOADHDR_RESERVED    0x10
#define USBVID_PAYLOADHDR_SCKLFIELD   0x08
#define USBVID_PAYLOADHDR_PREFIELD    0x04
#define USBVID_PAYLOADHDR_FRAMEEND    0x02
#define USBVID_PAYLOADHDR_FRAMEID     0x01

#define DRV_MAJORVER   1
#define DRV_MINORVER   0

// Debug zone support
//
// Used as a prefix string for all debug zone messages.
#define DTAG        TEXT ("USBCam: ")

/*
 * First six zones are taken by the MDD
 *

#define ZONE_ERROR           DEBUGZONE(0)
#define ZONE_WARN            DEBUGZONE(1)
#define ZONE_INIT            DEBUGZONE(2)
#define ZONE_FUNCTION        DEBUGZONE(3)
#define ZONE_IOCTL           DEBUGZONE(4)
#define ZONE_DEVICE          DEBUGZONE(5)

*
* Other zones are available to the PDD, though they won't be 
* listed by name in the Platform Builder debug zones window.
*/
#define ZONE_THREAD     DEBUGZONE(6)
//#define ZONE_THSTATE    DEBUGZONE(7)
#define ZONE_PACKETS    DEBUGZONE(8)
#define ZONE_TIMING     DEBUGZONE(9)
//#define ZONE_READDATA   DEBUGZONE(10)
#define ZONE_PROBE      DEBUGZONE(11)
#define ZONE_STILL      DEBUGZONE(12)
#define ZONE_VIDFRAME   DEBUGZONE(13)
#define ZONE_PROPERTY   DEBUGZONE(14)


#define DRIVER_NAME   TEXT("usbcam.dll")

#define DEVICE_PREFIX TEXT("CAM")

#define CLASS_NAME_SZ    TEXT("Video_Class")
#define CLIENT_REGKEY_SZ TEXT("Drivers\\USB\\ClientDrivers\\Video_Class")

//
// USB Video Interface Descriptor
//

#define DRIVER_SETTINGS \
            sizeof(USB_DRIVER_SETTINGS),  \
            USB_NO_INFO,   \
            USB_NO_INFO,   \
            USB_NO_INFO,   \
            USB_NO_INFO,   \
            USB_NO_INFO,   \
            USB_NO_INFO,   \
            USB_DEVICE_CLASS_VIDEO,   \
            USB_NO_INFO,   \
            USB_NO_INFO

typedef struct {
    USB_INTERFACE_DESCRIPTOR ifDesc;
    PUSBVIDCTLIFDESCRIPTOR lpifExtDesc;
    WORD wExtDescSize;
    BOOL fEndpoint;
    USB_ENDPOINT_DESCRIPTOR epDesc;
    LPVOID lpepExtDesc;
} USBCTLIF, *PUSBCTLIF;


typedef struct {
    USB_INTERFACE_DESCRIPTOR ifDesc;
    PUSBVIDSTREAMIFDESCRIPTOR lpifExtDesc;
    WORD wExtDescSize;
    BOOL fEndpoint;
    USB_ENDPOINT_DESCRIPTOR epDesc;
    LPVOID lpepExtDesc;
} USBSTRMIF, *PUSBSTRMIF;

typedef struct {
    // Device info
    DWORD dwSize;
    USBCTLIF usbctlIF;      // Control interface descriptor
    int nStreamInterfaces;
    PUSBSTRMIF usbstrmIF;   // Array of Stream interface descriptors
} STREAMINFO, *PSTREAMINFO;

typedef struct {
    DWORD dwSize;
    CRITICAL_SECTION csDCall;
    INT nNumOpens;
    HANDLE hStreamDevice;
    // USB support
    USB_HANDLE hDevice;
    USB_DEVICE usbDeviceInfo;
    LPCUSB_FUNCS lpUsbFuncs;
    HANDLE hUnload;
    BOOL fUnloadFlag;
    PSTREAMINFO pStreamInfo;
} DRVCONTEXT, *PDRVCONTEXT;

BOOL USBDeviceNotificationCallback (LPVOID lpvNotifyParameter, DWORD dwCode,
                                    LPDWORD* dwInfo1, LPDWORD* dwInfo2,
                                    LPDWORD* dwInfo3, LPDWORD* dwInfo4);

static int ExtractInfo (USB_HANDLE hDevice, LPCUSB_FUNCS lpUsbFuncs,
                  LPCUSB_INTERFACE lpInterface, LPCWSTR szUniqueDriverId,
                  LPCUSB_DRIVER_SETTINGS lpDriverSettings, DWORD *pdwStreamInfo);

static BOOL ParseStreamInterfaces (PSTREAMINFO pInfo, LPCUSB_DEVICE lpUsbDev, BYTE bIFCtl,
                            BYTE bIFSubCtl, BYTE bIFStrm, BYTE bIFSubStrm);


#endif
