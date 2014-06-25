// All rights reserved ADENEO EMBEDDED 2010
#include "omap.h"
#include "sdk_gpio.h"
#include "bsp_cfg.h"
#include "ceddkex.h"
#include <nkintr.h>

#include <initguid.h>
#include "gpio_ioctls.h"

static BOOL s_DispatcherInitialized=FALSE;
//------------------------------------------------------------------------------
//
//  Function:  GetGroupByID
//  Helper function to get the gpio group where a pio belongs (basedon the pio id)
static BOOL GetGroupByID(GpioDevice_t *pDevice,DWORD id,int* pGrp)
{    
    int i=0;
    while (pDevice->rgRanges[i] <= id)
    {
        i++;
        if (i >= pDevice->nbGpioGrp)
        {
            break;
        }
    }    
    *pGrp = i-1;
    return TRUE;
} 

BOOL GPIOInit()
{
    int i;
    GpioDevice_t* pDevice;

    BSPGpioInit();
    pDevice = BSPGetGpioDevicesTable();

    // setup range and get handles
    for (i = 0; i < pDevice->nbGpioGrp; ++i)
    {
        DWORD size;
        DEVICE_IFC_GPIO ifc;
        pDevice->rgHandles[i] = CreateFile(pDevice->name[i],0,0,NULL,OPEN_EXISTING,0,NULL);
        if (pDevice->rgHandles[i] != INVALID_HANDLE_VALUE)
        {            
            if (DeviceIoControl(pDevice->rgHandles[i],IOCTL_DDK_GET_DRIVER_IFC,
                (BYTE*) &DEVICE_IFC_GPIO_GUID,sizeof(DEVICE_IFC_GPIO_GUID),
                &ifc,sizeof(DEVICE_IFC_GPIO),&size,NULL) == FALSE)
            {
                pDevice->rgGpioTbls[i] = NULL;
            }
            else
            {            
                pDevice->rgGpioTbls[i] = LocalAlloc(0,sizeof(DEVICE_IFC_GPIO));
                if (pDevice->rgGpioTbls[i] == NULL)
                {
                    goto cleanUp;
                }
                memcpy(pDevice->rgGpioTbls[i],&ifc,sizeof(DEVICE_IFC_GPIO));
            }
        }
        else
        {
            pDevice->rgGpioTbls[i] = NULL;
        }
        

    }

    return TRUE;

cleanUp:
    return FALSE;
}

HANDLE GPIOOpen()
{
    if (s_DispatcherInitialized == FALSE)
    {
        s_DispatcherInitialized = GPIOInit();
    }
    if (s_DispatcherInitialized == FALSE)
    {
        return NULL;
    }
    return BSPGetGpioDevicesTable();
}

VOID GPIOClose(HANDLE hContext)
{
    UNREFERENCED_PARAMETER(hContext);
}

VOID GPIOSetBit(HANDLE hContext, DWORD id)
{
    GpioDevice_t *pDevice = (GpioDevice_t*)hContext;
    int grp;
    if (GetGroupByID(pDevice,id,&grp))
    {
        id -= pDevice->rgRanges[grp];

        if (pDevice->rgGpioTbls[grp])
            pDevice->rgGpioTbls[grp]->pfnSetBit(pDevice->rgGpioTbls[grp]->context, id);
        else
        {
            DeviceIoControl(pDevice->rgHandles[grp],IOCTL_GPIO_SETBIT,&id,sizeof(id),NULL,0,NULL,NULL);
        }
    }
}

VOID GPIOClrBit(HANDLE hContext, DWORD id)
{
    GpioDevice_t *pDevice = (GpioDevice_t*)hContext;
    int grp;
    if (GetGroupByID(pDevice,id,&grp))
    {
        id -= pDevice->rgRanges[grp];

        if (pDevice->rgGpioTbls[grp])
        {
            pDevice->rgGpioTbls[grp]->pfnClrBit(pDevice->rgGpioTbls[grp]->context, id);
        }
        else
        {
            DeviceIoControl(pDevice->rgHandles[grp],IOCTL_GPIO_CLRBIT,&id,sizeof(id),NULL,0,NULL,NULL);
        }
    }
}

