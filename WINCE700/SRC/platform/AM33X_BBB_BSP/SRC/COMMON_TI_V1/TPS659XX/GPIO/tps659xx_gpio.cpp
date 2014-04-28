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
//------------------------------------------------------------------------------
//
//  File: omap35xx_gpio.cpp
//
#include "omap.h"
#include "bsp_cfg.h"
#include "ceddkex.h"
#include "oalex.h"
#include <oal_clock.h>
#include "soc_cfg.h"
#include "omap_gpio_regs.h"

#include "sdk_gpio.h"
#ifdef OAL
#include "oal_alloc.h"
#endif
#include <nkintr.h>

#include "tps659xx.h"
#include "tps659xx_gpio.h"
#include "tps659xx_internals.h"



#ifdef DEVICE
#include <initguid.h>
#include "gpio_ioctls.h"
#endif

#include <initguid.h>
#include "twl.h"

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
    L"TPS659xx GPIO", {
        L"Errors",      L"Warnings",    L"Function",    L"Info",
            L"IST",         L"Undefined",   L"Undefined",   L"Undefined",
            L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined",
            L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined"
    },
    0x0003
};
#endif
#endif

#define GPIO_BITS_PER_BANK      (0x7)   // +1 last register only has 2 GPIO's

#define GPIO_BANK(x)            (x >> 3)
#define GPIO_BIT(x)             (x & GPIO_BITS_PER_BANK)
#define GPIO_SUBGROUP(x)        (GPIO_BIT(x) >> 2)
#define GPIO_SUBINDEX(x)        ((GPIO_BIT(x) & 0x3) * 2)

#define DETECT_RISING           (0x1)
#define DETECT_FALLING          (0x2)
#define DETECT_MASK             (0x3)

#define PULLDOWN_ENABLE         (1 << 0)
#define PULLUP_ENABLE           (1 << 1)


//------------------------------------------------------------------------------
//  Local Structures

static HANDLE                   s_hTritonDriver = NULL;

CRITICAL_SECTION csGpio;
BOOL g_fGpioPostInit = FALSE;


//------------------------------------------------------------------------------
//  Device registry parameters

static TPS659XX_GPIO_DATA_REGS s_rgGpioRegs[] = {
    {
    TWL_GPIODATAIN1, TWL_GPIODATADIR1, TWL_GPIODATAOUT1,
    TWL_CLEARGPIODATAOUT1, TWL_SETGPIODATAOUT1, TWL_GPIO_DEBEN1,
        {{
        TWL_GPIOPUPDCTR1, TWL_GPIO_EDR1
        }, {
        TWL_GPIOPUPDCTR2, TWL_GPIO_EDR2
        }}
    }, {
    TWL_GPIODATAIN2, TWL_GPIODATADIR2, TWL_GPIODATAOUT2,
    TWL_CLEARGPIODATAOUT2, TWL_SETGPIODATAOUT2, TWL_GPIO_DEBEN2,
        {{
        TWL_GPIOPUPDCTR3, TWL_GPIO_EDR3
        }, {
        TWL_GPIOPUPDCTR4, TWL_GPIO_EDR4
        }}
    }, {
    TWL_GPIODATAIN3, TWL_GPIODATADIR3, TWL_GPIODATAOUT3,
    TWL_CLEARGPIODATAOUT3, TWL_SETGPIODATAOUT3, TWL_GPIO_DEBEN3,
        {{
        TWL_GPIOPUPDCTR5, TWL_GPIO_EDR5
        }, {
        0, 0
        }}
    }
};

//------------------------------------------------------------------------------
//  Triton Intr Id mapping

static DWORD s_TritonIntrIdMap[] = 
{
    TWL_INTR_GPIO_0 ,
    TWL_INTR_GPIO_1 ,
    TWL_INTR_GPIO_2 ,
    TWL_INTR_GPIO_3 ,
    TWL_INTR_GPIO_4 ,
    TWL_INTR_GPIO_5 ,
    TWL_INTR_GPIO_6 ,
    TWL_INTR_GPIO_7 ,
    TWL_INTR_GPIO_8 ,
    TWL_INTR_GPIO_9 ,
    TWL_INTR_GPIO_10,
    TWL_INTR_GPIO_11,
    TWL_INTR_GPIO_12,
    TWL_INTR_GPIO_13,
    TWL_INTR_GPIO_14,
    TWL_INTR_GPIO_15,
    TWL_INTR_GPIO_16,
    TWL_INTR_GPIO_17
};

