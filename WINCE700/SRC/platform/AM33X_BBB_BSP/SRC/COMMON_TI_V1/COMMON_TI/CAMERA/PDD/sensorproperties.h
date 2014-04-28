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

#ifndef __SENSORPROPERTIES_H
#define __SENSORPROPERTIES_H

// ------------------------------------------------------------------------
static CSPROPERTY_STEPPING_LONG BrightnessRangeAndStep [] = 
{
    {
        1,                  // SteppingDelta (range / steps)
        0,                  // Reserved
        -10000,             // Minimum in (IRE * 100) units
        10000               // Maximum in (IRE * 100) units
    }
};

const static LONG BrightnessDefault = 750;

static CSPROPERTY_MEMBERSLIST BrightnessMembersList [] = 
{
    {
        /*CSPROPERTY_MEMBERSHEADER*/
        {    
            CSPROPERTY_MEMBER_RANGES,                /*MembersFlags*/
            sizeof (CSPROPERTY_STEPPING_LONG),       /*MembersSize*/
            _countof (BrightnessRangeAndStep),   /*MembersCount*/
            0                                        /*flags 0 or CSPROPERTY_MEMBER_FLAG_DEFAULT*/
        },
        /*Members*/
        (PVOID) BrightnessRangeAndStep,
     },
     {
        {
            CSPROPERTY_MEMBER_VALUES,
            sizeof (BrightnessDefault),
            1,
            CSPROPERTY_MEMBER_FLAG_DEFAULT
        },
        (PVOID) &BrightnessDefault,
    }    
};

static CSPROPERTY_VALUES BrightnessValues =
{
    {
        STATICGUIDOF (CSPROPTYPESETID_General),
        VT_I4,
        0
    },
    _countof (BrightnessMembersList),
    BrightnessMembersList
};

// ------------------------------------------------------------------------
// The contrast value is expressed as a gain factor multiplied by 100. 
static CSPROPERTY_STEPPING_LONG ContrastRangeAndStep [] = 
{
    {
        1,                  // SteppingDelta (range / steps)
        0,                  // Reserved
        0,                  // Minimum as a gain factor multiplied by 100
        10000               // Maximum as a gain factor multiplied by 100
    }
};

const static LONG ContrastDefault = 100;

static CSPROPERTY_MEMBERSLIST ContrastMembersList [] = 
{
    {
        /*CSPROPERTY_MEMBERSHEADER*/
        {    
            CSPROPERTY_MEMBER_RANGES,                /*MembersFlags*/
            sizeof (CSPROPERTY_STEPPING_LONG),       /*MembersSize*/
            _countof (ContrastRangeAndStep),     /*MembersCount*/
            0                                        /*flags 0 or CSPROPERTY_MEMBER_FLAG_DEFAULT*/
        },
        /*Members*/
        (PVOID) ContrastRangeAndStep,
     },
     {
        {
            CSPROPERTY_MEMBER_VALUES,
            sizeof (ContrastDefault),
            1,
            CSPROPERTY_MEMBER_FLAG_DEFAULT
        },
        (PVOID) &ContrastDefault,
    }    
};

static CSPROPERTY_VALUES ContrastValues =
{
    {
        STATICGUIDOF (CSPROPTYPESETID_General),
        VT_I4,
        0
    },
    _countof (ContrastMembersList),
    ContrastMembersList
};

// ------------------------------------------------------------------------
// The value of the hue setting is expressed in degrees multiplied by 100. 
static CSPROPERTY_STEPPING_LONG HueRangeAndStep [] = 
{
    {
        1,                  // SteppingDelta (range / steps)
        0,                  // Reserved
        -18000,            // Minimum in degrees multiplied by 100. 
        18000              // Maximum in degrees multiplied by 100. 
    }
};

const static LONG HueDefault = 0;

static CSPROPERTY_MEMBERSLIST HueMembersList [] = 
{
    {
        {
            CSPROPERTY_MEMBER_RANGES,
            sizeof (CSPROPERTY_STEPPING_LONG),
            _countof (HueRangeAndStep),
            0
        },
        (PVOID) HueRangeAndStep
     },
     {
        {
            CSPROPERTY_MEMBER_VALUES,
            sizeof (HueDefault),
            1,
            CSPROPERTY_MEMBER_FLAG_DEFAULT
        },
        (PVOID) &HueDefault,
    }    
};

