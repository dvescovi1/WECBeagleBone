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
//  File: tled.cpp
//
/*
#include <windows.h>
#include <ceddk.h>
#include <ceddkex.h>
#include <oal.h>
#include <oalex.h>
#include <omap35xx.h>
#include <tps659xx.h>
*/
#include "tps659xx.h"
#include "tps659xx_internals.h"

#include <initguid.h>
#include <twl.h>
#include "tps659xx_tled.h"

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
    L"TLED", {
        L"Errors",      L"Warnings",    L"Function",    L"Info",
        L"Undefined" ,  L"Undefined",   L"Undefined",   L"Undefined",
        L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined",
        L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined"
    },
    0x0003
};

#endif

//------------------------------------------------------------------------------
//  Local Definitions

#define TLED_DEVICE_COOKIE       'tldD'
#define TLED_INSTANCE_COOKIE     'tldI'

#define MIN_DUTYCYCLE            0
#define MAX_DUTYCYCLE            100

#define LEDEN_LEDON(x)           (1 << x)
#define LEDEN_LEDEXT(x)          (1 << (x + 2))
#define LEDEN_LEDPWM(x)          (1 << (x + 4))
#define LEDEN_LENGTH(x)          (1 << (x + 6))


//------------------------------------------------------------------------------
//  Local Structures

typedef struct {
    DWORD               cookie;
    CRITICAL_SECTION    cs;
    HANDLE              hTWL;
    LONG                refCount;
    DWORD               channelIncrement;           // 16.16
    DWORD               maxChannelValue;
    DWORD               channelCount;
} Device_t;

typedef struct {
    DWORD               cookie;
    Device_t           *pDevice;
    DWORD               channel;
    DWORD               dutyCycle;
} Instance_t;

//------------------------------------------------------------------------------
//  Device registry parameters

static const DEVICE_REGISTRY_PARAM s_deviceRegParams[] = {
    {
        L"Channels", PARAM_DWORD, FALSE, offset(Device_t, channelCount),
        fieldsize(Device_t, channelCount), (VOID*)2
    }, {
        L"MaxValue", PARAM_DWORD, FALSE, offset(Device_t, maxChannelValue),
        fieldsize(Device_t, maxChannelValue), (VOID*)127
    }
};

//------------------------------------------------------------------------------
//  led power offsets
//  Define offset to allow for the code to be written generically, independent
// of offset values

static const DWORD s_PWMAOffset[] = {
    TWL_PWMAOFF,
    TWL_PWMBOFF
};
        

//------------------------------------------------------------------------------
//  Local Functions
BOOL TLD_Deinit(DWORD context);