#define MAP_TO_TRITON_INTR(x) (s_TritonIntrIdMap[x])

//------------------------------------------------------------------------------
//  Local Functions

// Init function
static BOOL Tps659xxGpioInit(LPCTSTR szContext, HANDLE *phContext, UINT *pGpioCount);
static BOOL Tps659xxGpioPostInit(HANDLE hContext);
static BOOL Tps659xxGpioDeinit(HANDLE hContext);
static BOOL Tps659xxGpioSetMode(HANDLE hContext, UINT id, UINT mode);
static DWORD Tps659xxGpioGetMode(HANDLE hContext, UINT id);
static BOOL Tps659xxGpioPullup(HANDLE hcontext,  UINT id, UINT enable);
static BOOL Tps659xxGpioPulldown(HANDLE hcontext,  UINT id, UINT enable);
static BOOL Tps659xxGpioInterruptInitialize(HANDLE hContext, DWORD id, DWORD* sysintr, HANDLE hEvent);
static VOID Tps659xxGpioInterruptMask(HANDLE hContext, DWORD id, DWORD sysintr, BOOL fDisable);
static VOID Tps659xxGpioInterruptDone(HANDLE hcontext,  DWORD id, DWORD sysintr);
static VOID Tps659xxGpioInterruptDisable(HANDLE hcontext,DWORD id, DWORD sysintr);
static BOOL Tps659xxGpioSetBit(HANDLE hContext, UINT id);
static BOOL Tps659xxGpioClrBit(HANDLE hContext, UINT id);
static DWORD Tps659xxGpioGetBit(HANDLE hContext, UINT id);
static void Tps659xxGpioPowerUp(HANDLE hContext);
static void Tps659xxGpioPowerDown(HANDLE hContext);
static BOOL Tps659xxGpioIoControl(HANDLE hContext, UINT code,
                               UCHAR *pinVal, UINT inSize, UCHAR *poutVal,
                               UINT outSize, DWORD *pOutSize);
static BOOL Tps659xxGpioWakeEnable( HANDLE context, DWORD id, DWORD sysintr, BOOL bEnable);

//------------------------------------------------------------------------------
//  exported function table
extern "C" DEVICE_IFC_GPIO Tps659xx_Gpio;

DEVICE_IFC_GPIO Tps659xx_Gpio = {
    0,
    Tps659xxGpioInit,
    Tps659xxGpioPostInit,
    Tps659xxGpioDeinit,
    Tps659xxGpioSetBit,
    Tps659xxGpioClrBit,
    Tps659xxGpioGetBit,
    Tps659xxGpioSetMode,
    Tps659xxGpioGetMode,
    Tps659xxGpioPullup,
    Tps659xxGpioPulldown,
    Tps659xxGpioIoControl,
    NULL,NULL,NULL,NULL,
    
};

//------------------------------------------------------------------------------
static
BOOL
OpenTwl(
    )
{
    // Try open TWL device driver
    s_hTritonDriver = TWLOpen();

    // If we get handle, we succeded
    return (s_hTritonDriver != NULL);
}

//------------------------------------------------------------------------------
static
VOID
CloseTwl(
    )
{
    if (s_hTritonDriver != NULL)
        {
        TWLClose(s_hTritonDriver);
        s_hTritonDriver = NULL;
        }
}

//------------------------------------------------------------------------------
static
BOOL
WriteTwlReg(
    DWORD           address,
    UINT8          *pdata,
    UINT            dataSize
    )
{
    BOOL rc = FALSE;

    // If TWL isn't open, try to open it...
    if ((s_hTritonDriver == NULL) && !OpenTwl()) goto cleanUp;

    // Call driver
    rc = TWLWriteRegs(s_hTritonDriver, address, pdata, dataSize);

cleanUp:
    return rc;
}