static CSPROPERTY_VALUES HueValues =
{
    {
        STATICGUIDOF (CSPROPTYPESETID_General),
        VT_I4,
        0
    },
    _countof (HueMembersList),
    HueMembersList
};

// ------------------------------------------------------------------------
// The value of the saturation setting is expressed as gain multiplied by 100.

static CSPROPERTY_STEPPING_LONG SaturationRangeAndStep [] = 
{
    {
        1,                  // SteppingDelta (range / steps)
        0,                  // Reserved
        0,                  // Minimum in (gain * 100) units
        10000               // Maximum in (gain * 100) units
    }
};

const static LONG SaturationDefault = 100;

static CSPROPERTY_MEMBERSLIST SaturationMembersList [] = 
{
    {
        {
            CSPROPERTY_MEMBER_RANGES,
            sizeof (CSPROPERTY_STEPPING_LONG),
            _countof (SaturationRangeAndStep),
            0
        },
        (PVOID) SaturationRangeAndStep
     },
     {
        {
            CSPROPERTY_MEMBER_VALUES,
            sizeof (SaturationDefault),
            1,
            CSPROPERTY_MEMBER_FLAG_DEFAULT
        },
        (PVOID) &SaturationDefault,
    }    
};

static CSPROPERTY_VALUES SaturationValues =
{
    {
        STATICGUIDOF (CSPROPTYPESETID_General),
        VT_I4,
        0
    },
    _countof (SaturationMembersList),
    SaturationMembersList
};

// ------------------------------------------------------------------------
// Sharpness is expressed in arbitrary units
static CSPROPERTY_STEPPING_LONG SharpnessRangeAndStep [] = 
{
    {
        1,                  // SteppingDelta (range / steps)
        0,                  // Reserved
        0,                  // Minimum
        100                 // Maximum
    }
};

const static LONG SharpnessDefault = 50;

static CSPROPERTY_MEMBERSLIST SharpnessMembersList [] = 
{
    {
        {
            CSPROPERTY_MEMBER_RANGES,
            sizeof (CSPROPERTY_STEPPING_LONG),
            _countof (SharpnessRangeAndStep),
            0
        },
        (PVOID) SharpnessRangeAndStep
     },
     {
        {
            CSPROPERTY_MEMBER_VALUES,
            sizeof (SharpnessDefault),
            1,
            CSPROPERTY_MEMBER_FLAG_DEFAULT
        },
        (PVOID) &SharpnessDefault,
    }    
};

static CSPROPERTY_VALUES SharpnessValues =
{
    {
        STATICGUIDOF (CSPROPTYPESETID_General),
        VT_I4,
        0
    },
    _countof (SharpnessMembersList),
    SharpnessMembersList
};

// ------------------------------------------------------------------------
// The white balance value is expressed as a color temperature, in degrees Kelvin
static CSPROPERTY_STEPPING_LONG WhiteBalanceRangeAndStep [] = 
{
    {
        1,                  // SteppingDelta (range / steps)
        0,                  // Reserved
        273,                // Minimum in (degrees Kelvin)
        310                 // Maximum in degrees Kelvin)
    }
};

const static LONG WhiteBalanceDefault = 298;

static CSPROPERTY_MEMBERSLIST WhiteBalanceMembersList [] = 
{
    {
        {
            CSPROPERTY_MEMBER_RANGES,
            sizeof (CSPROPERTY_STEPPING_LONG),
            _countof (WhiteBalanceRangeAndStep),
            0
        },
        (PVOID) WhiteBalanceRangeAndStep
     },
     {
        {
            CSPROPERTY_MEMBER_VALUES,
            sizeof (WhiteBalanceDefault),
            1,
            CSPROPERTY_MEMBER_FLAG_DEFAULT
        },
        (PVOID) &WhiteBalanceDefault,
    }    
};

static CSPROPERTY_VALUES WhiteBalanceValues =
{
    {
        STATICGUIDOF (CSPROPTYPESETID_General),
        VT_I4,
        0
    },
    _countof (WhiteBalanceMembersList),
    WhiteBalanceMembersList
};

// ------------------------------------------------------------------------
// The value of the gamma setting is expressed in gamma multiplied by 100
static CSPROPERTY_STEPPING_LONG GammaRangeAndStep [] = 
{
    {
        1,                  // SteppingDelta (range / steps)
        0,                  // Reserved
        1,                  // Minimum (in gamma multiplied by 100)
        500                 // Maximum (in gamma multiplied by 100)
    }
};

const static LONG GammaDefault = 100; // gamma = 1