//------------------------------------------------------------------------------
//
//  Function:  DisableLed
//
//  Disables an LED channel
//
BOOL DisableLed(
    Device_t   *pDevice, 
    DWORD       channel
    )
{
    DEBUGMSG(ZONE_FUNCTION, (
        L"TLD: +DisableLed(0x%08x, 0x%08x)\r\n", 
        pDevice, channel
        ));
    
    // the channel should be always less than the channelCount
    ASSERT(channel < pDevice->channelCount);
        
    UINT8 val;
    BOOL rc = FALSE;

    if (TWLReadRegs(pDevice->hTWL, TWL_LEDEN, &val, sizeof(val)) == FALSE)
        {
        RETAILMSG(ZONE_ERROR, (L"TLD: !ERROR(DisableLed) - "
            L"failed to read triton register 0x%08X\r\n", TWL_LEDEN
            ));
        goto cleanUp;
        }

    val &= ~(LEDEN_LEDON(channel) | LEDEN_LEDPWM(channel));

    if (TWLWriteRegs(pDevice->hTWL, TWL_LEDEN, &val, sizeof(val)) == FALSE)
        {
        RETAILMSG(ZONE_ERROR, (L"TLD: !ERROR(DisableLed) - "
            L"failed to write triton register 0x%08X\r\n", TWL_LEDEN
            ));
        goto cleanUp;
        }

    rc = TRUE;
    
cleanUp:

    DEBUGMSG(ZONE_FUNCTION, (
        L"TLD: -DisableLed(0x%08x, 0x%08x) == %d\r\n", 
        pDevice, channel, rc
        ));
    
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  EnableLed
//
//  Enables an LED channel
//
BOOL EnableLed(
    Device_t   *pDevice, 
    DWORD       channel
    )
{
    DEBUGMSG(ZONE_FUNCTION, (
        L"TLD:  +EnableLed(0x%08x, 0x%08x)\r\n", 
        pDevice, channel
        ));
    
    // the channel should be always less than the channelCount
    ASSERT(channel < pDevice->channelCount);
        
    UINT8 val;
    BOOL rc = FALSE;

    if (TWLReadRegs(pDevice->hTWL, TWL_LEDEN, &val, sizeof(val)) == FALSE)
        {
        RETAILMSG(ZONE_ERROR, (L"TLD: !ERROR(EnableLed) - "
            L"failed to read triton register 0x%08X\r\n", TWL_LEDEN
            ));
        goto cleanUp;
        }

    val |= (LEDEN_LEDON(channel) | LEDEN_LEDPWM(channel));

    if (TWLWriteRegs(pDevice->hTWL, TWL_LEDEN, &val, sizeof(val)) == FALSE)
        {
        RETAILMSG(ZONE_ERROR, (L"TLD: !ERROR(EnableLed) - "
            L"failed to write triton register 0x%08X\r\n", TWL_LEDEN
            ));
        goto cleanUp;
        }

    rc = TRUE;
    
cleanUp:

    DEBUGMSG(ZONE_FUNCTION, (
        L"TLD:  -EnableLed(0x%08x, 0x%08x) == %d\r\n", 
        pDevice, channel, rc
        ));
    
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  SetChannel
//
//  sets the led channel for access
//
BOOL SetChannel(
    DWORD       context, 
    DWORD       channel
    )
{
    DEBUGMSG(ZONE_FUNCTION, (
        L"TLD:  +SetChannel(0x%08x, 0x%08x)\r\n", 
        context, channel
        ));
        
    BOOL rc = FALSE;
    Instance_t *pInstance = (Instance_t*)context;
    
    // do some sanity checks
    if ((pInstance == NULL) || (pInstance->cookie != TLED_INSTANCE_COOKIE))
        {
        RETAILMSG(ZONE_ERROR, (L"TLD: !ERROR(SetChannel) - "
            L"Incorrect context parameter\r\n"
            ));
        goto cleanUp;
        }

    Device_t *pDevice = pInstance->pDevice;
    if (channel >= pDevice->channelCount)
        {
        RETAILMSG(ZONE_ERROR, (L"TLD: !ERROR(SetChannel) - "
            L"Invalid channel\r\n"
            ));
        goto cleanUp;
        }

    pInstance->channel = channel;
    rc = TRUE;
    
cleanUp:

    DEBUGMSG(ZONE_FUNCTION, (
        L"TLD:  -SetChannel(0x%08x, 0x%08x) == %d\r\n", 
        context, channel, rc
        ));
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  SetDutyCycle
//
//  Sets the duty cycle for a led channel
//
BOOL SetDutyCycle(
    DWORD       context, 
    DWORD       value
    )
{
    DEBUGMSG(ZONE_FUNCTION, (
        L"TLD: +SetDutyCycle(0x%08x, 0x%08x)\r\n", 
        context, value
        ));
    
    DWORD offTick;
    BOOL rc = FALSE;
    Instance_t *pInstance = (Instance_t*)context;
    
    // do some sanity checks
    if ((pInstance == NULL) || (pInstance->cookie != TLED_INSTANCE_COOKIE))
        {
        RETAILMSG(ZONE_ERROR, (L"TLD: !ERROR(SetDutyCycle) - "
            L"Incorrect context parameter\r\n"
            ));
        goto cleanUp;
        }
    
    // check for valid value
    // duty cycle value is normalized to be between [0, 100] : 0 == off
    if (value > MAX_DUTYCYCLE)
        {
        RETAILMSG(ZONE_ERROR, (L"TLD: !ERROR(SetDutyCycle) - "
            L"invalid duty cycle value(%d)\r\n", value
            ));
        goto cleanUp;
        }

    // if we got here everything is okay
    rc = TRUE;

    // check if the duty cycle matches current duty cycle
    if (pInstance->dutyCycle == value)
        {
        goto cleanUp;
        }

    // disable led channel, if value is zero then leave led off and exit
    pInstance->dutyCycle = value;
    Device_t *pDevice = pInstance->pDevice;
    DisableLed(pDevice, pInstance->channel);
    if (value == 0)
        {
        goto cleanUp;
        }

    // calculate on/off period based on requested duty cycle,
    // channelIncrement is a fixed point 16.16 number 
    offTick = (pDevice->channelIncrement * value) >> 16;
    TWLWriteRegs(pDevice->hTWL, s_PWMAOffset[pInstance->channel],
        &offTick, 1);
    
    // enable led channel
    EnableLed(pInstance->pDevice, pInstance->channel);
    
cleanUp:    
    DEBUGMSG(ZONE_FUNCTION, (
        L"TLD: -SetDutyCycle(0x%08x, 0x%08x) == %d\r\n", 
        context, value, rc
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
        L"TLD:  +InitializeDevice(0x%08x)\r\n", pDevice
        ));

    BOOL rc = FALSE;
    UINT8 val;

    // disable all leds
    val = 0;
    if (TWLWriteRegs(pDevice->hTWL, TWL_LEDEN, &val, 1) == FALSE)
        {
        RETAILMSG(ZONE_ERROR, (L"TLD:  !ERROR - InitializeDevice "
            L"Failed to initialize LEDEN\r\n"
            ));
        goto cleanUp;
        }

    // leds are always enabled on first tick and use 128 clock
    val = 1;
    if (TWLWriteRegs(pDevice->hTWL, TWL_PWMAON, 
            &val, 1) == FALSE)
        {
        RETAILMSG(ZONE_ERROR, (L"TLD:  !ERROR - InitializeDevice "
            L"Failed to initialize TWL_PWMAON\r\n"
            ));
        goto cleanUp;
        }

    if (TWLWriteRegs(pDevice->hTWL, TWL_PWMBON, 
            &val, 1) == FALSE)
        {
        RETAILMSG(ZONE_ERROR, (L"TLD:  !ERROR - InitializeDevice "
            L"Failed to initialize TWL_PWMBON\r\n"
            ));
        goto cleanUp;
        }

    // calculate how much the incremental value for led channels 
    // as 16.16 fixed point value
    pDevice->channelIncrement = (pDevice->maxChannelValue << 16) / 100;

    rc = TRUE;

cleanUp:

    DEBUGMSG(ZONE_FUNCTION, (
        L"TLD:  -InitializeDevice(0x%08x)\r\n", pDevice
        ));

    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  TLD_Init
//
//  Called by device manager to initialize device.
//
DWORD
TLD_Init(
    LPCTSTR szContext, 
    LPCVOID pBusContext
    )
{
    DWORD rc = (DWORD)NULL;
    Device_t *pDevice;

    UNREFERENCED_PARAMETER(pBusContext);

    DEBUGMSG(ZONE_FUNCTION, (
        L"+TLD_Init(%s, 0x%08x)\r\n", szContext, pBusContext
        ));

    // Create device structure
    pDevice = (Device_t *)LocalAlloc(LPTR, sizeof(Device_t));
    if (pDevice == NULL)
        {
        RETAILMSG(ZONE_ERROR, (L"ERROR: TLD_Init: "
            L"Failed allocate Triton LED driver structure\r\n"
            ));

        goto cleanUp;
        }

    // Read device parameters
    if (GetDeviceRegistryParams(
            szContext, pDevice, dimof(s_deviceRegParams), s_deviceRegParams
            ) != ERROR_SUCCESS)
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: TLD_Init: "
            L"Failed read Triton LED register values\r\n"
            ));
        goto cleanUp;
        }

    // Set cookie
    pDevice->cookie = TLED_DEVICE_COOKIE;

    // new instance always starts with a refCount of 1
    pDevice->refCount = 1;

    // Initialize critical sections
    InitializeCriticalSection(&pDevice->cs);

    // Open Triton device driver
    pDevice->hTWL = TWLOpen();
    if ( pDevice->hTWL == NULL )
        {
        RETAILMSG(ZONE_ERROR, (L"ERROR: TLD_Init: "
            L"Failed open Triton device driver\r\n"
            ));
        goto cleanUp;
        }

    // initialize device with registry settings
    if (InitializeDevice(pDevice) == FALSE)
        {
        DEBUGMSG( ZONE_ERROR, (L"ERROR: TLD_Init: "
            L"Failed to initilialize Triton LED device\r\n"
            ));
        goto cleanUp;
        }

    // Return non-null value
    rc = (DWORD)pDevice;

cleanUp:
    if (rc == 0) TLD_Deinit((DWORD)pDevice);

    DEBUGMSG(ZONE_FUNCTION, (L"-TLD_Init(rc = %d\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  TLD_Deinit
//
//  Called by device manager to uninitialize device.
//
BOOL
TLD_Deinit(
    DWORD context
    )
{
    BOOL rc = FALSE;
    Device_t *pDevice = (Device_t*)context;

    DEBUGMSG(ZONE_FUNCTION, (L"+TLD_Deinit(0x%08x)\r\n", context));

    // Check if we get correct context
    if ((pDevice == NULL) || (pDevice->cookie != TLED_DEVICE_COOKIE))
        {
        DEBUGMSG (ZONE_ERROR, (L"TLD: !ERROR(TLD_Deinit) - "
            L"Incorrect context parameter\r\n"
            ));
        goto cleanUp;
        }

    if (InterlockedIncrement(&pDevice->refCount) != 0)
        {
        RETAILMSG(ZONE_WARN, (L"TWL: !ERROR(TLD_Deinit) - "
            L"Can't delete Triton Led device since refCount is %d",
            pDevice->refCount
            ));
        goto cleanUp;
        }

    //Close handle to TWL driver
    if (pDevice->hTWL != NULL)
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
    DEBUGMSG(ZONE_FUNCTION, (L"-TLD_Deinit(rc = %d)\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  TLD_Open
//
//  Called by device manager to open a device for reading and/or writing.
//
DWORD
TLD_Open(
    DWORD context, 
    DWORD accessCode, 
    DWORD shareMode
    )
{
    DEBUGMSG(ZONE_FUNCTION, (L"+TLD_Open(0x%08x, 0x%08x, 0x%08x)\r\n", 
        context, accessCode, shareMode
        ));

    UNREFERENCED_PARAMETER(accessCode);
    UNREFERENCED_PARAMETER(shareMode);

    DWORD dw = 0;
    Instance_t *pInstance;
    Device_t *pDevice = (Device_t*)context;
    if ((pDevice == NULL) || (pDevice->cookie != TLED_DEVICE_COOKIE))
        {
        DEBUGMSG (ZONE_ERROR, (L"TLD: !ERROR(TLD_Open) - "
            L"Incorrect context parameter\r\n"
            ));
        goto cleanUp;
        }

    // Create device structure
    pInstance = (Instance_t *)LocalAlloc(LPTR, sizeof(Instance_t));
    if (pInstance == NULL)
        {
        RETAILMSG(ZONE_ERROR, (L"TLD: !ERROR(TLD_Open) - "
            L"Failed allocate Triton LED instance structure\r\n"
            ));

        goto cleanUp;
        }

    // increment device refCount
    InterlockedIncrement(&pDevice->refCount);

    // initialize data structure
    pInstance->cookie = TLED_INSTANCE_COOKIE;
    pInstance->pDevice = pDevice;
    pInstance->channel = (DWORD) -1;
    pInstance->dutyCycle = (DWORD) -1;

    dw = (DWORD)pInstance;

cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-TLD_Open(0x%08x, 0x%08x, 0x%08x) == %d\r\n", 
        context, accessCode, shareMode, dw
        ));

    return dw;
}

//------------------------------------------------------------------------------
//
//  Function:  TLD_Close
//
//  This function closes the device context.
//
BOOL
TLD_Close(
    DWORD context
    )
{
    DEBUGMSG(ZONE_FUNCTION, (L"+TLD_Close(0x%08x)\r\n", 
        context
        ));

    Instance_t *pInstance = (Instance_t*)context;
    Device_t *pDevice = pInstance->pDevice;
    if ((pInstance == NULL) || (pInstance->cookie != TLED_INSTANCE_COOKIE))
        {
        DEBUGMSG (ZONE_ERROR, (L"TLD: !ERROR(TLD_Close) - "
            L"Incorrect context parameter\r\n"
            ));
        goto cleanUp;
        }

    // release memory
    LocalFree(pInstance);

    // decrement device refCount
    InterlockedDecrement(&pDevice->refCount);

    RETAILMSG(ZONE_ERROR && !pDevice->refCount, (L"TLD: !ERROR(TLD_Close) - "
        L"refCount is zero on close of instance handle\r\n"
        ));

cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-TLD_Close(0x%08x)\r\n", 
        context
        ));

    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  TLD_IOControl
//
//  This function sends a command to a device.
//
BOOL
TLD_IOControl(
    DWORD context, DWORD code, 
    UCHAR *pInBuffer, DWORD inSize, 
    UCHAR *pOutBuffer, DWORD outSize, 
    DWORD *pOutSize
    )
{
    BOOL rc = FALSE;
    DEVICE_IFC_TLED ifc;
    Instance_t *pInstance = (Instance_t*)context;
    

    DEBUGMSG(ZONE_FUNCTION, (
        L"+TLD_IOControl(0x%08x, 0x%08x, 0x%08x, %d, 0x%08x, %d, 0x%08x)\r\n",
        context, code, pInBuffer, inSize, pOutBuffer, outSize, pOutSize
        ));

    // Check if we get correct context
    if ((pInstance == NULL) || (pInstance->cookie != TLED_INSTANCE_COOKIE))
        {
        RETAILMSG(ZONE_ERROR, (L"ERROR: TLD_IOControl: "
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
                RETAILMSG(ZONE_ERROR, (L"ERROR: TLD_IOControl: "
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
            if (IsEqualGUID(*(GUID*)pInBuffer, DEVICE_IFC_TLED_GUID))
                {
                if (pOutSize != NULL) *pOutSize = sizeof(DEVICE_IFC_TLED);
                if (pOutBuffer == NULL || outSize < sizeof(DEVICE_IFC_TLED))
                    {
                    SetLastError(ERROR_INVALID_PARAMETER);
                    break;
                    }
                ifc.context = context;
                ifc.pfnSetChannel = SetChannel;
                ifc.pfnSetDutyCycle = SetDutyCycle;
                if (!CeSafeCopyMemory(pOutBuffer, &ifc, sizeof(DEVICE_IFC_TLED)))
                    {
                    SetLastError(ERROR_INVALID_PARAMETER);
                    break;
                    }
                rc = TRUE;
                break;
                }
            SetLastError(ERROR_INVALID_PARAMETER);
            break;
        }

cleanUp:

    DEBUGMSG(ZONE_FUNCTION, (L"-TLD_IOControl(rc = %d)\r\n", rc));
    return rc;
}


//------------------------------------------------------------------------------
//
//  Function:  DllMain
//
//  DLL entry point.
//

BOOL WINAPI DllMain(HANDLE hDLL, ULONG Reason, LPVOID pReserved)
{
    UNREFERENCED_PARAMETER(pReserved);
    switch(Reason) 
        {
        case DLL_PROCESS_ATTACH:
            RETAILREGISTERZONES((HMODULE)hDLL);
            DisableThreadLibraryCalls((HMODULE)hDLL);
            break;
        }
    return TRUE;
}




