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
//  File: Am3xx_gpio.cpp
//
//#include "omap.h"
#include "bsp_cfg.h"
#include "ceddkex.h"
#include "oalex.h"
#include <oal_clock.h>
#include "soc_cfg.h"
#include "am3xx_gpio.h"
#include "sdk_gpio.h"
#ifdef OAL
#include "oal_alloc.h"
#endif
#include <nkintr.h>

#ifdef DEVICE
#include <initguid.h>
#endif
#include "gpio_ioctls.h"


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
#ifdef DEVICE
DBGPARAM dpCurSettings = {
    L"OMAP GPIO", {
        L"Errors",      L"Warnings",    L"Function",    L"Info",
            L"IST",         L"Undefined",   L"Undefined",   L"Undefined",
            L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined",
            L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined"
    },
    0x0003
};
#endif
#endif

//------------------------------------------------------------------------------
//  Local Definitions

#define GPIO_BITS_PER_BANK      (0x1F)

#define GPIO_BANK(x)            (x >> 5)
#define GPIO_BIT(x)             (x & GPIO_BITS_PER_BANK)

//------------------------------------------------------------------------------
//  Local Structures

typedef struct 
{
    DWORD powerEnabled;
    CRITICAL_SECTION pCs;
    AM3XX_GPIO_REGS *ppGpioRegs; 
    OMAP_DEVICE DeviceID;
}Am3xxGpioBank_t;

typedef struct {
    DWORD cookie;
    DWORD nbBanks;
    BOOL fPostInit;
    Am3xxGpioBank_t *bank;
} Am3xxGpioDevice_t;

//------------------------------------------------------------------------------
//  Local Functions

// Init function
static BOOL Am3xxGpioInit(LPCTSTR szContext, HANDLE *phContext, UINT *pGpioCount);
static BOOL Am3xxGpioPostInit(HANDLE hContext);
static BOOL Am3xxGpioDeinit(HANDLE hContext);
static BOOL Am3xxGpioSetMode(HANDLE hContext, UINT id, UINT mode);
static DWORD Am3xxGpioGetMode(HANDLE hContext, UINT id);
static BOOL Am3xxGpioPullup(HANDLE hcontext, UINT id, UINT enable);
static BOOL Am3xxGpioPulldown(HANDLE hcontext, UINT id, UINT enable);
static DWORD Am3xxGpioGetSystemIrq(HANDLE hcontext, UINT id);
static BOOL Am3xxGpioInterruptWakeUp(HANDLE hContext, DWORD id, DWORD sysintr, BOOL fEnabled);
static BOOL Am3xxGpioInterruptInitialize(HANDLE hContext, DWORD id, DWORD* sysintr, HANDLE hEvent);
static VOID Am3xxGpioInterruptDone(HANDLE hcontext, DWORD id,DWORD sysintr);
static VOID Am3xxGpioInterruptDisable(HANDLE hcontext, DWORD id,DWORD sysintr);
static VOID Am3xxGpioInterruptMask(HANDLE hContext, DWORD id, DWORD sysintr, BOOL fDisable);
static BOOL Am3xxGpioSetBit(HANDLE hContext, UINT id);
static BOOL Am3xxGpioClrBit(HANDLE hContext, UINT id);
static DWORD Am3xxGpioGetBit(HANDLE hContext, UINT id);
#ifdef DEVICE
static void Am3xxGpioPowerUp(HANDLE hContext);
static void Am3xxGpioPowerDown(HANDLE hContext);
#endif
static BOOL Am3xxGpioIoControl(HANDLE hContext, UINT code,
                              UCHAR *pinVal, UINT inSize, UCHAR *poutVal,
                              UINT outSize, DWORD *pOutSize);

//------------------------------------------------------------------------------
//  exported function table
extern "C" DEVICE_IFC_GPIO Am3xx_Gpio;

DEVICE_IFC_GPIO Am3xx_Gpio = {
    0,
    Am3xxGpioInit,
    Am3xxGpioPostInit,
    Am3xxGpioDeinit,
    Am3xxGpioSetBit,   
    Am3xxGpioClrBit,     
    Am3xxGpioGetBit,    
    Am3xxGpioSetMode,
    Am3xxGpioGetMode,
    Am3xxGpioPullup,
    Am3xxGpioPulldown,
    Am3xxGpioIoControl,
    NULL,NULL,NULL,NULL,
};


#ifdef DEVICE
//------------------------------------------------------------------------------
//
//  Function:  SetGpioBankPowerState
//
//  Called by client to initialize device.
//
static
void
SetGpioBankPowerState(
                      Am3xxGpioDevice_t *pDevice,
                      UINT id,
                      CEDEVICE_POWER_STATE state
                      )
{
    // determine GPIO bank
    UINT bit = GPIO_BIT(id);
    UINT bank = GPIO_BANK(id);
    UINT prevPowerState = pDevice->bank[bank].powerEnabled;

    if (state < D3)
        pDevice->bank[bank].powerEnabled |= bit;
    else
        pDevice->bank[bank].powerEnabled &= ~bit;

    // check if power needs to be enabled/disabled for the gpio bank
    if (!prevPowerState != !pDevice->bank[bank].powerEnabled)
    {
        if (pDevice->bank[bank].powerEnabled == 0)
        {
            EnableDeviceClocks(pDevice->bank[bank].DeviceID, FALSE);
        }
        else
        {
            EnableDeviceClocks(pDevice->bank[bank].DeviceID, TRUE);
        }
    }
}
#endif

