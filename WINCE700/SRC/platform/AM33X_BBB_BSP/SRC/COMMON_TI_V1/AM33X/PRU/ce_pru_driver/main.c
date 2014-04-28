/* OMAP-L13x common code for PRU Software UART device driver
 *
 * Copyright MPC Data Limited 2011
 *
 */

#include <windows.h>
#include <winreg.h>
#include <devload.h>
#include <types.h>
#include <ceddk.h>
#include <math.h>
#include <pm.h>
#include <nkintr.h>

#include <pru.h>

#include <pru_drvr_api.h>
#include "am33x_base_regs.h"
#include "pruioctl.h"
#include "pru_drvr.h"

#include "memcopy_drvr.h"

// 256KB
#define AM33X_PRU_REGS_SIZE 0x40000  

// Defines
#ifdef DEBUG
#define ZONE_INIT       DEBUGZONE(0)
#define ZONE_IOCTL      DEBUGZONE(1)
#define ZONE_FUNCTION   DEBUGZONE(2)
#define ZONE_WARN       DEBUGZONE(3)
#define ZONE_ERROR      DEBUGZONE(4)
#define ZONE_TX         DEBUGZONE(5)
#define ZONE_RX         DEBUGZONE(6)
#define ZONE_IRQ        DEBUGZONE(7)

DBGPARAM dpCurSettings =
{
    L"PRU", {
        L"Init",        L"IOCTL",       L"Function",    L"Warnings",
        L"Errors",      L"TX",          L"RX",          L"IRQ",
        L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined",
        L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined"
    },
    0x0011
};
#endif //DEBUG

#define MAX_KEY_LENGTH 255


#define LOCK(pd)            EnterCriticalSection( &(pd.csDevice) )
#define UNLOCK(pd)          LeaveCriticalSection( &(pd.csDevice) )


//------------------------------------------------------------------------------
// Local functions


static BOOL PRUInitController(void);
static void PRUUninitController(void);
static CEDEVICE_POWER_STATE PRUSetPowerState(CEDEVICE_POWER_STATE PowerState);
DWORD WINAPI PRU_IST(LPVOID lpParameter);
static BOOL LoadFirmwareFile( PRU_FIRMWARE *pFirmware, LPWSTR pszFileName );
static UnLoadFirmwareFile( PRU_FIRMWARE *pFirmware );

static BOOL PRUExecuteFirmware(FIRMWARE_CONTROL *p_fw_ctrl);
FIRMWARE_CONTROL *PruGetFirmwareControlFname(LPWSTR lpszFileName);
FIRMWARE_CONTROL *PruGetFirmwareControlIoctl(DWORD dCode);
FIRMWARE_CONTROL *PruGetFirmwareControlCase(UINT32 case_no);

//------------------------------------------------------------------------------
// Global variables

PRUDEVICESTATE PRUDevice;


//-------------------------------
// firmware controls api table

FIRMWARE_CONTROL  g_firmwareControl_table [] = 
{
#include "memcopy_drvr_api.h"

    // this is always the last one
    { 0, 0, 0, NULL}
};


//------------------------------------------------------------------------------
//
//  Function:  DllEntry
//
//  Standard Windows DLL entry point.
//
BOOL WINAPI DllEntry(HANDLE hInstance, DWORD dwReason, VOID *pReserved)
{
	DEBUGMSG( ZONE_INIT | ZONE_FUNCTION, (L"%s()0x%x  %d\r\n", TEXT(__FUNCTION__), hInstance, dwReason));

    switch ( dwReason )
    {
    case DLL_PROCESS_ATTACH:
        DEBUGREGISTER((HMODULE)hInstance);
        DisableThreadLibraryCalls((HMODULE)hInstance);
        DEBUGMSG(ZONE_INIT, (TEXT("PRU: DLL_PROCESS_ATTACH\r\n")));
        break;

    case DLL_PROCESS_DETACH:
        DEBUGMSG(ZONE_INIT, (TEXT("PRU: DLL_PROCESS_DETACH\r\n")));
        break;
    }

    return TRUE;
}


