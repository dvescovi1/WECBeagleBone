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
//  File:  sdk_gpio.h
//
//  This header defines interface for GPIO device driver. This driver control
//  GPIO pins on hardware. It allows abstract GPIO interface and break up
//  physicall and logical pins. To avoid overhead involved the driver exposes
//  interface which allows obtain funtion pointers to base set/clr/get etc.
//  functions.
//
#ifndef __SDK_GPIO_H
#define __SDK_GPIO_H

#include "omap.h"


#ifdef __cplusplus
extern "C" {
#endif


//------------------------------------------------------------------------------
//
//  Define:  GPIO_DIR_xxx/GPIO_INT_xxx
//
#define GPIO_DIR_OUTPUT         (0 << 0)
#define GPIO_DIR_INPUT          (1 << 0)
#define GPIO_INT_LOW_HIGH       (1 << 1)
#define GPIO_INT_HIGH_LOW       (1 << 2)
#define GPIO_INT_LOW            (1 << 3)
#define GPIO_INT_HIGH           (1 << 4)
#define GPIO_DEBOUNCE_ENABLE    (1 << 5)

#define GPIO_PULLUP_DISABLE     0
#define GPIO_PULLUP_ENABLE      1
#define GPIO_PULLDOWN_DISABLE   0
#define GPIO_PULLDOWN_ENABLE    1


//------------------------------------------------------------------------------
BOOL GPIOInit();
BOOL GPIOPostInit();

HANDLE GPIOOpen();

VOID GPIOClose(
               HANDLE hContext
               );

VOID GPIOSetBit(
    HANDLE hContext, 
    DWORD id
    );

VOID
GPIOClrBit(
    HANDLE hContext,
    DWORD id
    );

DWORD
GPIOGetBit(
    HANDLE hContext, 
    DWORD id
    );

VOID
GPIOSetMode(
    HANDLE hContext, 
    DWORD id, 
    DWORD mode
    );

 DWORD
GPIOGetMode(
    HANDLE hContext, 
    DWORD id
    );

DWORD
GPIOPullup(
    HANDLE hContext, 
    DWORD id,
    DWORD enable
    );

DWORD
GPIOPulldown(
    HANDLE hContext, 
    DWORD id,
    DWORD enable
    );

BOOL GPIOInterruptInitialize(HANDLE hContext, DWORD id, DWORD* sysintr, HANDLE hEvent);

VOID GPIOInterruptMask(HANDLE hContext, DWORD id, DWORD sysintr, BOOL fDisable);

VOID GPIOInterruptDisable(HANDLE hContext,DWORD id, DWORD sysintr);

VOID GPIOInterruptDone(HANDLE hContext, DWORD id, DWORD sysintr);

DWORD GPIOGetSystemIrq(HANDLE hContext, DWORD id);

BOOL GPIOInterruptWakeUp(HANDLE hContext, DWORD id, DWORD sysintr, BOOL fEnabled);

DWORD
GPIOIoControl(
    HANDLE hContext,
    DWORD  id,
    DWORD  code, 
    UCHAR *pInBuffer, 
    DWORD  inSize, 
    UCHAR *pOutBuffer,
    DWORD  outSize, 
    DWORD *pOutSize
    );
//------------------------------------------------------------------------------
//
//  Type: function prototype
//
//  Predefines a set of function callbacks used to manage gpio's exposed by
//  multiple silicon
//
/*
typedef BOOL (*fnGpioInit)(LPCTSTR szContext, HANDLE *phContext, UINT *pGpioCount);
typedef BOOL (*fnGpioDeinit)(HANDLE hContext);
typedef BOOL (*fnGpioSetMode)(HANDLE hContext, UINT id, UINT mode);
typedef DWORD (*fnGpioGetMode)(HANDLE hContext, UINT id);
typedef BOOL (*fnGpioPullup)(HANDLE hContext, UINT id, UINT enable);
typedef BOOL (*fnGpioPulldown)(HANDLE hContext, UINT id, UINT enable);
typedef BOOL (*fnGpioInterruptInitialize)(HANDLE context, UINT intrID,HANDLE hEvent);
typedef BOOL (*fnGpioInterruptDone)(HANDLE hContext, UINT id);
typedef BOOL (*fnGpioInterruptDisable)(HANDLE hContext, UINT id);
typedef BOOL (*fnGpioSetBit)(HANDLE hContext, UINT id);
typedef BOOL (*fnGpioClrBit)(HANDLE hContext, UINT id);
typedef DWORD (*fnGpioGetBit)(HANDLE hContext, UINT id);
typedef void (*fnGpioPowerUp)(HANDLE hContext);
typedef void (*fnGpioPowerDown)(HANDLE hContext);
typedef BOOL (*fnGpioIoControl)(HANDLE hContext, UINT code, 
                                UCHAR *pinVal, UINT inSize, UCHAR *poutVal, 
                                UINT outSize, DWORD *pOutSize);
/*

//------------------------------------------------------------------------------
//  Type: GPIO_TABLE
//
//  Predefines structure containing gpio call back routines exposed for a
//  silicon
//
typedef struct {
    fnGpioInit      Init;
    fnGpioDeinit    Deinit;
    fnGpioSetMode   SetMode;
    fnGpioGetMode   GetMode;
    fnGpioPullup    Pullup;
    fnGpioPulldown  Pulldown;
    fnGpioInterruptInitialize   InterruptInitialize;
    fnGpioInterruptDone         InterruptDone;
    fnGpioInterruptDisable      InterruptDisable;
    fnGpioSetBit    SetBit;
    fnGpioClrBit    ClrBit;
    fnGpioGetBit    GetBit;
    fnGpioPowerUp   PowerUp;
    fnGpioPowerDown PowerDown;
    fnGpioIoControl IoControl;
} GPIO_TABLE;
*/

//------------------------------------------------------------------------------
//
//  Type:  DEVICE_IFC_GPIO
//
//  This structure is used to obtain GPIO interface funtion pointers used for
//  in-process calls via IOCTL_DDK_GET_DRIVER_IFC.
//
typedef struct {
    HANDLE context;
    BOOL  (*pfnInit)(LPCTSTR szContext, HANDLE *phContext, UINT *pGpioCount);
    BOOL  (*pfnPostInit)(HANDLE hContext);
    BOOL  (*pfnDeinit)(HANDLE hContext);
    BOOL  (*pfnSetBit)(HANDLE context, UINT id);
    BOOL  (*pfnClrBit)(HANDLE context, UINT id);
    DWORD (*pfnGetBit)(HANDLE context, UINT id);
    BOOL  (*pfnSetMode)(HANDLE context, UINT id, UINT mode);
    DWORD (*pfnGetMode)(HANDLE context, UINT id);
    BOOL  (*pfnPullup)(HANDLE context, UINT id, UINT enable);
    BOOL  (*pfnPulldown)(HANDLE context, UINT id, UINT enable);
    BOOL  (*pfnIoControl) (HANDLE context, UINT code, UCHAR *pInBuffer,
          UINT inSize,UCHAR *pOutBuffer,UINT outSize,DWORD *pOutSize);
    BOOL  (*pfnInterruptInitialize) (HANDLE hContext, DWORD id, DWORD* sysintr, HANDLE hEvent);
    VOID  (*pfnInterruptMask) (HANDLE hContext, DWORD id, DWORD sysintr, BOOL fDisable);
    VOID  (*pfnInterruptDisable) (HANDLE hContext,DWORD id, DWORD sysintr);
    VOID  (*pfnInterruptDone) (HANDLE hContext, DWORD id, DWORD sysintr);
    DWORD (*pfnGetSystemIrq) (HANDLE hContext, UINT id);    
    BOOL  (*pfnInterruptWakeUp) (HANDLE hContext, DWORD id, DWORD sysintr, BOOL fEnabled);


} DEVICE_IFC_GPIO;

//------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif // __SDK_GPIO_H