static CSPROPERTY_MEMBERSLIST GammaMembersList [] = 
{
    {
        /*CSPROPERTY_MEMBERSHEADER*/
        {    
            CSPROPERTY_MEMBER_RANGES,                /*MembersFlags*/
            sizeof (CSPROPERTY_STEPPING_LONG),       /*MembersSize*/
            _countof (GammaRangeAndStep),        /*MembersCount*/
            0                                        /*flags 0 or CSPROPERTY_MEMBER_FLAG_DEFAULT*/
        },
        /*Members*/
        (PVOID) GammaRangeAndStep,
     },
     {
        {
            CSPROPERTY_MEMBER_VALUES,
            sizeof (GammaDefault),
            1,
            CSPROPERTY_MEMBER_FLAG_DEFAULT
        },
        (PVOID) &GammaDefault,
    }    
};

static CSPROPERTY_VALUES GammaValues =
{
    {
        STATICGUIDOF (CSPROPTYPESETID_General),
        VT_I4,
        0
    },
    _countof (GammaMembersList),
    GammaMembersList
};

// ------------------------------------------------------------------------
static CSPROPERTY_STEPPING_LONG ColorEnableRangeAndStep [] = 
{
    {
        1,                  // SteppingDelta (range / steps)
        0,                  // Reserved
        0,                  // Minimum (Color is disabled)
        1                   // Maximum (Color is enabled)
    }
};

const static LONG ColorEnableDefault = 1;

static CSPROPERTY_MEMBERSLIST ColorEnableMembersList [] = 
{
    {
        /*CSPROPERTY_MEMBERSHEADER*/
        {    
            CSPROPERTY_MEMBER_RANGES,                /*MembersFlags*/
            sizeof (CSPROPERTY_STEPPING_LONG),       /*MembersSize*/
            _countof (ColorEnableRangeAndStep),  /*MembersCount*/
            0                                        /*flags 0 or CSPROPERTY_MEMBER_FLAG_DEFAULT*/
        },
        /*Members*/
        (PVOID) ColorEnableRangeAndStep,
     },
     {
        {
            CSPROPERTY_MEMBER_VALUES,
            sizeof (ColorEnableDefault),
            1,
            CSPROPERTY_MEMBER_FLAG_DEFAULT
        },
        (PVOID) &ColorEnableDefault,
    }    
};

static CSPROPERTY_VALUES ColorEnableValues =
{
    {
        STATICGUIDOF (CSPROPTYPESETID_General),
        VT_I4,
        0
    },
    _countof (ColorEnableMembersList),
    ColorEnableMembersList
};

// ------------------------------------------------------------------------
static CSPROPERTY_STEPPING_LONG BackLightCompensationRangeAndStep [] = 
{
    {
        1,                  // SteppingDelta (range / steps)
        0,                  // Reserved
        0,                  // Minimum ( BackLight Compensation is off
        1                   // Maximum ( BackLight Compensation is on )
    }
};

const static LONG BackLightCompensationDefault = 1;

static CSPROPERTY_MEMBERSLIST BackLightCompensationMembersList [] = 
{
    {
        /*CSPROPERTY_MEMBERSHEADER*/
        {    
            CSPROPERTY_MEMBER_RANGES,                /*MembersFlags*/
            sizeof (CSPROPERTY_STEPPING_LONG),        /*MembersSize*/
            _countof (BackLightCompensationRangeAndStep),    /*MembersCount*/
            0                                        /*flags 0 or CSPROPERTY_MEMBER_FLAG_DEFAULT*/
        },
        /*Members*/
        (PVOID) BackLightCompensationRangeAndStep,
     },
     {
        {
            CSPROPERTY_MEMBER_VALUES,
            sizeof (BackLightCompensationDefault),
            1,
            CSPROPERTY_MEMBER_FLAG_DEFAULT
        },
        (PVOID) &BackLightCompensationDefault,
    }    
};

static CSPROPERTY_VALUES BackLightCompensationValues =
{
    {
        STATICGUIDOF (CSPROPTYPESETID_General),
        VT_I4,
        0
    },
    _countof (BackLightCompensationMembersList),
    BackLightCompensationMembersList
};

// ------------------------------------------------------------------------
static CSPROPERTY_STEPPING_LONG GainRangeAndStep [] = 
{
    {
        1,                  // SteppingDelta (range / steps)
        0,                  // Reserved
        -10,                // Minimum 
        15                  // Maximum 
    }
};

