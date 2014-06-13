// All rights reserved ADENEO EMBEDDED 2010
/*
================================================================================
*             Texas Instruments OMAP(TM) Platform Software
* (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
*
* Use of this software is controlled by the terms and conditions found
* in the license agreement under which this software has been supplied.
*
================================================================================
*/
//
//  File: madc.c
//
#include "omap.h"
#include "ceddkex.h"
#include "tps659xx_internals.h"
#include "tps659xx_bci.h"

#include <triton.h>
#include <initguid.h>
#include "madc.h"
#include <twl.h>


//------------------------------------------------------------------------------
//
//  Global:  dpCurSettings
//
//  Set debug zones names and initial setting for driver
//
#ifndef SHIP_BUILD

#undef ZONE_ERROR
#undef ZONE_WARN
#undef ZONE_FUNCTION
#undef ZONE_INFO
#undef ZONE_IST

#define ZONE_ERROR          DEBUGZONE(0)
#define ZONE_WARN           DEBUGZONE(1)
#define ZONE_FUNCTION       DEBUGZONE(2)
#define ZONE_INFO           DEBUGZONE(3)
#define ZONE_IST            DEBUGZONE(4)

//------------------------------------------------------------------------------
//
//  Global:  dpCurSettings
//
DBGPARAM dpCurSettings = {
    L"MADC", {
        L"Errors",      L"Warnings",    L"Function",    L"Info",
        L"IST" ,        L"Undefined",   L"Undefined",   L"Undefined",
        L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined",
        L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined"
    },
    0x0003
};

#endif

//------------------------------------------------------------------------------
//  Local Definitions

#define MADC_DEVICE_COOKIE       'adcD'
#define FIXEDPOINT_SCALER        2097151     // 2^21 - 1
#define FIXEDPOINT_ROUNDUP       1048576

//------------------------------------------------------------------------------
//  Local Structures

typedef struct {
    DWORD               cookie;
    CRITICAL_SECTION    cs;
    HANDLE              hTWL;
    DWORD               priority256;
    DWORD               VBatRef;
    DWORD               GPChAvgEnable;
    DWORD               UsbBciAvgEnable;
} Device_t;


//------------------------------------------------------------------------------
//  Device registry parameters

static const DEVICE_REGISTRY_PARAM s_deviceRegParams[] = {
    {
        L"Priority256", PARAM_DWORD, FALSE, offset(Device_t, priority256),
        fieldsize(Device_t, priority256), (VOID*)110
    }, {
        L"VBAT-Reference", PARAM_DWORD, FALSE, offset(Device_t, VBatRef),
        fieldsize(Device_t, VBatRef), (VOID*)0x8A2
    }, {
        L"GPChannelAveraging", PARAM_DWORD, FALSE, offset(Device_t, GPChAvgEnable),
        fieldsize(Device_t, GPChAvgEnable), (VOID*)0x0
    },{
        L"USB-BCI Averaging", PARAM_DWORD, FALSE, offset(Device_t, UsbBciAvgEnable),
        fieldsize(Device_t, UsbBciAvgEnable), (VOID*)0x0
    }
};

//------------------------------------------------------------------------------
//  ADC scalars
//
// value is determined using the following equation
// scale = 1.5 / (2^10 - 1) * R where R is dependent on channel
// scale is then converted to a 0.21 fixed point value.  
static const DWORD s_ConversionValue[] = 
{
    3075,                           // 0) 1.5 / (2^10 - 1) * R where R = 1
    3075,                           // 1) 1.5 / (2^10 - 1) * R where R = 1
    5125,                           // 2) 1.5 / (2^10 - 1) * R where R = .6
    5125,                           // 3) 1.5 / (2^10 - 1) * R where R = .6
    5125,                           // 4) 1.5 / (2^10 - 1) * R where R = .6
    5125,                           // 5) 1.5 / (2^10 - 1) * R where R = .6
    5125,                           // 6) 1.5 / (2^10 - 1) * R where R = .6
    5125,                           // 7) 1.5 / (2^10 - 1) * R where R = .6
    14350,                          // 8) 1.5 / (2^10 - 1) * R where R = 3/14
    9225,                           // 9) 1.5 / (2^10 - 1) * R where R = 1/3
    3075,                           // 10) undefined assume R = 1.0
    15375,                          // 11) 1.5 / (2^10 - 1) * R where R = 0.2
    12300,                          // 12) 1.5 / (2^10 - 1) * R where R = 0.25
    3075,                           // 13) undefined assume R = 1.0
    3075,                           // 14) undefined assume R = 1.0
    6765                            // 15) 1.5 / (2^10 - 1) * R where R = 5/11
};