//------------------------------------------------------------------------------
//
//  Function:  InternalSetGpioBankPowerState
//
//  Called by client to initialize device.
//
void
InternalSetGpioBankPowerState(
                              Am3xxGpioDevice_t *pDevice,
                              UINT id,
                              CEDEVICE_POWER_STATE state
                              )
{
    // determine GPIO bank
    UINT bank = GPIO_BANK(id);

    // check if power is enabled by client
    if (pDevice->bank[bank].powerEnabled != 0) return;

    if (state < D3)
    {
        EnableDeviceClocks( pDevice->bank[bank].DeviceID, TRUE );
    }
    else
    {
		EnableDeviceClocks( pDevice->bank[bank].DeviceID, FALSE );
    }
}


//------------------------------------------------------------------------------
//
//  Function:  Am3xxGpioInit
//
//  Called by client to initialize device.
//
BOOL
Am3xxGpioInit(
             LPCTSTR szContext,
             HANDLE *phContext,
             UINT   *pGpioCount
             )
{
    BOOL rc = FALSE;
    Am3xxGpioDevice_t *pDevice = NULL;
    PHYSICAL_ADDRESS pa;
#ifdef DEVICE
	DWORD size;
#endif
    UINT8 i;

	UNREFERENCED_PARAMETER(szContext);

    DEBUGMSG(ZONE_FUNCTION, (
        L"+Am3xxGpioInit(%s)\r\n", szContext
        ));

    // Create device structure
    pDevice = (Am3xxGpioDevice_t *)LocalAlloc(LPTR, sizeof(Am3xxGpioDevice_t));
    if (pDevice == NULL)
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: Am3xxGpioInit: "
            L"Failed allocate GPIO driver structure\r\n"
            ));
        goto cleanUp;
    }
    memset(pDevice, 0, sizeof(Am3xxGpioDevice_t));
    // Set cookie
    pDevice->cookie = GPIO_DEVICE_COOKIE;
    // Get the number of for this SOC
    i=1;
    while (SOCGetGPIODeviceByBank(i++) != OMAP_DEVICE_NONE)
    {
        pDevice->nbBanks++;
    }
    // Allocate the bank structure
    pDevice->bank = (Am3xxGpioBank_t*) LocalAlloc(LPTR,sizeof(Am3xxGpioBank_t)*pDevice->nbBanks);
    if (pDevice->bank == NULL)
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: Am3xxGpioInit: "
            L"Failed allocate GPIO driver bank structures\r\n"
            ));
        goto cleanUp;
    }
    memset(pDevice->bank, 0, sizeof(Am3xxGpioBank_t)*pDevice->nbBanks);

    for (i = 0; i < pDevice->nbBanks; i++)
    {
        // Get device clock
        pDevice->bank[i].DeviceID = SOCGetGPIODeviceByBank(i+1);
        // Map GPIO registers  
        pa.QuadPart = GetAddressByDevice(pDevice->bank[i].DeviceID);
#ifdef DEVICE		
        size = sizeof(AM3XX_GPIO_REGS);
        pDevice->bank[i].ppGpioRegs = (AM3XX_GPIO_REGS*)MmMapIoSpace(pa, size, FALSE);
#endif
#ifdef OAL
		pDevice->bank[i].ppGpioRegs = (AM3XX_GPIO_REGS*)OALPAtoVA(pa.LowPart, FALSE);
#endif
        if (pDevice->bank[i].ppGpioRegs == NULL)
        {
            DEBUGMSG(ZONE_ERROR, (L"ERROR: Am3xxGpioInit: "
                L"Failed map GIO%d controller registers\r\n",i
                ));
            goto cleanUp;
        }
    }

    // Return non-null value
    rc = TRUE;
    *phContext = (HANDLE)pDevice;
    *pGpioCount = pDevice->nbBanks;

cleanUp:
    if (rc == FALSE) Am3xxGpioDeinit((HANDLE)pDevice);
    DEBUGMSG(ZONE_FUNCTION, (L"-Am3xxGpioInit()\r\n"));
    return rc;
}
//------------------------------------------------------------------------------
//
//  Function:  Am3xxGpioPostInit
//
//  Called by the dispactehr to complete the init
//
static BOOL Am3xxGpioPostInit(HANDLE hContext)
{
    UINT8 i;
    Am3xxGpioDevice_t *pDevice = (Am3xxGpioDevice_t*)hContext;

    // Initialize critical sections
    for (i = 0; i < pDevice->nbBanks; i++)
    {
        InitializeCriticalSection(&pDevice->bank[i].pCs);
    }
    pDevice->fPostInit = TRUE;
    return TRUE;
}
//------------------------------------------------------------------------------
//
//  Function:  Am3xxGpioDeinit
//
//  Called by device manager to uninitialize device.
//
BOOL
Am3xxGpioDeinit(
               HANDLE context
               )
{
    BOOL rc = FALSE;
    Am3xxGpioDevice_t *pDevice = (Am3xxGpioDevice_t*)context;
    UINT8 i = 0;

    DEBUGMSG(ZONE_FUNCTION, (L"+Am3xxGpioDeinit(0x%08x)\r\n", context));

    // Check if we get correct context
    if ((pDevice == NULL) || (pDevice->cookie != GPIO_DEVICE_COOKIE))
    {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: Am3xxGpioDeinit: "
            L"Incorrect context parameter\r\n"
            ));
        goto cleanUp;
    }

    // Delete critical sections
    if (pDevice->fPostInit)
    {
        for (i = 0; i < pDevice->nbBanks; i++)
            DeleteCriticalSection(&pDevice->bank[i].pCs);
    }

#ifdef DEVICE		
    // Unmap module registers
    for (i = 0 ; i < pDevice->nbBanks; i++)
    {
        if (pDevice->bank[i].ppGpioRegs != NULL)
        {
            DWORD size = sizeof(AM3XX_GPIO_REGS);
            MmUnmapIoSpace((VOID*)pDevice->bank[i].ppGpioRegs, size);
        }
    }
