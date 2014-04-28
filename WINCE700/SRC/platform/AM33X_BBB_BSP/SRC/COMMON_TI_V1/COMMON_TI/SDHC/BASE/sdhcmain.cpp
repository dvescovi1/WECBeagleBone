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
//  File: sdhcmain.cpp
//
//  Driver entry points
//
#include "SDHC.h"
#include <nkintr.h>

// disable PREFAST warning for use of EXCEPTION_EXECUTE_HANDLER
#pragma warning (disable: 6320)
#ifndef SHIP_BUILD

DBGPARAM dpCurSettings = {
    L"SDHC", {
        L"Interrupts",      L"Send Handler",    L"Responses",           L"Receive Data",
        L"Clock Control",   L"Transmit Data",   L"SDBus Interaction",   L"Card Busy State",
        L"DVFS",            L"CELOGMSG",        L"Functions",           L"Information",
        L"Init",            L"Warnings",        L"Errors",              L"Undefined"
    },
    ZONE_ENABLE_INIT | ZONE_ENABLE_ERROR | ZONE_ENABLE_WARN | ZONE_ENABLE_INFO
};

#endif


void SDHCDDeinit( HANDLE h );

///////////////////////////////////////////////////////////////////////////////
//  DllEntry - the main dll entry point
//  Input:  hInstance - the instance that is attaching
//          Reason - the reason for attaching
//          pReserved - not much
//  Output: 
//  Return: Always TRUE
//  Notes:  this is only used to initialize the zones
///////////////////////////////////////////////////////////////////////////////
STDAPI_(BOOL) DllEntry(HINSTANCE  hInstance,
                       ULONG      Reason,
                       LPVOID     pReserved)
{
	UNREFERENCED_PARAMETER(pReserved);
    if ( Reason == DLL_PROCESS_ATTACH ) {
        RETAILREGISTERZONES((HMODULE)hInstance);
        DisableThreadLibraryCalls((HMODULE) hInstance);

        if( !SDInitializeCardLib() )
        {
            return FALSE;
        }
        else if( !SD_API_SUCCESS( SDHCDInitializeHCLib() ) )
        {
            SDDeinitializeCardLib();
            return FALSE;
        }
    }
    else if ( Reason == DLL_PROCESS_DETACH ) 
    {
        SDHCDDeinitializeHCLib();
        SDDeinitializeCardLib();
    }
    
    return(TRUE);
}


extern "C"
BOOL SHC_Deinit(DWORD hDeviceContext);

///////////////////////////////////////////////////////////////////////////////
//  Init - the init entry point for the CE driver instance
//  Input:  dwContext - the context passed from device manager
//  Output: 
//  Return: return DWORD identifer for the instance
//  Notes:  
///////////////////////////////////////////////////////////////////////////////
extern "C"
DWORD SHC_Init(DWORD dwContext)
{
    LPCTSTR pszActiveKey = (LPCTSTR) dwContext;
    DWORD   dwRet = 0;

    DEBUGMSG(SDCARD_ZONE_INFO, (TEXT("SDHC +Init\r\n")));    
    DEBUGMSG(SDCARD_ZONE_INFO, (TEXT("SDHC Active RegPath: %s \r\n"), pszActiveKey));

    PCSDIOControllerBase pController = CreateSDIOController();
    if (!pController) {
        DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDHC Failed to create controller object\r\n")));
        goto EXIT;
    }

    if (!pController->Init(pszActiveKey)) {
        goto EXIT;
    }

    // Return the controller instance
    dwRet = (DWORD) pController;

EXIT:
    if ( (dwRet == 0) && pController ) {
        SHC_Deinit((DWORD) pController);
    }

    DEBUGMSG(SDCARD_ZONE_INFO, (TEXT("SDHC -Init\r\n")));

    return dwRet;
}


