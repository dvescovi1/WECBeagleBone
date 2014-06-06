// All rights reserved Texas Instruments, Inc. 2011
// All rights reserved ADENEO EMBEDDED 2010
//
// Copyright (c) MPC Data Limited 2007.  All rights reserved.
//
//------------------------------------------------------------------------------
//
//  File:  hcd_pdd.cpp
//
////////////////////////////////////////////////////////////////////////////////
//
// Copyright 1999,2000 by Texas Instruments Incorporated. All rights reserved.
// Property of Texas Instruments Incorporated. Restricted rights to use,
// duplicate or disclose this code are granted through contract.
//
////////////////////////////////////////////////////////////////////////////////
//
// Platform dependant part of the USB Open Host Controller Driver (OHCD).
//
// Functions:
//
//  HcdPdd_DllMain
//  HcdPdd_Init
//  HcdPdd_CheckConfigPower
//  HcdPdd_PowerUp
//  HcdPdd_PowerDown
//  HcdPdd_Deinit
//  HcdPdd_Open
//  HcdPdd_Close
//  HcdPdd_Read
//  HcdPdd_Write
//  HcdPdd_Seek
//  HcdPdd_IOControl
//
////////////////////////////////////////////////////////////////////////////////

#pragma warning(push)
#pragma warning(disable: 4510 4512 4610)
#include <windows.h>
#include <types.h>
#include <memory.h>
#include <ceddk.h>

#include <ohcdddsi.h>
#include <nkintr.h>

#include "am33x.h"
#include "am33x_usb.h"
#include "am33x_usbcdma.h"
#include "oal_clock.h"
#include "sdk_padcfg.h"
#include "drvcommon.h"
#include "testmode.h"

#pragma warning(pop)

#pragma warning(disable:4068 6320)

/* HcdPdd_DllMain */
extern BOOL HcdPdd_DllMain(HANDLE hinstDLL, DWORD dwReason, LPVOID lpvReserved)
{
	UNREFERENCED_PARAMETER(hinstDLL);
	UNREFERENCED_PARAMETER(lpvReserved);

#ifdef DEBUG 
	dpCurSettings.ulZoneMask |= 0x0180;
#endif

    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        DEBUGMSG(ZONE_INIT, (TEXT("USBH: HcdPdd_DllMain DLL_PROCESS_ATTACH\r\n")));
        break;
    case DLL_PROCESS_DETACH:
        DEBUGMSG(ZONE_INIT, (TEXT("USBH: HcdPdd_DllMain DLL_PROCESS_DETACH\r\n")));
        break;
    }

    return TRUE;
}