#endif
    // Free banks structures    
    LocalFree(pDevice->bank);

    // Free device structure
    LocalFree(pDevice);

    // Done
    rc = TRUE;

cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-Am3xxGpioDeinit()\r\n"));
    return rc;
}


//------------------------------------------------------------------------------
//
//  Function: Am3xxGpioSetMode
//
BOOL
Am3xxGpioSetMode(
                HANDLE context,
                UINT id,
                UINT mode
                )
{
    BOOL rc = FALSE;
    UINT bit = GPIO_BIT(id);
    UINT bank = GPIO_BANK(id);
    Am3xxGpioDevice_t *pDevice = (Am3xxGpioDevice_t*)context;

    // Check if we get correct context
    if ((pDevice == NULL) || (pDevice->cookie != GPIO_DEVICE_COOKIE))
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: Am3xxGpioSetMode: "
            L"Incorrect context\r\n"
            ));
        goto cleanUp;
    }

    if (id < pDevice->nbBanks*32)
    {
        UINT32 mask = 1 << (bit);
        CRITICAL_SECTION *pCs = &pDevice->bank[bank].pCs;
        AM3XX_GPIO_REGS *pGpio = pDevice->bank[bank].ppGpioRegs;

        if (pDevice->fPostInit) EnterCriticalSection(pCs);
        InternalSetGpioBankPowerState(pDevice, id, D0);

        // set gpio direction
        if ((mode & GPIO_DIR_INPUT) != 0)
        {
            SETREG32(&pGpio->OE, mask);
        }
        else
        {
            CLRREG32(&pGpio->OE, mask);
        }

        // set debounce mode
        if ((mode & GPIO_DEBOUNCE_ENABLE) != 0)
        {
            SETREG32(&pGpio->DEBOUNCENABLE, mask);
        }
        else
        {
            CLRREG32(&pGpio->DEBOUNCENABLE, mask);
        }

        // set edge/level detect mode
        if ((mode & GPIO_INT_LOW) != 0)
        {
            SETREG32(&pGpio->LEVELDETECT0, mask);
        }
        else
        {
            CLRREG32(&pGpio->LEVELDETECT0, mask);
        }

        if ((mode & GPIO_INT_HIGH) != 0)
        {
            SETREG32(&pGpio->LEVELDETECT1, mask);
        }
        else
        {
            CLRREG32(&pGpio->LEVELDETECT1, mask);
        }

        if ((mode & GPIO_INT_LOW_HIGH) != 0)
        {
            SETREG32(&pGpio->RISINGDETECT, mask);
        }
        else
        {
            CLRREG32(&pGpio->RISINGDETECT, mask);
        }

        if ((mode & GPIO_INT_HIGH_LOW) != 0)
        {
            SETREG32(&pGpio->FALLINGDETECT, mask);
        }
        else
        {
            CLRREG32(&pGpio->FALLINGDETECT, mask);
        }

        InternalSetGpioBankPowerState(pDevice, id, D4);
        if (pDevice->fPostInit)LeaveCriticalSection(pCs);

        rc = TRUE;
    }

cleanUp:
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function: Am3xxGpioGetMode
//
DWORD
Am3xxGpioGetMode(
                HANDLE context,
                UINT id
                )
{
    DWORD mode = (DWORD) -1;
    UINT bit = GPIO_BIT(id);
    UINT bank = GPIO_BANK(id);
    Am3xxGpioDevice_t *pDevice = (Am3xxGpioDevice_t*)context;

    // Check if we get correct context
    if ((pDevice == NULL) || (pDevice->cookie != GPIO_DEVICE_COOKIE))
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: Am3xxGpioGetMode: "
            L"Incorrect context\r\n"
            ));
        goto cleanUp;
    }

    if (id < pDevice->nbBanks*32)
    {
        mode = 0;
        AM3XX_GPIO_REGS *pGpio = pDevice->bank[bank].ppGpioRegs;
        UINT32 mask = 1 << (bit);
        CRITICAL_SECTION *pCs = &pDevice->bank[bank].pCs;

        if (pDevice->fPostInit) EnterCriticalSection(pCs);
        InternalSetGpioBankPowerState(pDevice, id, D0);

        // get edge mode
        if ((INREG32(&pGpio->OE) & mask) != 0)
        {
            mode |= GPIO_DIR_INPUT;
        }
        else
        {
            mode |= GPIO_DIR_OUTPUT;
        }

        // get debounce mode
        if ((INREG32(&pGpio->DEBOUNCENABLE) & mask) != 0)
        {
            mode |= GPIO_DEBOUNCE_ENABLE;
        }

        // get edge/level detect mode
        if ((INREG32(&pGpio->LEVELDETECT0) & mask) != 0)
        {
            mode |= GPIO_INT_LOW;
        }

        if ((INREG32(&pGpio->LEVELDETECT1) & mask) != 0)
        {
            mode |= GPIO_INT_HIGH;
        }

        if ((INREG32(&pGpio->RISINGDETECT) & mask) != 0)
        {
            mode |= GPIO_INT_LOW_HIGH;
        }

        if ((INREG32(&pGpio->FALLINGDETECT) & mask) != 0)
        {
            mode |= GPIO_INT_HIGH_LOW;
        }

        InternalSetGpioBankPowerState(pDevice, id, D4);
        if (pDevice->fPostInit) LeaveCriticalSection(pCs);

    }

cleanUp:
    return mode;
}




//------------------------------------------------------------------------------
//
//  Function: Am3xxGpioPullup
//
BOOL
Am3xxGpioPullup(
               HANDLE context,
               UINT id,
               UINT enable
               )
{
    UNREFERENCED_PARAMETER(context);
    UNREFERENCED_PARAMETER(id);
    UNREFERENCED_PARAMETER(enable);
    return TRUE;
}