//------------------------------------------------------------------------------
//
//  Function:  PRU_Init
//
//  Called by the device manager when loading the driver
//
DWORD PRU_Init(
               LPCTSTR pContext,
               DWORD dwBusContext
               )
{
    BOOL rc;
    HKEY hKey;
    DWORD kValueType;
    DWORD kDataSize;
    DWORD dwRegisterBaseAddress = 0;
    DWORD dwEPRUXA_Active = 0;
    DWORD dwEPRUXB_Active = 0;

	DEBUGMSG( ZONE_INIT | ZONE_FUNCTION, (L"%s() %s 0x%x\r\n", TEXT(__FUNCTION__), pContext, dwBusContext));

    
    rc = FALSE;

	// Cleardown the control structure
	memset( &PRUDevice, 0, sizeof(PRUDevice) );

	// Protect PRU resources from multiple MDD / PDD instances
    InitializeCriticalSection(&(PRUDevice.csDevice));

	// Read configuration settings from registry for this driver instance
    hKey = OpenDeviceKey((LPCTSTR)pContext);
    if (!hKey) 
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: PRU_Init: "
                              L"Failed to open registry key\r\n"));
        goto cleanup;
    }

	// IST priority
    kDataSize = sizeof(DWORD);
    if (RegQueryValueEx(hKey, L"Priority256", NULL, &kValueType,
                        (LPBYTE)&PRUDevice.ist_priority, &kDataSize) != ERROR_SUCCESS)
    {
        DEBUGMSG(1, (L"ERROR: PRU_Init: "
                      L"Failed to read PRU registry key 'Priority256'\r\n"));
        goto cleanup;
    }
	DEBUGMSG(1, (L"Priority256=%d\r\n", PRUDevice.ist_priority) );


	// First PRU IRQ allocated
	kDataSize = sizeof(DWORD);
    if (RegQueryValueEx(hKey, L"IrqBase", NULL, &kValueType,
                        (LPBYTE)&PRUDevice.irq_base, &kDataSize) != ERROR_SUCCESS)
    {
        DEBUGMSG(1, (L"ERROR: PRU_Init: "
                      L"Failed to read PRU registry key 'IrqBase'\r\n"));
        goto cleanup;
    }
	DEBUGMSG(1, (L"IrqBase=%d\r\n", PRUDevice.irq_base) );

    rc = TRUE;
    
cleanup:

	RegCloseKey( hKey );
    DEBUGMSG( ZONE_FUNCTION, (L"-PRU_Init: rc=%x\r\n", rc));
    RETAILMSG( 1, (L"-PRU_Init: rc=%x\r\n", rc));

    return rc;

}


//------------------------------------------------------------------------------
//
//  Function:  PRU_Open
//
//  Called by the device manager on a CreateFile
//
//
DWORD PRU_Open(
               DWORD hDeviceContext,
               DWORD AccessCode,
               DWORD ShareMode
               )
{

    DEBUGMSG( ZONE_INIT | ZONE_FUNCTION,
        (L"PRU_Open: hDeviceContext 0x%08x, AccessCode 0x%x, ShareMode 0x%x\r\n", 
         hDeviceContext, AccessCode, ShareMode));

    if (PRUDevice.CurrentDx != D0) {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: PRU_Open: Device powered off.\r\n"));
        return 0;
    }
    else {
        return (DWORD)&PRUDevice;
    }
}


//------------------------------------------------------------------------------
//
//  Function:  PRU_Close
//
//  Called by the device manager on a CloseHandle
//
//
BOOL PRU_Close(
               DWORD hOpenContext 
               )
{

    DEBUGMSG( ZONE_INIT | ZONE_FUNCTION,
        (L"PRU_Close: hOpenContext 0x%08x\r\n", hOpenContext));

    return FALSE;
}