static const int s_ConversionLookup[] = 
{
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,  // GP lines
    1, 8, 10, 11, 12                                       // BCI lines
};

//------------------------------------------------------------------------------
//  Local Functions
BOOL ADC_Deinit(DWORD context);

//------------------------------------------------------------------------------
//
//  Function:  ConvertToVolts
//
//  converts a given set of raw values to millivolts.  The equation used
// to caculate the millivolts depends on the channel
//
DWORD ConvertToVolts(
    DWORD       context, 
    DWORD       channels, 
    DWORD      *prgValues,
    DWORD      *prgResults,
    DWORD       count
    )
{
    DEBUGMSG(ZONE_FUNCTION, (
        L"ADC: +ConvertToVolts(0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x)\r\n", 
        context, channels, prgValues, prgResults, count
        ));
    
    int i;
    UINT64 dwTemp;
    DWORD dwCount = count;
    Device_t *pDevice = (Device_t*)context;

    // do some sanity checks
    if ((pDevice == NULL) || (pDevice->cookie != MADC_DEVICE_COOKIE) || 
        prgResults == NULL || count == 0)
        {
        DEBUGMSG(ZONE_ERROR, (L"ADC: !ERROR - ReadValue: "
            L"Incorrect context parameter\r\n"
            ));
        goto cleanUp;
        }

    // convert raw values to millivolts
    i = 0;
    channels &= 0x1FFFFF;
    while (channels && dwCount > 0)
        {
        if (channels & 1)
            {
            dwTemp = (*prgValues & 0x3FF) * 
                        s_ConversionValue[s_ConversionLookup[i]];   //10.21(V)

            dwTemp *= 1000;                                         //20.21(mV)
            dwTemp += FIXEDPOINT_ROUNDUP;                           //20.21(mV)
            *prgResults = (DWORD)(dwTemp >> 21);                    //20.0(mV)

            // next entry
            ++prgValues;
            ++prgResults;
            --dwCount;
            }        
        ++i;
        channels >>= 1;
        }
    
    count -= dwCount;
    
cleanUp:

    DEBUGMSG(ZONE_FUNCTION, (
        L"ADC: -ConvertToVolts(0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x)\r\n", 
        context, channels, prgValues, prgResults, count
        ));
    return count;
}


//------------------------------------------------------------------------------
//
//  Function:  ReadBCILines
//
//  Reads the general purpose lines
//
DWORD ReadBCILines(
    Device_t   *pDevice, 
    UINT16      channels, 
    DWORD      *prgResults, 
    DWORD       count
    )
{
    DEBUGMSG(ZONE_FUNCTION, (
        L"ADC: +ReadBCILines(0x%08x, 0x%08x, 0x%08x, 0x%08x)\r\n", 
        pDevice, channels, prgResults, count
        ));
    
    UINT8 val;
    UINT8 counter;
    UINT16 rgBCIRegisters[5];
    UINT16 *pBCIRegisters = rgBCIRegisters;
    DWORD dwReadCount = 0;

    // waits for conversion complete
    val = 0;
    counter = 0;
    while (counter++ < 0xFF)
        {
        TWLReadRegs(pDevice->hTWL, TWL_CTRL_SW1, &val, 1);
        if (val & TWL_MADC_CTRL_SW_EOC_BCI)
            {
            break;
            }
        StallExecution(10);
        }

    // read to results of the general purpose lines    
    if (TWLReadRegs(pDevice->hTWL, TWL_BCICH0_LSB, rgBCIRegisters, 
        sizeof(rgBCIRegisters)) == FALSE)
        {
        DEBUGMSG(ZONE_ERROR, (L"ADC: !ERROR - ReadBCILines: "
            L"Failed writing start conversion\r\n"
            ));
        goto cleanUp;
        }

    // copy raw results into return buffer
    counter = 0;
    while (counter < count && channels)
        {
        if (channels & 1)
            {
            // read raw value
            *prgResults = *pBCIRegisters >> 6;

            // move to next result and increment counters
            ++prgResults;
            ++counter;
            }
        pBCIRegisters++;
        channels >>= 1;
        }
    
    dwReadCount = counter;

cleanUp:    
    DEBUGMSG(ZONE_FUNCTION, (
        L"ADC: -ReadBCILines(0x%08x, 0x%08x, 0x%08x, 0x%08x)\r\n", 
        pDevice, channels, prgResults, count
        ));

    return dwReadCount;
}