//------------------------------------------------------------------------------
//
//  Function: Am3xxGpioPulldown
//
BOOL
Am3xxGpioPulldown(
                 HANDLE context,
                 UINT id,
                 UINT enable
                 )
{
    UNREFERENCED_PARAMETER(context);
    UNREFERENCED_PARAMETER(id);
    UNREFERENCED_PARAMETER(enable);
    return TRUE;
}

#if 0
//------------------------------------------------------------------------------
//
//  Function: Am3xxGpioSetIntrEvent
//
BOOL
Am3xxGpioInterruptInitialize(
                            HANDLE context,
                            UINT intrId,
                            HANDLE hEvent
                            )
{
    BOOL    rc = FALSE;
#ifdef DEVICE
    Am3xxGpioDevice_t *pDevice = (Am3xxGpioDevice_t*)context;

    // Check if we get correct context & pin id
    if ((pDevice == NULL) || (pDevice->cookie != GPIO_DEVICE_COOKIE))
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: Am3xxGpioSetBit: Incorrect context\r\n"));
    }
    else
    {
        // Make sure hEvent is not NULL. We can't disassociate system interrupt
        // from an event once the association is set as the T2 interrupts do. because
        // there is not API call to do so in CE.
        if (hEvent != NULL)
        {
            rc = InterruptInitialize(intrId, hEvent, NULL, 0);
        }
    }
#endif
#ifdef OAL
    UNREFERENCED_PARAMETER(context);
    UNREFERENCED_PARAMETER(intrId);
    UNREFERENCED_PARAMETER(hEvent);
#endif
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function: Am3xxGpioIntrEnable
//
BOOL
Am3xxGpioInterruptDone(
                      HANDLE context,
                      UINT intrId
                      )
{
    BOOL    rc = FALSE;
#ifdef DEVICE
    Am3xxGpioDevice_t *pDevice = (Am3xxGpioDevice_t*)context;

    // Check if we get correct context & pin id
    if ((pDevice == NULL) || (pDevice->cookie != GPIO_DEVICE_COOKIE))
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: Am3xxGpioSetBit: Incorrect context\r\n"));
    }
    else
    {
        // Simply call the kernel to enable the interrupt via InterruptDone.
        InterruptDone((DWORD)intrId);
        rc = TRUE;
    }
#endif
#ifdef OAL    
    UNREFERENCED_PARAMETER(context);
    UNREFERENCED_PARAMETER(intrId);
#endif
    return rc;
}


//------------------------------------------------------------------------------
//
//  Function: Am3xxGpioIntrDisable
//
BOOL
Am3xxGpioInterruptDisable(
                         HANDLE context,
                         UINT intrId
                         )
{
    BOOL    rc = FALSE;
#ifdef DEVICE
    Am3xxGpioDevice_t *pDevice = (Am3xxGpioDevice_t*)context;

    // Check if we get correct context & pin id
    if ((pDevice == NULL) || (pDevice->cookie != GPIO_DEVICE_COOKIE))
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: Am3xxGpioSetBit: Incorrect context\r\n"));
    }
    else
    {
        // Simply call the kernel to disable the interrupt via InterruptDisable.
        InterruptDisable((DWORD)intrId);
        rc = TRUE;
    }
#endif
#ifdef OAL
    UNREFERENCED_PARAMETER(context);
    UNREFERENCED_PARAMETER(intrId);    
#endif
    return rc;
}

#endif

//***************************************END ADDITION********************************************

//------------------------------------------------------------------------------
//
//  Function: Am3xxGpioSetBit - Set the value of the GPIO output pin
//
BOOL
Am3xxGpioSetBit(
               HANDLE context,
               UINT id
               )
{
    BOOL rc = FALSE;
    UINT bit = GPIO_BIT(id);
    UINT bank = GPIO_BANK(id);
    Am3xxGpioDevice_t *pDevice = (Am3xxGpioDevice_t*)context;
    // Check if we get correct context & pin id
    if ((pDevice == NULL) || (pDevice->cookie != GPIO_DEVICE_COOKIE))
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: Am3xxGpioSetBit: Incorrect context\r\n"));
        goto cleanUp;
    }

    if (id < pDevice->nbBanks*32)
    {
        volatile UINT *p = &pDevice->bank[bank].ppGpioRegs->DATAOUT;
        CRITICAL_SECTION *pCs = &pDevice->bank[bank].pCs;

        if (pDevice->fPostInit) EnterCriticalSection(pCs);
        InternalSetGpioBankPowerState(pDevice, id, D0);
        SETREG32(p, 1 << (bit));
        InternalSetGpioBankPowerState(pDevice, id, D4);
        if (pDevice->fPostInit) LeaveCriticalSection(pCs);

        rc = TRUE;
    }

cleanUp:
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function: Am3xxGpioClrBit
//
BOOL
Am3xxGpioClrBit(
               HANDLE context,
               UINT id
               )
{
    BOOL rc = FALSE;
    UINT bit = GPIO_BIT(id);
    UINT bank = GPIO_BANK(id);
    Am3xxGpioDevice_t *pDevice = (Am3xxGpioDevice_t*)context;
    // Check if we get correct context & pin id
    if ((pDevice == NULL) || (pDevice->cookie != GPIO_DEVICE_COOKIE))
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: Am3xxGpioClrBit: Incorrect context\r\n"));
        goto cleanUp;
    }
    if (id < pDevice->nbBanks*32)
    {
        volatile UINT *p = &pDevice->bank[bank].ppGpioRegs->DATAOUT;
        CRITICAL_SECTION *pCs = &pDevice->bank[bank].pCs;

        if (pDevice->fPostInit) EnterCriticalSection(pCs);
        InternalSetGpioBankPowerState(pDevice, id, D0);
        CLRREG32(p, 1 << (bit));
        InternalSetGpioBankPowerState(pDevice, id, D4);
        if (pDevice->fPostInit) LeaveCriticalSection(pCs);
        rc = TRUE;
    }