/******************************************************************************/
DWORD HcdPdd_SetDevicePower(SOhcdPdd *pPddObject)
// Updates device power state according the SOhcdPdd::CurrentDx field.
{
    DEBUGMSG(ZONE_FUNCTION, (TEXT("USBH: HcdPdd_SetDevicePower: State %d\r\n"),
        pPddObject->CurrentDx));

	if (!pPddObject)
        return ERROR_INVALID_PARAMETER;

    if(pPddObject->dwDisablePowerManagement != 0)
    {
        DEBUGMSG(ZONE_FUNCTION, (TEXT("USBH: HcdPdd_SetDevicePower: Power management disabled\r\n")));
		pPddObject->ioPortBase->DEVCTL |= BIT0;		
        return ERROR_SUCCESS;
    }

    switch (pPddObject->CurrentDx)
    {
    case D0: // Power on
	case D1: // Power on
	case D2: // Power on


// we are not coming here if OTG is enabled, so consider HOST mode only     
		USBCDMA_EnableClocks(pPddObject->dwIndex - 1 ,TRUE);

		if (pPddObject->dwIndex == 1){
			pPddObject->pSysConfReg->USB_CTRL0 = 0x3; 
			StallExecution(100);
			pPddObject->pSysConfReg->USB_CTRL0 |= (3 << 19);
			pPddObject->pSysConfReg->USB_CTRL0 &= ~0x03;
		} else {
			pPddObject->pSysConfReg->USB_CTRL1 = 0x3; 
			StallExecution(100);
			pPddObject->pSysConfReg->USB_CTRL1 |= (3 << 19);
			pPddObject->pSysConfReg->USB_CTRL1 &= ~0x03;
		}
		StallExecution(100);
		
	    // Reset the controller
	    pPddObject->ioPortBase->CTRLR |= 1;
		StallExecution(10);
		while (pPddObject->ioPortBase->CTRLR & 1); //  wait for reset done

		pPddObject->ioPortBase->DEVCTL |= BIT0;

        RETAILMSG(1, (L"UsbPowerModule: 2 DEVCTL = 0x%08X\r\n", pPddObject->ioPortBase->DEVCTL));
        DEBUGMSG(0, (L"UsbPowerModule: 2 DEVCTL = 0x%08X\r\n", pPddObject->ioPortBase->DEVCTL));
        break;

    case D3: // Sleep
    case D4: // Power off

		pPddObject->ioPortBase->DEVCTL &= ~BIT0;

		if (pPddObject->dwIndex == 0){
			pPddObject->pSysConfReg->USB_CTRL0 = 0x3; 
		} else {
			pPddObject->pSysConfReg->USB_CTRL1 = 0x3; 
		}
		StallExecution(100);

		USBCDMA_EnableClocks(pPddObject->dwIndex - 1 ,FALSE);

        break;

    default:

        return ERROR_INVALID_PARAMETER;
    }

    return ERROR_SUCCESS;
}