//------------------------------------------------------------------------------
//
//  Function:  ReadGeneralPurposeLines
//
//  Reads the general purpose lines
//
DWORD ReadGeneralPurposeLines(
    Device_t   *pDevice, 
    UINT16      channels, 
    DWORD      *prgResults, 
    DWORD       count
    )
{
    DEBUGMSG(ZONE_FUNCTION, (
        L"ADC: +ReadGeneralPurposeLines(0x%08x, 0x%08x, 0x%08x, 0x%08x)\r\n", 
        pDevice, channels, prgResults, count
        ));
    
    UINT8 val;
    UINT8 counter;
    UINT16 rgGPRegisters[16];
    UINT16 *pGPRegisters = rgGPRegisters;
    DWORD dwReadCount = 0;

    // waits for conversion complete
    val = 0;
    counter = 0;
    while (counter++ < 0xFF)
        {
        TWLReadRegs(pDevice->hTWL, TWL_CTRL_SW1, &val, 1);
        if (val & TWL_MADC_CTRL_SW_EOC_SW1)
            {
            break;
            }
        StallExecution(10);
        }

    // read to results of the general purpose lines    
    if (TWLReadRegs(pDevice->hTWL, TWL_GPCH0_LSB, rgGPRegisters, 
        sizeof(rgGPRegisters)) == FALSE)
        {
        DEBUGMSG(ZONE_ERROR, (L"ADC: !ERROR - ReadGeneralPurposeLines: "
            L"Failed writing start conversion\r\n"
            ));
        goto cleanUp;
        }

    // copy raw results into return buffer
    counter = 0;
    while (counter < count && channels)
        {
        if (channels & 1)
            {
            // read raw value
            *prgResults = *pGPRegisters >> 6;

            // move to next result and increment counter
            ++prgResults;
            ++counter;
            }
        pGPRegisters++;
        channels >>= 1;
        }
    
    // return number of values read
    dwReadCount = counter;

cleanUp:    
    DEBUGMSG(ZONE_FUNCTION, (
        L"ADC: -ReadGeneralPurposeLines(0x%08x, 0x%08x, 0x%08x, 0x%08x)\r\n", 
        pDevice, channels, prgResults, count
        ));

    return dwReadCount;
}