cleanUp:
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function: Am3xxGpioGetBit
//
DWORD
Am3xxGpioGetBit(
               HANDLE context,
               UINT id
               )
{
    DWORD value = (DWORD) -1;
    UINT bit = GPIO_BIT(id);
    UINT bank = GPIO_BANK(id);
    Am3xxGpioDevice_t *pDevice = (Am3xxGpioDevice_t*)context;

    // Check if we get correct context & pin id
    if ((pDevice == NULL) || (pDevice->cookie != GPIO_DEVICE_COOKIE))
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: Am3xxGpioGetBit: Incorrect context\r\n"));
        goto cleanUp;
    }

    if (id < pDevice->nbBanks*32)
    {
        volatile UINT *p = &pDevice->bank[bank].ppGpioRegs->DATAIN;
        CRITICAL_SECTION *pCs = &pDevice->bank[bank].pCs;
        if (pDevice->fPostInit) EnterCriticalSection(pCs);
        InternalSetGpioBankPowerState(pDevice, id, D0);
        value = (INREG32(p) >> (bit)) & 0x01;
        InternalSetGpioBankPowerState(pDevice, id, D4);
        if (pDevice->fPostInit) LeaveCriticalSection(pCs);
    }

cleanUp:
    return value;
}

//------------------------------------------------------------------------------
//
//  Function:  Am3xxGpioIoControl
//
//  This function sends a command to a device.
//
BOOL
Am3xxGpioIoControl(
                  HANDLE  context,
                  UINT    code,
                  UCHAR  *pInBuffer,
                  UINT    inSize,
                  UCHAR  *pOutBuffer,
                  UINT    outSize,
                  DWORD   *pOutSize
                  )
{
#ifdef DEVICE
    UINT id;
    DWORD value,mode;
    DEVICE_IFC_GPIO ifc;
    UINT bit;
    UINT bank;
    BOOL rc = FALSE;
    Am3xxGpioDevice_t *pDevice = (Am3xxGpioDevice_t*)context;

    UNREFERENCED_PARAMETER(pOutSize);

    DEBUGMSG(ZONE_FUNCTION, (
        L"+Am3xxGpioIOControl(0x%08x, 0x%08x, 0x%08x, %d, 0x%08x, %d, 0x%08x)\r\n",
        context, code, pInBuffer, inSize, pOutBuffer, outSize, pOutSize
        ));

    // Check if we get correct context
    if ((pDevice == NULL) || (pDevice->cookie != GPIO_DEVICE_COOKIE))
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: Am3xxGpioIOControl: "
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
            DEBUGMSG(ZONE_ERROR, (L"ERROR: GIO_IOControl: "
                L"IOCTL_DDK_GET_DRIVER_IFC can be called only from "
                L"device process (caller process id 0x%08x)\r\n",
                GetCurrentProcessId()
                ));
            SetLastError(ERROR_ACCESS_DENIED);
            goto cleanUp;
        }
        if ((pInBuffer == NULL) || (inSize < sizeof(GUID)))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            break;
        }
        if (IsEqualGUID(*(GUID*)pInBuffer, DEVICE_IFC_GPIO_GUID))
        {
            if (pOutSize != NULL) *pOutSize = sizeof(DEVICE_IFC_GPIO);
            if ((pOutBuffer == NULL) || (outSize < sizeof(DEVICE_IFC_GPIO)))
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                break;
            }
            ifc.context                 = context;
            ifc.pfnSetBit               = Am3xxGpioSetBit;
            ifc.pfnClrBit               = Am3xxGpioClrBit;
            ifc.pfnGetBit               = Am3xxGpioGetBit;
            ifc.pfnSetMode              = Am3xxGpioSetMode;
            ifc.pfnGetMode              = Am3xxGpioGetMode;
            ifc.pfnPullup               = Am3xxGpioPullup;
            ifc.pfnPulldown             = Am3xxGpioPulldown;
            ifc.pfnInterruptInitialize  = Am3xxGpioInterruptInitialize;
            ifc.pfnInterruptMask        = Am3xxGpioInterruptMask;
            ifc.pfnInterruptDisable     = Am3xxGpioInterruptDisable;
            ifc.pfnInterruptDone        = Am3xxGpioInterruptDone;
			ifc.pfnGetSystemIrq			= Am3xxGpioGetSystemIrq;
            ifc.pfnInterruptWakeUp      = Am3xxGpioInterruptWakeUp;

            if (!CeSafeCopyMemory(pOutBuffer, &ifc, sizeof(ifc))) break;
            rc = TRUE;
            break;
        }
        SetLastError(ERROR_INVALID_PARAMETER);
        break;

    case IOCTL_GPIO_SETBIT:
        if (pOutSize != 0) *pOutSize = 0;
        if ((pInBuffer == NULL) || (inSize < sizeof(DWORD)) ||
            !CeSafeCopyMemory(&id, pInBuffer, sizeof(DWORD)))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            break;
        }
        Am3xxGpioSetBit(context, id);
        rc = TRUE;
        break;

    case IOCTL_GPIO_CLRBIT:
        if (pOutSize != 0) *pOutSize = 0;
        if ((pInBuffer == NULL) || (inSize < sizeof(DWORD)) ||
            !CeSafeCopyMemory(&id, pInBuffer, sizeof(DWORD)))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            break;
        }
        Am3xxGpioClrBit(context, id);
        rc = TRUE;
        break;

    case IOCTL_GPIO_GETBIT:
        if (pOutSize != 0) *pOutSize = sizeof(DWORD);
        if ((pInBuffer == NULL) || (inSize < sizeof(DWORD)) ||
            !CeSafeCopyMemory(&id, pInBuffer, sizeof(DWORD)) ||
            (pOutBuffer == NULL) || (outSize < sizeof(DWORD)))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            break;
        }
        value = Am3xxGpioGetBit(context, id);
        if (!CeSafeCopyMemory(pOutBuffer, &value, sizeof(value)))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            break;
        }
        rc = TRUE;
        break;

    case IOCTL_GPIO_SETMODE:
        if (pOutSize != 0) *pOutSize = 0;
        if ((pInBuffer == NULL) || (inSize < 2 * sizeof(DWORD)) ||
            !CeSafeCopyMemory(
            &id, &((DWORD*)pInBuffer)[0], sizeof(DWORD)
            ) ||
            !CeSafeCopyMemory(
            &mode, &((DWORD*)pInBuffer)[1], sizeof(DWORD)
            ))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            break;
        }
        Am3xxGpioSetMode(context, id, mode);
        rc = TRUE;
        break;

    case IOCTL_GPIO_GETMODE:
        if (pOutSize != 0) *pOutSize = sizeof(DWORD);
        if ((pInBuffer == NULL) || (inSize < sizeof(DWORD)) ||
            !CeSafeCopyMemory(&id, pInBuffer, sizeof(DWORD)) ||
            (pOutBuffer == NULL) || (outSize < sizeof(DWORD)))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            break;
        }
        mode = Am3xxGpioGetMode(context, id);
        if (!CeSafeCopyMemory(pOutBuffer, &mode, sizeof(mode)))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            break;
        }
        rc = TRUE;
        break;
        /*
        case IOCTL_GPIO_GETIRQ:
        if (pOutSize != 0) *pOutSize = sizeof(DWORD);
        if ((pInBuffer == NULL) || (inSize < sizeof(DWORD)) ||
        !CeSafeCopyMemory(&id, pInBuffer, sizeof(DWORD)) ||
        (pOutBuffer == NULL) || (outSize < sizeof(DWORD)))
        {
        SetLastError(ERROR_INVALID_PARAMETER);
        break;
        }
        value = Am3xxGpioGetSystemIrq(context, id);
        if (!CeSafeCopyMemory(pOutBuffer, &value, sizeof(value)))
        {
        SetLastError(ERROR_INVALID_PARAMETER);
        break;
        }
        rc = TRUE;
        break;

        case IOCTL_GPIO_INIT_INTERRUPT:
        if ((pInBuffer == NULL) || (inSize != sizeof(IOCTL_GPIO_INIT_INTERRUPT_INFO)))
        {
        SetLastError(ERROR_INVALID_PARAMETER);
        break;
        }
        else
        {
        HANDLE      hLocalEvent         = INVALID_HANDLE_VALUE;
        HANDLE      hCallerHandle       = GetCallerProcess();
        HANDLE      hCurrentProcHandle  = GetCurrentProcess();
        PIOCTL_GPIO_INIT_INTERRUPT_INFO pInitIntrInfo = (PIOCTL_GPIO_INIT_INTERRUPT_INFO)pInBuffer;

        if (hCurrentProcHandle != hCallerHandle)
        {
        BOOL    bStatus;

        bStatus = DuplicateHandle(hCallerHandle, pInitIntrInfo->hEvent,
        hCurrentProcHandle, &hLocalEvent,
        DUPLICATE_SAME_ACCESS,
        FALSE,
        DUPLICATE_SAME_ACCESS);

        if ((bStatus == FALSE) || (hLocalEvent == INVALID_HANDLE_VALUE))
        {
        RETAILMSG(1, (TEXT("GIO_IOControl: IOCTL_GPIO_INIT_INTERRUPT unable to duplicate event handle.\r\n")));
        break;
        }
        }
        else
        {
        hLocalEvent = pInitIntrInfo->hEvent;
        if (hLocalEvent == INVALID_HANDLE_VALUE)
        {
        SetLastError(ERROR_INVALID_PARAMETER);
        break;
        }
        }

        rc = InterruptInitialize(pInitIntrInfo->dwSysIntrID, hLocalEvent, NULL, 0);
        CloseHandle(hLocalEvent);
        }
        break;

        case IOCTL_GPIO_ACK_INTERRUPT:
        if ((pInBuffer == NULL) || (inSize != sizeof(IOCTL_GPIO_INTERRUPT_INFO)))
        {
        SetLastError(ERROR_INVALID_PARAMETER);
        break;
        }
        else
        {
        PIOCTL_GPIO_INTERRUPT_INFO pIntrInfo = (PIOCTL_GPIO_INTERRUPT_INFO)pInBuffer;

        rc = Am3xxGpioInterruptDone(context, pIntrInfo->uGpioID, pIntrInfo->dwSysIntrID);
        }
        break;

        case IOCTL_GPIO_DISABLE_INTERRUPT:
        if ((pInBuffer == NULL) || (inSize != sizeof(IOCTL_GPIO_INTERRUPT_INFO)))
        {
        SetLastError(ERROR_INVALID_PARAMETER);
        break;
        }
        else
        {
        PIOCTL_GPIO_INTERRUPT_INFO pIntrInfo = (PIOCTL_GPIO_INTERRUPT_INFO)pInBuffer;

        rc = Am3xxGpioInterruptDisable(context, pIntrInfo->uGpioID, pIntrInfo->dwSysIntrID);
        }
        break;
        }
        */
    case IOCTL_GPIO_SET_POWER_STATE:
        {
            if ((pInBuffer == NULL) || inSize != sizeof(IOCTL_GPIO_POWER_STATE_IN))
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                ERRORMSG(ZONE_ERROR, (L"ERROR: Am3xxGpioIOControl: "
                    L"IOCTL_GPIO_SET_POWER_STATE - Invalid parameter\r\n"
                    ));
                break;
            }

            IOCTL_GPIO_POWER_STATE_IN *pPowerIn;
            pPowerIn = (IOCTL_GPIO_POWER_STATE_IN*)pInBuffer;

            bank = GPIO_BANK(pPowerIn->gpioId);

            CRITICAL_SECTION *pCs = &pDevice->bank[bank].pCs;
            if (pDevice->fPostInit) EnterCriticalSection(pCs);
            SetGpioBankPowerState(pDevice, pPowerIn->gpioId, pPowerIn->state);
            if (pDevice->fPostInit) LeaveCriticalSection(pCs);
            rc = TRUE;
        }
        break;

    case IOCTL_GPIO_GET_POWER_STATE:
        {
            if ((pInBuffer == NULL) || (pOutBuffer == NULL) ||
                inSize != sizeof(IOCTL_GPIO_POWER_STATE_IN) ||
                outSize != sizeof(IOCTL_GPIO_GET_POWER_STATE_OUT))
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                ERRORMSG(ZONE_ERROR, (L"ERROR: Am3xxGpioIOControl: "
                    L"IOCTL_GPIO_SET_POWER_STATE - Invalid parameter\r\n"
                    ));
                break;
            }

            IOCTL_GPIO_POWER_STATE_IN *pPowerIn;
            IOCTL_GPIO_GET_POWER_STATE_OUT *pPowerOut;

            pPowerIn = (IOCTL_GPIO_POWER_STATE_IN*)pInBuffer;
            pPowerOut = (IOCTL_GPIO_GET_POWER_STATE_OUT*)pOutBuffer;

            bit = GPIO_BIT(pPowerIn->gpioId);
            bank = GPIO_BANK(pPowerIn->gpioId);

            // get power state for gpio
            CRITICAL_SECTION *pCs = &pDevice->bank[bank].pCs;
            if (pDevice->fPostInit) EnterCriticalSection(pCs);
            pPowerOut->gpioState = (pDevice->bank[bank].powerEnabled & (1 << bit)) ? D0 : D4;
            pPowerOut->bankState = (pDevice->bank[bank].powerEnabled) ? D0 : D4;
            if (pDevice->fPostInit) LeaveCriticalSection(pCs);

            rc = TRUE;
        }

    case IOCTL_GPIO_SET_DEBOUNCE_TIME:
        {
            if ((pInBuffer == NULL) ||
                (inSize < sizeof(IOCTL_GPIO_SET_DEBOUNCE_TIME_IN)))
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                break;
            }

            IOCTL_GPIO_SET_DEBOUNCE_TIME_IN *pDebounce;

            pDebounce = (IOCTL_GPIO_SET_DEBOUNCE_TIME_IN*)pInBuffer;

            if (pDebounce->gpioId < pDevice->nbBanks*32)
            {
                CRITICAL_SECTION *pCs;
                bank = GPIO_BANK(pDebounce->gpioId);
                pCs = &pDevice->bank[bank].pCs;
                if (pDevice->fPostInit) EnterCriticalSection(pCs);
                InternalSetGpioBankPowerState(pDevice, pDebounce->gpioId, D0);
                OUTREG32(&pDevice->bank[bank].ppGpioRegs->DEBOUNCINGTIME,
                    pDebounce->debounceTime);
                InternalSetGpioBankPowerState(pDevice, pDebounce->gpioId, D4);
                if (pDevice->fPostInit) LeaveCriticalSection(pCs);

                rc = TRUE;
            }
        }
        break;

    case IOCTL_GPIO_GET_DEBOUNCE_TIME:
        {
            if ((pInBuffer == NULL) || (pOutBuffer == NULL) ||
                (inSize < sizeof(UINT)) ||
                (outSize < sizeof(UINT)))
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                break;
            }

            UINT *pId;
            UINT *pOut;

            pId = (UINT*)pInBuffer;
            pOut = (UINT*)pOutBuffer;

            if (*pId < pDevice->nbBanks*32)
            {

                bank = GPIO_BANK(*pId);

                CRITICAL_SECTION *pCs = &pDevice->bank[bank].pCs;
                if (pDevice->fPostInit) EnterCriticalSection(pCs);
                InternalSetGpioBankPowerState(pDevice, *pId, D0);
                *pOut = INREG32(&pDevice->bank[bank].ppGpioRegs->DEBOUNCINGTIME);
                InternalSetGpioBankPowerState(pDevice, *pId, D4);
                if (pDevice->fPostInit) LeaveCriticalSection(pCs);

                rc = TRUE;
            }
        }
        break;
    }

cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-Am3xxGpioIOControl(rc = %d)\r\n", rc));
    return rc;
#endif

#ifdef OAL
    UNREFERENCED_PARAMETER(context);
    UNREFERENCED_PARAMETER(code);
    UNREFERENCED_PARAMETER(pInBuffer);
    UNREFERENCED_PARAMETER(inSize);
    UNREFERENCED_PARAMETER(pOutBuffer);
    UNREFERENCED_PARAMETER(outSize);
    UNREFERENCED_PARAMETER(pOutSize);        
    return FALSE;
#endif
}

//------------------------------------------------------------------------------
//
//  Function:  Am3xxGpioPowerUp
//
//  This function restores power to a device.
//
VOID
Am3xxGpioPowerUp(
                HANDLE hContext
                )
{
    UNREFERENCED_PARAMETER(hContext);
}

//------------------------------------------------------------------------------
//
//  Function:  Am3xxGpioPowerDown
//
//  This function suspends power to the device.
//
VOID
Am3xxGpioPowerDown(
                  HANDLE hContext
                  )
{
    UNREFERENCED_PARAMETER(hContext);
}

//------------------------------------------------------------------------------
#ifdef DEVICE

DWORD Am3xxGpioGetSystemIrq(HANDLE hContext, UINT id)
{
    UNREFERENCED_PARAMETER(hContext);
	//get the interrupt number associated with this pin
    return BSPGetGpioIrq(id);
}
//------------------------------------------------------------------------------
//
//  Function:  Am3xxGpioInterruptInitialize
//
//  This function intialize the interrupt for the pin
//
BOOL Am3xxGpioInterruptInitialize(HANDLE hContext, DWORD id, DWORD* sysintr, HANDLE hEvent)
{
    DWORD logintr;
    Am3xxGpioDevice_t *pDevice = (Am3xxGpioDevice_t*)hContext;

    if ((pDevice == NULL) || (sysintr == NULL) || (hEvent == NULL) || (id >= pDevice->nbBanks*32))
    {
        return FALSE;
    }
    
    *sysintr = (DWORD) SYSINTR_UNDEFINED;

    //get the interrupt number associated with this pin
    logintr = BSPGetGpioIrq(id);

    //Get a valid sysintr for this interrupt
    if (!KernelIoControl(IOCTL_HAL_REQUEST_SYSINTR,&logintr,sizeof(logintr),sysintr,sizeof(*sysintr),NULL))
    {
        goto failed;
    }

    // Initialize the interrupt
    if (InterruptInitialize(*sysintr,hEvent,NULL,0) == FALSE)
    {
        goto failed;
    }
    return TRUE;

failed:
    if (*sysintr != SYSINTR_UNDEFINED)
    {
        KernelIoControl(IOCTL_HAL_RELEASE_SYSINTR,sysintr,sizeof(*sysintr),NULL,0,NULL);
    }

    return FALSE;
}
//------------------------------------------------------------------------------
//
//  Function:  Am3xxGpioInterruptMask
//
//  
//
VOID Am3xxGpioInterruptMask(HANDLE hContext, DWORD id, DWORD sysintr, BOOL fDisable)
{
    UNREFERENCED_PARAMETER(hContext);
    UNREFERENCED_PARAMETER(id);

    InterruptMask(sysintr,fDisable);
}
//------------------------------------------------------------------------------
//
//  Function:  Am3xxGpioInterruptDisable
//
// 
//
VOID Am3xxGpioInterruptDisable(HANDLE hContext, DWORD id, DWORD sysintr)
{
    UNREFERENCED_PARAMETER(hContext);
    UNREFERENCED_PARAMETER(id);

    InterruptDisable(sysintr);

    if (sysintr != SYSINTR_UNDEFINED)
    {
        KernelIoControl(IOCTL_HAL_RELEASE_SYSINTR,&sysintr,sizeof(sysintr),NULL,0,NULL);
    }
}
//------------------------------------------------------------------------------
//
//  Function:  Am3xxGpioInterruptDone
//
//  This function acknowledges the interrupt;
//
VOID Am3xxGpioInterruptDone(HANDLE hContext, DWORD id, DWORD sysintr)
{
    UNREFERENCED_PARAMETER(hContext);
    UNREFERENCED_PARAMETER(id);
    InterruptDone(sysintr);
}