///////////////////////////////////////////////////////////////////////////////
//  Deinit - the deinit entry point for the driver
//  Input:  hDeviceContext - the context returned from SHC_Init
//  Output: 
//  Return: TRUE
//  Notes:  
///////////////////////////////////////////////////////////////////////////////
extern "C"
BOOL SHC_Deinit(DWORD hDeviceContext)
{    
    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("SDHC +Deinit\r\n")));
    
    CSDIOControllerBase *pController = (CSDIOControllerBase*) hDeviceContext;    
    pController->FreeHostContext( TRUE, TRUE );
    
    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("SDHC -Deinit\r\n")));
    
    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
//  Open - the open entry point for the driver
//  Input:  hDeviceContext - the context returned from SHC_Init
//  Output: 
//  Return: pController context
//  Notes:  
///////////////////////////////////////////////////////////////////////////////
extern "C"
DWORD SHC_Open(
    DWORD hDeviceContext,
    DWORD,
    DWORD
    )
{
    CSDIOControllerBase * pController = (CSDIOControllerBase*) hDeviceContext;
    return (DWORD) pController;
}


///////////////////////////////////////////////////////////////////////////////
//  Close - the close entry point for the driver
//  Input:  hOpenContext - the context returned from SHC_Open
//  Output: 
//  Return: TRUE
//  Notes:  
///////////////////////////////////////////////////////////////////////////////
extern "C"
BOOL SHC_Close(
    DWORD hOpenContext
    )
{
    CSDIOControllerBase * pController = (CSDIOControllerBase *) hOpenContext;
	UNREFERENCED_PARAMETER(pController);
    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
//  PowerDown - the power down entry point for the driver
//  Input:  hDeviceContext - the device context from SHC_Init
//  Output: 
//  Return: 
//  Notes:  
//      Indicates powerdown 
///////////////////////////////////////////////////////////////////////////////
extern "C"
void SHC_PowerDown(DWORD hDeviceContext)
{
    CSDIOControllerBase * pController = (CSDIOControllerBase*) hDeviceContext;
    ASSERT( pController != NULL );
    pController->PowerDown();
}

///////////////////////////////////////////////////////////////////////////////
//  PowerUp - the power up entry point for the driver
//  Input:  hDeviceContext - the device context from SHC_Init
//  Output: 
//  Return:
//  Notes: 
//          On power up, the indication is made to the bus driver and the
//          IST is triggered in order to remove the current instance 
///////////////////////////////////////////////////////////////////////////////
extern "C"
void SHC_PowerUp(DWORD hDeviceContext)
{
    CSDIOControllerBase * pController = (CSDIOControllerBase*) hDeviceContext;
    ASSERT( pController != NULL );
    pController->PowerUp();
}

///////////////////////////////////////////////////////////////////////////////
//  IOControl - the iocontrol entry point for the driver
//  Input:  hOpenContext - the context returned from SHC_Open
//  Output: 
//  Return: TRUE, if success
//  Notes:  handles power management IOCTLs
///////////////////////////////////////////////////////////////////////////////
extern "C"
BOOL SHC_IOControl(
    DWORD hOpenContext,
    DWORD dwCode,
    PBYTE pBufIn,
    DWORD dwLenIn,
    PBYTE pBufOut,
    DWORD dwLenOut,
    PDWORD pdwActualOut 
)
{
	UNREFERENCED_PARAMETER(dwLenIn);
	UNREFERENCED_PARAMETER(pBufIn);
    BOOL rc = FALSE;
    CSDIOControllerBase * pController = (CSDIOControllerBase*) hOpenContext; 

    DEBUGMSG(SDCARD_ZONE_FUNC, (
        L"+SHC_IOControl(0x%08x, 0x%08x, 0x%08x, %d, 0x%08x, %d, 0x%08x)\r\n",
        hOpenContext, dwCode, pBufIn, dwLenIn, pBufOut, dwLenOut, pdwActualOut
        ));

    // Check if we get correct context
    if (pController == NULL)
    {
        DEBUGMSG(SDCARD_ZONE_ERROR, (L"ERROR: SHC_IOControl: "
            L"Incorrect context paramer\r\n"
            ));
        goto cleanUp;
    }

    switch (dwCode)
    {
        case IOCTL_POWER_CAPABILITIES: 
            DEBUGMSG(SDCARD_ZONE_INFO, (L"SHC: Received IOCTL_POWER_CAPABILITIES\r\n"));
            if (pBufOut && dwLenOut >= sizeof (POWER_CAPABILITIES) && 
                pdwActualOut) 
            {
                    __try 
                    {
                        PPOWER_CAPABILITIES PowerCaps;
                        PowerCaps = (PPOWER_CAPABILITIES)pBufOut;
         
                        // Only supports D0 (permanently on) and D4(off.         
                        memset(PowerCaps, 0, sizeof(*PowerCaps));
                        PowerCaps->DeviceDx = (DX_MASK(D0) | DX_MASK(D4)); 
                        *pdwActualOut = sizeof(*PowerCaps);                        
                        rc = TRUE;
                    }
                    __except(EXCEPTION_EXECUTE_HANDLER) 
                    {
                        DEBUGMSG(SDCARD_ZONE_ERROR, (L"exception in ioctl\r\n"));
                    }
            }
            break;

        // requests a change from one device power state to another
        case IOCTL_POWER_SET: 
            DEBUGMSG(SDCARD_ZONE_INFO,(L"SHC: Received IOCTL_POWER_SET\r\n"));
            if (pBufOut && dwLenOut >= sizeof(CEDEVICE_POWER_STATE)) 
            {
                __try 
                {
                    CEDEVICE_POWER_STATE ReqDx = *(PCEDEVICE_POWER_STATE)pBufOut;
                    switch (ReqDx)
                    {
                        case D0:
                            pController->SetPower(D0);
                            break;

                        case D4:
                            pController->SetPower(D4);
                            break;
                    }
                        
                    *pdwActualOut = sizeof(CEDEVICE_POWER_STATE);
                    DEBUGMSG(SDCARD_ZONE_INFO, (L"SHC: IOCTL_POWER_SET to D%u, D%u \r\n",
                        ReqDx, pController->m_ActualPowerState
                        ));

                    rc = TRUE;
                }
                __except(EXCEPTION_EXECUTE_HANDLER) 
                {
                    DEBUGMSG(SDCARD_ZONE_ERROR, (L"Exception in ioctl\r\n"));
                }
            }
            break;

        // gets the current device power state
        case IOCTL_POWER_GET: 
            DEBUGMSG(SDCARD_ZONE_INFO, (L"SHC: Received IOCTL_POWER_GET\r\n"));
            if (pBufOut != NULL && dwLenOut >= sizeof(CEDEVICE_POWER_STATE)) 
            {
                __try 
                {
                    *(PCEDEVICE_POWER_STATE)pBufOut = pController->m_ActualPowerState;
 
                    rc = TRUE;

                    DEBUGMSG(SDCARD_ZONE_INFO, (L"SHC: "
                            L"IOCTL_POWER_GET to D%u \r\n",
                            pController->m_ActualPowerState
                            ));
                }
                __except(EXCEPTION_EXECUTE_HANDLER) 
                {
                    DEBUGMSG(SDCARD_ZONE_ERROR, (L"Exception in ioctl\r\n"));
                }
            }     
            break;
            
        case IOCTL_CONTEXT_RESTORE:
            if(pController != NULL)
                {
                pController->ContextRestore();
                }
            else
                {
                DEBUGMSG( ZONE_ERROR, (L"SHC_IOControl: IOCTL_CONTEXT_RESTORE_NOTIFY\r\n"));
                }

            rc = TRUE;
            break;

    }

cleanUp:
    DEBUGMSG(SDCARD_ZONE_FUNC, (L"-SHC_IOControl(rc = %d)\r\n", rc));
    return rc;
}