const static LONG GainDefault = 1;

static CSPROPERTY_MEMBERSLIST GainMembersList [] = 
{
    {
        /*CSPROPERTY_MEMBERSHEADER*/
        {    
            CSPROPERTY_MEMBER_RANGES,                /*MembersFlags*/
            sizeof (CSPROPERTY_STEPPING_LONG),       /*MembersSize*/
            _countof (GainRangeAndStep),         /*MembersCount*/
            0                                        /*flags 0 or CSPROPERTY_MEMBER_FLAG_DEFAULT*/
        },
        /*Members*/
        (PVOID) GainRangeAndStep,
     },
     {
        {
            CSPROPERTY_MEMBER_VALUES,
            sizeof (GainDefault),
            1,
            CSPROPERTY_MEMBER_FLAG_DEFAULT
        },
        (PVOID) &GainDefault,
    }    
};

static CSPROPERTY_VALUES GainValues =
{
    {
        STATICGUIDOF (CSPROPTYPESETID_General),
        VT_I4,
        0
    },
    _countof (GainMembersList),
    GainMembersList
};

// ------------------------------------------------------------------------
static CSPROPERTY_STEPPING_LONG PanRangeAndStep [] = 
{
    {
        1,                  // SteppingDelta (range / steps)
        0,                  // Reserved
        -180,               // Minimum in Degrees
        180                 // Maximum in Degrees
    }
};

const static LONG PanDefault = 0;

static CSPROPERTY_MEMBERSLIST PanMembersList [] = 
{
    {
        {
            CSPROPERTY_MEMBER_RANGES,
            sizeof (CSPROPERTY_STEPPING_LONG),
            _countof (PanRangeAndStep),
            0
        },
        (PVOID) PanRangeAndStep,
     },
     {
        {
            CSPROPERTY_MEMBER_VALUES,
            sizeof (PanDefault),
            1,
            CSPROPERTY_MEMBER_FLAG_DEFAULT
        },
        (PVOID) &PanDefault,
    }    
};

static CSPROPERTY_VALUES PanValues =
{
    {
        STATICGUIDOF (CSPROPTYPESETID_General),
        VT_I4,
        0
    },
    _countof (PanMembersList),
    PanMembersList
};

// ------------------------------------------------------------------------
static CSPROPERTY_STEPPING_LONG TiltRangeAndStep [] = 
{
    {
        1,                  // SteppingDelta (range / steps)
        0,                  // Reserved
        -180,               // Minimum in Degrees
        180                 // Maximum in Degrees
    }
};

const static LONG TiltDefault = 0;

static CSPROPERTY_MEMBERSLIST TiltMembersList [] = 
{
    {
        {
            CSPROPERTY_MEMBER_RANGES,
            sizeof (CSPROPERTY_STEPPING_LONG),
            _countof (TiltRangeAndStep),
            0
        },
        (PVOID) TiltRangeAndStep,
     },
     {
        {
            CSPROPERTY_MEMBER_VALUES,
            sizeof (TiltDefault),
            1,
            CSPROPERTY_MEMBER_FLAG_DEFAULT
        },
        (PVOID) &TiltDefault,
    }    
};

static CSPROPERTY_VALUES TiltValues =
{
    {
        STATICGUIDOF (CSPROPTYPESETID_General),
        VT_I4,
        0
    },
    _countof (TiltMembersList),
    TiltMembersList
};

// ------------------------------------------------------------------------
static CSPROPERTY_STEPPING_LONG RollRangeAndStep [] = 
{
    {
        1,                  // SteppingDelta (range / steps)
        0,                  // Reserved
        -180,               // Minimum in Degrees
        180                 // Maximum in Degrees
    }
};

const static LONG RollDefault = 0;

static CSPROPERTY_MEMBERSLIST RollMembersList [] = 
{
    {
        {
            CSPROPERTY_MEMBER_RANGES,
            sizeof (CSPROPERTY_STEPPING_LONG),
            _countof (RollRangeAndStep),
            0
        },
        (PVOID) RollRangeAndStep,
     },
     {
        {
            CSPROPERTY_MEMBER_VALUES,
            sizeof (RollDefault),
            1,
            CSPROPERTY_MEMBER_FLAG_DEFAULT
        },
        (PVOID) &RollDefault,
    }    
};

static CSPROPERTY_VALUES RollValues =
{
    {
        STATICGUIDOF (CSPROPTYPESETID_General),
        VT_I4,
        0
    },
    _countof (RollMembersList),
    RollMembersList
};