//------------------------------------------------------------------------------
//
//  Function:  Am3xxGpioInterruptWakeUp
//
//  This function enable the interrupt to wake up the processor from suspend
//

BOOL Am3xxGpioInterruptWakeUp(HANDLE hContext, DWORD id, DWORD sysintr, BOOL fEnabled)
{
    UNREFERENCED_PARAMETER(hContext);
    UNREFERENCED_PARAMETER(id);
    return KernelIoControl(fEnabled ? IOCTL_HAL_ENABLE_WAKE : IOCTL_HAL_DISABLE_WAKE, 
        &sysintr,sizeof(sysintr), NULL, 0, NULL );
}

#endif

#ifdef DEVICE
DWORD GIO_Init(
               LPCTSTR pContext,
               DWORD dwBusContext
               )
{
    HANDLE devCtxt;
    UINT count;

    UNREFERENCED_PARAMETER(dwBusContext);

    if (Am3xxGpioInit(pContext,&devCtxt,&count))
    {
        if (Am3xxGpioPostInit(devCtxt))
        {
            return (DWORD) devCtxt;
        }
    }
    return NULL;
}

BOOL GIO_Deinit(DWORD hDeviceContext)
{
    return Am3xxGpioDeinit((HANDLE)hDeviceContext);
}

DWORD GIO_Open(
               DWORD hDeviceContext,
               DWORD AccessCode,
               DWORD ShareMode 
               )
{
    UNREFERENCED_PARAMETER(AccessCode);
    UNREFERENCED_PARAMETER(ShareMode);
    return hDeviceContext;
}

BOOL GIO_Close(
               DWORD hOpenContext 
               )
{
    UNREFERENCED_PARAMETER(hOpenContext);
    return TRUE;
}

void GIO_PowerDown(DWORD hDeviceContext)
{
    Am3xxGpioPowerDown((HANDLE) hDeviceContext);
}
void GIO_PowerUp(DWORD hDeviceContext )
{
    Am3xxGpioPowerUp((HANDLE) hDeviceContext);
}



BOOL GIO_IOControl(
                   DWORD hOpenContext,
                   DWORD dwCode,
                   PBYTE pBufIn,
                   DWORD dwLenIn,
                   PBYTE pBufOut,
                   DWORD dwLenOut,
                   PDWORD pdwActualOut 
                   )
{
    return Am3xxGpioIoControl(
        (HANDLE)hOpenContext,
        dwCode,
        pBufIn,
        dwLenIn,
        pBufOut,
        dwLenOut,
        pdwActualOut 
        );
}


#endif