//------------------------------------------------------------------------------
static
BOOL
ReadTwlReg(
    DWORD   address,
    UINT8  *pdata,
    UINT    dataSize
    )
{
    BOOL rc = FALSE;

    // If TWL isn't open, try to open it...
    if ((s_hTritonDriver == NULL) && !OpenTwl()) goto cleanUp;

    // Call driver
    rc = TWLReadRegs(s_hTritonDriver, address, pdata, dataSize);

cleanUp:
    return rc;
}

//------------------------------------------------------------------------------
inline
void
Lock()
{
    if (g_fGpioPostInit)
        EnterCriticalSection(&csGpio);
}

//------------------------------------------------------------------------------
inline
void
Unlock()
{
    if (g_fGpioPostInit)
        LeaveCriticalSection(&csGpio);
}

//------------------------------------------------------------------------------
static
void
SetGpioBankPowerState(
    UINT id,
    CEDEVICE_POWER_STATE state
    )
{
    UNREFERENCED_PARAMETER(id);
    UNREFERENCED_PARAMETER(state);
}

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
//  Function:  Tps659xxGpioInit
//
//  Called by device manager to initialize device.
//
BOOL
Tps659xxGpioInit(
    LPCTSTR szContext,
    HANDLE *phContext,
    UINT   *pGpioCount
    )
{

    UNREFERENCED_PARAMETER(szContext);
    UNREFERENCED_PARAMETER(phContext);

    DEBUGMSG(ZONE_FUNCTION, (
        L"+Tps659xxGpioInit(%s)\r\n", szContext
        ));

    *pGpioCount = TPS659XX_MAX_GPIO_COUNT;

    DEBUGMSG(ZONE_FUNCTION, (L"-Tps659xxGpioInit()\r\n"));
    return TRUE;
}


