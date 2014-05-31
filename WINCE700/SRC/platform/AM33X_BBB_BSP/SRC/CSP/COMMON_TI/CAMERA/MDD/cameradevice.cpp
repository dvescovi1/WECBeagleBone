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

#include <windows.h>
#include <pm.h>
#include "Cs.h"
#include "Csmedia.h"

#include "CameraPDDProps.h"
#include "dstruct.h"
#include "CameraDebug.h"
#include "CameraDriver.h"

#include "camera.h"
#include "CameraPinDriver.h"

#include "CameraPDD.h"


DWORD MDD_HandleIO(LPVOID ModeContext, ULONG /*ulModeType*/)
{
    PPINDEVICE pPinDevice = (PPINDEVICE)ModeContext;
    if (NULL == pPinDevice)
    {
        return ERROR_INVALID_PARAMETER;
    }
    return pPinDevice->HandlePinIO();
}

DWORD MDD_HandleNotification(LPVOID MDDContext, LONG lModeType, ULONG NotificationId, LPVOID Context)
{
    PCAMERADEVICE pCamDevice = (PCAMERADEVICE)MDDContext;
    if (NULL == pCamDevice)
    {
        return ERROR_INVALID_PARAMETER;
    }
    return pCamDevice->HandleNotification(lModeType, NotificationId, Context);
}

CCameraDevice::CCameraDevice()
{
    m_hStream        = NULL;
    m_hCallerProcess = NULL;

    m_dwVersion = 0;
    m_PowerState = D0;
    m_PDDContext = NULL;
    m_pPDDFuncTbl = NULL;
    memset(&m_PDDFuncTbl2, 0, sizeof(PDDFUNCTBL2));
    memset(&m_AdapterInfo, 0, sizeof(ADAPTERINFO));
    InitializeCriticalSection(&m_csDevice);
    m_pStrmInstances = NULL;
    m_hAdapterMessageQueue = NULL;
    m_dwRefCount = 0;
}


CCameraDevice::~CCameraDevice()
{
    DeleteCriticalSection(&m_csDevice);

    if (m_pPDDFuncTbl)
    {
        m_pPDDFuncTbl->PDD_DeInit(m_PDDContext);
    }

    if (m_hAdapterMessageQueue)
    {
        CloseMsgQueue(m_hAdapterMessageQueue);
    }

    if (NULL != m_pStrmInstances)
    {
        delete [] m_pStrmInstances;
        m_pStrmInstances = NULL;
    }

    if (NULL != m_hStream)
    {
        DeactivateDevice(m_hStream);
        m_hStream = NULL;
    }
}


bool
CCameraDevice::Initialize(PVOID context)
{
    DWORD dwRet = ERROR_SUCCESS;
    HKEY hKey;

    if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, (LPWSTR)context, 0, 0, &hKey))
    {
        return false;
    }

    DWORD dwType;
    WCHAR szDeviceName[MAX_PINNAME_LENGTH];
    DWORD dwSize = sizeof(szDeviceName);

    // Get the device name
    if (ERROR_SUCCESS != RegQueryValueEx(hKey, L"Name", 0, &dwType, (BYTE *)&szDeviceName, &dwSize))
    {
        RegCloseKey(hKey);
        return false;
    }

    WCHAR szDriverRegPath[MAX_PATH];
    dwSize = sizeof(szDriverRegPath);

    if (ERROR_SUCCESS != RegQueryValueEx(hKey, L"Key", 0, &dwType, (BYTE *)szDriverRegPath, &dwSize))
    {
        RegCloseKey(hKey);
        return false;
    }

    RegCloseKey(hKey);

    if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, szDriverRegPath, 0, 0, &hKey))
    {
        return false;
    }

    WCHAR szDriverName[MAX_PATH];
    dwSize = sizeof(szDriverName);

    // Get the driver module name
    if (ERROR_SUCCESS != RegQueryValueEx(hKey, L"Dll", 0, &dwType, (BYTE *)szDriverName, &dwSize))
    {
        RegCloseKey(hKey);
        return false;
    }

    RegCloseKey(hKey);

    WCHAR chPinInstance = szDeviceName[3];

    for (int i = 0; i < MAX_SUPPORTED_PINS; ++i)
    {
        g_wszPinDeviceNames[i][3] = chPinInstance;
    }

    WCHAR szPinRegPath[MAX_PATH] = PIN_REG_PATH;
    size_t cchPinRegPath;

    if (FAILED(StringCchLength(szPinRegPath, MAX_PATH, &cchPinRegPath)) ||
        FAILED(StringCchCatN(szPinRegPath, MAX_PATH, L"\\", MAX_PATH - cchPinRegPath - 1)) ||
        FAILED(StringCchLength(szPinRegPath, MAX_PATH, &cchPinRegPath)) ||
        FAILED(StringCchCatN(szPinRegPath, MAX_PATH, g_wszPinDeviceNames[0], MAX_PATH - cchPinRegPath - 1)) ||
        FAILED(StringCchLength(szPinRegPath, MAX_PATH, &cchPinRegPath)))
    {
        return false;
    }

    // Drop the colon from the path name
    szPinRegPath[cchPinRegPath - 1] = L'\0';

    DWORD dwDisposition;

    // Now try to create the key for the PIN driver
    if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_LOCAL_MACHINE, szPinRegPath, 0, NULL,
                                        REG_OPTION_NON_VOLATILE, 0, NULL, &hKey, &dwDisposition))
    {
        return false;
    }

    // If the key already exists then we are good to go. Otherwise, we will create the values on the fly.
    if (REG_CREATED_NEW_KEY == dwDisposition)
    {
        BYTE *pData = (BYTE *)L"PIN";
        RegSetValueEx(hKey, L"Prefix", 0, REG_SZ, pData, (wcslen((WCHAR *)pData) + 1) * sizeof(WCHAR));

        RegSetValueEx(hKey, L"Dll", 0, REG_SZ, (BYTE *)szDriverName, (wcslen(szDriverName) + 1) * sizeof(WCHAR));

        DWORD dwPinInstance = chPinInstance - 0x0030;
        RegSetValueEx(hKey, L"Index", 0, REG_DWORD, (BYTE *)&dwPinInstance, sizeof(DWORD));

        pData = (BYTE *)L"{C9D092D6-827A-45E2-8144-DE1982BFC3A8}";
        RegSetValueEx(hKey, L"IClass", 0, REG_SZ, pData, (wcslen((WCHAR *)pData) + 1) * sizeof(WCHAR));
    }

    RegCloseKey(hKey);

    m_hStream = ActivateDeviceEx(szPinRegPath, NULL, 0, reinterpret_cast<LPVOID>(this));
    if (NULL == m_hStream)
    {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T("CAM_Init: ActivateDevice on Pin failed\r\n")));
        return false;
    }

    m_PDDFuncTbl2.PDDfuncTble1.dwSize = sizeof(PDDFUNCTBL2);
    m_PDDContext = PDD_Init(context, (PPDDFUNCTBL)&m_PDDFuncTbl2);
    if (m_PDDContext == NULL)
    {
        return false;
    }

    m_pPDDFuncTbl = (PPDDFUNCTBL)&m_PDDFuncTbl2;
    if (m_pPDDFuncTbl->dwSize < sizeof(PDDFUNCTBL) ||
        NULL == m_pPDDFuncTbl->PDD_Init ||
        NULL == m_pPDDFuncTbl->PDD_DeInit ||
        NULL == m_pPDDFuncTbl->PDD_GetAdapterInfo ||
        NULL == m_pPDDFuncTbl->PDD_HandleVidProcAmpChanges ||
        NULL == m_pPDDFuncTbl->PDD_HandleCamControlChanges ||
        NULL == m_pPDDFuncTbl->PDD_HandleVideoControlCapsChanges ||
        NULL == m_pPDDFuncTbl->PDD_SetPowerState ||
        NULL == m_pPDDFuncTbl->PDD_HandleAdapterCustomProperties ||
        NULL == m_pPDDFuncTbl->PDD_InitSensorMode ||
        NULL == m_pPDDFuncTbl->PDD_DeInitSensorMode ||
        NULL == m_pPDDFuncTbl->PDD_SetSensorState ||
        NULL == m_pPDDFuncTbl->PDD_TakeStillPicture ||
        NULL == m_pPDDFuncTbl->PDD_GetSensorModeInfo ||
        NULL == m_pPDDFuncTbl->PDD_SetSensorModeFormat ||
        NULL == m_pPDDFuncTbl->PDD_FillBuffer ||
        NULL == m_pPDDFuncTbl->PDD_HandleModeCustomProperties)
    {
        return false;
    }

    if (m_pPDDFuncTbl->dwSize >= sizeof(PDDFUNCTBL2))
    {
        if (NULL == m_PDDFuncTbl2.PDD_Open ||
            NULL == m_PDDFuncTbl2.PDD_Close ||
            NULL == m_PDDFuncTbl2.PDD_GetMetadata)
        {
            return false;
        }
    }

    dwRet = m_pPDDFuncTbl->PDD_GetAdapterInfo(m_PDDContext, &m_AdapterInfo);
    if (ERROR_SUCCESS != dwRet || DRIVER_VERSION > m_AdapterInfo.ulVersionID || DRIVER_VERSION_2 < m_AdapterInfo.ulVersionID)
    {
        return false;
    }

    m_dwVersion = m_AdapterInfo.ulVersionID;

    m_pStrmInstances = new STREAM_INSTANCES[m_AdapterInfo.ulCTypes];
    if (NULL == m_pStrmInstances)
    {
        return false;
    }

    memset(m_pStrmInstances, 0x0, sizeof(STREAM_INSTANCES) * m_AdapterInfo.ulCTypes);

    if (false == GetPDDPinInfo())
    {
        delete m_pStrmInstances;
        m_pStrmInstances = NULL;
        DEBUGMSG(ZONE_IOCTL|ZONE_ERROR, (_T("CAM_IOControl(%08x): Failed to retrieve sub-device information\r\n"), this)) ;
        return false;
    }

    return true;
}

bool
CCameraDevice::GetPDDPinInfo()
{
    SENSORMODEINFO SensorModeInfo;
    if (NULL == m_pStrmInstances)
    {
        return false;
    }

    for (UINT i=0; i < m_AdapterInfo.ulCTypes; i++)
    {
        if (ERROR_SUCCESS != PDDGetPinInfo(i, &SensorModeInfo))
        {
            return false ;
        }
        m_pStrmInstances[i].ulPossibleCount = SensorModeInfo.PossibleCount ;
        m_pStrmInstances[i].VideoCaps.DefaultVideoControlCaps = SensorModeInfo.VideoCaps.DefaultVideoControlCaps;
        m_pStrmInstances[i].VideoCaps.CurrentVideoControlCaps = SensorModeInfo.VideoCaps.CurrentVideoControlCaps;
        m_pStrmInstances[i].pVideoFormat = SensorModeInfo.pVideoFormat;

        if (SensorModeInfo.MemoryModel == CSPROPERTY_BUFFER_DRIVER &&
            (NULL == m_pPDDFuncTbl->PDD_AllocateBuffer || NULL == m_pPDDFuncTbl->PDD_DeAllocateBuffer))
        {
            return false;
        }

        if (SensorModeInfo.MemoryModel != CSPROPERTY_BUFFER_DRIVER &&
            (NULL == m_pPDDFuncTbl->PDD_RegisterClientBuffer || NULL == m_pPDDFuncTbl->PDD_UnRegisterClientBuffer))
        {
            return false;
        }
    }

    return true;
}