DWORD GPIOGetBit(HANDLE hContext, DWORD id)
{
    GpioDevice_t *pDevice = (GpioDevice_t*)hContext;
    int grp;
    if (GetGroupByID(pDevice,id,&grp))
    {
        id -= pDevice->rgRanges[grp];

        if (pDevice->rgGpioTbls[grp])
        {
            return pDevice->rgGpioTbls[grp]->pfnGetBit(pDevice->rgGpioTbls[grp]->context, id);
        }
        else
        {
            DWORD value;
            if (DeviceIoControl(pDevice->rgHandles[grp],IOCTL_GPIO_GETBIT,&id,sizeof(id),&value,sizeof(value),NULL,NULL))
            {
                return value;
            }
        }
    }
    return (DWORD) -1;
}

VOID GPIOSetMode(HANDLE hContext, DWORD id, DWORD mode)
{
    GpioDevice_t *pDevice = (GpioDevice_t*)hContext;
    int grp;
    if (GetGroupByID(pDevice,id,&grp))
    {
        id -= pDevice->rgRanges[grp];

        if (pDevice->rgGpioTbls[grp])
        {
            pDevice->rgGpioTbls[grp]->pfnSetMode((HANDLE) pDevice->rgGpioTbls[grp]->context, id, mode);
        }
        else
        {
            DWORD config[2];
            config[0] = id;
            config[1] = mode;

            DeviceIoControl(pDevice->rgHandles[grp],IOCTL_GPIO_SETMODE,config,sizeof(config),NULL,0,NULL,NULL);

        }
    }
}

DWORD GPIOGetMode(HANDLE hContext, DWORD id)
{
    GpioDevice_t *pDevice = (GpioDevice_t*)hContext;
    int grp;
    if (GetGroupByID(pDevice,id,&grp))
    {
        id -= pDevice->rgRanges[grp];

        if (pDevice->rgGpioTbls[grp])
        {
            return pDevice->rgGpioTbls[grp]->pfnGetMode(pDevice->rgGpioTbls[grp]->context, id);
        }
        else
        {
            DWORD mode;
            if (DeviceIoControl(pDevice->rgHandles[grp],IOCTL_GPIO_GETMODE,&id,sizeof(id),&mode,sizeof(mode),NULL,NULL))
            {
                return mode;
            }
        }
    }
    return (DWORD) -1;
}

DWORD GPIOPullup(HANDLE hContext,DWORD id,DWORD enable)
{
    GpioDevice_t *pDevice = (GpioDevice_t*)hContext;
    int grp;
    if (GetGroupByID(pDevice,id,&grp))
    {
        id -= pDevice->rgRanges[grp];

        if (pDevice->rgGpioTbls[grp])
        {
            return pDevice->rgGpioTbls[grp]->pfnPullup((HANDLE) pDevice->rgGpioTbls[grp]->context, id, enable);
        }
    }
    return (DWORD) FALSE;
}

DWORD GPIOPulldown(HANDLE hContext, DWORD id,DWORD enable)
{
    GpioDevice_t *pDevice = (GpioDevice_t*)hContext;
    int grp;
    if (GetGroupByID(pDevice,id,&grp))
    {
        id -= pDevice->rgRanges[grp];

        if (pDevice->rgGpioTbls[grp])
        {
            return pDevice->rgGpioTbls[grp]->pfnPulldown((HANDLE) pDevice->rgGpioTbls[grp]->context, id, enable);            
        }
    }
    return (DWORD) FALSE;
}

BOOL GPIOInterruptInitialize(HANDLE hContext, DWORD id, DWORD* sysintr, HANDLE hEvent)
{
    GpioDevice_t *pDevice = (GpioDevice_t*)hContext;
    int grp;
    if (GetGroupByID(pDevice,id,&grp))
    {        
        id -= pDevice->rgRanges[grp];

        if (pDevice->rgGpioTbls[grp])
        {
            return pDevice->rgGpioTbls[grp]->pfnInterruptInitialize((HANDLE) pDevice->rgGpioTbls[grp]->context, id, sysintr, hEvent);            
        }
    }
    return FALSE;
}

