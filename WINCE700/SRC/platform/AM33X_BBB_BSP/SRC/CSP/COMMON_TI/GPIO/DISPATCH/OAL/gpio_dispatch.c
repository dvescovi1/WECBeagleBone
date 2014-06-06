// All rights reserved ADENEO EMBEDDED 2010
#include "omap.h"
#include "sdk_gpio.h"
#include "bsp_cfg.h"

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
BOOL GPIOPostInit()
{
    int i;
    GpioDevice_t* pDevice;
    pDevice= BSPGetGpioDevicesTable();
    for (i = 0; i < pDevice->nbGpioGrp; ++i)
    {   
        if (pDevice->rgGpioTbls[i]->pfnPostInit)
        {
            if (pDevice->rgGpioTbls[i]->pfnPostInit(pDevice->rgHandles[i]) == FALSE)
            {
                return FALSE;
            }
        }
    }
    return TRUE;
}

BOOL GPIOInit()
{
    int i;
    UINT size;
    GpioDevice_t* pDevice;
    HANDLE hGpio;

    BSPGpioInit();
    pDevice= BSPGetGpioDevicesTable();
    // setup range and get handles
    for (i = 0; i < pDevice->nbGpioGrp; ++i)
    {
        if (pDevice->rgGpioTbls[i]->pfnInit(L"boot", &hGpio, &size) == FALSE)
        {
            RETAILMSG(ZONE_ERROR, (L"ERROR: GIO_Init: "
                L"Failed to initialize Gpio table index(%d)\r\n", i
                ));
            goto cleanUp;
        }
        pDevice->rgHandles[i] = hGpio;
    }
    return TRUE;
    
cleanUp:

    return FALSE;
}

HANDLE GPIOOpen()
{
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
        pDevice->rgGpioTbls[grp]->pfnSetBit(pDevice->rgHandles[grp], id - pDevice->rgRanges[grp]);
    }
}

VOID GPIOClrBit(HANDLE hContext, DWORD id)
{
    GpioDevice_t *pDevice = (GpioDevice_t*)hContext;
    int grp;
    if (GetGroupByID(pDevice,id,&grp))
    {
        pDevice->rgGpioTbls[grp]->pfnClrBit(pDevice->rgHandles[grp], id - pDevice->rgRanges[grp]);
    }
}

DWORD GPIOGetBit(HANDLE hContext, DWORD id)
{
    GpioDevice_t *pDevice = (GpioDevice_t*)hContext;
    int grp;
    if (GetGroupByID(pDevice,id,&grp))
    {
        return pDevice->rgGpioTbls[grp]->pfnGetBit(pDevice->rgHandles[grp], id - pDevice->rgRanges[grp]);
    }
    return (DWORD) -1;
}

VOID GPIOSetMode(HANDLE hContext, DWORD id, DWORD mode)
{
    GpioDevice_t *pDevice = (GpioDevice_t*)hContext;
    int grp;
    if (GetGroupByID(pDevice,id,&grp))
    {
        pDevice->rgGpioTbls[grp]->pfnSetMode((HANDLE) pDevice->rgHandles[grp], id - pDevice->rgRanges[grp], mode);
    }
}

DWORD GPIOGetMode(HANDLE hContext, DWORD id)
{
    GpioDevice_t *pDevice = (GpioDevice_t*)hContext;
    int grp;
    if (GetGroupByID(pDevice,id,&grp))
    {
        return pDevice->rgGpioTbls[grp]->pfnGetMode((HANDLE) pDevice->rgHandles[grp], id - pDevice->rgRanges[grp]);        
    }
    return (DWORD) -1;
}

DWORD GPIOPullup(HANDLE hContext,DWORD id,DWORD enable)
{
    GpioDevice_t *pDevice = (GpioDevice_t*)hContext;
    int grp;
    if (GetGroupByID(pDevice,id,&grp))
    {
        pDevice->rgGpioTbls[grp]->pfnPullup((HANDLE) pDevice->rgHandles[grp], id - pDevice->rgRanges[grp], enable);
    }
    return (DWORD) TRUE;
}

DWORD GPIOPulldown(HANDLE hContext, DWORD id,DWORD enable)
{
    GpioDevice_t *pDevice = (GpioDevice_t*)hContext;
    int grp;
    if (GetGroupByID(pDevice,id,&grp))
    {
        pDevice->rgGpioTbls[grp]->pfnPulldown((HANDLE) pDevice->rgHandles[grp], id - pDevice->rgRanges[grp], enable);
    }
    return (DWORD) TRUE;
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
    //UNREFERENCED_PARAMETER(pOverlap);
    GpioDevice_t *pDevice = (GpioDevice_t*)hContext;
    int grp;
    BOOL rc = FALSE;
    
    if (GetGroupByID(pDevice,id,&grp))
    {
        rc = pDevice->rgGpioTbls[grp]->pfnIoControl((HANDLE) pDevice->rgHandles[grp], code, pInBuffer,
                inSize, pOutBuffer, outSize, pOutSize);
    }    
    return (DWORD) rc;    
}