// ------------------------------------------------------------------------
static CSPROPERTY_STEPPING_LONG ZoomRangeAndStep [] = 
{
    {
        1,                  // SteppingDelta (range / steps)
        0,                  // Reserved
        10,                 // Minimum in Millimeters
        600                 // Maximum in Millimeters 
    }
};

const static LONG ZoomDefault = 10;

static CSPROPERTY_MEMBERSLIST ZoomMembersList [] = 
{
    {
        {
            CSPROPERTY_MEMBER_RANGES,
            sizeof (CSPROPERTY_STEPPING_LONG),
            _countof (ZoomRangeAndStep),
            0
        },
        (PVOID) ZoomRangeAndStep,
     },
     {
        {
            CSPROPERTY_MEMBER_VALUES,
            sizeof (ZoomDefault),
            1,
            CSPROPERTY_MEMBER_FLAG_DEFAULT
        },
        (PVOID) &ZoomDefault,
    }    
};

static CSPROPERTY_VALUES ZoomValues =
{
    {
        STATICGUIDOF (CSPROPTYPESETID_General),
        VT_I4,
        0
    },
    _countof (ZoomMembersList),
    ZoomMembersList
};

// ------------------------------------------------------------------------
static CSPROPERTY_STEPPING_LONG IrisRangeAndStep [] = 
{
    {
        1,                  // SteppingDelta (range / steps)
        0,                  // Reserved
        0,                  // Minimum in units of fstop * 10
        3456                // Maximum in units of fstop * 10
    }
};

const static LONG IrisDefault = 2500;

static CSPROPERTY_MEMBERSLIST IrisMembersList [] = 
{
    {
        {
            CSPROPERTY_MEMBER_RANGES,
            sizeof (CSPROPERTY_STEPPING_LONG),
            _countof (IrisRangeAndStep),
            0
        },
        (PVOID) IrisRangeAndStep,
     },
     {
        {
            CSPROPERTY_MEMBER_VALUES,
            sizeof (IrisDefault),
            1,
            CSPROPERTY_MEMBER_FLAG_DEFAULT
        },
        (PVOID) &IrisDefault,
    }    
};

static CSPROPERTY_VALUES IrisValues =
{
    {
        STATICGUIDOF (CSPROPTYPESETID_General),
        VT_I4,
        0
    },
    _countof (IrisMembersList),
    IrisMembersList
};

// ------------------------------------------------------------------------
static CSPROPERTY_STEPPING_LONG ExposureRangeAndStep [] = 
{
    {
        1,                  // SteppingDelta (range / steps)
        0,                  // Reserved
        -9,                 // Minimum (This value is expressed in log base 2 seconds, thus, 
                            //         for values less than zero, the exposure time is 1/2n seconds. For positive values and zero, the exposure time is 2n seconds)
        4                   // Maximum (This value is expressed in log base 2 seconds, thus, 
                            //         for values less than zero, the exposure time is 1/2n seconds. For positive values and zero, the exposure time is 2n seconds)
    }
};

const static LONG ExposureDefault = -4;

static CSPROPERTY_MEMBERSLIST ExposureMembersList [] = 
{
    {
        {
            CSPROPERTY_MEMBER_RANGES,
            sizeof (CSPROPERTY_STEPPING_LONG),
            _countof (ExposureRangeAndStep),
            0
        },
        (PVOID) ExposureRangeAndStep,
     },
     {
        {
            CSPROPERTY_MEMBER_VALUES,
            sizeof (ExposureDefault),
            1,
            CSPROPERTY_MEMBER_FLAG_DEFAULT
        },
        (PVOID) &ExposureDefault,
    }    
};

static CSPROPERTY_VALUES ExposureValues =
{
    {
        STATICGUIDOF (CSPROPTYPESETID_General),
        VT_I4,
        0
    },
    _countof (ExposureMembersList),
    ExposureMembersList
};

// ------------------------------------------------------------------------
static CSPROPERTY_STEPPING_LONG FocusRangeAndStep [] = 
{
    {
        1,                  // SteppingDelta (range / steps)
        0,                  // Reserved
        0,                  // Minimum in millimeters
        200                // Maximum in millimeters
    }
};

const static LONG FocusDefault = 100;

