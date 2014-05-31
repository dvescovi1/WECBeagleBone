// All rights reserved ADENEO EMBEDDED 2010
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this source code is subject to the terms of the Microsoft end-user
// license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
// If you did not accept the terms of the EULA, you are not authorized to use
// this source code. For a copy of the EULA, please see the LICENSE.RTF on your
// install media.
//
//------------------------------------------------------------------------------
//
//  File:  backlight_class.h
//
#ifndef __BACKLIGHT_CLASS_H
#define __BACKLIGHT_CLASS_H


#ifndef SHIP_BUILD
#undef ZONE_ERROR
#undef ZONE_WARN
#undef ZONE_FUNCTION
#undef ZONE_INFO

#define ZONE_ERROR          DEBUGZONE(0)
#define ZONE_WARN           DEBUGZONE(1)
#define ZONE_FUNCTION       DEBUGZONE(2)
#define ZONE_INFO           DEBUGZONE(3)
#endif



//------------------------------------------------------------------------------
//  CBacklight class
//
class CBacklightRoot
{
// public methods
//---------------
//
public:

    virtual BOOL Initialize(LPCTSTR szContext)              {UNREFERENCED_PARAMETER(szContext); return TRUE;}
    virtual BOOL Uninitialize()                             {return TRUE;}
    virtual BOOL SetPowerState(CEDEVICE_POWER_STATE power)  {UNREFERENCED_PARAMETER(power); return TRUE;}
    virtual UCHAR BacklightGetSupportedStates( )            { return 0;}
};

extern CBacklightRoot* GetBacklightObject();

#endif //__BACKLIGHT_H