extern DWORD HcdPdd_Init(DWORD dwContext)
//	PDD Entry point - called at system init to detect and configure OHCI card.
{
    SOhcdPdd *pPddObject = NULL;
    BOOL fRet = FALSE;
    PHYSICAL_ADDRESS PhysAddr = {0};
    DWORD IrqVal = IRQ_USBINT0;
    DWORD BytesRet;
    HKEY hKey = NULL;
 
    DEBUGMSG(ZONE_FUNCTION, (TEXT("USBH:+HcdPdd_Init:\r\n")));
    // Initialise platform PDD
    if (!USBHPDD_Init())
    {
        DEBUGMSG(ZONE_ERROR, (L"HcdPdd_Init: Failed to initialise platform USBH PDD\r\n"));
        goto _clean;
    }
    // Start with VBUS power off
    USBHPDD_PowerVBUS(FALSE);

    // PDD context
    pPddObject = new SOhcdPdd;

    if (!pPddObject)
    {
        DEBUGMSG(ZONE_ERROR, (L"HcdPdd_Init: Failed to alloc PDD context!\r\n"));
        goto _clean;
    }

    memset(pPddObject, 0, sizeof(*pPddObject));

    // Save reg path
    _tcsncpy(pPddObject->szDriverRegKey, (LPCTSTR)dwContext, MAX_PATH);

    // Read registry settings
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, (LPCTSTR)dwContext, 0, 0, &hKey) != ERROR_SUCCESS)
    {
        DEBUGMSG(ZONE_ERROR, (L"HcdPdd_Init: Failed to open registry key\r\n"));
        goto _clean;
    }
    else
    {
        DWORD dind = sizeof(pPddObject->dwIndex);
        if (RegQueryValueEx(hKey, _T("Index"), NULL, NULL, 
			(LPBYTE)&pPddObject->dwIndex, &dind) != ERROR_SUCCESS)
        {
            RETAILMSG(1, (TEXT("USBH: HcdPdd_Init: Failed to read Index from registry\r\n")));
            goto _clean;
        }

        // Read descriptor count value
        pPddObject->dwDescriptorCount = CPPI_HD_COUNT; // hardcoded as must be the same for USBH and USBFN on the both ports
#if 0
        DWORD cb = sizeof(pPddObject->dwDescriptorCount);
        if (RegQueryValueEx(hKey, _T("DescriptorCount"),NULL,NULL,
            (LPBYTE)&pPddObject->dwDescriptorCount, &cb) != ERROR_SUCCESS)
        {
            DEBUGMSG(ZONE_FUNCTION, (TEXT("USBH: HcdPdd_Init: Failed to read DescriptorCount from registry\r\n")));
            goto _clean;
        }
#endif
        // Read power management disable flag
        DWORD dpm = sizeof(pPddObject->dwDisablePowerManagement);
        if (RegQueryValueEx(hKey, _T("DisablePowerManagement"), NULL, NULL,
            (LPBYTE)&pPddObject->dwDisablePowerManagement, &dpm) != ERROR_SUCCESS)
        {
            // Enable power management by default
            pPddObject->dwDisablePowerManagement = 0;
        }
    }

    // Map registers
	IrqVal = (pPddObject->dwIndex == 1) ? IRQ_USBINT0 : IRQ_USBINT1;

    // map the HC register space to VM
    PhysAddr.LowPart = (pPddObject->dwIndex == 1) ? AM33X_USB0_REGS_PA : AM33X_USB1_REGS_PA;
    pPddObject->ioPortBase = (CSL_UsbRegs*) MmMapIoSpace (PhysAddr, sizeof(CSL_UsbRegs), FALSE);

    // Map the CPPI register space to VM
    PhysAddr.LowPart = AM33X_USBSS_REGS_PA;
    pPddObject->usbss = (CSL_Usbss_Regs*) MmMapIoSpace(PhysAddr, sizeof(CSL_Usbss_Regs), FALSE);

	// Map the CPPI register space to VM
    PhysAddr.LowPart = AM33X_CPPI_REGS_PA;
    pPddObject->ioCppiBase = (CSL_CppiRegs *)MmMapIoSpace(PhysAddr, sizeof(CSL_CppiRegs), FALSE);

	// Map the SYS CONF register space to VM
    PhysAddr.LowPart = AM33X_DEVICE_CONF_REGS_PA;
    pPddObject->pSysConfReg = (AM33X_DEVICE_CONF_REGS *)MmMapIoSpace(PhysAddr, sizeof(AM33X_DEVICE_CONF_REGS), FALSE);


    if ((NULL == pPddObject->ioPortBase) || (NULL == pPddObject->ioCppiBase) ||
        (NULL == pPddObject->usbss) || (NULL == pPddObject->pSysConfReg)) {
        DEBUGMSG(ZONE_ERROR, (L"HcdPdd_Init: Failed to map registers!\r\n"));
        goto _clean;
    }

    // Request SYSINTR ID
    if (!KernelIoControl(IOCTL_HAL_REQUEST_SYSINTR,  &IrqVal, sizeof(DWORD),
            &pPddObject->dwSysIntr, sizeof(DWORD), &BytesRet))
    {
        DEBUGMSG(ZONE_ERROR, (L"HcdPdd_Init: Failed to request SYSINTR for IRQ%d!\r\n",
            IrqVal));
        goto _clean;
    }

#ifdef ENABLE_TESTMODE_SUPPORT
    InitUsbTestMode(pPddObject->ioPortBase);
#endif