static CSPROPERTY_MEMBERSLIST FocusMembersList [] = 
{
    {
        {
            CSPROPERTY_MEMBER_RANGES,
            sizeof (CSPROPERTY_STEPPING_LONG),
            _countof (FocusRangeAndStep),
            0
        },
        (PVOID) FocusRangeAndStep,
     },
     {
        {
            CSPROPERTY_MEMBER_VALUES,
            sizeof (FocusDefault),
            1,
            CSPROPERTY_MEMBER_FLAG_DEFAULT
        },
        (PVOID) &FocusDefault,
    }    
};

static CSPROPERTY_VALUES FocusValues =
{
    {
        STATICGUIDOF (CSPROPTYPESETID_General),
        VT_I4,
        0
    },
    _countof (FocusMembersList),
    FocusMembersList
};

// ------------------------------------------------------------------------
static CSPROPERTY_STEPPING_LONG FlashRangeAndStep [] = 
{
    {
        1,                  // SteppingDelta (range / steps)
        0,                  // Reserved
        0,                  // Minimum (means flash is turned off)
        200                 // Maximum (in millimeter units. Values greater than 0 means 
                            // flash is turned on and the value reflects the distance of the object from the camera.
    }
};

const static LONG FlashDefault = 0; // By default flash is turned off

static CSPROPERTY_MEMBERSLIST FlashMembersList [] = 
{
    {
        {
            CSPROPERTY_MEMBER_RANGES,
            sizeof (CSPROPERTY_STEPPING_LONG),
            _countof (FlashRangeAndStep),
            0
        },
        (PVOID) FlashRangeAndStep,
     },
     {
        {
            CSPROPERTY_MEMBER_VALUES,
            sizeof (FlashDefault),
            1,
            CSPROPERTY_MEMBER_FLAG_DEFAULT
        },
        (PVOID) &FlashDefault,
    }    
};

static CSPROPERTY_VALUES FlashValues =
{
    {
        STATICGUIDOF (CSPROPTYPESETID_General),
        VT_I4,
        0
    },
    _countof (FlashMembersList),
    FlashMembersList
};


// ------------------------------------------------------------------------

