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
//  Module Name:    misc.cpp
//
// -----------------------------------------------------------------------------


#include "precomp.h"

//#include <gxinfo.h>

#if !defined(ESC_SUCCESS)
#define ESC_SUCCESS             (1)
#define ESC_FAILED              (-1)
#define ESC_NOTIMPLEMENTED      (0)
#endif
ULONG
LCDDDGPE::DrvEscape(
    SURFOBJ * pso,
    ULONG     iEsc,
    ULONG     cjIn,
    void    * pvIn,
    ULONG     cjOut,
    void    * pvOut
    )
{
    ULONG rc = 0;
    POWER_CAPABILITIES *pPowerCaps;
    CEDEVICE_POWER_STATE dx;

    //DEBUGMSG(1, (L"LCDDDGPE::DrvEsc: 0x%x\r\n", iEsc));
    
    switch(iEsc) {
        case QUERYESCSUPPORT:
    {
        if (*(DWORD*)pvIn == *(DWORD*)pvIn == IOCTL_POWER_CAPABILITIES
            || *(DWORD*)pvIn == IOCTL_POWER_QUERY
            || *(DWORD*)pvIn == IOCTL_POWER_SET
            || *(DWORD*)pvIn == IOCTL_POWER_GET     
            || *(DWORD*)pvIn == DRVESC_GETSCREENROTATION
            || *(DWORD*)pvIn == DRVESC_SETSCREENROTATION)
        {
            // The escape is supported.
                rc = ESC_SUCCESS;
        }
        else
        {
            // The escape isn't supported.
                rc = ESC_NOTIMPLEMENTED;
        }
    }
        break;
        case DRVESC_GETSCREENROTATION:
        *(int *)pvOut = ((DMDO_0 | DMDO_90 | DMDO_180 | DMDO_270) << 8) | ((BYTE)m_iRotate);
        return DISP_CHANGE_SUCCESSFUL;

        case DRVESC_SETSCREENROTATION:
        if ((cjIn == DMDO_0)   ||
            (cjIn == DMDO_90)  ||
            (cjIn == DMDO_180) ||
            (cjIn == DMDO_270) )
            {
                return DynRotate(cjIn);
            }

        return DISP_CHANGE_BADMODE;

#if 0
        case SETPOWERMANAGEMENT:
            DEBUGMSG(GPE_ZONE_PM, (L"LCDDDGPE::DrvEsc: SETPOWERMANAGEMENT\r\n"));
            if (pvIn == NULL || cjIn < sizeof(VIDEO_POWER_MANAGEMENT)) {
                SetLastError(ERROR_INVALID_PARAMETER);
                rc = (ULONG)ESC_FAILED;
                break;

            pvpm = (VIDEO_POWER_MANAGEMENT *)pvIn;
            if (pvpm->Length < sizeof(VIDEO_POWER_MANAGEMENT)) {
                SetLastError(ERROR_INVALID_PARAMETER);
                rc = (ULONG)ESC_FAILED;
                break;
            }
            DEBUGMSG(GPE_ZONE_PM, (L"LCDDDGPE::DrvEsc: SETPOWERMANAGEMENT: State %d\r\n",
                           pvpm->PowerState));
            rc = ESC_SUCCESS;
            switch (pvpm->PowerState)
            {
            case VideoPowerOn:       SetPower(D0); break;
            case VideoPowerOff:      SetPower(D4); break;
            default:                 rc = (ULONG)ESC_FAILED;   break;
            }
            break;
   
        case GETPOWERMANAGEMENT:
            DEBUGMSG(GPE_ZONE_PM, (L"LCDDDGPE::DrvEsc: GETPOWERMANAGEMENT\r\n"));
            if (pvOut == NULL || cjOut < sizeof(VIDEO_POWER_MANAGEMENT)) {
                SetLastError(ERROR_INVALID_PARAMETER);
                rc = (ULONG)ESC_FAILED;
                break;
            }
            pvpm = (VIDEO_POWER_MANAGEMENT*)pvOut;
            pvpm->Length = sizeof(VIDEO_POWER_MANAGEMENT);
            pvpm->DPMSVersion = 0;
            switch (m_externalPowerState) {
                case D0:    pvpm->PowerState = VideoPowerOn;      break;
                case D1:    pvpm->PowerState = VideoPowerOn;      break;
                case D2:    pvpm->PowerState = VideoPowerStandBy; break;
                case D3:    pvpm->PowerState = VideoPowerSuspend; break;
                case D4:    pvpm->PowerState = VideoPowerOff;     break;
            }
            rc = ESC_SUCCESS;
            break;
#endif
        case IOCTL_POWER_CAPABILITIES:
            DEBUGMSG(1, (L"LCDDDGPE::DrvEsc: IOCTL_POWER_CAPABILITIES\r\n"));
            if (pvOut == NULL || cjOut != sizeof(POWER_CAPABILITIES)) {
                SetLastError(ERROR_INVALID_PARAMETER);
                rc = (ULONG)ESC_FAILED;
                break;
            }
            pPowerCaps = (POWER_CAPABILITIES*)pvOut;
            memset(pPowerCaps, 0, sizeof(POWER_CAPABILITIES));
            pPowerCaps->DeviceDx = DX_MASK(D0) | DX_MASK(D4);
            rc = ESC_SUCCESS;
            break;
        case IOCTL_POWER_QUERY:
            DEBUGMSG(1, (L"LCDDDGPE::DrvEsc: IOCTL_POWER_QUERY\r\n"));
            if (pvOut == NULL || cjOut != sizeof(CEDEVICE_POWER_STATE)) {
                SetLastError(ERROR_INVALID_PARAMETER);
                rc = (ULONG)ESC_FAILED;
                break;
            }
            dx = *(CEDEVICE_POWER_STATE*)pvOut;
            rc = VALID_DX(dx) ? ESC_SUCCESS : ESC_FAILED;
            break;
        case IOCTL_POWER_SET:
            DEBUGMSG(1, (L"LCDDDGPE::DrvEsc: IOCTL_POWER_SET %d\r\n",
                           *(CEDEVICE_POWER_STATE*)pvOut));
            if (pvOut == NULL || cjOut != sizeof(CEDEVICE_POWER_STATE)) {
                SetLastError(ERROR_INVALID_PARAMETER);
                rc = (ULONG)ESC_FAILED;
                DEBUGMSG(1, (L"***INVALID POWER SET!\r\n"));                
                break;
            }
            dx = *(CEDEVICE_POWER_STATE*)pvOut;
            SetPower(dx);
            *(CEDEVICE_POWER_STATE*)pvOut = m_currentDX;
            rc = ESC_SUCCESS;
            break;
        case IOCTL_POWER_GET:
            DEBUGMSG(1, (L"LCDDDGPE::DrvEsc: IOCTL_POWER_GET\r\n"));
            if (pvOut == NULL || cjOut != sizeof(CEDEVICE_POWER_STATE)) {
                SetLastError(ERROR_INVALID_PARAMETER);
                rc = (ULONG)ESC_FAILED;
                break;
            }
            *(CEDEVICE_POWER_STATE*)pvOut = m_currentDX;
            rc = ESC_SUCCESS;
            break;
    }
    return rc;
}

void LCDDDGPE::WaitForNotBusy(void)
{
    DEBUGMSG (GPE_ZONE_INIT, (TEXT("LCDDDGPE::WaitForNotBusy\r\n")));
    WaitForSingleObject( m_VSyncFlipFinished, WAIT_FOR_FLIP_TIMEOUT ); 
    return;
}

void LCDDDGPE::WaitForVBlank(void)
{
    DEBUGMSG (GPE_ZONE_INIT, (TEXT("LCDDDGPE::WaitForVBlank\r\n")));
    WaitForSingleObject( m_VSyncEvent, WAIT_FOR_VSYNC_TIMEOUT );
    return;
}

int LCDDDGPE::IsBusy(void)
{
    DEBUGMSG (GPE_ZONE_INIT, (TEXT("LCDDDGPE::IsBusy\r\n")));
    return    0;
}

INT LCDDDGPE::InVBlank(void)
{
    static  BOOL  value = FALSE;
    DEBUGMSG (GPE_ZONE_INIT, (TEXT("LCDDDGPE::InVBlank\r\n")));
    value = !value;
    return value;
}

SCODE LCDDDGPE::SetPalette(const PALETTEENTRY *source, USHORT firstEntry, USHORT numEntries)
{
    DEBUGMSG (GPE_ZONE_INIT, (TEXT("LCDDDGPE::SetPalette\r\n")));

    if (firstEntry < 0 || firstEntry + numEntries > 256 || source == NULL)
    {
        return E_INVALIDARG;
    }

    /*for(; numEntries; numEntries--)
    {
        char red, green, blue, index;

        index = (unsigned char)(firstEntry++);
        red = source->peRed >> 2;
        green = source->peGreen >> 2;
        blue = source->peBlue >> 2;

        // Store next entry
        //...

        source++;
     }


     // Apply new palette
     // ...

     */

    return S_OK;
}

// this routine converts a string into a GUID and returns TRUE if the
// conversion was successful.
BOOL
   ConvertStringToGuid (LPCTSTR pszGuid, GUID *pGuid)
{
    UINT Data4[8];
    int  Count;
    BOOL fOk = FALSE;
    TCHAR *pszGuidFormat = _T("{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}");

    DEBUGCHK(pGuid != NULL && pszGuid != NULL);
    __try
    {
        if (_stscanf(pszGuid, pszGuidFormat, &pGuid->Data1,
                     &pGuid->Data2, &pGuid->Data3, &Data4[0], &Data4[1], &Data4[2], &Data4[3],
                     &Data4[4], &Data4[5], &Data4[6], &Data4[7]) == 11)
        {
            for(Count = 0; Count < (sizeof(Data4) / sizeof(Data4[0])); Count++)
            {
                pGuid->Data4[Count] = (UCHAR) Data4[Count];
            }
        }
        fOk = TRUE;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    return fOk;
}

// This routine notifies the OS that we support the Power Manager IOCTLs (through
// ExtEscape(), which calls DrvEscape()).
BOOL LCDDDGPE::AdvertisePowerInterface(void)
{
    BOOL fOk = FALSE;
    GUID gClass;
    TCHAR szModuleFileName[MAX_PATH];

    // assume we are advertising the default class
    ConvertStringToGuid(PMCLASS_DISPLAY, &gClass);
    DEBUGMSG(1,(TEXT("LCDDDGPE::AdvertisePowerInterface: (%s)\r\n"), PMCLASS_DISPLAY));
    DEBUGMSG(1,(TEXT("LCDDDGPE::AdvertisePowerInterface: 0x%x-0x%x-0x%x 0x%x-0x%x-0x%x-0x%x-0x%x-0x%x-0x%x-0x%x\r\n"),
                  gClass.Data1, gClass.Data2, gClass.Data3,
                  gClass.Data4[0], gClass.Data4[1], gClass.Data4[2], gClass.Data4[3],
                  gClass.Data4[4], gClass.Data4[5], gClass.Data4[6], gClass.Data4[7]));

    // Figure out what device name to advertise
    // Note - g_hmodDisplayDll is initialized in the DLL_PROCESS_ATTACH of DllMain()
    fOk = GetModuleFileName(g_hmodDisplayDll, szModuleFileName, sizeof(szModuleFileName) / sizeof(szModuleFileName[0]));
    if (!fOk) {
        RETAILMSG(1,(TEXT("LCDDDGPE::AdvertisePowerInterface: Failed to obtain DLL name. Driver is not power managed!\r\n")));
        return FALSE;
    }
    
    // now advertise the interface
    fOk = AdvertiseInterface(&gClass, szModuleFileName, TRUE);
    
    return fOk; 
}