//------------------------------------------------------------------------------
//
//  Function:  ReadValue
//
//  Entry point to start process of reading the ADC value for any given 
// channel
//
DWORD ReadValue(
    DWORD context, 
    DWORD mask, 
    DWORD *prgResults, 
    DWORD count
    )
{
    DEBUGMSG(ZONE_FUNCTION, (
        L"ADC: +ReadValue(0x%08x, 0x%08x, 0x%08x, 0x%08x)\r\n", 
        context, mask, prgResults, count
        ));

    UINT8 pwr;
    UINT8 val;
    int readCount;
    UINT16 channels;
    DWORD rc = 0;
    BOOL bLocked = FALSE;
    Device_t *pDevice = (Device_t*)context;

    // do some sanity checks
    if ((pDevice == NULL) || (pDevice->cookie != MADC_DEVICE_COOKIE) || 
        prgResults == NULL || count == 0)
        {
        DEBUGMSG(ZONE_ERROR, (L"ADC: !ERROR - ReadValue: "
            L"Incorrect context parameter\r\n"
            ));
        goto cleanUp;
        }

    // serialize access
    bLocked = TRUE;
    EnterCriticalSection(&pDevice->cs);

    // power on MADC
    pwr = MADCON;
    if (TWLWriteRegs(pDevice->hTWL, TWL_CTRL1, &pwr, 1) == FALSE)
        {
        DEBUGMSG(ZONE_ERROR, (L"ADC: !ERROR - ReadValue: "
            L"Unable to enable MADCON\r\n"
            ));
        goto cleanUp;
        }

    // select the channels for conversion 
    channels = (UINT16)(mask & 0xFFFF);
    if (TWLWriteRegs(pDevice->hTWL, TWL_SW1SELECT_LSB, &channels, 2) == FALSE)
        {
        DEBUGMSG(ZONE_ERROR, (L"ADC: !ERROR - ReadGeneralPurposeLines: "
            L"Failed writing sw1 selection\r\n"
            ));
        goto cleanUp;
        }

    // start conversion
    val = TWL_MADC_CTRL_SW_TOGGLE;
    if (TWLWriteRegs(pDevice->hTWL, TWL_CTRL_SW1, &val, 1) == FALSE)
        {
        DEBUGMSG(ZONE_ERROR, (L"ADC: !ERROR - ReadGeneralPurposeLines: "
            L"Failed writing start conversion\r\n"
            ));
        goto cleanUp;
        }


    // first read general purpose lines
    channels = (UINT16)(mask & 0xFFFF);
    if (channels)
        {
        readCount = ReadGeneralPurposeLines(pDevice, channels, 
                            prgResults, count);
        if (readCount == 0)
            {
            DEBUGMSG(ZONE_ERROR, (L"ADC: !ERROR - ReadValue: "
                L"Unable to read general purpose lines\r\n"
                ));
            goto cleanUp;
            }

        // move pointer to next set of 
        rc += readCount;
        prgResults += readCount;
        count -= readCount;
        }

    // next read bci lines
    channels = (UINT16)(mask >> 16);
    if (channels != 0 && count > 0)
        {
        readCount = ReadBCILines(pDevice, channels, 
                            prgResults, count);
        if (readCount == 0)
            {
            DEBUGMSG(ZONE_ERROR, (L"ADC: !ERROR - ReadValue: "
                L"Unable to read BCI lines\r\n"
                ));
            goto cleanUp;
            }
        rc += readCount;
        }

cleanUp:    
    if (bLocked == TRUE) 
        {
        pwr = 0;
        TWLWriteRegs(pDevice->hTWL, TWL_CTRL1, &pwr, 1);
        LeaveCriticalSection(&pDevice->cs);
        }

    DEBUGMSG(ZONE_FUNCTION, (
        L"ADC: -ReadValue(0x%08x, 0x%08x, 0x%08x, 0x%08x)\r\n", 
        pDevice, mask, prgResults, count
        ));

    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  InitializeDevice
//
//  Initializes the MADC registers for USB-VBat reference value and
//  channel averaging.
//
BOOL InitializeDevice(
    Device_t *pDevice
    )
{
    DEBUGMSG(ZONE_FUNCTION, (
        L"ADC: +InitializeDevice(0x%08x)\r\n", pDevice
        ));

    BOOL rc = FALSE;

    if (TWLWriteRegs(pDevice->hTWL, TWL_SW1AVERAGE_LSB, 
            &(pDevice->GPChAvgEnable), 2) == FALSE)
        {
        DEBUGMSG(ZONE_ERROR, (L"ADC: !ERROR - InitializeDevice "
            L"Failed to initialize GP average registers\r\n"
            ));
        goto cleanUp;
        }

    if (TWLWriteRegs(pDevice->hTWL, TWL_BCI_USBAVERAGE, 
            &(pDevice->UsbBciAvgEnable), 1) == FALSE)
        {
        DEBUGMSG(ZONE_ERROR, (L"ADC: !ERROR - InitializeDevice "
            L"Failed to initialize BCI-USB average registers\r\n"
            ));
        goto cleanUp;
        }

    // format to appropriate value
    pDevice->VBatRef <<= 6;
    if (TWLWriteRegs(pDevice->hTWL, TWL_USBREF_LSB, 
            &(pDevice->VBatRef), 2) == FALSE)
        {
        DEBUGMSG(ZONE_ERROR, (L"ADC: !ERROR - InitializeDevice "
            L"Failed to initialize VBAT reference registers\r\n"
            ));
        goto cleanUp;
        }

    rc = TRUE;

cleanUp:

    DEBUGMSG(ZONE_FUNCTION, (
        L"ADC: -InitializeDevice(0x%08x)\r\n", pDevice
        ));

    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  ADC_Init
//
//  Called by device manager to initialize device.
//
DWORD
ADC_Init(
    LPCTSTR szContext, 
    LPCVOID pBusContext
    )
{
    DWORD rc = (DWORD)NULL;
    Device_t *pDevice;

    UNREFERENCED_PARAMETER(pBusContext);
    UNREFERENCED_PARAMETER(szContext);  

    DEBUGMSG(ZONE_FUNCTION, (
        L"+ADC_Init(%s, 0x%08x)\r\n", szContext, pBusContext
        ));

    // Create device structure
    pDevice = (Device_t *)LocalAlloc(LPTR, sizeof(Device_t));
    if (pDevice == NULL)
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: ADC_Init: "
            L"Failed allocate MADC driver structure\r\n"
            ));

        goto cleanUp;
        }

    // Set cookie
    pDevice->cookie = MADC_DEVICE_COOKIE;

    // Initialize critical sections
    InitializeCriticalSection(&pDevice->cs);

    // Open Triton device driver
    pDevice->hTWL = TWLOpen();
    if ( pDevice->hTWL == NULL )
        {
        DEBUGMSG( ZONE_ERROR, (L"ERROR: ADC_Init: "
            L"Failed open Triton device driver\r\n"
            ));
        goto cleanUp;
        }

    // initialize device with registry settings
    if (InitializeDevice(pDevice) == FALSE)
        {
        DEBUGMSG( ZONE_ERROR, (L"ERROR: ADC_Init: "
            L"Failed to initilialize MADC device\r\n"
            ));
        goto cleanUp;
        }

    // Return non-null value
    rc = (DWORD)pDevice;