DEFINE_CSPROPERTY_TABLE(VideoProcAmpProperties)
{
    DEFINE_CSPROPERTY_ITEM
    (
        CSPROPERTY_VIDEOPROCAMP_BRIGHTNESS,
        FALSE,                                  // GetSupported or Handler
        sizeof(CSPROPERTY_VIDEOPROCAMP_S),      // MinProperty
        sizeof(CSPROPERTY_VIDEOPROCAMP_S),      // MinData
        FALSE,                                  // SetSupported or Handler
        &BrightnessValues,                      // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        sizeof(ULONG)                           // SerializedSize
    ),

    DEFINE_CSPROPERTY_ITEM
    (
        CSPROPERTY_VIDEOPROCAMP_CONTRAST,
        FALSE,                                  // GetSupported or Handler
        sizeof(CSPROPERTY_VIDEOPROCAMP_S),      // MinProperty
        sizeof(CSPROPERTY_VIDEOPROCAMP_S),      // MinData
        FALSE,                                  // SetSupported or Handler
        &ContrastValues,                        // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        sizeof(ULONG)                           // SerializedSize
    ),

    DEFINE_CSPROPERTY_ITEM
    (
        CSPROPERTY_VIDEOPROCAMP_HUE,
        FALSE,                                  // GetSupported or Handler
        sizeof(CSPROPERTY_VIDEOPROCAMP_S),      // MinProperty
        sizeof(CSPROPERTY_VIDEOPROCAMP_S),      // MinData
        FALSE,                                  // SetSupported or Handler
        &HueValues,                             // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        sizeof(ULONG)                           // SerializedSize
    ),    

    DEFINE_CSPROPERTY_ITEM
    (
        CSPROPERTY_VIDEOPROCAMP_SATURATION,
        FALSE,                                  // GetSupported or Handler
        sizeof(CSPROPERTY_VIDEOPROCAMP_S),      // MinProperty
        sizeof(CSPROPERTY_VIDEOPROCAMP_S),      // MinData
        FALSE,                                  // SetSupported or Handler
        &SaturationValues,                      // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        sizeof(ULONG)                           // SerializedSize
    ),

    DEFINE_CSPROPERTY_ITEM
    (
        CSPROPERTY_VIDEOPROCAMP_SHARPNESS,
        FALSE,                                  // GetSupported or Handler
        sizeof(CSPROPERTY_VIDEOPROCAMP_S),      // MinProperty
        sizeof(CSPROPERTY_VIDEOPROCAMP_S),      // MinData
        FALSE,                                  // SetSupported or Handler
        &SharpnessValues,                       // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        sizeof(ULONG)                           // SerializedSize
    ),

    DEFINE_CSPROPERTY_ITEM
    (
        CSPROPERTY_VIDEOPROCAMP_GAMMA,
        FALSE,                                  // GetSupported or Handler
        sizeof(CSPROPERTY_VIDEOPROCAMP_S),      // MinProperty
        sizeof(CSPROPERTY_VIDEOPROCAMP_S),      // MinData
        FALSE,                                  // SetSupported or Handler
        &GammaValues,                           // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        sizeof(ULONG)                           // SerializedSize
    ),

    DEFINE_CSPROPERTY_ITEM
    (
        CSPROPERTY_VIDEOPROCAMP_COLORENABLE,
        FALSE,                                  // GetSupported or Handler
        sizeof(CSPROPERTY_VIDEOPROCAMP_S),      // MinProperty
        sizeof(CSPROPERTY_VIDEOPROCAMP_S),      // MinData
        FALSE,                                  // SetSupported or Handler
        &ColorEnableValues,                     // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        sizeof(ULONG)                           // SerializedSize
    ),

    DEFINE_CSPROPERTY_ITEM
    (
        CSPROPERTY_VIDEOPROCAMP_WHITEBALANCE,
        FALSE,                                  // GetSupported or Handler
        sizeof(CSPROPERTY_VIDEOPROCAMP_S),      // MinProperty
        sizeof(CSPROPERTY_VIDEOPROCAMP_S),      // MinData
        FALSE,                                  // SetSupported or Handler
        &WhiteBalanceValues,                    // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        sizeof(ULONG)                           // SerializedSize
    ),

    DEFINE_CSPROPERTY_ITEM
    (
        CSPROPERTY_VIDEOPROCAMP_BACKLIGHT_COMPENSATION,
        FALSE,                                  // GetSupported or Handler
        sizeof(CSPROPERTY_VIDEOPROCAMP_S),      // MinProperty
        sizeof(CSPROPERTY_VIDEOPROCAMP_S),      // MinData
        FALSE,                                  // SetSupported or Handler
        &BackLightCompensationValues,           // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        sizeof(ULONG)                           // SerializedSize
    ),

    DEFINE_CSPROPERTY_ITEM
    (
        CSPROPERTY_VIDEOPROCAMP_GAIN,
        FALSE,                                  // GetSupported or Handler
        sizeof(CSPROPERTY_VIDEOPROCAMP_S),      // MinProperty
        sizeof(CSPROPERTY_VIDEOPROCAMP_S),      // MinData
        FALSE,                                  // SetSupported or Handler
        &GainValues,                            // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        sizeof(ULONG)                           // SerializedSize
    )

};