VOID GPIOInterruptMask(HANDLE hContext, DWORD id, DWORD dwSysintr, BOOL fDisable)
{
    GpioDevice_t *pDevice = (GpioDevice_t*)hContext;
    int grp;
    if (GetGroupByID(pDevice,id,&grp))
    {
        id -= pDevice->rgRanges[grp];

        if (pDevice->rgGpioTbls[grp])
        {
            pDevice->rgGpioTbls[grp]->pfnInterruptMask((HANDLE) pDevice->rgGpioTbls[grp]->context, id, dwSysintr, fDisable);
        }
    }
}

VOID GPIOInterruptDisable(HANDLE hContext,DWORD id,DWORD dwSysintr)
{
    GpioDevice_t *pDevice = (GpioDevice_t*)hContext;
    int grp;
    if (GetGroupByID(pDevice,id,&grp))
    {
        id -= pDevice->rgRanges[grp];

        if (pDevice->rgGpioTbls[grp])
        {
            pDevice->rgGpioTbls[grp]->pfnInterruptDisable((HANDLE) pDevice->rgGpioTbls[grp]->context, id, dwSysintr);            
        }
    }
}

VOID GPIOInterruptDone(HANDLE hContext, DWORD id, DWORD dwSysintr)
{
    GpioDevice_t *pDevice = (GpioDevice_t*)hContext;
    int grp;
    if (GetGroupByID(pDevice,id,&grp))
    {
        id -= pDevice->rgRanges[grp];

        if (pDevice->rgGpioTbls[grp])
        {
            pDevice->rgGpioTbls[grp]->pfnInterruptDone((HANDLE) pDevice->rgGpioTbls[grp]->context, id, dwSysintr);            
        }
    }
}

DWORD GPIOGetSystemIrq(HANDLE hContext, DWORD id)
{
    GpioDevice_t *pDevice = (GpioDevice_t*)hContext;
    int grp;
    if (GetGroupByID(pDevice,id,&grp))
    {
        id -= pDevice->rgRanges[grp];

        if (pDevice->rgGpioTbls[grp])
        {
            return pDevice->rgGpioTbls[grp]->pfnGetSystemIrq((HANDLE) pDevice->rgGpioTbls[grp]->context, id);            
        }
    }
	return (DWORD)SYSINTR_UNDEFINED;
}

BOOL GPIOInterruptWakeUp(HANDLE hContext, DWORD id, DWORD sysintr, BOOL fEnabled)
{
    BOOL rc = FALSE;
    GpioDevice_t *pDevice = (GpioDevice_t*)hContext;
    int grp;
    if (GetGroupByID(pDevice,id,&grp))
    {
        id -= pDevice->rgRanges[grp];

        if (pDevice->rgGpioTbls[grp])
        {
            return pDevice->rgGpioTbls[grp]->pfnInterruptWakeUp((HANDLE) pDevice->rgGpioTbls[grp]->context, id, sysintr, fEnabled);
        }
    }
	return rc;
}

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
    )
{    
    GpioDevice_t *pDevice = (GpioDevice_t*)hContext;
    int grp;
    BOOL rc = FALSE;

    if (GetGroupByID(pDevice,id,&grp))
    {
        id -= pDevice->rgRanges[grp];
        
        if (pDevice->rgGpioTbls[grp])
            rc = pDevice->rgGpioTbls[grp]->pfnIoControl(pDevice->rgGpioTbls[grp]->context, code, pInBuffer,
                inSize, pOutBuffer, outSize, pOutSize);            
        else
        {
            rc=DeviceIoControl(pDevice->rgHandles[grp], code, pInBuffer,
                inSize, pOutBuffer, outSize, pOutSize, NULL);
        }
        
    }
    return (DWORD) rc;
    
}