//------------------------------------------------------------------------------
//
//  Function:  Tps659xxGpioPostInit
//
//  Called by device manager to complete the init
//
BOOL
Tps659xxGpioPostInit(
    HANDLE context
    )
{

    UNREFERENCED_PARAMETER(context);
    InitializeCriticalSection(&csGpio);
    g_fGpioPostInit = TRUE;

    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  Tps659xxGpioDeinit
//
//  Called by device manager to uninitialize device.
//
BOOL
Tps659xxGpioDeinit(
    HANDLE context
    )
{    
    UNREFERENCED_PARAMETER(context);
    DEBUGMSG(ZONE_FUNCTION, (L"+Tps659xxGpioDeinit(0x%08x)\r\n", context));

    CloseTwl();
    if (g_fGpioPostInit) DeleteCriticalSection(&csGpio);

    DEBUGMSG(ZONE_FUNCTION, (L"-Tps659xxGpioDeinit()\r\n"));
    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function: Tps659xxGpioSetMode
//
BOOL
Tps659xxGpioSetMode(
    HANDLE context,
    UINT id,
    UINT mode
    )
{
    BOOL rc = FALSE;
    UINT bit = GPIO_BIT(id);
    UINT bank = GPIO_BANK(id);
    UINT subGroup = GPIO_SUBGROUP(id);
    UINT subIndex = GPIO_SUBINDEX(id);

    UNREFERENCED_PARAMETER(context);

//    RETAILMSG(1,(TEXT("+Tps659xxGpioSetMode  %d\r\n"),id));
    if (id < TPS659XX_MAX_GPIO_COUNT)
        {
        UINT8 val;
        UINT8 edgeMode = 0;

        Lock();
        SetGpioBankPowerState(id, D0);

        // set direction
        if ((mode & GPIO_DIR_INPUT) != 0)
            {
            ReadTwlReg(s_rgGpioRegs[bank].GPIODATADIR, &val, sizeof(val));
            val &= ~(1 << bit);
            WriteTwlReg(s_rgGpioRegs[bank].GPIODATADIR, &val, sizeof(val));
            }
        else
            {
            ReadTwlReg(s_rgGpioRegs[bank].GPIODATADIR, &val, sizeof(val));
            val |= (1 << bit);
            WriteTwlReg(s_rgGpioRegs[bank].GPIODATADIR, &val, sizeof(val));
            }

        // enable debouncing
        if ((mode & GPIO_DEBOUNCE_ENABLE) != 0)
            {
            ReadTwlReg(s_rgGpioRegs[bank].GPIO_DEBEN, &val, sizeof(val));
            val |= (1 << bit);
            WriteTwlReg(s_rgGpioRegs[bank].GPIO_DEBEN, &val, sizeof(val));
            }
        else
            {
            ReadTwlReg(s_rgGpioRegs[bank].GPIO_DEBEN, &val, sizeof(val));
            val &= ~(1 << bit);
            WriteTwlReg(s_rgGpioRegs[bank].GPIO_DEBEN, &val, sizeof(val));
            }

        // set edge interrupt type
        if (mode & GPIO_INT_HIGH_LOW) edgeMode |= DETECT_FALLING;
        if (mode & GPIO_INT_LOW_HIGH) edgeMode |= DETECT_RISING;

        ReadTwlReg(s_rgGpioRegs[bank].rgSubGroup[subGroup].GPIO_EDR, &val, sizeof(val));
        val &= ~(DETECT_MASK << subIndex);
        val |= (edgeMode << subIndex);
        WriteTwlReg(s_rgGpioRegs[bank].rgSubGroup[subGroup].GPIO_EDR, &val, sizeof(val));

        SetGpioBankPowerState(id, D4);
        Unlock();
        rc = TRUE;
        }
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function: Tps659xxGpioGetMode
//
DWORD
Tps659xxGpioGetMode(
    HANDLE hContext, 
    UINT id
    )
{ 
    UINT bit = GPIO_BIT(id);
    UINT bank = GPIO_BANK(id);
    UINT subGroup = GPIO_SUBGROUP(id);
    UINT subIndex = GPIO_SUBINDEX(id);
    DWORD mode =(DWORD) -1;;

    UNREFERENCED_PARAMETER(hContext);


    if (id < TPS659XX_MAX_GPIO_COUNT)
        {
        UINT8 val;
        SetGpioBankPowerState(id, D0);

        // get direction
        ReadTwlReg(s_rgGpioRegs[bank].GPIODATADIR, &val, sizeof(val));
        mode = (val & (1 << bit)) ? GPIO_DIR_OUTPUT : GPIO_DIR_INPUT;

        // get debounce state
        ReadTwlReg(s_rgGpioRegs[bank].GPIO_DEBEN, &val, sizeof(val));
        mode |= (val & (1 << bit)) ? GPIO_DEBOUNCE_ENABLE : 0;

        // get edge detection mode
        ReadTwlReg(s_rgGpioRegs[bank].rgSubGroup[subGroup].GPIO_EDR, &val, sizeof(val));
        val = (UINT8) ((val >> subIndex) & DETECT_MASK);

        if (val & DETECT_FALLING) mode |= GPIO_INT_HIGH_LOW;
        if (val & DETECT_RISING) mode |= GPIO_INT_LOW_HIGH;

        SetGpioBankPowerState(id, D4);        
        }

    return mode;
}



//------------------------------------------------------------------------------
//
//  Function: Tps659xxGpioPullup - Pullup enable/disable
//
BOOL
Tps659xxGpioPullup(
    HANDLE context,
    UINT id,
    UINT enable
    )
{
    BOOL rc = FALSE;
    UINT bank = GPIO_BANK(id);
    UINT subGroup = GPIO_SUBGROUP(id);
    UINT subIndex = GPIO_SUBINDEX(id);

    UNREFERENCED_PARAMETER(context);

    if (id < TPS659XX_MAX_GPIO_COUNT)
        {
        UINT8 val;
        UINT pullupState = 0;

        Lock();
        SetGpioBankPowerState(id, D0);

        // set pullup state
        if (enable) pullupState = PULLUP_ENABLE;

        ReadTwlReg(s_rgGpioRegs[bank].rgSubGroup[subGroup].GPIOPUPDCTR, &val, sizeof(val));
        val &= ~(PULLUP_ENABLE << subIndex);
        val |= (pullupState << subIndex);
        WriteTwlReg(s_rgGpioRegs[bank].rgSubGroup[subGroup].GPIOPUPDCTR, &val, sizeof(val));

        SetGpioBankPowerState(id, D4);
        Unlock();
        rc = TRUE;
        }

    return rc;
}


//------------------------------------------------------------------------------
//
//  Function: Tps659xxGpioPulldown - Pulldown enable/disable
//
BOOL
Tps659xxGpioPulldown(
    HANDLE context,
    UINT id,
    UINT enable
    )
{
    BOOL rc = FALSE;
    UINT bank = GPIO_BANK(id);
    UINT subGroup = GPIO_SUBGROUP(id);
    UINT subIndex = GPIO_SUBINDEX(id);

    UNREFERENCED_PARAMETER(context);

    if (id < TPS659XX_MAX_GPIO_COUNT)
        {
        UINT8 val;
        UINT pulldownState = 0;

        Lock();
        SetGpioBankPowerState(id, D0);

        // set pullup state
        if (enable) pulldownState = PULLDOWN_ENABLE;

        ReadTwlReg(s_rgGpioRegs[bank].rgSubGroup[subGroup].GPIOPUPDCTR, &val, sizeof(val));
        val &= ~(PULLDOWN_ENABLE << subIndex);
        val |= (pulldownState << subIndex);
        WriteTwlReg(s_rgGpioRegs[bank].rgSubGroup[subGroup].GPIOPUPDCTR, &val, sizeof(val));

        SetGpioBankPowerState(id, D4);
        Unlock();
        rc = TRUE;
        }

    return rc;
}
#ifdef DEVICE

//------------------------------------------------------------------------------
//
//  Function: Tps659xxGpioInterruptInitialize - Sets the interrupt event
//

BOOL
Tps659xxGpioInterruptInitialize(
    HANDLE context,
    DWORD id,
    DWORD* sysintr,
    HANDLE hEvent
    )
{
    DWORD dwTritonIntr;
    BOOL rc = FALSE;

    UNREFERENCED_PARAMETER(context);

    // If TWL isn't open, try to open it...
    if ((s_hTritonDriver == NULL) && !OpenTwl()) goto cleanUp;
    
    dwTritonIntr = MAP_TO_TRITON_INTR(id);
    // Call driver
    rc = TWLInterruptInitialize(s_hTritonDriver, dwTritonIntr, hEvent);

    if (rc == TRUE)
    {
        rc = TWLInterruptMask(s_hTritonDriver, dwTritonIntr, FALSE);        
    }

    if (rc == TRUE)
    {
        *sysintr = dwTritonIntr;
    }

cleanUp:
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  Tps659xxGpioInterruptMask
//
//  
//
VOID Tps659xxGpioInterruptMask(HANDLE hContext, DWORD id, DWORD sysintr, BOOL fDisable)
{
    
    UNREFERENCED_PARAMETER(hContext);
    UNREFERENCED_PARAMETER(sysintr);
    // If TWL isn't open, try to open it...
    if ((s_hTritonDriver == NULL) && !OpenTwl()) goto cleanUp;

    TWLInterruptMask(s_hTritonDriver, MAP_TO_TRITON_INTR(id), fDisable);
    


cleanUp:
    return;
}



VOID
Tps659xxGpioInterruptDone(
    HANDLE context,
    DWORD id,
    DWORD sysintr
    )
{
    BOOL rc = FALSE;

    UNREFERENCED_PARAMETER(context);
    UNREFERENCED_PARAMETER(sysintr);

    // If TWL isn't open, try to open it...
    if ((s_hTritonDriver == NULL) && !OpenTwl()) goto cleanUp;

    // Call driver
    rc = TWLInterruptMask(s_hTritonDriver, MAP_TO_TRITON_INTR(id), FALSE);

cleanUp:
    return;
}

VOID
Tps659xxGpioInterruptDisable(
    HANDLE context,
    DWORD id,
    DWORD sysintr
    )
{
    BOOL rc = FALSE;

    UNREFERENCED_PARAMETER(context);
    UNREFERENCED_PARAMETER(sysintr);

    // If TWL isn't open, try to open it...
    if ((s_hTritonDriver == NULL) && !OpenTwl()) goto cleanUp;

    // Call driver
    rc = TWLInterruptDisable(s_hTritonDriver, MAP_TO_TRITON_INTR(id));

cleanUp:
    return ;
}

//------------------------------------------------------------------------------
//
//  Function: Tps659xxGpioWakeEnable - Enable GPIO pin as a wake up 
//                interrupt pin
//
BOOL
Tps659xxGpioWakeEnable(
    HANDLE context,
    DWORD id,
    DWORD sysintr,
    BOOL bEnable
    )
{
    BOOL rc = FALSE;

    UNREFERENCED_PARAMETER(context);
    UNREFERENCED_PARAMETER(sysintr);

    // If TWL isn't open, try to open it...
    if ((s_hTritonDriver == NULL) && !OpenTwl()) goto cleanUp;

    // Call driver
    rc = TWLWakeEnable(s_hTritonDriver, MAP_TO_TRITON_INTR(id), bEnable);

cleanUp:
    return rc;
}

#endif



//***************************************END ADDITION********************************************


//------------------------------------------------------------------------------
//
//  Function: Tps659xxGpioSetBit - Set the value of the GPIO output pin
//
BOOL
Tps659xxGpioSetBit(
    HANDLE context,
    UINT id
    )
{
    BOOL rc = FALSE;
    UINT bit = GPIO_BIT(id);
    UINT bank = GPIO_BANK(id);

    UNREFERENCED_PARAMETER(context);

//    RETAILMSG(1,(TEXT("+Tps659xxGpioSetBit %d\r\n"),id));
    if (id < TPS659XX_MAX_GPIO_COUNT)
        {
        UINT8 val;

        SetGpioBankPowerState(id, D0);
        val = (UINT8) ((1 << bit));
        WriteTwlReg(s_rgGpioRegs[bank].SETGPIODATAOUT, &val, sizeof(val));
        SetGpioBankPowerState(id, D4);
        rc = TRUE;
        }
    return rc;
}




//------------------------------------------------------------------------------
//
//  Function: Tps659xxGpioClrBit
//
BOOL
Tps659xxGpioClrBit(
    HANDLE context,
    UINT id
    )
{
    BOOL rc = FALSE;
    UINT bit = GPIO_BIT(id);
    UINT bank = GPIO_BANK(id);

    UNREFERENCED_PARAMETER(context);

    if (id < TPS659XX_MAX_GPIO_COUNT)
        {
        UINT8 val;

        SetGpioBankPowerState(id, D0);
        val = (UINT8) ((1 << bit));
        WriteTwlReg(s_rgGpioRegs[bank].CLEARGPIODATAOUT, &val, sizeof(val));
        SetGpioBankPowerState(id, D4);
        rc = TRUE;
        }

    return rc;
}

//------------------------------------------------------------------------------
//
//  Function: Tps659xxGpioGetBit
//
DWORD
Tps659xxGpioGetBit(
               HANDLE context,
               UINT id
               )
{    
    DWORD value = (DWORD) -1;
    UINT bit = GPIO_BIT(id);
    UINT bank = GPIO_BANK(id);

    UNREFERENCED_PARAMETER(context);
    if (id < TPS659XX_MAX_GPIO_COUNT)
        {
        UINT8 val;
        SetGpioBankPowerState(id, D0);
        ReadTwlReg(s_rgGpioRegs[bank].GPIODATAIN, &val, sizeof(val));
        value = (val & (1 << bit)) ? 1 : 0;
        SetGpioBankPowerState(id, D4);        
        }

    return value;
}

//------------------------------------------------------------------------------
//
//  Function:  Tps659xxGpioIoControl
//
//  This function sends a command to a device.
//
BOOL
Tps659xxGpioIoControl(
    HANDLE  context,
    UINT    code,
    UCHAR  *pInBuffer,
    UINT    inSize,
    UCHAR  *pOutBuffer,
    UINT    outSize,
    DWORD  *pOutSize
    )
{
#ifdef DEVICE
    UINT id;
    DWORD value,mode;
    DEVICE_IFC_GPIO ifc;
    BOOL rc = FALSE;
//    Tps659xxGpioDevice_t *pDevice = (Tps659xxGpioDevice_t*)context;

    UNREFERENCED_PARAMETER(pOutSize);

    DEBUGMSG(ZONE_FUNCTION, (
        L"+Tps659xxGpioIOControl(0x%08x, 0x%08x, 0x%08x, %d, 0x%08x, %d, 0x%08x)\r\n",
        context, code, pInBuffer, inSize, pOutBuffer, outSize, pOutSize
        ));

    // Check if we get correct context
    /*if ((pDevice == NULL) || (pDevice->cookie != GPIO_DEVICE_COOKIE))
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: Tps659xxGpioIOControl: "
            L"Incorrect context parameter\r\n"
            ));
        goto cleanUp;
    }*/
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
            ifc.pfnSetBit               = Tps659xxGpioSetBit;
            ifc.pfnClrBit               = Tps659xxGpioClrBit;
            ifc.pfnGetBit               = Tps659xxGpioGetBit;
            ifc.pfnSetMode              = Tps659xxGpioSetMode;
            ifc.pfnGetMode              = Tps659xxGpioGetMode;
            ifc.pfnPullup               = Tps659xxGpioPullup;
            ifc.pfnPulldown             = Tps659xxGpioPulldown;
            ifc.pfnInterruptInitialize  = Tps659xxGpioInterruptInitialize;
            ifc.pfnInterruptMask        = Tps659xxGpioInterruptMask;
            ifc.pfnInterruptDisable     = Tps659xxGpioInterruptDisable;
            ifc.pfnInterruptDone        = Tps659xxGpioInterruptDone;
			ifc.pfnGetSystemIrq			= NULL;
            ifc.pfnInterruptWakeUp      = Tps659xxGpioWakeEnable;
            ifc.pfnIoControl            = Tps659xxGpioIoControl;

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
        Tps659xxGpioSetBit(context, id);
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
        Tps659xxGpioClrBit(context, id);
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
        value = Tps659xxGpioGetBit(context, id);
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
        Tps659xxGpioSetMode(context, id, mode);
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
        mode = Tps659xxGpioGetMode(context, id);
        if (!CeSafeCopyMemory(pOutBuffer, &mode, sizeof(mode)))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            break;
        }
        rc = TRUE;
        break;
    }

cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"-Tps659xxGpioIOControl(rc = %d)\r\n", rc));
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

#ifdef DEVICE
//------------------------------------------------------------------------------
//
//  Function:  Tps659xxGpioPowerUp
//
//  This function restores power to a device.
//
VOID
Tps659xxGpioPowerUp(
    HANDLE context
    )
{
    UNREFERENCED_PARAMETER(context);
}

//------------------------------------------------------------------------------
//
//  Function:  Tps659xxGpioPowerDown
//
//  This function suspends power to the device.
//
VOID
Tps659xxGpioPowerDown(
    HANDLE context
    )
{
    UNREFERENCED_PARAMETER(context);
}
#endif
//------------------------------------------------------------------------------


#ifdef DEVICE
DWORD GIO_Init(
               LPCTSTR pContext,
               DWORD dwBusContext
               )
{
    HANDLE devCtxt = (HANDLE) 0x1;
    UINT count;

    UNREFERENCED_PARAMETER(dwBusContext);

    if (Tps659xxGpioInit(pContext,&devCtxt,&count))
    {
        if (Tps659xxGpioPostInit(devCtxt))
        {
            return (DWORD) devCtxt;
        }
    }
    return NULL;
}

BOOL GIO_Deinit(
                DWORD hDeviceContext 
                )
{
    return Tps659xxGpioDeinit((HANDLE)hDeviceContext);
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

void GIO_PowerDown(
                   DWORD hDeviceContext 
                   )
{
    Tps659xxGpioPowerDown((HANDLE) hDeviceContext);
}
void GIO_PowerUp(
                 DWORD hDeviceContext 
                 )
{
    Tps659xxGpioPowerUp((HANDLE) hDeviceContext);
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
    return Tps659xxGpioIoControl(
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