cleanUp:
    if (rc == 0) ADC_Deinit((DWORD)pDevice);

    DEBUGMSG(ZONE_FUNCTION, (L"-ADC_Init(rc = %d\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  ADC_Deinit
//
//  Called by device manager to uninitialize device.
//
BOOL
ADC_Deinit(
    DWORD context
    )
{
    BOOL rc = FALSE;
    Device_t *pDevice = (Device_t*)context;

    DEBUGMSG(ZONE_FUNCTION, (L"+ADC_Deinit(0x%08x)\r\n", context));

    // Check if we get correct context
    if ((pDevice == NULL) || (pDevice->cookie != MADC_DEVICE_COOKIE))
        {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: ADC_Deinit: "
            L"Incorrect context parameter\r\n"
            ));
        goto cleanUp;
        }

    //Close handle to TWL driver
    if ( pDevice->hTWL != NULL )
        {
        CloseHandle(pDevice->hTWL);
        pDevice->hTWL = NULL;
        }

    // Delete critical sections
    DeleteCriticalSection(&pDevice->cs);

    // Free device structure
    LocalFree(pDevice);

    // Done
    rc = TRUE;

cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-ADC_Deinit(rc = %d)\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  ADC_Open
//
//  Called by device manager to open a device for reading and/or writing.
//
DWORD
ADC_Open(
    DWORD context, 
    DWORD accessCode, 
    DWORD shareMode
    )
{
    UNREFERENCED_PARAMETER(context);
    UNREFERENCED_PARAMETER(accessCode);
    UNREFERENCED_PARAMETER(shareMode);
    return context;
}

//------------------------------------------------------------------------------
//
//  Function:  ADC_Close
//
//  This function closes the device context.
//
BOOL
ADC_Close(
    DWORD context
    )
{
    UNREFERENCED_PARAMETER(context);
    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  ADC_IOControl
//
//  This function sends a command to a device.
//
BOOL
ADC_IOControl(
    DWORD context, DWORD code, 
    UCHAR *pInBuffer, DWORD inSize, 
    UCHAR *pOutBuffer, DWORD outSize, 
    DWORD *pOutSize
    )
{
    BOOL rc = FALSE;
    DEVICE_IFC_MADC ifc;
    IOCTL_MADC_CONVERTTOVOLTS_IN *pConvertToVolts;
    Device_t *pDevice = (Device_t*)context;

    DEBUGMSG(ZONE_FUNCTION, (
        L"+ADC_IOControl(0x%08x, 0x%08x, 0x%08x, %d, 0x%08x, %d, 0x%08x)\r\n",
        context, code, pInBuffer, inSize, pOutBuffer, outSize, pOutSize
        ));

    // Check if we get correct context
    if ((pDevice == NULL) || (pDevice->cookie != MADC_DEVICE_COOKIE))
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: ADC_IOControl: "
            L"Incorrect context parameter\r\n"
            ));
        goto cleanUp;
        }

    switch (code)
        {
        case IOCTL_DDK_GET_DRIVER_IFC:
            // We can give interface only to our peer in device process
            if (GetCurrentProcessId() != (DWORD)GetCallerProcess())
                {
                DEBUGMSG(ZONE_ERROR, (L"ERROR: ADC_IOControl: "
                    L"IOCTL_DDK_GET_DRIVER_IFC can be called only from "
                    L"device process (caller process id 0x%08x)\r\n",
                    GetCallerProcess()
                    ));
                SetLastError(ERROR_ACCESS_DENIED);
                break;
                }
            // Check input parameters
            if ((pInBuffer == NULL) || (inSize < sizeof(GUID)))
                {
                SetLastError(ERROR_INVALID_PARAMETER);
                break;
                }
            if (IsEqualGUID(*(GUID*)pInBuffer, DEVICE_IFC_MADC_GUID))
                {
                if (pOutSize != NULL) *pOutSize = sizeof(DEVICE_IFC_MADC);
                if (pOutBuffer == NULL || outSize < sizeof(DEVICE_IFC_MADC))
                    {
                    SetLastError(ERROR_INVALID_PARAMETER);
                    break;
                    }
                ifc.context = context;
                ifc.pfnReadValue = ReadValue;
                ifc.pfnConvertToVolts = ConvertToVolts;
                if (!CeSafeCopyMemory(pOutBuffer, &ifc, sizeof(DEVICE_IFC_MADC)))
                    {
                    SetLastError(ERROR_INVALID_PARAMETER);
                    break;
                    }
                rc = TRUE;
                break;
                }
            SetLastError(ERROR_INVALID_PARAMETER);
            break;

        case IOCTL_MADC_READVALUE:
            if (pInBuffer == NULL || pOutBuffer == NULL || 
                inSize != sizeof(DWORD) || outSize < sizeof(DWORD))
                {
                DEBUGMSG(ZONE_ERROR, (L"ERROR: ADC_IOControl: "
                    L"Invalid parameters for IOCTL_MADC_READVOLTS\r\n"
                    ));
                SetLastError(ERROR_INVALID_PARAMETER);
                break;
                }

            *pOutSize = ReadValue(context, *(DWORD*)pInBuffer, (DWORD*)pOutBuffer, 
                            outSize/sizeof(DWORD));

            if (*pOutSize == 0 && *(DWORD*)pInBuffer)
                {
                DEBUGMSG(ZONE_ERROR, (L"ERROR: ADC_IOControl: "
                    L"Failed to retrieve voltage readings\r\n"
                    ));
                break;
                }
            *pOutSize *= sizeof(DWORD);
            rc = TRUE;
            break;

        case IOCTL_MADC_CONVERTTOVOLTS:
            if (pInBuffer == NULL || pOutBuffer == NULL || 
                inSize != sizeof(IOCTL_MADC_CONVERTTOVOLTS_IN))
                {
                DEBUGMSG(ZONE_ERROR, (L"ERROR: ADC_IOControl: "
                    L"Invalid parameters for IOCTL_MADC_CONVERTTOVOLTS\r\n"
                    ));
                SetLastError(ERROR_INVALID_PARAMETER);
                break;
                }

            pConvertToVolts = (IOCTL_MADC_CONVERTTOVOLTS_IN*)pInBuffer;

            if (pConvertToVolts->count * sizeof(DWORD) != outSize)
                {
                DEBUGMSG(ZONE_ERROR, (L"ERROR: ADC_IOControl: "
                    L"Invalid parameters for IOCTL_MADC_CONVERTTOVOLTS\r\n"
                    ));
                SetLastError(ERROR_INVALID_PARAMETER);
                break;
                }

            *pOutSize = ConvertToVolts(context, pConvertToVolts->mask,
                            pConvertToVolts->pdwValues, (DWORD*)pOutBuffer, 
                            pConvertToVolts->count);
            *pOutSize *= sizeof(DWORD);
            rc = TRUE;
            break;
        }

cleanUp:

    DEBUGMSG(ZONE_FUNCTION, (L"-ADC_IOControl(rc = %d)\r\n", rc));
    return rc;
}


//------------------------------------------------------------------------------
//
//  Function:  DllMain
//
//  DLL entry point.
//

BOOL WINAPI DllMain(HANDLE hDLL, ULONG Reason, LPVOID Reserved)
{
    UNREFERENCED_PARAMETER(Reserved);
    switch(Reason) 
        {
        case DLL_PROCESS_ATTACH:
            RETAILREGISTERZONES((HMODULE)hDLL);
            DisableThreadLibraryCalls((HMODULE)hDLL);
            break;
        }
    return TRUE;
}