bool
CCameraDevice::BindApplicationProc(HANDLE hCurrentProc)
{
    // We only allow one user-mode instance
    if ((HANDLE)GetDirectCallerProcessId() != hCurrentProc)
    {
        if (NULL != m_hCallerProcess)
        {
            return false;
        }

        m_hCallerProcess = hCurrentProc;

        if (m_hAdapterMessageQueue)
        {
            CloseMsgQueue(m_hAdapterMessageQueue);
            m_hAdapterMessageQueue = NULL;
        }
    }

    if (0 == m_dwRefCount)
    {
        if (m_pPDDFuncTbl->dwSize >= sizeof(PDDFUNCTBL2))
        {
            if (ERROR_SUCCESS != m_PDDFuncTbl2.PDD_Open(m_PDDContext, (LPVOID)this))
            {
                m_hCallerProcess = NULL;
                return false;
            }
        }
    }

    ++m_dwRefCount;

    return true;
}


bool
CCameraDevice::UnBindApplicationProc()
{
    DEBUGMSG(ZONE_FUNCTION, (_T("CAM_Close: Unbind application from camera device\r\n")));

    if ((HANDLE)GetCallerVMProcessId() == m_hCallerProcess)
    {
        m_hCallerProcess = NULL;

        if (m_hAdapterMessageQueue)
        {
            CloseMsgQueue(m_hAdapterMessageQueue);
            m_hAdapterMessageQueue = NULL;
        }

        if (NULL != m_pStrmInstances)
        {
            for (ULONG ulPinId = 0; IsValidPin(ulPinId); ++ulPinId)
            {
                if (NULL != m_pStrmInstances[ulPinId].pPinDev)
                {
                    m_pStrmInstances[ulPinId].pPinDev->SetState(CSSTATE_STOP, NULL);
                    m_pStrmInstances[ulPinId].pPinDev->ReleaseBuffers();
                }
            }
        }
    }

    ASSERT(0 < m_dwRefCount);

    if (0 < m_dwRefCount)
    {
        --m_dwRefCount;

        if (0 == m_dwRefCount)
        {
            if (m_pPDDFuncTbl->dwSize >= sizeof(PDDFUNCTBL2))
            {
                if (ERROR_SUCCESS != m_PDDFuncTbl2.PDD_Close(m_PDDContext, (LPVOID)this))
                {
                    return false;
                }
            }
        }
    }

    return true;
}

bool
CCameraDevice::IsValidPin(ULONG ulPinId)
{
    if (ulPinId >= m_AdapterInfo.ulCTypes)
    {
        return false;
    }

    return true;
}

bool
CCameraDevice::IncrCInstances(
    ULONG        ulPinId,
    CPinDevice * pPinDev
   )
{
    bool bRet = false;

    if (IsValidPin(ulPinId))
    {
        PSTREAM_INSTANCES pStreamInst = &m_pStrmInstances[ulPinId];
        if (pStreamInst->ulCInstances < pStreamInst->ulPossibleCount)
        {
            pStreamInst->ulCInstances++;
// TODO This is memory leak
            pStreamInst->pPinDev = pPinDev;
            bRet = true;
        }

    }

    return bRet;
}

bool
CCameraDevice::DecrCInstances(
    ULONG ulPinId
   )
{
    bool bRet = false;

    if (IsValidPin(ulPinId))
    {
        PSTREAM_INSTANCES pStreamInst = &m_pStrmInstances[ulPinId];
        if (0 < pStreamInst->ulCInstances)
        {
            pStreamInst->ulCInstances--;
// TODO This is memory leak
            pStreamInst->pPinDev = NULL;
            bRet = true;
        }
    }
    return bRet;
}

bool
CCameraDevice::PauseCaptureAndPreview(void)
{
    if (PREVIEW < m_AdapterInfo.ulCTypes)
    {
        if (NULL != m_pStrmInstances[PREVIEW].pPinDev)
        {
            m_pStrmInstances[PREVIEW].pPinDev->SetState(CSSTATE_PAUSE, &m_pStrmInstances[PREVIEW].CsPrevState);
        }
    }

    if (NULL != m_pStrmInstances[CAPTURE].pPinDev)
    {
        m_pStrmInstances[CAPTURE].pPinDev->SetState(CSSTATE_PAUSE, &m_pStrmInstances[CAPTURE].CsPrevState);
    }

    return true;
}

bool
CCameraDevice::RevertCaptureAndPreviewState(void)
{
    if (PREVIEW < m_AdapterInfo.ulCTypes)
    {
        m_pStrmInstances[PREVIEW].pPinDev->SetState(m_pStrmInstances[PREVIEW].CsPrevState, NULL);
    }

    m_pStrmInstances[CAPTURE].pPinDev->SetState(m_pStrmInstances[CAPTURE].CsPrevState, NULL);

    return true;
}


bool
CCameraDevice::GetPinFormat(
    ULONG                 ulPinId,
    ULONG                 ulIndex,
    PCS_DATARANGE_VIDEO * ppCsDataRangeVid
   )
{
    if (false == IsValidPin(ulPinId))
    {
        return false;
    }

    if (0 >= ulIndex || ulIndex > m_pStrmInstances[ulPinId].pVideoFormat->ulAvailFormats)
    {
        return false;
    }

    *ppCsDataRangeVid = m_pStrmInstances[ulPinId].pVideoFormat->pCsDataRangeVideo[ulIndex-1];

    return true;
}


bool
CCameraDevice::AdapterCompareFormat(
    ULONG                       ulPinId,
    const PCS_DATARANGE_VIDEO   pCsDataRangeVideoToCompare,
    PCS_DATARANGE_VIDEO       * ppCsDataRangeVideoMatched,
    bool                        fDetailedComparison
   )
{
    for (ULONG ulCount = 0 ; ulCount < m_pStrmInstances[ulPinId].pVideoFormat->ulAvailFormats ; ulCount++)
    {
        PCS_DATARANGE_VIDEO pCsDataRangeVideo = m_pStrmInstances[ulPinId].pVideoFormat->pCsDataRangeVideo[ulCount];

        if (false == AdapterCompareGUIDsAndFormatSize(reinterpret_cast<PCSDATARANGE>(pCsDataRangeVideo),
                                                        reinterpret_cast<PCSDATARANGE>(pCsDataRangeVideoToCompare)))
        {
            continue;
        }

        if (true == fDetailedComparison)
        {
            if ((pCsDataRangeVideoToCompare->bFixedSizeSamples != pCsDataRangeVideo->bFixedSizeSamples)
                  || (pCsDataRangeVideoToCompare->bTemporalCompression   != pCsDataRangeVideo->bTemporalCompression)
                  || (pCsDataRangeVideoToCompare->StreamDescriptionFlags != pCsDataRangeVideo->StreamDescriptionFlags)
                  || (pCsDataRangeVideoToCompare->MemoryAllocationFlags  != pCsDataRangeVideo->MemoryAllocationFlags))
            {
                continue;
            }
            if (pCsDataRangeVideoToCompare->VideoInfoHeader.AvgTimePerFrame < pCsDataRangeVideo->ConfigCaps.MinFrameInterval  ||
                pCsDataRangeVideoToCompare->VideoInfoHeader.AvgTimePerFrame > pCsDataRangeVideo->ConfigCaps.MaxFrameInterval )
            {
                continue;
            }

            if (0 != memcmp(&pCsDataRangeVideoToCompare->VideoInfoHeader.bmiHeader, &pCsDataRangeVideo->VideoInfoHeader.bmiHeader, sizeof(pCsDataRangeVideo->VideoInfoHeader.bmiHeader)))
            {
                continue;
            }

        }

        // You can now perform more granular comparison involving ConfigCaps and VIDOINFOHEADER etc.

        /////////////////////////////////////////
        if (NULL != ppCsDataRangeVideoMatched)
        {
            *ppCsDataRangeVideoMatched = pCsDataRangeVideo;
        }

        return true;
    }

    return false;
}


bool
CCameraDevice::AdapterCompareFormat(
    ULONG                                  ulPinId,
    const PCS_DATAFORMAT_VIDEOINFOHEADER   pCsDataVIHToCompare,
    PCS_DATARANGE_VIDEO                  * ppCsDataRangeVideoMatched,
    bool                                   fDetailedComparison
   )
{
    for (ULONG ulCount = 0 ; ulCount < m_pStrmInstances[ulPinId].pVideoFormat->ulAvailFormats ; ulCount++)
    {
        PCS_DATARANGE_VIDEO pCsDataRangeVideo = m_pStrmInstances[ulPinId].pVideoFormat->pCsDataRangeVideo[ulCount];

        if (false == AdapterCompareGUIDsAndFormatSize(reinterpret_cast<PCSDATARANGE>(pCsDataRangeVideo),
                                                        reinterpret_cast<PCSDATARANGE>(pCsDataVIHToCompare)))
        {
            continue;
        }

        if (true == fDetailedComparison)
        {
            if (pCsDataVIHToCompare->VideoInfoHeader.AvgTimePerFrame < pCsDataRangeVideo->ConfigCaps.MinFrameInterval  ||
                pCsDataVIHToCompare->VideoInfoHeader.AvgTimePerFrame > pCsDataRangeVideo->ConfigCaps.MaxFrameInterval )
            {
                continue;
            }

            if (0 != memcmp(&pCsDataVIHToCompare->VideoInfoHeader.bmiHeader, &pCsDataRangeVideo->VideoInfoHeader.bmiHeader, sizeof(pCsDataRangeVideo->VideoInfoHeader.bmiHeader)))
            {
                continue;
            }
        }

        // You can now perform more granular comparison involving ConfigCaps and VIDOINFOHEADER etc.

        /////////////////////////////////////////
        if (NULL != ppCsDataRangeVideoMatched)
        {
            *ppCsDataRangeVideoMatched = pCsDataRangeVideo;
        }

        return true;
    }

    return false;
}


bool
CCameraDevice::AdapterCompareGUIDsAndFormatSize(
    const PCSDATARANGE DataRange1,
    const PCSDATARANGE DataRange2
   )
{
    return (IsEqualGUID(DataRange1->MajorFormat, DataRange2->MajorFormat)
             && IsEqualGUID(DataRange1->SubFormat, DataRange2->SubFormat)
             && IsEqualGUID(DataRange1->Specifier, DataRange2->Specifier));
}