DEFINE_CSPROPERTY_TABLE(CameraControlProperties)
{
    DEFINE_CSPROPERTY_ITEM
    (
        CSPROPERTY_CAMERACONTROL_PAN,
        FALSE,                                  // GetSupported or Handler
        sizeof(CSPROPERTY_CAMERACONTROL_S),     // MinProperty
        sizeof(CSPROPERTY_CAMERACONTROL_S),     // MinData
        FALSE,                                  // SetSupported or Handler
        &PanValues,                             // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        sizeof(ULONG)                           // SerializedSize
    ),
    DEFINE_CSPROPERTY_ITEM
    (
        CSPROPERTY_CAMERACONTROL_TILT,
        FALSE,                                  // GetSupported or Handler
        sizeof(CSPROPERTY_CAMERACONTROL_S),     // MinProperty
        sizeof(CSPROPERTY_CAMERACONTROL_S),     // MinData
        FALSE,                                  // SetSupported or Handler
        &TiltValues,                            // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        sizeof(ULONG)                           // SerializedSize
    ),
    DEFINE_CSPROPERTY_ITEM
    (
        CSPROPERTY_CAMERACONTROL_ROLL,
        FALSE,                                  // GetSupported or Handler
        sizeof(CSPROPERTY_CAMERACONTROL_S),     // MinProperty
        sizeof(CSPROPERTY_CAMERACONTROL_S),     // MinData
        FALSE,                                  // SetSupported or Handler
        &RollValues,                            // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        sizeof(ULONG)                           // SerializedSize
    ),
    DEFINE_CSPROPERTY_ITEM
    (
        CSPROPERTY_CAMERACONTROL_ZOOM,
        FALSE,                                  // GetSupported or Handler
        sizeof(CSPROPERTY_CAMERACONTROL_S),     // MinProperty
        sizeof(CSPROPERTY_CAMERACONTROL_S),     // MinData
        FALSE,                                  // SetSupported or Handler
        &ZoomValues,                            // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        sizeof(ULONG)                           // SerializedSize
    ),
    DEFINE_CSPROPERTY_ITEM
    (
        CSPROPERTY_CAMERACONTROL_IRIS,
        FALSE,                                  // GetSupported or Handler
        sizeof(CSPROPERTY_CAMERACONTROL_S),     // MinProperty
        sizeof(CSPROPERTY_CAMERACONTROL_S),     // MinData
        FALSE,                                  // SetSupported or Handler
        &IrisValues,                            // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        sizeof(ULONG)                           // SerializedSize
    ),
    DEFINE_CSPROPERTY_ITEM
    (
        CSPROPERTY_CAMERACONTROL_EXPOSURE,
        FALSE,                                  // GetSupported or Handler
        sizeof(CSPROPERTY_CAMERACONTROL_S),     // MinProperty
        sizeof(CSPROPERTY_CAMERACONTROL_S),     // MinData
        FALSE,                                  // SetSupported or Handler
        &ExposureValues,                        // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        sizeof(ULONG)                           // SerializedSize
    ),
    DEFINE_CSPROPERTY_ITEM
    (
        CSPROPERTY_CAMERACONTROL_FOCUS,
        FALSE,                                  // GetSupported or Handler
        sizeof(CSPROPERTY_CAMERACONTROL_S),     // MinProperty
        sizeof(CSPROPERTY_CAMERACONTROL_S),     // MinData
        FALSE,                                  // SetSupported or Handler
        &FocusValues,                           // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        sizeof(ULONG)                           // SerializedSize
    ),
    DEFINE_CSPROPERTY_ITEM
    (
        CSPROPERTY_CAMERACONTROL_FLASH,
        FALSE,                                  // GetSupported or Handler
        sizeof(CSPROPERTY_CAMERACONTROL_S),     // MinProperty
        sizeof(CSPROPERTY_CAMERACONTROL_S),     // MinData
        FALSE,                                  // SetSupported or Handler
        &FlashValues,                           // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        sizeof(ULONG)                           // SerializedSize
    )
};


// ------------------------------------------------------------------------
// Array of all of the property sets supported by the adapter
// ------------------------------------------------------------------------

DEFINE_CSPROPERTY_SET_TABLE(AdapterPropertyTable)
{
    DEFINE_CSPROPERTY_SET
    ( 
        &PROPSETID_VIDCAP_VIDEOPROCAMP,
        _countof(VideoProcAmpProperties),
        VideoProcAmpProperties,
        0, 
        NULL
    ),

    DEFINE_CSPROPERTY_SET
    ( 
        &PROPSETID_VIDCAP_CAMERACONTROL,
        _countof(CameraControlProperties),
        CameraControlProperties,
        0, 
        NULL
    )
};

#define NUMBER_OF_ADAPTER_PROPERTY_SETS (_countof (AdapterPropertyTable))

// ----------------------------------------------------------------------------
// Default Video Control Caps 
// ----------------------------------------------------------------------------
ULONG DefaultVideoControlCaps[] = {
    0x0                               /*CAPTURE*/,
    0x0                              /*STILL*/,
    0x0                              /*PREVIEW*/
    };

// ----------------------------------------------------------------------------
// Metadata
// ----------------------------------------------------------------------------
const CS_PROPERTYITEM Metadata[] =
{
    {CS_PROPID_MAKE, 32, CS_PROPITEM_TYPE_ASCII, (ULONG)"Texas Instruments"},
    {CS_PROPID_MODEL, 33, CS_PROPITEM_TYPE_ASCII, (ULONG)"TVP5146"},
    {CS_PROPID_MAKER_NOTE, 36, CS_PROPITEM_TYPE_UNDEFINED, (ULONG)"Video Input"},
    {CS_PROPID_SENSING_METHOD, sizeof(SHORT), CS_PROPITEM_TYPE_SHORT, (ULONG)"\x01\x00"},
    {CS_PROPID_CFA_PATTERN, 37, CS_PROPITEM_TYPE_UNDEFINED, (ULONG)"Placeholder for 'CFAPattern' tag data"}
};

#define NUMBER_OF_PROPERTYITEMS (_countof (Metadata))

#endif