if (pPddObject->dwIndex == 1){
#ifdef USB0_OTG_EN
    pPddObject->ioPortBase->MODE_R &=  ~0x080; // USB0 controlled by USB-ID Pin
#else
	// make the port be host w/o sensing ID pin for USB0 
	pPddObject->ioPortBase->MODE_R &= ~0x100;
    pPddObject->ioPortBase->MODE_R |=  0x080;
#endif
} else {
#ifdef USB1_OTG_EN
    pPddObject->ioPortBase->MODE_R &=  ~0x080; // USB1 controlled by USB-ID Pin
#else
	// make the port be host w/o sensing ID pin for USB1 
	pPddObject->ioPortBase->MODE_R &= ~0x100;
    pPddObject->ioPortBase->MODE_R |=  0x080;
#endif
}

	pPddObject->ioPortBase->PHY_UTMI_R = 0x2;   // ???
    // Set the initial power state to Full On
    pPddObject->CurrentDx = D0;
    fRet = (HcdPdd_SetDevicePower(pPddObject) == ERROR_SUCCESS);
    if (fRet)
    {
        // Create the memory and hcd object's
        pPddObject->lpvMemoryObject = HcdMdd_CreateMemoryObject(gcTotalAvailablePhysicalMemory,
                                                                gcHighPriorityPhysicalMemory, NULL, NULL);

        pPddObject->lpvOhcdMddObject = HcdMdd_CreateHcdObject(pPddObject,
                                                              pPddObject->lpvMemoryObject,
                                                              pPddObject->szDriverRegKey,
                                                              (PUCHAR)pPddObject->ioPortBase,
                                                              pPddObject->dwSysIntr);

        if (!pPddObject->lpvOhcdMddObject || !pPddObject->lpvMemoryObject)
        {
            DEBUGCHK(FALSE);
            fRet = FALSE;
        }
    }
    else
    {
        DEBUGMSG(ZONE_FUNCTION, (TEXT("USBH: HcdPdd_Init: HcdPdd_SetDevicePower failed\r\n")));
    }

_clean:
    if (hKey)
        RegCloseKey(hKey);

    if (!fRet)
    {
        // Cleanup
        if (pPddObject)
        {
            if (pPddObject->usbss)
                MmUnmapIoSpace(pPddObject->usbss, sizeof(CSL_Usbss_Regs));

            if (pPddObject->ioCppiBase)
                MmUnmapIoSpace(pPddObject->ioCppiBase, sizeof(CSL_CppiRegs));

            if (pPddObject->ioPortBase)
                MmUnmapIoSpace(pPddObject->ioPortBase, sizeof(CSL_UsbRegs));

            delete pPddObject;
            pPddObject = NULL;
        }
    }

    DEBUGMSG(ZONE_FUNCTION, (TEXT("USBH:-HcdPdd_Init:(0x%X)\r\n"), pPddObject));
    return (DWORD)pPddObject;
}

/* HcdPdd_CheckConfigPower
 *
 *    Check power required by specific device configuration and return whether it
 *    can be supported on this platform.  For CEPC, this is trivial, just limit to
 *    the 500mA requirement of USB.  For battery powered devices, this could be
 *    more sophisticated, taking into account current battery status or other info.
 *
 * Return Value:
 *    Return TRUE if configuration can be supported, FALSE if not.
 */
extern BOOL HcdPdd_CheckConfigPower(
                                   UCHAR bPort,         // IN - Port number
                                   DWORD dwCfgPower,    // IN - Power required by configuration
                                   DWORD dwTotalPower)  // IN - Total power currently in use on port
{
	UNREFERENCED_PARAMETER(bPort);
	UNREFERENCED_PARAMETER(dwCfgPower);
	UNREFERENCED_PARAMETER(dwTotalPower);

    DEBUGMSG(ZONE_FUNCTION, (TEXT("USBH: HcdPdd_CheckConfigPower:\r\n")));
    return TRUE;
}

extern void HcdPdd_PowerUp(DWORD hDeviceContext)
{
    SOhcdPdd *pPddObject = (SOhcdPdd *)hDeviceContext;

    DEBUGMSG(ZONE_FUNCTION, (TEXT("USBH: HcdPdd_PowerUp:\r\n")));
    DEBUGCHK(pPddObject);

    HcdMdd_PowerUp(pPddObject->lpvOhcdMddObject);

    return;
}

extern void HcdPdd_PowerDown(DWORD hDeviceContext)
{
    SOhcdPdd *pPddObject = (SOhcdPdd *)hDeviceContext;

    DEBUGMSG(ZONE_FUNCTION, (TEXT("USBH: HcdPdd_PowerDown:\r\n")));
    DEBUGCHK(pPddObject);

    HcdMdd_PowerDown(pPddObject->lpvOhcdMddObject);

    return;
}