//------------------------------------------------------------------------------
//
//  Function:  PRU_Deinit
//
BOOL PRU_Deinit(
                DWORD hDeviceContext 
                )
{
	DEBUGMSG( ZONE_INIT | ZONE_FUNCTION, (L"%s() hDeviceContext=0x%08x", TEXT(__FUNCTION__), hDeviceContext) );
    PRUUninitController();
    
    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  PRU_PowerDown
//
// Legacy power management function - not used
//
void PRU_PowerDown(
                   DWORD hDeviceContext 
                   )
{
	DEBUGMSG( ZONE_INIT | ZONE_FUNCTION, (L"%s() hDeviceContext=0x%08x", TEXT(__FUNCTION__), hDeviceContext) );
}

//------------------------------------------------------------------------------
//
//  Function:  PRU_PowerUp
//
// Legacy power management function - not used
//
void PRU_PowerUp(
                 DWORD hDeviceContext 
                 )
{
	DEBUGMSG( ZONE_INIT | ZONE_FUNCTION, (L"%s() hDeviceContext=0x%08x", TEXT(__FUNCTION__), hDeviceContext) );
}

//------------------------------------------------------------------------------
//
//  Function:  PRU_Read
//
DWORD PRU_Read(
               DWORD hOpenContext,
               LPVOID pBuffer,
               DWORD Count 
               )
{
	DEBUGMSG( ZONE_FUNCTION, (L"%s() hOpenContext=0x%08x", TEXT(__FUNCTION__), hOpenContext) );
    return FALSE;
}

//------------------------------------------------------------------------------
//
//  Function:  PRU_Seek
//
DWORD PRU_Seek(
               DWORD hOpenContext,
               long Amount,
               WORD Type 
			   )
{
	DEBUGMSG( ZONE_FUNCTION, (L"%s() hOpenContext=0x%08x", TEXT(__FUNCTION__), hOpenContext) );
    return FALSE;
}

//------------------------------------------------------------------------------
//
//  Function:  PRU_Write
//
DWORD PRU_Write(
                DWORD hOpenContext,
                LPCVOID pBuffer,
                DWORD Count 
                )
{
	DEBUGMSG( ZONE_FUNCTION, (L"%s() hOpenContext=0x%08x", TEXT(__FUNCTION__), hOpenContext) );
    return FALSE;
}


DWORD PRUExecuteFirmwareProc(
                   DWORD hOpenContext,
                   DWORD dwCode,
                   PBYTE pBufIn,
                   DWORD dwSizeIn,
                   PBYTE pBufOut,
                   DWORD dwSizeOut,
                   PDWORD pdwActualOut
)
{
    WCHAR wcFname[MAX_PATH];
    int len;
    FIRMWARE_CONTROL *p_fw_ctrl = NULL;

    if (dwCode == PRUIOCTL_EXECUTE_CASE_NO)
    {
        RETAILMSG( 1, (L"PRUExecuteFirmwareProc: firmware case no.=%d\r\n", *((DWORD *)pBufIn)));
        p_fw_ctrl = PruGetFirmwareControlCase(*((DWORD *)pBufIn));
    }
    else if (dwCode == PRUIOCTL_EXECUTE)
    {
        len = mbstowcs(wcFname, pBufIn, 100);
        RETAILMSG( 1, (L"PRUExecuteFirmwareProc: firmware=%s\r\n", wcFname));
        p_fw_ctrl = PruGetFirmwareControlFname(wcFname);
    }
	else
        return ERROR_NOT_SUPPORTED;

    if (!p_fw_ctrl)
    {
        RETAILMSG(1,(L"FAILED to get firmware control API\r\n") );
        return ERROR_NOT_SUPPORTED;
    }

    PRUExecuteFirmware(p_fw_ctrl);

    return ERROR_SUCCESS;
}


//------------------------------------------------------------------------------
//
//  Function:  PRU_IOControl
//
//  Called to configure the PRU peripheral
//
BOOL PRU_IOControl(
                   DWORD hOpenContext,
                   DWORD dwCode,
                   PBYTE pBufIn,
                   DWORD dwSizeIn,
                   PBYTE pBufOut,
                   DWORD dwSizeOut,
                   PDWORD pdwActualOut 
                   )
{
    BYTE BufIn = 0;
    BOOL rc;
    DWORD dwErr = ERROR_SUCCESS;
    int i = 0;

	DEBUGMSG( ZONE_IOCTL | ZONE_FUNCTION, (L"%s() hOpenContext=0x%08x", TEXT(__FUNCTION__), hOpenContext) );

    switch(dwCode)
	{
        case PRUIOCTL_EXECUTE:
        case PRUIOCTL_EXECUTE_CASE_NO:
            PRUExecuteFirmwareProc(hOpenContext, dwCode, pBufIn, dwSizeIn, pBufOut, dwSizeOut, pdwActualOut);
        break;

        case PRUIOCTL_SHOW_TEST_CASES:
            i=0;
            RETAILMSG( 1, (L"Test cases: \r\n", i, g_firmwareControl_table[i].pDesc));
            while (g_firmwareControl_table[i].pDesc)
            {
                RETAILMSG( 1, (L"%d  %s\r\n", i, g_firmwareControl_table[i].pDesc));
                ++i;
            }
        break;

        // --------------------- POWER MANAGEMENT IOCTLs --------------------
        case IOCTL_POWER_CAPABILITIES:
            // Tell the power manager about ourselves.
            DEBUGMSG(ZONE_IOCTL, (_T("PRU_IOControl: IOCTL_POWER_CAPABILITIES\r\n")));
            if (pBufOut != NULL 
                  && dwSizeOut >= sizeof(POWER_CAPABILITIES) 
                  && pdwActualOut != NULL) {
                __try {
                    PPOWER_CAPABILITIES ppc = (PPOWER_CAPABILITIES) pBufOut;
                    memset(ppc, 0, sizeof(*ppc));
                    ppc->DeviceDx = (DX_MASK(D0) | DX_MASK(D4));    // Support D0 and D4 only
                    *pdwActualOut = sizeof(*ppc);
                    dwErr = ERROR_SUCCESS;
                }
                __except(EXCEPTION_EXECUTE_HANDLER) {
                    DEBUGMSG(ZONE_IOCTL, (_T("PRU_IOControl: exception in ioctl\r\n")));
                }
            }
            break;

        case IOCTL_POWER_QUERY: 
            if(pBufOut != NULL 
               && dwSizeOut == sizeof(CEDEVICE_POWER_STATE) 
               && pdwActualOut != NULL) {
                // Even though we don't really support D1, D2 or D3 we will not return an error on
                // the query.  Instead, we will go to D4 when asked to
                // go to D3, and D0 when asked to go to D1 or D2.
                __try {
                    CEDEVICE_POWER_STATE NewDx = *(PCEDEVICE_POWER_STATE) pBufOut;
                    if(VALID_DX(NewDx)) {
                        // this is a valid Dx state so return a good status
                        *pdwActualOut = sizeof(CEDEVICE_POWER_STATE);
                        dwErr = ERROR_SUCCESS;
                    }
                    DEBUGMSG(ZONE_IOCTL, (_T("PRU_IOControl: IOCTL_POWER_QUERY %u %s\r\n"),
                                          NewDx, dwErr == ERROR_SUCCESS ? _T("succeeded") : _T("failed")));
                } 
                __except(EXCEPTION_EXECUTE_HANDLER) {
                    DEBUGMSG(ZONE_IOCTL, (_T("PRU_IOControl: exception in ioctl\r\n")));
                }
            }
            break;

        case IOCTL_POWER_SET: 
            if(pBufOut != NULL 
               && dwSizeOut == sizeof(CEDEVICE_POWER_STATE) 
               && pdwActualOut != NULL) {
                // Allow a set to any state, but if requested to go to
                // D3, go to D4 instead. If D1 or D2 is requested, go
                // to D0
                LOCK(PRUDevice);
                __try {
                    CEDEVICE_POWER_STATE NewDx = *(PCEDEVICE_POWER_STATE) pBufOut;
                    if(VALID_DX(NewDx)) {
                        // Map states to those that we support
                        if(NewDx == D3) {
                            NewDx = D4;
                        }
                        else if (NewDx == D1 || NewDx == D2) {
                            NewDx = D0;
                        }
                        *(PCEDEVICE_POWER_STATE) pBufOut = NewDx;
                        *pdwActualOut = sizeof(CEDEVICE_POWER_STATE);
                        PRUSetPowerState(NewDx);
                        dwErr = ERROR_SUCCESS;
                    }
                    DEBUGMSG(ZONE_IOCTL, (_T("PRU_IOControl: IOCTL_POWER_SET %u %s; passing back %u\r\n"), 
                                          NewDx, 
                                          dwErr == ERROR_SUCCESS ? 
                                          _T("succeeded") : _T("failed"), 
                                          PRUDevice.CurrentDx));
                } 
                __except(EXCEPTION_EXECUTE_HANDLER) {
                    DEBUGMSG(ZONE_IOCTL, (_T("%PRU_IOControl: exception in ioctl\r\n")));
                }
                UNLOCK(PRUDevice);
            }
            break;

        case IOCTL_POWER_GET: 
            if(pBufOut != NULL 
               && dwSizeOut == sizeof(CEDEVICE_POWER_STATE) 
               && pdwActualOut != NULL) {
                // Just return our CurrentDx value
                LOCK(PRUDevice);
                __try {
                    *(PCEDEVICE_POWER_STATE) pBufOut = PRUDevice.CurrentDx;
                    *pdwActualOut = sizeof(CEDEVICE_POWER_STATE);
                    dwErr = ERROR_SUCCESS;
                    DEBUGMSG(ZONE_IOCTL, (_T("PRU_IOControl: IOCTL_POWER_GET %s; passing back %u\r\n"), 
                                          dwErr == ERROR_SUCCESS ? _T("succeeded") : _T("failed"), PRUDevice.CurrentDx));
                }
                __except(EXCEPTION_EXECUTE_HANDLER) {
                    DEBUGMSG(ZONE_IOCTL, (_T("PRU_IOControl: exception in ioctl\r\n")));
                }
                UNLOCK(PRUDevice);
            }
            break;
        
        default:
            dwErr = ERROR_NOT_SUPPORTED;
            break;
    };
    
    DEBUGMSG( ZONE_FUNCTION,
        (L"-PRU_IOControl\r\n"));

    // pass back appropriate response codes
    SetLastError(dwErr);
    if(dwErr != ERROR_SUCCESS) {
        rc = FALSE;
    } else {
        rc = TRUE;
    }
    
    return rc;
}


// ----------------------------------------------------------------
// ................................................................
// ----------------------------------------------------------------

/*
 * program execute
 *     returns 0 for success
 */
static BOOL pru_execute_program (
    FIRMWARE_CONTROL *p_fw_ctrl,
	unsigned char *pru_emu_code,
	unsigned int fw_size,
	arm_pru_iomap * arm_iomap_pru
)
{
	unsigned int am33x_addr;
	unsigned int u32loop;
	UINT32 status = 0;
    UINT32 rc = E_FAIL;
    UINT32 pru0 = 0, pru1 = 0;

    if (!p_fw_ctrl || !pru_emu_code || !arm_iomap_pru)
        return FALSE;

    pru0 = p_fw_ctrl->LOCAL_examplePru() & 0x00000001;
    pru1 = p_fw_ctrl->LOCAL_examplePru() & 0x00000002;

    if (pru0)
    {
    	status = pru_enable(PRU_NUM0, arm_iomap_pru);
        if(status)
        {
            RETAILMSG( 1, (L">>>>> pru_execute_program: pru_enable %d FAILED\r\n", PRU_NUM0));
            return FALSE;
        }
    }
    
    if (pru1)
    {
    	status = pru_enable(PRU_NUM1, arm_iomap_pru);
        if(status)
        {
            RETAILMSG( 1, (L">>>>> pru_execute_program: pru_enable %d FAILED\r\n", PRU_NUM1));
            return FALSE;
        }
    }

	am33x_addr = (unsigned int) arm_iomap_pru->pru_io_addr;

	// [BA] fixed stupid unaligned accesses
	for (u32loop = 0; u32loop < PRU_DATARAM01_SIZE; u32loop+=sizeof(unsigned int))
	{
		if (pru0)
			*(unsigned int *)(am33x_addr + u32loop) = 0x0;

		if (pru1)
			*(unsigned int *)(am33x_addr + DATARAM1_BASE_ADDRESS + u32loop) = 0x0;
	}

    if(pru0)
    {
    	status = pru_load(PRU_NUM0, (unsigned int *)pru_emu_code,
    				 (fw_size / sizeof(unsigned int)), arm_iomap_pru);
        if(status)
        {
            RETAILMSG( 1, (L">>>>> pru_execute_program: pru_enable FAILED\r\n"));
            return FALSE;
        }
    }
    
    if(pru1)
    {
    	pru_load(PRU_NUM1, (unsigned int *)pru_emu_code,
    		 (fw_size / sizeof(unsigned int)), arm_iomap_pru);
        if(status)
        {
            RETAILMSG( 1, (L">>>>> pru_execute_program: pru_enable FAILED\r\n"));
            return FALSE;
        }
    }

#if 0
   	retval = arm_to_pru_intr_init();
   	if (-1 == retval) {
       	return status;
   	}

    pru_set_ram_data (arm_iomap_pru);
#endif

    p_fw_ctrl->LOCAL_exampleInit((void *)&PRUDevice);

    if (pru0)
    {
    	status = pru_run(PRU_NUM0, arm_iomap_pru);
        if(status)
        {
            RETAILMSG( 1, (L">>>>> pru_execute_program: pru_run FAILED\r\n"));
            return FALSE;
        }
    }
    
    if (pru1)
    {
    	status = pru_run(PRU_NUM1, arm_iomap_pru); 
        if(status)
        {
            RETAILMSG( 1, (L">>>>> pru_execute_program: pru_run FAILED\r\n"));
            return FALSE;
        }
    }
    
    if(pru0)
    {
        rc = pru_waitForHalt(PRU_NUM0, 0xEFFFFFFF, arm_iomap_pru);
        if (rc == E_FAIL)
        {
            RETAILMSG( 1, (L"pru_waitForHalt - E_FAIL\r\n"));
            return FALSE;
        }
        else if (rc == E_TIMEOUT)
        {
            RETAILMSG( 1, (L"pru_waitForHalt - E_TIMEOUT\r\n"));
            return FALSE;
        }
    }
    
    if(pru1)
    {
        rc = pru_waitForHalt(PRU_NUM1, 0xEFFFFFFF, arm_iomap_pru);
        if (rc == E_FAIL)
        {
            RETAILMSG( 1, (L"pru_waitForHalt - E_FAIL\r\n"));
            return FALSE;
        }
        else if (rc == E_TIMEOUT)
        {
            RETAILMSG( 1, (L"pru_waitForHalt - E_TIMEOUT\r\n"));
            return FALSE;
        }
    }

    return TRUE;
}

static BOOL PRUMemMapInit(arm_pru_iomap *pIOMap)
{
	PHYSICAL_ADDRESS PhysicalAddr;

    // Map the PRU address space
    PhysicalAddr.HighPart = 0;
    PhysicalAddr.LowPart = AM33X_ICSS_PRUSS1_REGS_PA;
    pIOMap->pru_io_addr = (void*)MmMapIoSpace(PhysicalAddr, AM33X_PRU_REGS_SIZE, FALSE);
    if(PRUDevice.pru_iomap.pru_io_addr == NULL)
		{
        DEBUGMSG( ZONE_ERROR, (L"PRUInitController:: Error when mapping pointer to the PRUs (%d).\r\n",
            GetLastError()));
        return FALSE;
    }

	DEBUGMSG( ZONE_INIT, (L"mapped pru: phy=0x%08x vir=0x%08x\r\n", 
             PhysicalAddr.LowPart, PRUDevice.pru_iomap.pru_io_addr) );

RETAILMSG( 1, (L"mapped pru: phy=0x%08x vir=0x%08x\r\n", 
             PhysicalAddr.LowPart, PRUDevice.pru_iomap.pru_io_addr) );

    pIOMap->pru0_dataram_base = 
        (void *)((UINT32)pIOMap->pru_io_addr + DATARAM0_BASE_ADDRESS);
    
    pIOMap->pru1_dataram_base = 
        (void *)((UINT32)pIOMap->pru0_dataram_base + DATARAM1_BASE_ADDRESS);
    
    pIOMap->intc_base = 
        (void *)((UINT32)pIOMap->pru0_dataram_base + PRU_INTC_BASE_ADDRESS);

    pIOMap->pru0_control_base = 
        (void *)((UINT32)pIOMap->pru0_dataram_base + PRU0_CONTROL_BASE_ADDRESS);

    pIOMap->pru0_debug_base = 
        (void *)((UINT32)pIOMap->pru0_dataram_base + PRU0_DEBUG_BASE_ADDRESS);

    pIOMap->pru1_control_base = 
        (void *)((UINT32)pIOMap->pru0_dataram_base + PRU1_CONTROL_BASE_ADDRESS);

    pIOMap->pru1_debug_base = 
        (void *)((UINT32)pIOMap->pru0_dataram_base + PRU1_DEBUG_BASE_ADDRESS);

    pIOMap->pru0_iram_base = 
        (void *)((UINT32)pIOMap->pru0_dataram_base + PRU0_IRAM_BASE_ADDRESS);

    pIOMap->pru1_iram_base = 
        (void *)((UINT32)pIOMap->pru0_dataram_base + PRU1_IRAM_BASE_ADDRESS);

    return TRUE;
}


/*****************************************************************************
* Local Function Definitions                                                 *
*****************************************************************************/

/* Initialise the PRU controller. */
static BOOL PRUExecuteFirmware(FIRMWARE_CONTROL *p_fw_ctrl)
{
    static BOOL bFirstInit = TRUE;
    BOOL rc = TRUE;
    
    DEBUGMSG( ZONE_INIT | ZONE_FUNCTION, (L"PRUExecuteFirmware called....\r\n"));

    if (!p_fw_ctrl)
        return FALSE;

    _tcsncpy(PRUDevice.FirmwareFile, p_fw_ctrl->pFname, MAX_PATH);

	// Map memory for PRU, SYSCFG.

    // read firmware file to buffer
	rc = LoadFirmwareFile(&PRUDevice.Firmware, PRUDevice.FirmwareFile);
    if (rc == FALSE)
    {
        RETAILMSG(1,(L">>>>> LoadFirmwareFile %s FAILED\r\n", PRUDevice.FirmwareFile) );
        goto cleanup;
    }

    PRUInitController();

#if 0
    if(pru_enable(0, &(PRUDevice.pru_iomap)))
    {
RETAILMSG( 1, (L">>>>> pru_execute_program: pru_enable FAILED\r\n"));
        return FALSE;
    }

        LOCAL_exampleInit(0, 0);
#endif

    rc = pru_execute_program(p_fw_ctrl,
                             PRUDevice.Firmware.lpData, 
                             PRUDevice.Firmware.dwSize, 
                             &(PRUDevice.pru_iomap));
    if (rc == FALSE)
    {
        RETAILMSG(1,(L"pru_execute_program FAILED\r\n", PRUDevice.FirmwareFile) );
        goto cleanup;
    }
        
    if (p_fw_ctrl->LOCAL_examplePassed(NULL))
    {
        RETAILMSG(1,(L"%s executed successfully\r\n", p_fw_ctrl->pFname) );
    }
    else
    {
        RETAILMSG(1,(L"%s FAILED\r\n", p_fw_ctrl->pFname) );
    }

	// completed ok!
    return TRUE;

cleanup:
	PRUUninitController();
    return FALSE;
}


static BOOL PRUInitController(void)
{
    BOOL rc = TRUE;

	if ( PRUDevice.pru_iomap.pru_io_addr != NULL )
        return rc;
    
    rc = PRUMemMapInit(&(PRUDevice.pru_iomap));
    if (rc == FALSE)
    {
        RETAILMSG(1,(L">>>>> PRUMemMapInit FAILED\r\n", PRUDevice.FirmwareFile) );
    }

    return rc;
}


/* Disable the PRU controller */
static void PRUUninitController(void)
{
    // Unmap the PRU address space
	if ( PRUDevice.pru_iomap.pru_io_addr != NULL )
	{
		MmUnmapIoSpace( PRUDevice.pru_iomap.pru_io_addr, AM33X_PRU_REGS_SIZE );
		PRUDevice.pru_iomap.pru_io_addr = NULL;
	}

	// Free up firmware
	UnLoadFirmwareFile( &PRUDevice.Firmware );
}



// --------------------------------------------------------
// --------------------------------------------------------
//
// --------------------------------------------------------
// --------------------------------------------------------

static CEDEVICE_POWER_STATE PRUSetPowerState(CEDEVICE_POWER_STATE PowerState)
{
    switch (PowerState)
    {
        /////// ON ///////
        case D1:
        case D2:
            // Not supported directly,  go to next best supported state
            PowerState = D0;
            // fall through to...
        case D0:
			if (PowerState != PRUDevice.CurrentDx) {
				// Power device on again
// [BA] power management needs looking at. Currently stalls here.
//				PSCSetModuleState(PSC_MODULE_PRU, PSC_MDCTL_NEXT_ENABLE);

				PRUDevice.CurrentDx = PowerState;
			}
            break;

        /////// OFF //////
        case D3:
            PowerState = D4;
            // fall through to...
        case D4:
            if (PowerState != PRUDevice.CurrentDx) {
                // Power device off
// [BA] power management needs looking at. Currently stalls during power up.
//                PSCSetModuleState(PSC_MODULE_PRU, PSC_MDCTL_NEXT_DISABLE);

                // Store new state
                PRUDevice.CurrentDx = PowerState;
            }
            break;
        default:
            PRUDevice.CurrentDx = PwrDeviceUnspecified;
            break;    
    };

    return PowerState;
}


// Read firmware from file system
// We assume the buffer is already empty...
static BOOL LoadFirmwareFile( PRU_FIRMWARE *pFirmware, LPWSTR lpszFileName )
{
	HANDLE hLocalFile = INVALID_HANDLE_VALUE;
	DWORD dwFileSize;
	BYTE *lpBuffer = NULL;
	BOOL bResult;
	DWORD dwBytesRead;

	// safety
	DEBUGMSG( ZONE_INIT | ZONE_FUNCTION, (L"%s()\r\n", TEXT(__FUNCTION__)) );
	if ( !pFirmware || !lpszFileName )
	{
		DEBUGMSG(ZONE_ERROR, (L"Error: invalid parameters\r\n") );
		goto cleanup;
	}

	// open the file
    hLocalFile = CreateFile(lpszFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL);
	if ( hLocalFile == INVALID_HANDLE_VALUE )
	{
		DEBUGMSG( ZONE_ERROR, (L"Error: could not open file [%s]\r\n", lpszFileName) );
		goto cleanup;
	}  

    // get file size
	dwFileSize = GetFileSize (hLocalFile, NULL);
	SetFilePointer(hLocalFile, 0, 0, FILE_BEGIN);

    DEBUGMSG( ZONE_INIT, (L"input file [%s] is %d bytes long\r\n", lpszFileName, dwFileSize) );
    DEBUGMSG( ZONE_INIT, (L"reading input file..\r\n") );

	// Allocate buffer space to read firmware into.  This does not
	// have to be DMA aligned or contiguous because [pru_load] uses
	// a simple for loop to copy the bytes into PRU instruction RAM.
	lpBuffer = (BYTE*)malloc( dwFileSize );
	if ( lpBuffer == NULL )
	{
		DEBUGMSG( ZONE_ERROR, (L"unable to allocate %d bytes for input buffer\r\n\r\n", dwFileSize) );
		goto cleanup;
	}

	// Read file
	bResult = ReadFile( hLocalFile, lpBuffer, dwFileSize, &dwBytesRead, NULL ); 
    if ( (bResult == FALSE) || (dwBytesRead != dwFileSize) )
    {
        DEBUGMSG( ZONE_ERROR, (L"error reading firmware file") );
		goto cleanup;
    }
    DEBUGMSG( ZONE_INIT, (L"firmware file read ok!\n") );
    CloseHandle( hLocalFile );

	// completed ok
	pFirmware->lpData = lpBuffer;
	pFirmware->dwSize = dwFileSize;
	return TRUE;

	// some errors
cleanup:
	if ( lpBuffer != NULL )
	{
		free( lpBuffer );
	}
	if ( hLocalFile != INVALID_HANDLE_VALUE )
	{
		CloseHandle( hLocalFile );
	}
	return FALSE;
}


// Dump firmware structure
static UnLoadFirmwareFile( PRU_FIRMWARE *pFirmware )
{
	// Free memory and release structure
	if ( pFirmware != NULL && pFirmware->lpData != NULL )
	{
		free( pFirmware->lpData );
	}
}


#if 0
FIRMWARE_CONTROL *PruGetFirmwareControlIoctl(DWORD fw_ioctl)
{
    FIRMWARE_CONTROL *pCtrl = &(g_firmwareControl_table[0]);

    while (pCtrl->ioctl != PRUIOCTL_EXECUTE)
    {
        if (pCtrl->ioctl == fw_ioctl)
            return pCtrl;
        else
            ++pCtrl;
    }
    
    return NULL;
}
#endif


FIRMWARE_CONTROL *PruGetFirmwareControlFname(LPWSTR lpszFileName)
{
    FIRMWARE_CONTROL *pCtrl = &(g_firmwareControl_table[0]);

    while (pCtrl->pFname != 0)
    {
        if ( _tcscmp(pCtrl->pFname, lpszFileName) == 0)
            return pCtrl;
        else
            ++pCtrl;
    }
    
    return NULL;
}


FIRMWARE_CONTROL *PruGetFirmwareControlCase(UINT32 case_no)
{
    FIRMWARE_CONTROL *pCtrl = NULL;
    UINT32 i = 0;

    while ((i < case_no) && (g_firmwareControl_table[i].pFname))
    {
        ++i;
    }

    if ((i==case_no) && (g_firmwareControl_table[i].pFname))
    {
        pCtrl =  &(g_firmwareControl_table[i]);
    }

    return pCtrl;
}


// <end of file>

