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
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES.
//
// -----------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Module Name:    rotate.cpp
//
// -----------------------------------------------------------------------------


#include "precomp.h"

int LCDDDGPE::GetRotateModeFromReg()
{
    HKEY hKey;
    int nRet = DMDO_0;

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\GDI\\ROTATION"),0,0, &hKey))
    {
        DWORD dwAngle;
        
        DWORD dwType = REG_DWORD;
        DWORD dwSize = sizeof(DWORD);
        if (ERROR_SUCCESS == RegQueryValueEx(hKey,
                                               TEXT("ANGLE"),
                                               NULL,
                                               &dwType,
                                               (LPBYTE)&dwAngle,
                                               &dwSize))
        {
            switch (dwAngle)
            {
            case 90:
                nRet = DMDO_90;
                break;
            case 180:
                nRet = DMDO_180;
                break;
            case 270:
                nRet = DMDO_270;
                break;
            case 0:
                // fall through
            default:
                nRet = DMDO_0;
                break;
            }
        }

        RegCloseKey(hKey);
    }

    return nRet;
}

void LCDDDGPE::SetRotateParams()
{
    int iswap;

    switch(m_iRotate)
    {
    case DMDO_0:
        m_nScreenHeightSave = m_nScreenHeight;
        m_nScreenWidthSave  = m_nScreenWidth;
        break;

    case DMDO_180:
        m_nScreenHeightSave = m_nScreenHeight;
        m_nScreenWidthSave  = m_nScreenWidth;
        break;

    case DMDO_90:
    case DMDO_270:
        iswap               = m_nScreenHeight;
        m_nScreenHeight     = m_nScreenWidth;
        m_nScreenWidth      = iswap;
        m_nScreenHeightSave = m_nScreenWidth;
        m_nScreenWidthSave  = m_nScreenHeight;
        break;

    default:
        m_nScreenHeightSave = m_nScreenHeight;
        m_nScreenWidthSave  = m_nScreenWidth;
        break;
    }

    return;
}


LONG LCDDDGPE::DynRotate(int angle)
{
    GPESurf * pSurf = (GPESurf *)m_pPrimarySurface;

    if (angle == m_iRotate)
    {
        return DISP_CHANGE_SUCCESSFUL;
    }

    DisableCursor();

    m_iRotate = angle;

    switch(m_iRotate)
    {
    case DMDO_0:
    case DMDO_180:
        m_nScreenHeight = m_nScreenHeightSave;
        m_nScreenWidth  = m_nScreenWidthSave;
        break;

    case DMDO_90:
    case DMDO_270:
        m_nScreenHeight = m_nScreenWidthSave;
        m_nScreenWidth  = m_nScreenHeightSave;
        break;
    }

    m_pMode->width  = m_nScreenWidth;
    m_pMode->height = m_nScreenHeight;

    pSurf->SetRotation(m_nScreenWidth, m_nScreenHeight, angle);

    EnableCursor();

    return DISP_CHANGE_SUCCESSFUL;
}