extern BOOL HcdPdd_Deinit(DWORD hDeviceContext)
{
    SOhcdPdd * pPddObject = (SOhcdPdd *)hDeviceContext;

    DEBUGMSG(ZONE_FUNCTION, (TEXT("USBH: HcdPdd_Deinit:\r\n")));
    DEBUGCHK(pPddObject);


    if (pPddObject->lpvOhcdMddObject)
        HcdMdd_DestroyHcdObject(pPddObject->lpvOhcdMddObject);
    if (pPddObject->lpvMemoryObject)
        HcdMdd_DestroyMemoryObject(pPddObject->lpvMemoryObject);

    if (pPddObject)
    {

        // Release SYSINTR
        if (!KernelIoControl(IOCTL_HAL_RELEASE_SYSINTR, &pPddObject->dwSysIntr, sizeof(DWORD),
            NULL, 0, NULL))
        {
            DEBUGMSG(ZONE_ERROR, (L"HcdPdd_Deinit: Failed to release SYSINTR for IRQ %d!\r\n",
                IRQ_USBINT0));
        }       

        if (pPddObject->usbss)
            MmUnmapIoSpace(pPddObject->usbss, sizeof(CSL_Usbss_Regs));

        if (pPddObject->ioCppiBase)
            MmUnmapIoSpace(pPddObject->ioCppiBase, sizeof(CSL_CppiRegs));

        if (pPddObject->ioPortBase)
            MmUnmapIoSpace(pPddObject->ioPortBase, sizeof(CSL_UsbRegs));

        if (pPddObject->pSysConfReg)
            MmUnmapIoSpace((PVOID)(pPddObject->pSysConfReg), sizeof(AM33X_DEVICE_CONF_REGS_PA));


        delete pPddObject;
        pPddObject = NULL;
    }

    return TRUE;
}


extern DWORD HcdPdd_Open(DWORD hDeviceContext, DWORD AccessCode,
                         DWORD ShareMode)
{
	UNREFERENCED_PARAMETER(AccessCode);
	UNREFERENCED_PARAMETER(ShareMode);

    DEBUGMSG(ZONE_FUNCTION, (TEXT("USBH: HcdPdd_Open:\r\n")));
    return (DWORD) (((SOhcdPdd*)hDeviceContext)->lpvOhcdMddObject);
//    return hDeviceContext;
}

extern BOOL HcdPdd_Close(DWORD hOpenContext)
{
	UNREFERENCED_PARAMETER(hOpenContext);

    DEBUGMSG(ZONE_FUNCTION, (TEXT("USBH: HcdPdd_Close:\r\n")));
    return TRUE;
}

extern DWORD HcdPdd_Read(DWORD hOpenContext, LPVOID pBuffer, DWORD Count)
{
	UNREFERENCED_PARAMETER(hOpenContext);
	UNREFERENCED_PARAMETER(pBuffer);
	UNREFERENCED_PARAMETER(Count);

    DEBUGMSG(ZONE_FUNCTION, (TEXT("USBH: HcdPdd_Read:\r\n")));
    return (DWORD)-1;
}

extern DWORD HcdPdd_Write(DWORD hOpenContext, LPCVOID pSourceBytes,
                          DWORD NumberOfBytes)
{
	UNREFERENCED_PARAMETER(hOpenContext);
	UNREFERENCED_PARAMETER(pSourceBytes);
	UNREFERENCED_PARAMETER(NumberOfBytes);

    DEBUGMSG(ZONE_FUNCTION, (TEXT("USBH: HcdPdd_Write:\r\n")));
    return (DWORD)-1;
}

extern DWORD HcdPdd_Seek(DWORD hOpenContext, LONG Amount, DWORD Type)
{
	UNREFERENCED_PARAMETER(hOpenContext);
	UNREFERENCED_PARAMETER(Amount);
	UNREFERENCED_PARAMETER(Type);

    DEBUGMSG(ZONE_FUNCTION, (TEXT("USBH: HcdPdd_Seek:\r\n")));
    return(DWORD)-1;
}