LPVOID
CCameraDevice::ValidateBuffer(
    LPVOID   lpBuff,
    ULONG    ulActualBufLen,
    ULONG    ulExpectedBuffLen,
    DWORD  * dwError
   )
{
    if (NULL == lpBuff)
    {
        *dwError = ERROR_INSUFFICIENT_BUFFER;

        return NULL;
    }

    if (ulActualBufLen < ulExpectedBuffLen)
    {
        DEBUGMSG(ZONE_IOCTL|ZONE_ERROR, (_T("IOControl(%08x): buffer is not large enough\r\n"), this));
        *dwError = ERROR_INSUFFICIENT_BUFFER;

        return NULL;
    }


    //If the size is good, just return the buffer passed in
    return lpBuff;
}


DWORD
CCameraDevice::AdapterHandleVersion(
    PUCHAR pOutBuf,
    DWORD  OutBufLen,
    PDWORD pdwBytesTransferred
   )
{
    if (OutBufLen < sizeof(DWORD))
    {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    *pdwBytesTransferred = sizeof(DWORD);

    if (!CeSafeCopyMemory(pOutBuf, &m_dwVersion, sizeof(DWORD)))
    {
        return ERROR_INVALID_USER_BUFFER;
    }

    return ERROR_SUCCESS;
}


DWORD
CCameraDevice::AdapterHandlePinRequests(
    PUCHAR pInBuf,
    DWORD  InBufLen,
    PUCHAR pOutBuf,
    DWORD  OutBufLen,
    PDWORD pdwBytesTransferred
   )
{
    DEBUGMSG(ZONE_IOCTL, (_T("CAM_IOControl(%08x): HandlePinRequests\r\n"), this));

    DWORD                          dwError           = ERROR_INVALID_PARAMETER;
    PCSP_PIN                       pCsPin            = NULL;
    PCS_DATARANGE_VIDEO            pCsDataRangeVideo = NULL;
    PCSMULTIPLE_ITEM               pCsMultipleItem   = NULL;
    PCS_DATAFORMAT_VIDEOINFOHEADER pCsDataFormatVih  = NULL;
    PCSPIN_CINSTANCES              pCsPinCinstances  = NULL;

    CSPROPERTY                      csProp = {0};
    LONG                            lPinId = 0;

    if (InBufLen < sizeof(CSPROPERTY) ||
        !CeSafeCopyMemory(&csProp, pInBuf, sizeof(CSPROPERTY)))
    {
        return dwError;
    }

    *pdwBytesTransferred = 0;

    // we support PROPSETID_Pin, so just return success
    if (CSPROPERTY_TYPE_SETSUPPORT == csProp.Flags)
    {
        return ERROR_SUCCESS;
    }

    // we are here so the request is not SETSUPPORT and after this only GET requests will be entertained since
    // PROPSETID_Pin contains READ-ONLY properties
    if (CSPROPERTY_TYPE_GET != csProp.Flags)
    {
        return dwError;
    }

    switch (csProp.Id)
    {
        case CSPROPERTY_PIN_CTYPES:
        {
            DEBUGMSG(ZONE_IOCTL, (_T("CAM_IOControl(%08x): CSPROPERTY_PIN_CTYPES\r\n"), this));

            *pdwBytesTransferred = sizeof(ULONG);

            EnterCriticalSection(&m_csDevice);

            if (OutBufLen < sizeof(ULONG))
            {
                dwError = ERROR_MORE_DATA;
                break;
            }

            if (!CeSafeCopyMemory(pOutBuf, &(m_AdapterInfo.ulCTypes), sizeof(ULONG)))
            {
                dwError = ERROR_INVALID_PARAMETER;
                break;
            }

            LeaveCriticalSection(&m_csDevice);

            dwError = ERROR_SUCCESS;
            break;
        }

        case CSPROPERTY_PIN_CINSTANCES:
        {
            DEBUGMSG(ZONE_IOCTL, (_T("CAM_IOControl(%08x): CSPROPERTY_PIN_CINSTANCES\r\n"), this));

            if (NULL == (pCsPin = reinterpret_cast<PCSP_PIN>(ValidateBuffer(pInBuf, InBufLen, sizeof(CSP_PIN), &dwError))))
            {
                break;
            }

            lPinId = pCsPin->PinId;
            if (false == IsValidPin(lPinId))
            {
                DEBUGMSG(ZONE_IOCTL, (_T("CAM_IOControl(%08x): Invalid PinId\r\n"), this));
                break;
            }

            *pdwBytesTransferred = sizeof(CSPIN_CINSTANCES);

            if (NULL == (pCsPinCinstances = reinterpret_cast<PCSPIN_CINSTANCES>(ValidateBuffer(pOutBuf, OutBufLen, sizeof(CSPIN_CINSTANCES), &dwError))))
            {
                dwError = ERROR_MORE_DATA;
                break;
            }

            pCsPinCinstances->PossibleCount = m_pStrmInstances[lPinId].ulPossibleCount;
            pCsPinCinstances->CurrentCount  = m_pStrmInstances[lPinId].ulCInstances;

            dwError = ERROR_SUCCESS;
            break;
        }

        case CSPROPERTY_PIN_DATARANGES:
        {
            DEBUGMSG(ZONE_IOCTL, (_T("CAM_IOControl(%08x): CSPROPERTY_PIN_DATARANGES\r\n"), this));

            if (NULL == (pCsPin = reinterpret_cast<PCSP_PIN>(ValidateBuffer(pInBuf, InBufLen, sizeof(CSP_PIN), &dwError))))
            {
                break;
            }

            lPinId = pCsPin->PinId;
            if (false == IsValidPin(lPinId))
            {
                DEBUGMSG(ZONE_IOCTL, (_T("CAM_IOControl(%08x): Invalid PinId\r\n"), this));
                break;
            }

            *pdwBytesTransferred = sizeof(CSMULTIPLE_ITEM) + (m_pStrmInstances[lPinId].pVideoFormat->ulAvailFormats * sizeof(CS_DATARANGE_VIDEO));

            if (NULL == (pCsMultipleItem = reinterpret_cast<PCSMULTIPLE_ITEM>(ValidateBuffer(pOutBuf, OutBufLen, *pdwBytesTransferred, &dwError))))
            {
                dwError = ERROR_MORE_DATA ;
                break ;
            }

            pCsMultipleItem->Count = m_pStrmInstances[lPinId].pVideoFormat->ulAvailFormats;
            pCsMultipleItem->Size  = *pdwBytesTransferred;

            pCsDataRangeVideo = NULL;
            pCsDataRangeVideo = reinterpret_cast<PCS_DATARANGE_VIDEO>(pCsMultipleItem + 1);

            for (int iCount = 0 ; iCount < static_cast<int>(m_pStrmInstances[lPinId].pVideoFormat->ulAvailFormats); iCount++)
            {
                memcpy(pCsDataRangeVideo, m_pStrmInstances[lPinId].pVideoFormat->pCsDataRangeVideo[iCount], sizeof(CS_DATARANGE_VIDEO));
                pCsDataRangeVideo = reinterpret_cast<PCS_DATARANGE_VIDEO>(pCsDataRangeVideo + 1);
            }

            dwError = ERROR_SUCCESS;

            break;
        }

        case CSPROPERTY_PIN_DATAINTERSECTION:
        {
            DEBUGMSG(ZONE_IOCTL, (_T("CAM_IOControl(%08x): CSPROPERTY_PIN_DATAINTERSECTION\r\n"), this));
            if (NULL == (pCsPin = reinterpret_cast<PCSP_PIN>(ValidateBuffer(pInBuf, InBufLen, sizeof(CSP_PIN), &dwError))))
            {
                break;
            }

            lPinId = pCsPin->PinId;
            if (false == IsValidPin(lPinId))
            {
                DEBUGMSG(ZONE_IOCTL, (_T("CAM_IOControl(%08x): Invalid PinId\r\n"), this));
                break;
            }

            pCsMultipleItem = NULL;
            pCsMultipleItem = reinterpret_cast<PCSMULTIPLE_ITEM>(pCsPin + 1);

            if (NULL == ValidateBuffer(pInBuf, InBufLen, (sizeof(CSP_PIN) + (pCsMultipleItem->Count * pCsMultipleItem->Size)), &dwError))
            {
                break;
            }

            {
                *pdwBytesTransferred = sizeof(CS_DATAFORMAT_VIDEOINFOHEADER);
            }

            if (NULL == (pCsDataFormatVih = reinterpret_cast<PCS_DATAFORMAT_VIDEOINFOHEADER>(ValidateBuffer(pOutBuf, OutBufLen, *pdwBytesTransferred, &dwError))))
            {
                dwError = ERROR_MORE_DATA;
                break;
            }

            pCsDataRangeVideo = NULL;
            pCsDataRangeVideo = reinterpret_cast<PCS_DATARANGE_VIDEO>(pCsMultipleItem + 1);


            {
                for (int iCount = 0 ; iCount < (int)pCsMultipleItem->Count ; iCount++)
                {
                    // First check whether the GUIDs match or not. This driver also tests other high level attributes of KS_DATARANGE_VIDEO
                    if (true == AdapterCompareFormat(lPinId, pCsDataRangeVideo, NULL, true))
                    {
                        // We found our format
                        memcpy(&pCsDataFormatVih->DataFormat,&pCsDataRangeVideo->DataRange, sizeof(CSDATAFORMAT)) ;
                        memcpy(&pCsDataFormatVih->VideoInfoHeader,&pCsDataRangeVideo->VideoInfoHeader, sizeof(CS_VIDEOINFOHEADER)) ;
                        dwError = ERROR_SUCCESS ;
                        break ;
                    }
                    pCsDataRangeVideo = reinterpret_cast<PCS_DATARANGE_VIDEO>(pCsDataRangeVideo + 1) ;
                }
            }
            break;
        }

        case CSPROPERTY_PIN_CATEGORY:
        {
            DEBUGMSG(ZONE_IOCTL, (_T("CAM_IOControl(%08x): CSPROPERTY_PIN_CATEGORY\r\n"), this));

            if (NULL == (pCsPin = reinterpret_cast<PCSP_PIN>(ValidateBuffer(pInBuf, InBufLen, sizeof(CSP_PIN), &dwError))))
            {
                break;
            }

            lPinId = pCsPin->PinId;
            if (false == IsValidPin(lPinId))
            {
                DEBUGMSG(ZONE_IOCTL, (_T("CAM_IOControl(%08x): Invalid PinId\r\n"), this));
                break;
            }

            *pdwBytesTransferred = sizeof(GUID);

            if (OutBufLen < *pdwBytesTransferred)
            {
                dwError = ERROR_MORE_DATA;
                break;
            }

            if (CeSafeCopyMemory(pOutBuf, &(m_pStrmInstances[lPinId].pVideoFormat->categoryGUID), sizeof(GUID)))
            {
                dwError = ERROR_SUCCESS;
            }

            break;
        }

        case CSPROPERTY_PIN_NAME:
        {
            DEBUGMSG(ZONE_IOCTL, (_T("CAM_IOControl(%08x): Property CSPROPERTY_PIN_NAME\r\n"), this));

            if (NULL == (pCsPin = reinterpret_cast<PCSP_PIN>(ValidateBuffer(pInBuf, InBufLen, sizeof(CSP_PIN), &dwError))))
            {
                break;
            }

            lPinId = pCsPin->PinId;

            if (false == IsValidPin(lPinId))
            {
                DEBUGMSG(ZONE_IOCTL, (_T("CAM_IOControl(%08x): Invalid PinId\r\n"), this));
                break;
            }

            *pdwBytesTransferred = (wcslen(g_wszPinNames[lPinId]) + 1) * sizeof(g_wszPinNames[0][0]);
            if (OutBufLen < *pdwBytesTransferred)
            {
                dwError = ERROR_MORE_DATA;
                break;
            }

            if (CeSafeCopyMemory(pOutBuf, g_wszPinNames[lPinId], *pdwBytesTransferred))
            {
                dwError = ERROR_SUCCESS;
            }

            break;
        }

        case CSPROPERTY_PIN_DEVICENAME:
        {
            DEBUGMSG(ZONE_IOCTL, (_T("CAM_IOControl(%08x): Property CSPROPERTY_PIN_NAME\r\n"), this));

            if (NULL == (pCsPin = reinterpret_cast<PCSP_PIN>(ValidateBuffer(pInBuf, InBufLen, sizeof(CSP_PIN), &dwError))))
            {
                break;
            }

            lPinId = pCsPin->PinId;
            if (false == IsValidPin(lPinId))
            {
                DEBUGMSG(ZONE_IOCTL, (_T("CAM_IOControl(%08x): Invalid PinId\r\n"), this));
                break;
            }

            *pdwBytesTransferred = (wcslen(g_wszPinDeviceNames[lPinId]) + 1) * sizeof(g_wszPinDeviceNames[0][0]) ;
            if (OutBufLen < *pdwBytesTransferred)
            {
                dwError = ERROR_MORE_DATA;
                break;
            }

            if (CeSafeCopyMemory(pOutBuf, g_wszPinDeviceNames[lPinId], *pdwBytesTransferred))
            {
                dwError = ERROR_SUCCESS;
            }

            break;
        }

        default:
        {
            DEBUGMSG(ZONE_IOCTL, (_T("CAM_IOControl(%08x): Invalid Property\r\n"), this));

            break;
        }
    }

    return dwError;
}


DWORD
CCameraDevice::AdapterHandlePropSetAdapter(
    PUCHAR pInBuf,
    DWORD  InBufLen,
    PUCHAR /*pOutBuf*/,
    DWORD  /*OutBufLen*/,
    PDWORD pdwBytesTransferred
   )
{
    DEBUGMSG(ZONE_IOCTL, (_T("CAM_IOControl(%08x): HandlePinRequests\r\n"), this));

    DWORD       dwError = ERROR_INVALID_PARAMETER;
    CSPROPERTY  csProp;
    CSPROPERTY_ADAPTER_NOTIFICATION_S csPropAdapterNotification;
    HANDLE  hProcess = NULL;

    if (InBufLen < sizeof(CSPROPERTY) ||
        !CeSafeCopyMemory(&csProp, pInBuf, sizeof(CSPROPERTY)))
    {
        return dwError;
    }

    *pdwBytesTransferred = 0;

    // we support PROPSETID_Adapter, so just return success
    if (CSPROPERTY_TYPE_SETSUPPORT == csProp.Flags)
    {
        return ERROR_SUCCESS;
    }

    switch (csProp.Id)
    {
        case CSPROPERTY_ADAPTER_NOTIFICATION:
        {
            DEBUGMSG(ZONE_IOCTL, (_T("CAM_IOControl(%08x): CSPROPERTY_PIN_CTYPES\r\n"), this));
            // We only support SET for CSPROPERTY_ADAPTER_NOTIFICATION
            if (CSPROPERTY_TYPE_SET != csProp.Flags)
            {
                return dwError;
            }

            if (InBufLen < sizeof(CSPROPERTY_ADAPTER_NOTIFICATION_S))
            {
                break;
            }

            CeSafeCopyMemory(&csPropAdapterNotification, pInBuf, sizeof(CSPROPERTY_ADAPTER_NOTIFICATION_S));

            MSGQUEUEOPTIONS msgQueueOptions = {0};
            msgQueueOptions.bReadAccess = FALSE; // we need write-access to msgqueue
            msgQueueOptions.dwSize      = sizeof(MSGQUEUEOPTIONS);

            hProcess = OpenProcess(NULL, FALSE, GetCallerVMProcessId());
            if (NULL == hProcess)
            {
                DEBUGMSG(ZONE_IOCTL|ZONE_ERROR, (_T("CAM_IOControl(%08x): Failed to open Process\r\n"), this)) ;
                return dwError ;
            }

            if (m_hAdapterMessageQueue)
            {
                // We can only have a single instance at a time. If we get here,
                // most likely we have failed to clean up properly on the previous
                // CAM_Close. In that case, we will close the handle here to avoid
                // leaking.
                CloseMsgQueue(m_hAdapterMessageQueue);
            }

            m_hAdapterMessageQueue = OpenMsgQueue(hProcess, csPropAdapterNotification.hMsgQueue, &msgQueueOptions);

            if (NULL == m_hAdapterMessageQueue)
            {
                DEBUGMSG(ZONE_IOCTL|ZONE_ERROR, (_T("CAM_IOControl(%08x): Failed to open MsgQueue\r\n"), this)) ;
                CloseHandle(hProcess);
                return dwError ;
            }

            CloseHandle(hProcess);

            dwError = ERROR_SUCCESS;
            break;
        }


        default:
        {
            DEBUGMSG(ZONE_IOCTL, (_T("CAM_IOControl(%08x): Invalid Property\r\n"), this));

            break;
        }
    }

    return dwError;
}


DWORD
CCameraDevice::AdapterHandleVidProcAmpRequests(
    PUCHAR pInBuf,                  // Warning: This is an unsafe buffer, access with care
    DWORD  InBufLen,
    PUCHAR pOutBuf,                 // Warning: This is an unsafe buffer, access with care
    DWORD  OutBufLen,
    PDWORD pdwBytesTransferred      // Warning: This is an unsafe buffer, access with care
   )
{
    // Note: This whole function is wrapped in a __try/__except block

    DEBUGMSG(ZONE_IOCTL, (_T("CAM_IOControl(%08x): HandleVidProcAmpRequests\r\n"), this));

    DWORD                      dwError                 = ERROR_INVALID_PARAMETER;
    LONG                       lValue                  = 0;
    LONG                       lFlags                  = 0;
    PSENSOR_PROPERTY           pSensorProp             = NULL;
    PCSPROPERTY_VIDEOPROCAMP_S pCsPropVidProcAmpOutput = NULL;
    PCSPROPERTY_VIDEOPROCAMP_S pCsPropVidProcAmpInput  = NULL;

    CSPROPERTY                 csProp = {0};

    if (InBufLen < sizeof(CSPROPERTY) ||
        !CeSafeCopyMemory(&csProp, pInBuf, sizeof(CSPROPERTY)))
    {
        return dwError;
    }

    *pdwBytesTransferred = 0;

    // we support PROPSETID_Pin, so just return success
    if (CSPROPERTY_TYPE_SETSUPPORT == csProp.Flags)
    {
        return ERROR_SUCCESS;
    }

    if (csProp.Id < CSPROPERTY_VIDEOPROCAMP_BRIGHTNESS || csProp.Id > ENUM_GAIN)
    {
        DEBUGMSG(ZONE_IOCTL, (_T("CAM_IOControl(%08x): Invalid Property\r\n"), this));

        return dwError;
    }

    if (ERROR_SUCCESS != m_pPDDFuncTbl->PDD_GetAdapterInfo(m_PDDContext, &m_AdapterInfo))
    {
        return dwError;
    }

    pSensorProp = m_AdapterInfo.SensorProps + csProp.Id;

    switch (csProp.Flags)
    {
    case CSPROPERTY_TYPE_GET:


        if (FALSE == pSensorProp->fGetSupported)
        {
            SetLastError(ERROR_INVALID_OPERATION);
            break;
        }

        *pdwBytesTransferred = sizeof(CSPROPERTY_VIDEOPROCAMP_S);
        if (NULL == (pCsPropVidProcAmpInput = reinterpret_cast<PCSPROPERTY_VIDEOPROCAMP_S>(ValidateBuffer(pInBuf, InBufLen, sizeof(CSPROPERTY_VIDEOPROCAMP_S), &dwError))))
        {
            return dwError;
        }

        if (NULL == (pCsPropVidProcAmpOutput = reinterpret_cast<PCSPROPERTY_VIDEOPROCAMP_S>(ValidateBuffer(pOutBuf, OutBufLen, *pdwBytesTransferred, &dwError))))
        {
            dwError = ERROR_MORE_DATA;
            break;
        }

        //Copy the CSPROPERTY structure to the output buffer just in case!
        memcpy(pCsPropVidProcAmpOutput, pCsPropVidProcAmpInput, sizeof(CSPROPERTY));

        pCsPropVidProcAmpOutput->Value        = pSensorProp->ulCurrentValue;
        pCsPropVidProcAmpOutput->Flags        = pSensorProp->ulFlags;
        pCsPropVidProcAmpOutput->Capabilities = pSensorProp->ulCapabilities;

        dwError = ERROR_SUCCESS;
        break;

    case CSPROPERTY_TYPE_SET:

        if (NULL == (pCsPropVidProcAmpInput = reinterpret_cast<PCSPROPERTY_VIDEOPROCAMP_S>(ValidateBuffer(pInBuf, InBufLen, sizeof(CSPROPERTY_VIDEOPROCAMP_S), &dwError))))
        {
            return dwError;
        }

        if (FALSE == pSensorProp->fSetSupported)
        {
            SetLastError(ERROR_INVALID_OPERATION);
            break;
        }

        // CSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL and CSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO are mutually exclusive
        if (pCsPropVidProcAmpInput->Flags != CSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL &&
            pCsPropVidProcAmpInput->Flags != CSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO)
        {
            SetLastError(ERROR_INVALID_OPERATION);
            break;
        }

        lValue = pCsPropVidProcAmpInput->Value;
        lFlags = pCsPropVidProcAmpInput->Flags;

        if (pCsPropVidProcAmpInput->Flags & ~pSensorProp->ulCapabilities)
        {
            break;
        }

        if (CSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL == pCsPropVidProcAmpInput->Flags)
        {
            if (lValue < pSensorProp->pRangeNStep->Bounds.SignedMinimum ||
                lValue > pSensorProp->pRangeNStep->Bounds.SignedMaximum ||
                0 != ((lValue - pSensorProp->pRangeNStep->Bounds.SignedMinimum) % pSensorProp->pRangeNStep->SteppingDelta))
            {
                break;
            }
        }

        dwError = m_pPDDFuncTbl->PDD_HandleVidProcAmpChanges(m_PDDContext, csProp.Id, lFlags, lValue);
        break;

    case CSPROPERTY_TYPE_DEFAULTVALUES:

        GetDefaultValues(pOutBuf, OutBufLen, pdwBytesTransferred, pSensorProp, &dwError);
        break;

    case CSPROPERTY_TYPE_BASICSUPPORT:

        GetBasicSupportInfo(pOutBuf, OutBufLen, pdwBytesTransferred, pSensorProp, &dwError);
        break;

    default :
        DEBUGMSG(ZONE_IOCTL, (_T("CAM_IOControl(%08x): Invalid Request\r\n"), this));
        return dwError;
    }

    return dwError;
}

DWORD
CCameraDevice::AdapterHandleCamControlRequests(
    PUCHAR pInBuf,              // Warning: This is an unsafe buffer, access with care
    DWORD  InBufLen,
    PUCHAR pOutBuf,             // Warning: This is an unsafe buffer, access with care
    DWORD  OutBufLen,
    PDWORD pdwBytesTransferred  // Warning: This is an unsafe buffer, access with care
   )
{
    // Note: This whole function is wrapped in a __try/__except block

    DEBUGMSG(ZONE_IOCTL, (_T("CAM_IOControl(%08x): HandleCamControlRequests\r\n"), this));
    DWORD dwError = ERROR_INVALID_PARAMETER;
    LONG  lValue  = 0;
    LONG  lFlags  = 0;
    PSENSOR_PROPERTY pSensorProp = NULL;
    PCSPROPERTY_CAMERACONTROL_S pCsPropCamControlOutput = NULL;
    PCSPROPERTY_CAMERACONTROL_S pCsPropCamControlInput  = NULL;

    CSPROPERTY   csProp = {0};

    if (InBufLen < sizeof(CSPROPERTY) ||
        !CeSafeCopyMemory(&csProp, pInBuf, sizeof(CSPROPERTY)))
    {
        return dwError;
    }

    *pdwBytesTransferred = 0;

    // we support PROPSETID_Pin, so just return success
    if (CSPROPERTY_TYPE_SETSUPPORT == csProp.Flags)
    {
        return ERROR_SUCCESS;
    }

    if (csProp.Id < CSPROPERTY_CAMERACONTROL_PAN || csProp.Id > CSPROPERTY_CAMERACONTROL_FLASH)
    {
        DEBUGMSG(ZONE_IOCTL, (_T("CAM_IOControl(%08x): Invalid Property\r\n"), this));
        return dwError;
    }

    if (ERROR_SUCCESS != m_pPDDFuncTbl->PDD_GetAdapterInfo(m_PDDContext, &m_AdapterInfo))
    {
        return dwError;
    }

    pSensorProp = m_AdapterInfo.SensorProps + (csProp.Id + (NUM_VIDEOPROCAMP_ITEMS));

    switch (csProp.Flags)
    {
    case CSPROPERTY_TYPE_GET:

        if (NULL == (pCsPropCamControlInput = reinterpret_cast<PCSPROPERTY_CAMERACONTROL_S>(ValidateBuffer(pInBuf, InBufLen, sizeof(CSPROPERTY_CAMERACONTROL_S), &dwError))))
        {
            return dwError;
        }

        if (FALSE == pSensorProp->fGetSupported)
        {
            SetLastError(ERROR_INVALID_OPERATION);
            break;
        }

        *pdwBytesTransferred = sizeof(CSPROPERTY_CAMERACONTROL_S);
        if (NULL == (pCsPropCamControlOutput = reinterpret_cast<PCSPROPERTY_CAMERACONTROL_S>(ValidateBuffer(pOutBuf, OutBufLen, *pdwBytesTransferred, &dwError))))
        {
            dwError = ERROR_MORE_DATA;
            break;
        }

        //Copy the CSPROPERTY structure to the output buffer just in case!
        memcpy(pCsPropCamControlOutput, pCsPropCamControlInput, sizeof(CSPROPERTY));

        pCsPropCamControlOutput->Value         = pSensorProp->ulCurrentValue;
        pCsPropCamControlOutput->Flags         = pSensorProp->ulFlags;
        pCsPropCamControlOutput->Capabilities  = pSensorProp->ulCapabilities;

        dwError = ERROR_SUCCESS;
        break;

    case CSPROPERTY_TYPE_SET:

        if (NULL == (pCsPropCamControlInput = reinterpret_cast<PCSPROPERTY_CAMERACONTROL_S>(ValidateBuffer(pInBuf, InBufLen, sizeof(CSPROPERTY_CAMERACONTROL_S), &dwError))))
        {
            return dwError;
        }

        if (FALSE == pSensorProp->fSetSupported)
        {
            SetLastError(ERROR_INVALID_OPERATION);
            break;
        }

        // CSPROPERTY_CAMERACONTROL_FLAGS_MANUAL, CSPROPERTY_CAMERACONTROL_FLAGS_AUTO and CSPROPERTY_CAMERACONTROL_FLAGS_ASYNCHRONOUS_AUTO are mutually exclusive
        if (pCsPropCamControlInput->Flags != CSPROPERTY_CAMERACONTROL_FLAGS_MANUAL &&
            pCsPropCamControlInput->Flags != CSPROPERTY_CAMERACONTROL_FLAGS_AUTO &&
            pCsPropCamControlInput->Flags != CSPROPERTY_CAMERACONTROL_FLAGS_ASYNCHRONOUS_AUTO)
        {
            SetLastError(ERROR_INVALID_OPERATION);
            break;
        }

        lValue = pCsPropCamControlInput->Value;
        lFlags = pCsPropCamControlInput->Flags;

        if (lFlags & ~pSensorProp->ulCapabilities)
        {
            break;
        }

        if (CSPROPERTY_CAMERACONTROL_FLAGS_MANUAL == lFlags)
        {
            if (lValue < pSensorProp->pRangeNStep->Bounds.SignedMinimum ||
                lValue > pSensorProp->pRangeNStep->Bounds.SignedMaximum ||
                0 != ((lValue - pSensorProp->pRangeNStep->Bounds.SignedMinimum) % pSensorProp->pRangeNStep->SteppingDelta))
            {
                break;
            }

        }

        dwError = m_pPDDFuncTbl->PDD_HandleCamControlChanges(m_PDDContext, csProp.Id + (NUM_VIDEOPROCAMP_ITEMS), lFlags, lValue);

        break;

    case CSPROPERTY_TYPE_DEFAULTVALUES:

        GetDefaultValues(pOutBuf, OutBufLen, pdwBytesTransferred, pSensorProp, &dwError);
        break;

    case CSPROPERTY_TYPE_BASICSUPPORT:

        GetBasicSupportInfo(pOutBuf, OutBufLen, pdwBytesTransferred, pSensorProp, &dwError);
        break;

    default :
        DEBUGMSG(ZONE_IOCTL, (_T("CAM_IOControl(%08x): Invalid Request\r\n"), this));
        return dwError;
    }

    return dwError;
}

DWORD
CCameraDevice::AdapterHandleVideoControlRequests(
    PUCHAR pInBuf,                  // Warning: This is an unsafe buffer, access with care
    DWORD  InBufLen,
    PUCHAR pOutBuf,                 // Warning: This is an unsafe buffer, access with care
    DWORD  OutBufLen,
    PDWORD pdwBytesTransferred      // Warning: This is an unsafe buffer, access with care
   )
{
    // Note: This whole function is wrapped in a __try/__except block
    DEBUGMSG(ZONE_IOCTL, (_T("CAM_IOControl(%08x): HandleVideoControlRequests\r\n"), this));

    DWORD dwError = ERROR_INVALID_PARAMETER;
    ULONG ulCaps  = 0;
    LONG lIndex = 0;

    PCSPROPERTY_VIDEOCONTROL_CAPS_S pCsPropVideoControlCapsOutput = NULL;
    PCSPROPERTY_VIDEOCONTROL_CAPS_S pCsPropVideoControlCapsInput  = NULL;
    PCSPROPERTY_VIDEOCONTROL_MODE_S pCsPropVideoControlModeInput  = NULL;

    CSPROPERTY csProp = {0};

    if (InBufLen < sizeof(CSPROPERTY) ||
        !CeSafeCopyMemory(&csProp, pInBuf, sizeof(CSPROPERTY)))
    {
        return dwError;
    }

    *pdwBytesTransferred = 0;

    // we support PROPSETID_Pin, so just return success
    if (CSPROPERTY_TYPE_SETSUPPORT == csProp.Flags)
    {
        return ERROR_SUCCESS;
    }

    switch (csProp.Id)
    {
    case CSPROPERTY_VIDEOCONTROL_CAPS:
        if (NULL == (pCsPropVideoControlCapsInput = reinterpret_cast<PCSPROPERTY_VIDEOCONTROL_CAPS_S>(ValidateBuffer(pInBuf, InBufLen, sizeof(CSPROPERTY_VIDEOCONTROL_CAPS_S), &dwError))))
        {
            break;
        }

        lIndex = pCsPropVideoControlCapsInput->StreamIndex;
        if (false == IsValidPin(lIndex))
        {
            DEBUGMSG(ZONE_IOCTL, (_T("CAM_IOControl(%08x): Invalid PinId\r\n"), this));
            break;
        }

        switch (csProp.Flags)
        {
        case CSPROPERTY_TYPE_GET:
            *pdwBytesTransferred = sizeof(CSPROPERTY_VIDEOCONTROL_CAPS_S);
            if (NULL == (pCsPropVideoControlCapsOutput = reinterpret_cast<PCSPROPERTY_VIDEOCONTROL_CAPS_S>(ValidateBuffer(pOutBuf, OutBufLen, *pdwBytesTransferred, &dwError))))
            {
                dwError = ERROR_MORE_DATA;
                break;
            }

            //Copy the CSPROPERTY structure to the output buffer just in case!
            memcpy(pCsPropVideoControlCapsOutput, pCsPropVideoControlCapsInput, sizeof(CSPROPERTY));

            pCsPropVideoControlCapsOutput->StreamIndex      = lIndex;

            if (true == GetPDDPinInfo())
            {
                pCsPropVideoControlCapsOutput->VideoControlCaps = m_pStrmInstances[lIndex].VideoCaps.CurrentVideoControlCaps;

                dwError = ERROR_SUCCESS;
            }
            break;

        case CSPROPERTY_TYPE_SET:
            lIndex = pCsPropVideoControlCapsInput->StreamIndex;

            ulCaps = pCsPropVideoControlCapsInput->VideoControlCaps;
            if (ulCaps & ~(CS_VideoControlFlag_FlipHorizontal        |
                             CS_VideoControlFlag_FlipVertical          |
                             CS_VideoControlFlag_ExternalTriggerEnable |
                             CS_VideoControlFlag_Trigger               |
                             CS_VideoControlFlag_Sample_Scanned_Notification))
            {
                DEBUGMSG(ZONE_IOCTL, (_T("CAM_IOControl(%08x): Invalid flag specified as Video Control Caps.\r\n"), this));
                break;
            }

            if (false == IsValidPin(lIndex))
            {
                DEBUGMSG(ZONE_IOCTL, (_T("CAM_IOControl(%08x): Invalid pin index.\r\n"), this));
                break;
            }

            dwError = m_pPDDFuncTbl->PDD_HandleVideoControlCapsChanges(m_PDDContext, lIndex ,ulCaps);
            break;

        case CSPROPERTY_TYPE_DEFAULTVALUES:
            *pdwBytesTransferred = sizeof(CSPROPERTY_VIDEOCONTROL_CAPS_S);

            if (NULL == (pCsPropVideoControlCapsOutput = reinterpret_cast<PCSPROPERTY_VIDEOCONTROL_CAPS_S>(ValidateBuffer(pOutBuf, OutBufLen, *pdwBytesTransferred, &dwError))))
            {
                dwError = ERROR_MORE_DATA;
                break;
            }

            lIndex = pCsPropVideoControlCapsInput->StreamIndex;
            if (false == IsValidPin(lIndex))
            {
                DEBUGMSG(ZONE_IOCTL, (_T("CAM_IOControl(%08x): Invalid pin index.\r\n"), this));
                break;
            }

            //Copy the CSPROPERTY structure to the output buffer just in case!
            memcpy(pCsPropVideoControlCapsOutput, pCsPropVideoControlCapsInput, sizeof(CSPROPERTY));

            pCsPropVideoControlCapsOutput->StreamIndex      = lIndex;

            pCsPropVideoControlCapsOutput->VideoControlCaps =  m_pStrmInstances[lIndex].VideoCaps.DefaultVideoControlCaps;

            dwError = ERROR_SUCCESS;

            break;

        default:
            DEBUGMSG(ZONE_IOCTL, (_T("CAM_IOControl(%08x): Invalid Request\r\n"), this));
            break;
        }

        break;

    case CSPROPERTY_VIDEOCONTROL_ACTUAL_FRAME_RATE:
    case CSPROPERTY_VIDEOCONTROL_FRAME_RATES:
        switch (csProp.Flags)
        {
        case CSPROPERTY_TYPE_GET:
            DEBUGMSG(ZONE_IOCTL, (_T("CAM_IOControl(%08x): Get not supported.\r\n"), this));
            dwError = ERROR_NOT_SUPPORTED;
            break;

        default :
            DEBUGMSG(ZONE_IOCTL, (_T("CAM_IOControl(%08x): Invalid Request\r\n"), this)) ;
            break ;
        }

        break;

    case CSPROPERTY_VIDEOCONTROL_MODE:
        if (NULL == (pCsPropVideoControlModeInput = reinterpret_cast<PCSPROPERTY_VIDEOCONTROL_MODE_S>(ValidateBuffer(pInBuf, InBufLen, sizeof(CSPROPERTY_VIDEOCONTROL_MODE_S), &dwError))))
        {
            break;
        }

        lIndex = pCsPropVideoControlModeInput->StreamIndex;
        if (false == IsValidPin(lIndex))
        {
            DEBUGMSG(ZONE_IOCTL, (_T("CAM_IOControl(%08x): Invalid PinId\r\n"), this));
            break;
        }

        switch (csProp.Flags)
        {
        case CSPROPERTY_TYPE_GET:
            DEBUGMSG(ZONE_IOCTL, (_T("CAM_IOControl(%08x): Get not supported.\r\n"), this));
            dwError = ERROR_NOT_SUPPORTED;
            break;

        case CSPROPERTY_TYPE_SET:
            ulCaps = pCsPropVideoControlModeInput->Mode;
            if (ulCaps != CS_VideoControlFlag_Trigger)
            {
                DEBUGMSG(ZONE_IOCTL, (_T("CAM_IOControl(%08x): Invalid flag specified as Video Control Mode.\r\n"), this));
                break;
            }

            //Only STILL pin supports CS_VideoControlFlag_Trigger for the current CAMERA MDD
            if (m_pStrmInstances[lIndex].pPinDev &&  ( lIndex == STILL))
            {
                if (CSSTATE_STOP == m_pStrmInstances[lIndex].pPinDev->GetState())
                {
                    dwError = ERROR_INVALID_STATE;
                }
                else
                {
                    m_pStrmInstances[lIndex].pPinDev->IncrementStillCount();
                    m_pStrmInstances[lIndex].pPinDev->SetState(CSSTATE_RUN, NULL);
                    dwError = m_pPDDFuncTbl->PDD_TakeStillPicture(m_PDDContext, NULL);
                }
            }
            else
            {
                DEBUGMSG(ZONE_IOCTL, (_T("CAM_IOControl(%08x): Invalid pin for Trigger.\r\n"), this));
            }

            break ;

        default :
            DEBUGMSG(ZONE_IOCTL, (_T("CAM_IOControl(%08x): Invalid Request\r\n"), this)) ;
            break ;
        }

        break;

    default:
        DEBUGMSG(ZONE_IOCTL, (_T("CAM_IOControl(%08x): Invalid Property\r\n"), this)) ;
        return dwError ;
    }

    return dwError;
}

DWORD
CCameraDevice::AdapterHandleDroppedFramesRequests(
    PUCHAR pInBuf,              // Warning: This is an unsafe buffer, access with care
    DWORD  InBufLen,
    PUCHAR pOutBuf,             // Warning: This is an unsafe buffer, access with care
    DWORD  OutBufLen,
    PDWORD pdwBytesTransferred  // Warning: This is an unsafe buffer, access with care
   )
{
    // Note: This whole function is wrapped in a __try/__except block
    DEBUGMSG(ZONE_IOCTL, (_T("CAM_IOControl(%08x): HandleDroppedFramesRequests\r\n"), this));

    DWORD dwError = ERROR_INVALID_PARAMETER;

    CSPROPERTY csProp;
    CSPROPERTY_DROPPEDFRAMES_CURRENT_S csPropDroppedFramesOutput = {0};

    if (InBufLen < sizeof(CSPROPERTY) ||
        !CeSafeCopyMemory(&csProp, pInBuf, sizeof(CSPROPERTY)))
    {
        return dwError;
    }

    *pdwBytesTransferred = 0;

    switch (csProp.Id)
    {
    case CSPROPERTY_DROPPEDFRAMES_CURRENT:
        {
            switch (csProp.Flags)
            {
            case CSPROPERTY_TYPE_GET:
                *pdwBytesTransferred = sizeof(CSPROPERTY_DROPPEDFRAMES_CURRENT_S);
                if (OutBufLen < *pdwBytesTransferred)
                {
                    dwError = ERROR_MORE_DATA;
                    break;
                }

                if (NULL == pOutBuf)
                {
                    break;
                }

                //Copy the CSPROPERTY structure to the output buffer just in case!
                memcpy(&csPropDroppedFramesOutput, &csProp, sizeof(CSPROPERTY));

                if (m_pStrmInstances[CAPTURE].pPinDev)
                {
                    csPropDroppedFramesOutput.PictureNumber    = m_pStrmInstances[CAPTURE].pPinDev->PictureNumber();
                    csPropDroppedFramesOutput.DropCount        = m_pStrmInstances[CAPTURE].pPinDev->FramesDropped();
                    csPropDroppedFramesOutput.AverageFrameSize = m_pStrmInstances[CAPTURE].pPinDev->FrameSize();

                    if (CeSafeCopyMemory(pOutBuf, &csPropDroppedFramesOutput, sizeof(CSPROPERTY_DROPPEDFRAMES_CURRENT_S)))
                    {
                        dwError = ERROR_SUCCESS;
                    }
                }
                break;
            }
        }
        break;

    default:
        DEBUGMSG(ZONE_IOCTL, (_T("CAM_IOControl(%08x): Invalid Property\r\n"), this)) ;
        return dwError;
    }

    return dwError;
}

DWORD
CCameraDevice::AdapterHandleMetadataRequests(
    PUCHAR pInBuf,
    DWORD  InBufLen,
    PUCHAR pOutBuf,
    DWORD  OutBufLen,
    PDWORD pdwBytesTransferred
   )
{
    // Note: This whole function is wrapped in a __try/__except block

    DEBUGMSG(ZONE_IOCTL, (_T("CAM_IOControl(%08x): AdapterHandleMetadataRequests\r\n"), this));
    DWORD dwError = ERROR_INVALID_PARAMETER;
    CSPROPERTY csProp = {0};

    if (m_pPDDFuncTbl->dwSize < sizeof(PDDFUNCTBL2))
    {
        return ERROR_NOT_SUPPORTED;
    }

    if (InBufLen < sizeof(CSPROPERTY) ||
        !CeSafeCopyMemory(&csProp, pInBuf, sizeof(CSPROPERTY)))
    {
        return dwError;
    }

    *pdwBytesTransferred = 0;

    // We support CSPROPSETID_Metadata, so just return success
    if (CSPROPERTY_TYPE_SETSUPPORT == csProp.Flags)
    {
        return ERROR_SUCCESS;
    }

    if (CSPROPERTY_METADATA_ALL != csProp.Id)
    {
        DEBUGMSG(ZONE_IOCTL, (_T("CAM_IOControl(%08x): Invalid Property\r\n"), this));
        return dwError;
    }

    switch (csProp.Flags)
    {
        case CSPROPERTY_TYPE_GET:
        {
            // Assume 'Get' is supported if we can successfully call into the PDD and there is data.
            DWORD dwRet = m_PDDFuncTbl2.PDD_GetMetadata(m_PDDContext, csProp.Id, NULL, 0, pdwBytesTransferred);

            if (ERROR_SUCCESS != dwRet || *pdwBytesTransferred <= sizeof(ULONG))
            {
                SetLastError(ERROR_INVALID_OPERATION);
                break;
            }

            dwError = m_PDDFuncTbl2.PDD_GetMetadata(m_PDDContext, csProp.Id, pOutBuf, OutBufLen, pdwBytesTransferred);
            break;
        }
        case CSPROPERTY_TYPE_SET:
        {
            SetLastError(ERROR_INVALID_OPERATION);
            break;
        }
        case CSPROPERTY_TYPE_BASICSUPPORT:
        {
            if (NULL == pOutBuf)
            {
                break;
            }

            if (OutBufLen >= sizeof(ULONG))
            {
                // Assume 'Get' is supported if we can successfully call into the PDD and there is data.
                DWORD dwRet = m_PDDFuncTbl2.PDD_GetMetadata(m_PDDContext, csProp.Id, NULL, 0, pdwBytesTransferred);

                ULONG ulAccessFlags = ERROR_SUCCESS == dwRet && *pdwBytesTransferred > sizeof(ULONG) ? CSPROPERTY_TYPE_GET : 0;

                if (ulAccessFlags == 0)
                {
                    dwError = ERROR_INVALID_PARAMETER;
                }
                else
                {
                    *(ULONG *)pOutBuf = ulAccessFlags;
                    dwError = ERROR_SUCCESS;
                }
            }
            else
            {
                DEBUGMSG(ZONE_IOCTL, (_T("CAM_IOControl(%08x): Output buffer is not large enough\r\n"), this));
                dwError = ERROR_INSUFFICIENT_BUFFER;
            }

            break;
        }
        default:
        {
            DEBUGMSG(ZONE_IOCTL, (_T("CAM_IOControl(%08x): Invalid Request\r\n"), this));
            break;
        }
    }

    return dwError;
}

DWORD
CCameraDevice::AdapterHandlePowerRequests(
    DWORD  dwCode,
    PUCHAR /*pInBuf*/,
    DWORD  /*InBufLen*/,
    PUCHAR pOutBuf,
    DWORD  OutBufLen,
    PDWORD pdwBytesTransferred
   )
{
    DWORD dwErr = ERROR_INVALID_PARAMETER;

    DEBUGMSG(ZONE_IOCTL, (_T("CAM_IOControl(%08x): HandlePowerRequests\r\n"), this));

    switch (dwCode)
    {
    //
    // Power Management Support.
    //
    case IOCTL_POWER_CAPABILITIES:

        DEBUGMSG(ZONE_IOCTL, (TEXT("CAM: IOCTL_POWER_CAPABILITIES\r\n")));
        if (pOutBuf && OutBufLen >= sizeof(POWER_CAPABILITIES))
        {
             memcpy(pOutBuf, &m_AdapterInfo.PowerCaps, sizeof(POWER_CAPABILITIES));
             if (pdwBytesTransferred)
             {
                 *pdwBytesTransferred = sizeof(POWER_CAPABILITIES);
             }
             dwErr = ERROR_SUCCESS;
        }
        break;

    case IOCTL_POWER_SET:

        DEBUGMSG(ZONE_IOCTL, (TEXT("CAM: IOCTL_POWER_SET\r\n")));
        if (pOutBuf && OutBufLen >= sizeof(CEDEVICE_POWER_STATE))
        {
            PCEDEVICE_POWER_STATE pState = (PCEDEVICE_POWER_STATE) pOutBuf;

            if (VALID_DX(*pState))
            {
                CEDEVICE_POWER_STATE ReqState = *pState;
                CEDEVICE_POWER_STATE OldState = m_PowerState;

                // Verify that the requested state is supported. If not, try to set the next higher supported state.
                while (VALID_DX(ReqState) && !(DX_MASK(ReqState) & m_AdapterInfo.PowerCaps.DeviceDx))
                {
                    ReqState = (CEDEVICE_POWER_STATE)((int)ReqState + 1);
                }

                if (ReqState == m_PowerState)
                {
                    dwErr = ERROR_SUCCESS;
                }
                else
                {
                    switch (ReqState)
                    {
                    case D0:
                        RETAILMSG(ZONE_IOCTL, (TEXT("CAM: D0\r\n")));

                        m_PowerState = D0;
                        dwErr = PowerUp();
                        break;

                    case D1:
                        RETAILMSG(ZONE_IOCTL, (TEXT("CAM: D1\r\n")));

                        m_PowerState = D1;
                        dwErr = PowerUp();
                        break;


                    case D2:
                        RETAILMSG(ZONE_IOCTL, (TEXT("CAM: D2\r\n")));

                        m_PowerState = D2;
                        dwErr = PowerUp();
                        break;

                    case D3:
                        RETAILMSG(ZONE_IOCTL, (TEXT("CAM: D3\r\n")));

                        m_PowerState = D3;
                        dwErr = PowerDown();
                        break;

                    case D4:
                        RETAILMSG(ZONE_IOCTL, (TEXT("CAM: D4\r\n")));

                        m_PowerState = D4;
                        dwErr = PowerDown();
                        break;

                    default:
                        RETAILMSG(ZONE_IOCTL, (TEXT("CAM: Unsupported power state request.\r\n")));

                        dwErr = ERROR_NOT_SUPPORTED;
                        break;
                    }
                }

                if (dwErr == ERROR_SUCCESS)
                {
                    if (*pState != m_PowerState)
                    {
                        *pState = m_PowerState;

                        if (pdwBytesTransferred)
                        {
                            *pdwBytesTransferred = sizeof(CEDEVICE_POWER_STATE);
                        }
                    }
                }
                else
                {
                    m_PowerState = OldState;
                }
            }
            else
            {
                RETAILMSG(ZONE_IOCTL|ZONE_WARN, (TEXT("CAM: Unrecognized power state.\r\n")));
            }
        }
        break;

    case IOCTL_POWER_GET:

        DEBUGMSG(ZONE_IOCTL, (TEXT("CAM: IOCTL_POWER_GET\r\n")));
        if (pOutBuf && OutBufLen >= sizeof(CEDEVICE_POWER_STATE))
        {
            *((PCEDEVICE_POWER_STATE) pOutBuf) = m_PowerState;
            if (pdwBytesTransferred)
            {
                *pdwBytesTransferred = sizeof(CEDEVICE_POWER_STATE);
            }
            dwErr = ERROR_SUCCESS;
        }
        break;

    default:
        break;
    }

    return dwErr;
}

DWORD
CCameraDevice::AdapterHandleCustomRequests(
    PUCHAR pInBuf,              // Warning: This is an unsafe buffer, access with care
    DWORD  InBufLen,
    PUCHAR pOutBuf,             // Warning: This is an unsafe buffer, access with care
    DWORD  OutBufLen,
    PDWORD pdwBytesTransferred  // Warning: This is an unsafe buffer, access with care
   )
{
    // Note: This whole function is wrapped in a __try/__except block
    DEBUGMSG(ZONE_IOCTL, (_T("CAM_IOControl(%08x): AdapterHandleCustomRequests\r\n"), this));
    return m_pPDDFuncTbl->PDD_HandleAdapterCustomProperties(m_PDDContext, pInBuf, InBufLen, pOutBuf, OutBufLen, pdwBytesTransferred);
}

DWORD
CCameraDevice::PowerDown()
{
    DWORD dwErr;

    if (m_dwRefCount)
    {
        for (UINT i = 0; i < m_AdapterInfo.ulCTypes; i++)
        {
            if (NULL != m_pStrmInstances[i].pPinDev)
            {
                m_pStrmInstances[i].pPinDev->SetState(CSSTATE_STOP, &m_pStrmInstances[i].CsPrevState);
            }
        }

        dwErr = m_pPDDFuncTbl->PDD_SetPowerState(m_PDDContext, m_PowerState);

        // Restore the sensor state on failure
        if (dwErr != ERROR_SUCCESS)
        {
            for (UINT i = 0; i < m_AdapterInfo.ulCTypes; i++)
            {
                if (NULL != m_pStrmInstances[i].pPinDev)
                {
                    m_pStrmInstances[i].pPinDev->SetState(m_pStrmInstances[i].CsPrevState, NULL);
                }
            }
        }
    }
    else
    {
        dwErr = ERROR_INVALID_STATE;
    }

    return dwErr;
}

DWORD
CCameraDevice::PowerUp()
{
    DWORD dwErr;

    if (m_dwRefCount)
    {
        dwErr = m_pPDDFuncTbl->PDD_SetPowerState(m_PDDContext, m_PowerState);

        if (dwErr == ERROR_SUCCESS)
        {
            for (UINT i = 0; i < m_AdapterInfo.ulCTypes; i++)
            {
                if (NULL != m_pStrmInstances[i].pPinDev)
                {
                    m_pStrmInstances[i].pPinDev->SetState(m_pStrmInstances[i].CsPrevState, NULL);
                }
            }
        }
    }
    else
    {
        dwErr = ERROR_INVALID_STATE;
    }

    return dwErr;
}

void
CCameraDevice::GetBasicSupportInfo(
    PUCHAR pOutBuf,
    DWORD  OutBufLen,
    PDWORD pdwBytesTransferred,
    PSENSOR_PROPERTY pSensorProp,
    PDWORD pdwError
   )
{
    PCSPROPERTY_DESCRIPTION   pCsPropDesc          = NULL;
    PCSPROPERTY_MEMBERSHEADER pCsPropMembersHeader = NULL;

    if (OutBufLen >= sizeof(CSPROPERTY_DESCRIPTION))
    {
        if (OutBufLen >= StandardSizeOfBasicValues)
        {
            *pdwBytesTransferred = StandardSizeOfBasicValues;
        }
        else
        {
            *pdwBytesTransferred = sizeof(CSPROPERTY_DESCRIPTION);
        }

        pCsPropDesc = reinterpret_cast<PCSPROPERTY_DESCRIPTION>(ValidateBuffer(pOutBuf, OutBufLen, sizeof(CSPROPERTY_DESCRIPTION), pdwError));

        if (NULL == pCsPropDesc)
        {
            *pdwError = ERROR_MORE_DATA;
            return;
        }

        ULONG lAccessFlags = (pSensorProp->fGetSupported ? CSPROPERTY_TYPE_GET : 0) | (pSensorProp->fSetSupported ? CSPROPERTY_TYPE_SET : 0);

        if (lAccessFlags == 0)
        {
            *pdwError = ERROR_INVALID_PARAMETER;
            return;
        }

        pCsPropDesc->AccessFlags = lAccessFlags;
        pCsPropDesc->MembersListCount = 1;
        pCsPropDesc->Reserved         = 0;
        pCsPropDesc->DescriptionSize  = StandardSizeOfBasicValues;

        memcpy(&pCsPropDesc->PropTypeSet, &pSensorProp->pCsPropValues->PropTypeSet, sizeof(CSIDENTIFIER));

        if (OutBufLen >= StandardSizeOfBasicValues)
        {
            pCsPropMembersHeader = reinterpret_cast<PCSPROPERTY_MEMBERSHEADER>(pCsPropDesc + 1);

            // Copy the CSPROPERTY_MEMBERSHEADER
            memcpy(pCsPropMembersHeader, pSensorProp->pCsPropValues->MembersList, sizeof(CSPROPERTY_MEMBERSHEADER));

            // Copy the CSPROPERTY_STEPPING_LONG
            memcpy(pCsPropMembersHeader + 1, pSensorProp->pCsPropValues->MembersList->Members, sizeof(CSPROPERTY_STEPPING_LONG));

            *pdwError = ERROR_SUCCESS;
        }
        else
        {
            DEBUGMSG(ZONE_IOCTL|ZONE_ERROR, (_T("CAM_IOControl(%08x): Not all available data could be written\r\n"), this));
            *pdwError = ERROR_MORE_DATA;
        }
    }
    else if (OutBufLen >= sizeof(ULONG))
    {
        // We just need to return AccessFlags.
        DEBUGMSG(ZONE_IOCTL, (_T("CAM_IOControl(%08x): Returning AccessFlags\r\n"), this));

        *pdwBytesTransferred = sizeof(ULONG);

        if (NULL == ValidateBuffer(pOutBuf, OutBufLen, sizeof(ULONG), pdwError))
        {
            *pdwError = ERROR_MORE_DATA;
            return;
        }

        ULONG lAccessFlags = (pSensorProp->fGetSupported ? CSPROPERTY_TYPE_GET : 0) | (pSensorProp->fSetSupported ? CSPROPERTY_TYPE_SET : 0);

        if (lAccessFlags == 0)
        {
            *pdwError = ERROR_INVALID_PARAMETER;
        }
        else
        {
            *((PULONG)pOutBuf) = lAccessFlags;
            *pdwError = ERROR_SUCCESS;
        }
    }
    else
    {
        DEBUGMSG(ZONE_IOCTL, (_T("CAM_IOControl(%08x): Output buffer is not large enough\r\n"), this));
        *pdwError = ERROR_INSUFFICIENT_BUFFER;
    }

    return;
}

void
CCameraDevice::GetDefaultValues(
    PUCHAR pOutBuf,
    DWORD  OutBufLen,
    PDWORD pdwBytesTransferred,
    PSENSOR_PROPERTY pSensorProp,
    PDWORD pdwError
   )
{
    PCSPROPERTY_DESCRIPTION   pCsPropDesc          = NULL;
    PCSPROPERTY_MEMBERSHEADER pCsPropMembersHeader = NULL;

    if (OutBufLen >= sizeof(CSPROPERTY_DESCRIPTION))
    {
        if (OutBufLen >= StandardSizeOfDefaultValues)
        {
            *pdwBytesTransferred = StandardSizeOfDefaultValues;
        }
        else
        {
            *pdwBytesTransferred = sizeof(CSPROPERTY_DESCRIPTION);
        }

        pCsPropDesc = reinterpret_cast<PCSPROPERTY_DESCRIPTION>(ValidateBuffer(pOutBuf, OutBufLen, sizeof(CSPROPERTY_DESCRIPTION), pdwError));

        if (NULL == pCsPropDesc)
        {
            *pdwError = ERROR_MORE_DATA;

            return;
        }

        pCsPropDesc->AccessFlags = (pSensorProp->fGetSupported ? CSPROPERTY_TYPE_GET : 0) | (pSensorProp->fSetSupported ? CSPROPERTY_TYPE_SET : 0);

        if (pCsPropDesc->AccessFlags == 0)
        {
            *pdwError = ERROR_INVALID_PARAMETER;
            return;
        }

        pCsPropDesc->MembersListCount = 1;
        pCsPropDesc->Reserved         = 0;
        pCsPropDesc->DescriptionSize  = StandardSizeOfDefaultValues;

        memcpy(&pCsPropDesc->PropTypeSet, &pSensorProp->pCsPropValues->PropTypeSet, sizeof(CSIDENTIFIER));

        if (OutBufLen >= StandardSizeOfDefaultValues)
        {
            pCsPropMembersHeader = reinterpret_cast<PCSPROPERTY_MEMBERSHEADER>(pCsPropDesc + 1);

            // Copy the CSPROPERTY_MEMBERSHEADER
            memcpy(pCsPropMembersHeader, &(pSensorProp->pCsPropValues->MembersList[1]), sizeof(CSPROPERTY_MEMBERSHEADER));

            // Copy the LONG (default value)
            memcpy(pCsPropMembersHeader + 1, pSensorProp->pCsPropValues->MembersList[1].Members, sizeof(LONG));

            *pdwError = ERROR_SUCCESS;
        }
        else
        {
            DEBUGMSG(ZONE_IOCTL|ZONE_ERROR, (_T("CAM_IOControl(%08x): Not all available data could be written\r\n"), this));
            *pdwError = ERROR_MORE_DATA;
        }
    }
    else if (OutBufLen >= sizeof(ULONG))
    {
        // We just need to return AccessFlags.
        DEBUGMSG(ZONE_IOCTL, (_T("CAM_IOControl(%08x): Returning AccessFlags\r\n"), this));

        *pdwBytesTransferred = sizeof(ULONG);

        if (NULL == ValidateBuffer(pOutBuf, OutBufLen, sizeof(ULONG), pdwError))
        {
            *pdwError = ERROR_MORE_DATA;
            return;
        }

        *((PULONG)pOutBuf) =
            static_cast<ULONG>((pSensorProp->fGetSupported ? CSPROPERTY_TYPE_GET : 0) | (pSensorProp->fSetSupported ? CSPROPERTY_TYPE_SET : 0));
        *pdwError = *((PULONG)pOutBuf) == 0 ? ERROR_INVALID_PARAMETER : ERROR_SUCCESS;
    }
    else
    {
        DEBUGMSG(ZONE_IOCTL, (_T("CAM_IOControl(%08x): Output buffer is not large enough\r\n"), this));
        *pdwError = ERROR_INSUFFICIENT_BUFFER;
    }

    return;
}

DWORD CCameraDevice::PDDClosePin(ULONG ulPinId)
{
    PDDSetPinState(ulPinId, CSSTATE_STOP);
    return m_pPDDFuncTbl->PDD_DeInitSensorMode(m_PDDContext, ulPinId);
}

DWORD CCameraDevice::PDDGetPinInfo(ULONG ulPinId, PSENSORMODEINFO pSensorModeInfo)
{
    return m_pPDDFuncTbl->PDD_GetSensorModeInfo(m_PDDContext, ulPinId, pSensorModeInfo);
}


DWORD CCameraDevice::PDDSetPinState(ULONG ulPinId, CSSTATE State)
{
    return m_pPDDFuncTbl->PDD_SetSensorState(m_PDDContext, ulPinId, State);
}


DWORD CCameraDevice::PDDFillPinBuffer(ULONG ulPinId, PUCHAR pImage)
{
    return m_pPDDFuncTbl->PDD_FillBuffer(m_PDDContext, ulPinId, pImage);
}


DWORD CCameraDevice::PDDInitPin(ULONG ulPinId, CPinDevice *pPin)
{
    return m_pPDDFuncTbl->PDD_InitSensorMode(m_PDDContext, ulPinId, pPin);
}

DWORD CCameraDevice::PDDSetPinFormat(ULONG ulPinId, PCS_DATARANGE_VIDEO pCsDataRangeVideo)
{
    if (NULL == pCsDataRangeVideo)
    {
        return ERROR_INVALID_PARAMETER;
    }

    return m_pPDDFuncTbl->PDD_SetSensorModeFormat(m_PDDContext, ulPinId, pCsDataRangeVideo);
}


PVOID CCameraDevice::PDDAllocatePinBuffer(ULONG ulPinId)
{
    return m_pPDDFuncTbl->PDD_AllocateBuffer(m_PDDContext, ulPinId);
}


DWORD CCameraDevice::PDDDeAllocatePinBuffer(ULONG ulPinId, PVOID pBuffer)
{
    return m_pPDDFuncTbl->PDD_DeAllocateBuffer(m_PDDContext, ulPinId, pBuffer);
}

DWORD CCameraDevice::PDDRegisterClientBuffer(ULONG ulPinId, PVOID pBuffer)
{
    return m_pPDDFuncTbl->PDD_RegisterClientBuffer(m_PDDContext, ulPinId, pBuffer);
}

DWORD CCameraDevice::PDDUnRegisterClientBuffer(ULONG ulPinId, PVOID pBuffer)
{
    return m_pPDDFuncTbl->PDD_UnRegisterClientBuffer(m_PDDContext, ulPinId, pBuffer);
}

DWORD
CCameraDevice::PDDHandlePinCustomProperties(
    ULONG ulPinId,
    PUCHAR pInBuf,
    DWORD  InBufLen,
    PUCHAR pOutBuf,
    DWORD  OutBufLen,
    PDWORD pdwBytesTransferred
   )
{
    return m_pPDDFuncTbl->PDD_HandleModeCustomProperties(m_PDDContext, ulPinId, pInBuf, InBufLen, pOutBuf, OutBufLen, pdwBytesTransferred);
}


DWORD CCameraDevice::HandleNotification(LONG lModeType, ULONG NotificationId, LPVOID Context)
{

    if (PDD_NOTIFICATION_SAMPLE_SCANNED == NotificationId)
    {
        // We only support PDD_NOTIFICATION_SAMPLE_SCANNED on Still Pin
        // For PDD_NOTIFICATION_SAMPLE_SCANNED, Context can not be equal to NULL
        if (lModeType != STILL || Context == NULL)
        {
            return ERROR_INVALID_PARAMETER;
        }

        if (m_pStrmInstances == NULL || m_pStrmInstances[lModeType].pPinDev == NULL)
        {
            return ERROR_INVALID_OPERATION;
        }

        PCAM_NOTIFICATION_CONTEXT pCamNotificationContext = (PCAM_NOTIFICATION_CONTEXT)Context;
        pCamNotificationContext->Data = m_pStrmInstances[lModeType].pPinDev->PictureNumber() + 1;

        return m_pStrmInstances[lModeType].pPinDev->WriteMessage(FLAG_MSGQ_SAMPLE_SCANNED, Context);
    }
    else if (PDD_NOTIFICATION_ASYNCHRONOUS_FOCUS == NotificationId)
    {
        if (lModeType != ADAPTER || Context == NULL)
        {
            return ERROR_INVALID_PARAMETER;
        }

        return WriteMessage(FLAG_MSGQ_ASYNCHRONOUS_FOCUS, Context);
    }

    return ERROR_INVALID_PARAMETER;
}


DWORD CCameraDevice :: WriteMessage(ULONG flag, LPVOID Context)
{

    if (flag != FLAG_MSGQ_ASYNCHRONOUS_FOCUS)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (NULL == m_hAdapterMessageQueue)
    {
        return ERROR_INVALID_STATE;
    }

    CS_MSGQUEUE_BUFFER csMsgQueueBuf;
    csMsgQueueBuf.CsMsgQueueHeader.Size     = sizeof(CS_MSGQUEUE_HEADER);
    csMsgQueueBuf.CsMsgQueueHeader.Flags    = flag;
    csMsgQueueBuf.CsMsgQueueHeader.Context  = NULL;
    csMsgQueueBuf.pStreamDescriptor         = NULL;

    if (false == WriteMsgQueue(m_hAdapterMessageQueue, reinterpret_cast<LPVOID>(&csMsgQueueBuf),  sizeof(CS_MSGQUEUE_BUFFER), CAM_MSGQ_IMEOUT, 0))
    {
        DEBUGMSG(ZONE_FUNCTION|ZONE_ERROR, (_T("CAM_Function(%08x): WriteMsgQueue returned false while sending notification\r\n"), this));
        return ERROR_WRITE_FAULT;
    }

    if (false == WriteMsgQueue(m_hAdapterMessageQueue, reinterpret_cast<LPVOID>(Context),  sizeof(CAM_NOTIFICATION_CONTEXT), CAM_MSGQ_IMEOUT, 0))
    {
        DEBUGMSG(ZONE_FUNCTION|ZONE_ERROR, (_T("CAM_Function(%08x): WriteMsgQueue returned false while sending notification\r\n"), this));
        return ERROR_WRITE_FAULT;
    }

    return ERROR_SUCCESS;
}