extern BOOL HcdPdd_IOControl(DWORD hOpenContext, DWORD dwCode, PBYTE pBufIn,
                             DWORD dwLenIn, PBYTE pBufOut, DWORD dwLenOut, PDWORD pdwActualOut)
{
    SOhcdPdd *pPddObject = (SOhcdPdd *)hOpenContext;
    DWORD dwError = ERROR_INVALID_PARAMETER;

	UNREFERENCED_PARAMETER(dwLenIn);
	UNREFERENCED_PARAMETER(pBufIn);

    DEBUGMSG(ZONE_FUNCTION, (_T("USBH: HcdPdd_IOControl: IOCTL:0x%x, InBuf:0x%x, InBufLen:%d, OutBuf:0x%x, OutBufLen:0x%x\r\n"),
        dwCode, pBufIn, dwLenIn, pBufOut, dwLenOut));

    if (!pdwActualOut)
        goto Exit;

    switch (dwCode)
    {
    case IOCTL_POWER_CAPABILITIES:

        if ((pBufOut != NULL) && (dwLenOut >= sizeof(POWER_CAPABILITIES)))
        {
            __try
            {
                PPOWER_CAPABILITIES pPC = (PPOWER_CAPABILITIES) pBufOut;

                //  set power consumption ( in mW)
                pPC->Power[D0] = 0;
                pPC->Power[D1] = 0;
                pPC->Power[D2] = 0;
                pPC->Power[D3] = 0;
                pPC->Power[D4] = 0;

                //  set latency ( time to return to D0 in ms )
                pPC->Latency[D0] = 0;
                pPC->Latency[D1] = 0;
                pPC->Latency[D2] = 0;
                pPC->Latency[D3] = 100;
                pPC->Latency[D4] = 100;

                //  set device wake caps (BITMASK)
                pPC->WakeFromDx = 0;

                //  set inrush (BITMASK)
                pPC->InrushDx = 0;

                //  set supported device states (BITMASK)
                pPC->DeviceDx = 0x1F;   // support D0- D4

                // set flags
                pPC->Flags = 0;

                if (pdwActualOut)
                    (*pdwActualOut) = sizeof(*pPC);

                dwError = ERROR_SUCCESS;
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                DEBUGMSG(ZONE_ERROR, (_T("HcdPdd_IOControl: IOCTL_POWER_CAPABILITIES: exception in ioctl\r\n")));
            }
        }

        break;

    case IOCTL_POWER_GET:

        if ((pBufOut != NULL) && (dwLenOut >= sizeof(CEDEVICE_POWER_STATE)))
        {
            __try
            {
                (*(PCEDEVICE_POWER_STATE)pBufOut) = pPddObject->CurrentDx;

                if (pdwActualOut)
                    (*pdwActualOut) = sizeof(CEDEVICE_POWER_STATE);

                dwError = ERROR_SUCCESS;

                DEBUGMSG(ZONE_VERBOSE, (_T("HcdPdd_IOControl: IOCTL_POWER_GET %s; passing back %u\r\n"),
                    dwError == ERROR_SUCCESS ? _T("succeeded") : _T("failed"), pPddObject->CurrentDx));
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                DEBUGMSG(ZONE_ERROR, (_T("HcdPdd_IOControl: IOCTL_POWER_GET: exception in ioctl\r\n")));
            }
        }

        break;

#if (_WINCEOSVER<700)
    case IOCTL_POWER_QUERY:

        if ((pBufOut != NULL) && (dwLenOut >= sizeof(CEDEVICE_POWER_STATE)))
        {
            __try
            {
                CEDEVICE_POWER_STATE NewDx = (*(PCEDEVICE_POWER_STATE)pBufOut);

                switch(NewDx)
                {
                case D0:
				case D1:
				case D2:
                case D3:
                case D4:
                    break;

                default:
                    (*(PCEDEVICE_POWER_STATE)pBufOut) = PwrDeviceUnspecified;
                }

                if (pdwActualOut)
                    (*pdwActualOut) = sizeof(CEDEVICE_POWER_STATE);

                dwError = ERROR_SUCCESS;

                DEBUGMSG(ZONE_VERBOSE, (_T("HcdPdd_IOControl: IOCTL_POWER_QUERY %u %s\r\n"),
                    NewDx, dwError == ERROR_SUCCESS ? _T("succeeded") : _T("failed")));
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                DEBUGMSG(ZONE_ERROR, (_T("HcdPdd_IOControl: IOCTL_POWER_QUERY: exception in ioctl\r\n")));
            }
        }

        break;
#endif
    case IOCTL_POWER_SET:

        if ((pBufOut != NULL) && (dwLenOut >= sizeof(CEDEVICE_POWER_STATE)))
        {
            __try
            {
                CEDEVICE_POWER_STATE NewDx = (*(PCEDEVICE_POWER_STATE)pBufOut);

                if (VALID_DX(NewDx))
                {
                	pPddObject->CurrentDx = NewDx;

                    if (pPddObject->CurrentDx == D3 || pPddObject->CurrentDx == D4)
                    {
                        HcdPdd_PowerDown((DWORD)pPddObject);
                    }

                    if ((dwError = HcdPdd_SetDevicePower(pPddObject)) == ERROR_SUCCESS)
                    {
                        if (pPddObject->CurrentDx == D0 || pPddObject->CurrentDx == D1 || 
							pPddObject->CurrentDx == D2)
                        {
                            HcdPdd_PowerUp((DWORD)pPddObject);
                        }

                        *(PCEDEVICE_POWER_STATE)pBufOut = pPddObject->CurrentDx;

                        if (pdwActualOut)
                            (*pdwActualOut) = sizeof(CEDEVICE_POWER_STATE);
                    }
                }

                DEBUGMSG(ZONE_VERBOSE, (_T("HcdPdd_IOControl: IOCTL_POWER_SET %u %s; passing back %u\r\n"),
                    NewDx, dwError == ERROR_SUCCESS ? _T("succeeded") : _T("failed"), NewDx));
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                DEBUGMSG(ZONE_ERROR, (_T("HcdPdd_IOControl: exception in ioctl\r\n")));
            }
        }

        break;
#ifdef USB_STAT		
	case 0x43210001:
        if ((pBufOut != NULL) && (dwLenOut >= (sizeof(DWORD)*(6*15+2))) && (pdwActualOut != 0)){
			ChwDmaGetStat(pPddObject->lpvOhcdMddObject, (UINT32*)pBufOut);
			*pdwActualOut = 6*15+2;			
			dwError = ERROR_SUCCESS;
        }
		break;
		case 0x43210002:
			if ((pBufOut != NULL) && (dwLenOut >= (sizeof(DWORD)*(32))) && (pdwActualOut != 0)){
				ChwGetInfo(pPddObject->lpvOhcdMddObject, (UINT32*)pBufOut);
				*pdwActualOut = 32; 		
				dwError = ERROR_SUCCESS;
			}
			break;
#endif
    default:
        DEBUGMSG(ZONE_WARNING, (_T("USBH: HcdPdd_IOControl: Unsupported IOCTL code %u\r\n"), dwCode));
        dwError = ERROR_NOT_SUPPORTED;
    }

Exit:
    // Pass back appropriate response codes
    SetLastError(dwError);

    return (dwError == ERROR_SUCCESS);
}

// This gets called by the MDD's IST when it detects a power resume.
// By default it has nothing to do.
extern void HcdPdd_InitiatePowerUp(void)
{
    DEBUGMSG(ZONE_FUNCTION, (_T("USBH: HcdPdd_InitiatePowerUp:\r\n")));
    return;
}

