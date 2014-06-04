// All rights reserved ADENEO EMBEDDED 2010
//
// Copyright (c) MPC Data Limited 2010.  All rights reserved.
//
//
// Module Name: 
//        usbotgpdd.cpp
//
// Abstract:
//        USB OTG Driver
//

#pragma warning(push)
#pragma warning(disable: 4100 4245 4512 6287 6258)
#include <windows.h>
#include <types.h>
#include <nkintr.h>

#include "am33x.h"
#include "am33x_usb.h"
#include "am33x_usbcdma.h"

#include "oal_clock.h"
#include "sdk_padcfg.h"
#include "usbotgpdd.h"
#pragma warning(pop)

//#ifdef DEBUG
LPCTSTR g_cppOtgStateString[]    = {
    TEXT("USBOTG_states_unknown"),
// A Port States
    TEXT("USBOTG_a_idle"),
    TEXT("USBOTG_a_wait_vrise"),
    TEXT("USBOTG_a_wait_bcon"),
    TEXT("USBOTG_a_host"),
    TEXT("USBOTG_a_suspend"),
    TEXT("USBOTG_a_peripheral"),
    TEXT("USBOTG_a_wait_vfall"),
    TEXT("USBOTG_a_vbus_err"),
// B Port States
    TEXT("USBOTG_b_idle"),
    TEXT("USBOTG_b_srp_init"),
    TEXT("USBOTG_b_peripheral"),
    TEXT("USBOTG_b_wait_acon"),
    TEXT("USBOTG_b_host")
    };
//#endif

//========================================================================
void CAM3xxOTG::ChipCfgLock(BOOL lock)
//  Lock or unlock the Chip Cfg MMR registers. - Not available on AM3XX
{
	UNREFERENCED_PARAMETER(lock);
}

void CAM3xxOTG::StartUSBModule(void)
{
    DEBUGMSG(ZONE_OTG_FUNCTION,(TEXT("+CAM3xxOTG::StartUSBModule()\r\n")));
    // Enable USB state polling
    m_bEnablePolling = TRUE;
    DEBUGMSG(ZONE_OTG_FUNCTION,(TEXT("-CAM3xxOTG::StartUSBModule()\r\n")));
}

CAM3xxOTG::CAM3xxOTG(LPCTSTR lpActivePath)
:   USBOTG(lpActivePath)
{
    DEBUGMSG(ZONE_OTG_FUNCTION,(TEXT("+CAM3xxOTG::CAM3xxOTG()\r\n")));

    m_hParent = CreateBusAccessHandle(lpActivePath);

    // Initialize hardware register pointers
    m_pUsbRegs = NULL;
    m_pSys     = NULL;
	m_bFunctionMode = FALSE;
	m_bHostMode = FALSE;

    // Initialize polling parameters
    m_hPollThreadExitEvent = CreateEvent( NULL, TRUE, FALSE, NULL);
    m_PollTimeout = INFINITE; // Initialise poll period to safe value
    m_dwOldPowerState = PwrDeviceUnspecified; // invalid state

    if (lpActivePath) 
    {
        DWORD dwLength = _tcslen(lpActivePath) + 1;
        m_ActiveKeyPath = new TCHAR [ dwLength ] ;
        if (m_ActiveKeyPath)
            StringCchCopy( m_ActiveKeyPath, dwLength, lpActivePath );
    }

    DEBUGMSG(ZONE_OTG_FUNCTION,(TEXT("-CAM3xxOTG::CAM3xxOTG()\r\n")));
}

BOOL CAM3xxOTG::Init()
{
    PHYSICAL_ADDRESS PA;
    BOOL rc = FALSE;
    DWORD dwType, dwLength;

    DEBUGMSG(ZONE_OTG_FUNCTION,(TEXT("+CAM3xxOTG::Init()\r\n")));

	if (m_ActiveKeyPath != NULL){
		// Read port index
		dwLength = sizeof(UINT32);
		if (!RegQueryValueEx((L"Index"),&dwType,(PBYTE)&m_Index,&dwLength)) 
		{
			DEBUGMSG(ZONE_OTG_ERROR, (TEXT("CAM3xxOTG::Init: Failed to read Port number from registry\r\n")));
			goto cleanUp;
		}
	} else {
        DEBUGMSG(ZONE_OTG_ERROR,(TEXT("CAM3xxOTG::Init: ERROR: NULL ActiveKeyPath or base Init() failed\r\n")));
        goto cleanUp;
    }

	PA.QuadPart = (m_Index == 1) ?  AM33X_USB0_REGS_PA : AM33X_USB1_REGS_PA; 
    m_pUsbRegs  = (CSL_UsbRegs *)MmMapIoSpace (PA, sizeof(CSL_UsbRegs), FALSE);

    PA.QuadPart = AM33X_DEVICE_CONF_REGS_PA;
    m_pSys      = (AM33X_DEVICE_CONF_REGS *)MmMapIoSpace (PA, sizeof(AM33X_DEVICE_CONF_REGS), FALSE);

    if (m_pUsbRegs == NULL || m_pSys == NULL ){
        DEBUGMSG(ZONE_OTG_ERROR,(TEXT("CAM3xxOTG::Init: ERROR: failed to map hardware registers\r\n")));
        goto cleanUp;
    }

    if (USBOTG::Init()){
        // Create thread to poll USB status
        m_hPollThread = CreateThread(NULL, 0, PollThread, this, 0, NULL);
        if (m_hPollThread == NULL){
            DEBUGMSG(ZONE_OTG_ERROR,(TEXT("CAM3xxOTG::Init: ERROR: failed to create poll thread\r\n")));
            goto cleanUp;
        }

        // Determine if USB 1.1 is in use
		m_bUSB11Enabled = FALSE; // ??????? Not available on AM335

        // Read poll timeout from registry
        dwLength = sizeof(DWORD);
        if (!RegQueryValueEx((L"PollTimeout"),&dwType,(PBYTE)&m_PollTimeout,&dwLength)){
            DEBUGMSG(ZONE_OTG_ERROR, (TEXT("CAM3xxOTG::Init: Failed to read PollTimeout from registry\r\n")));
            goto cleanUp;
        }

    } else {
        DEBUGMSG(ZONE_OTG_ERROR,(TEXT("CAM3xxOTG::Init: ERROR: NULL ActiveKeyPath or base Init() failed\r\n")));
        goto cleanUp;
    }

	m_RequestedPowerState = D0;

RETAILMSG(1,(L"CAM3xxOTG::Init DEVCTL %02X\r\n", m_pUsbRegs->DEVCTL));

    StartUSBModule();
    rc = TRUE;

cleanUp:
    DEBUGMSG(ZONE_OTG_FUNCTION,(TEXT("-CAM3xxOTG::Init(%d)\r\n"), rc));
    return rc;
}

BOOL CAM3xxOTG::PostInit()
{
    BOOL rc = FALSE;
    DEBUGMSG(ZONE_OTG_FUNCTION,(TEXT("+CAM3xxOTG::PostInit()\r\n")));

    // Default setting to be client mode
    m_UsbOtgState = USBOTG_b_idle;
    m_UsbOtgInput.bit.id = 1;
    rc = USBOTG::PostInit();

    DEBUGMSG(ZONE_OTG_FUNCTION,(TEXT("-CAM3xxOTG::PostInit(%d)\r\n"), rc));
    return rc;
}

CAM3xxOTG::~CAM3xxOTG() 
{
    DEBUGMSG(ZONE_OTG_FUNCTION,(TEXT("+CAM3xxOTG::~CAM3xxOTG()\r\n")));

    // Terminate polling thread
    SetEvent(m_hPollThreadExitEvent);
    if(WaitForSingleObject(m_hPollThread, m_PollTimeout) == WAIT_TIMEOUT)
    {
        // force thread termination
        DEBUGMSG(ZONE_OTG_FUNCTION,(TEXT("CAM3xxOTG::~CAM3xxOTG(): terminating poll thread...\r\n")));
#pragma warning(push)
#pragma warning(disable: 6258)
        TerminateThread(m_hPollThread, 0);
#pragma warning(push)
    }

    // Clean up event handle
    CloseHandle(m_hPollThreadExitEvent);
    CloseHandle(m_hPollThread);

    // Deallocate hardware register pointers
    if (m_pUsbRegs)
        MmUnmapIoSpace(m_pUsbRegs, sizeof(CSL_UsbRegs));
    if (m_pSys)
        MmUnmapIoSpace((PVOID)m_pSys, sizeof(AM33X_DEVICE_CONF_REGS));

    DEBUGMSG(ZONE_OTG_FUNCTION,(TEXT("-CAM3xxOTG::~CAM3xxOTG()\r\n")));
}

BOOL CAM3xxOTG::PowerUp()
{
    DEBUGMSG(ZONE_OTG_FUNCTION,(TEXT("+CAM3xxOTG::PowerUp()\r\n")));
	DEBUGMSG(ZONE_OTG_FUNCTION,(TEXT("-CAM3xxOTG::PowerUp()\r\n")));
    return TRUE;
}

BOOL CAM3xxOTG::PowerDown()
{
    DEBUGMSG(ZONE_OTG_FUNCTION,(TEXT("+CAM3xxOTG::PowerDown()\r\n")));
	DEBUGMSG(ZONE_OTG_FUNCTION,(TEXT("-CAM3xxOTG::PowerDown()\r\n")));
    return TRUE;
}
DWORD UnloadDrivers(CAM3xxOTG* pOTG)
{
    pOTG->HostMode(FALSE);
    pOTG->FunctionMode(FALSE);
    return 0;
}

BOOL CAM3xxOTG::UpdatePowerState()
{
    BOOL rc = FALSE;
//	UINT32 nPhyCtl = 0;
    
	DEBUGMSG(ZONE_OTG_FUNCTION,(TEXT("+CAM3xxOTG::UpdatePowerState()\r\n")));

    if (m_dwOldPowerState==m_RequestedPowerState) return TRUE;

    // Apply the requested power state
    switch(m_RequestedPowerState)
    {
    case D0:
	case D1:
	case D2:
		if ((m_dwOldPowerState == D0)||(m_dwOldPowerState == D1)
			||(m_dwOldPowerState == D2))
		{
			rc = TRUE;
			break;
		}
		
/****************************************************************/		
	    // Global Reset; I don't think we can reset the entire USBSS, but one port only

		// Enable Clocks
		USBCDMA_EnableClocks(m_Index - 1,TRUE);
		// Request pads
		
		if (m_Index == 1){
			// turn off the PHY
			m_pSys->USB_CTRL0 = 0x3; 
			StallExecution(100);
			
			m_pSys->USB_CTRL0 |= (3 << 19);
			m_pSys->USB_CTRL0 &= ~(1 << 23);
			
			m_pUsbRegs->MODE_R &= ~0x100;
			m_pUsbRegs->MODE_R |= 0x080;

			// turn on the PHYs
			m_pSys->USB_CTRL0 &= ~0x03;
		} else {
			// turn off the PHY
			m_pSys->USB_CTRL1 = 0x3; 
			StallExecution(100);
			
			m_pSys->USB_CTRL1 |= (3 << 19);
			m_pSys->USB_CTRL1 &= ~(1 << 23);
			
			m_pUsbRegs->MODE_R &= ~0x100;
			m_pUsbRegs->MODE_R |= 0x080;

			// turn on the PHYs
			m_pSys->USB_CTRL1 &= ~0x03;
		}
		StallExecution(100);
		
	    // Reset the controller
	    m_pUsbRegs->CTRLR |= 1;
		StallExecution(10);
		while (m_pUsbRegs->CTRLR & 1); //  wait for reset done
/*********************************************************/

        if(m_bFunctionMode && (m_pUsbRegs->DEVCTL & CSL_USB_DEVCTL_BDEVICE_MASK)) 
		{
            //SOFTCONN
            m_pUsbRegs->POWER |= CSL_USB_POWER_SOFTCONN_MASK;
        }

        // Enable USB state polling
        m_bEnablePolling = TRUE;
        rc = TRUE;
        break;
    case D3:
    case D4:
        if ((m_dwOldPowerState >=D3) && (m_dwOldPowerState <= D4)) 
        {
            rc=TRUE;
            break;
        }

        // Disable USB state polling
        m_bEnablePolling = FALSE;
        if (WaitForSingleObject(
            CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)UnloadDrivers,this,0,NULL)
            ,2000) != WAIT_OBJECT_0)
        {
            RETAILMSG(1,(TEXT("driver unloading timed out. maybe because the suspend order came from the USB device (power key of a keybaord)\r\n")));
        }
        
/**********************************************************************/
        m_pUsbRegs->DEVCTL &= ~CSL_USB_DEVCTL_SESSION_MASK; // End the session for host
        m_pUsbRegs->POWER &= ~CSL_USB_POWER_SOFTCONN_MASK;  // Disconnect peripheral
        
		// turn off the PHY
		if (m_Index == 1)
			m_pSys->USB_CTRL0 = 0x3;
		else
			m_pSys->USB_CTRL1 = 0x3;
		
		// Power down the OTG
		// Release pads

		// Clocks off
        USBCDMA_EnableClocks(m_Index - 1,FALSE);
/**********************************************************************/
        rc = TRUE;    
        break;
    default:
        DEBUGMSG(ZONE_OTG_ERROR,(TEXT("COMAPL13xOTG::UpdatePowerState: do not support power state %d\r\n"), m_RequestedPowerState));
        break;
    }

    if (rc) m_dwOldPowerState=m_RequestedPowerState;             

    DEBUGMSG(ZONE_OTG_FUNCTION,(TEXT("-COMAPL13xOTG::UpdatePowerState(%d)\r\n"), rc));

    return rc;
}

BOOL CAM3xxOTG::IOControl(DWORD dwCode, PBYTE pBufIn, DWORD dwLenIn, PBYTE pBufOut, DWORD dwLenOut, PDWORD pdwActualOut)
{
    BOOL bReturn = FALSE;

    DEBUGMSG(ZONE_OTG_FUNCTION,(TEXT("+CAM3xxOTG::IoControl(%d)\r\n"), dwCode));

    switch (dwCode) 
    {
        // Reimplemented IOCTL_POWER_SET in PDD layer, as the code
        // in the MDD has a bug (it always returns FALSE!)
        case IOCTL_POWER_SET:
            DEBUGMSG(ZONE_OTG_STATE,(TEXT("CAM3xxOTG::IOControl: IOCTL_POWER_SET\r\n")));
            if (!pBufOut || dwLenOut < sizeof(CEDEVICE_POWER_STATE)) {
                SetLastError(ERROR_INVALID_PARAMETER);
                bReturn = FALSE;
            } else {
                m_dwOldPowerState = m_RequestedPowerState;
                m_RequestedPowerState = *(PCEDEVICE_POWER_STATE) pBufOut;
                m_IsThisPowerManaged = TRUE;
                DEBUGMSG(ZONE_OTG_STATE, (TEXT("CAM3xxOTG::IOControl:IOCTL_POWER_SET: D%d\r\n"), m_RequestedPowerState));
                if (!UpdatePowerState()) m_RequestedPowerState = m_dwOldPowerState;
                bReturn = TRUE;
                // did we set the device power?
                if (pdwActualOut) {
                    *pdwActualOut = sizeof(CEDEVICE_POWER_STATE);
                }
            }
            break;
        // Implemented IOCTL_POWER_GET in PDD layer, as the MDD
        // layer does not implement this IOCTL
        case IOCTL_POWER_GET: // gets the current device power state
            DEBUGMSG(ZONE_OTG_STATE,(TEXT("CAM3xxOTG::IOControl: IOCTL_POWER_GET\r\n")));
            if (!pBufOut || dwLenOut < sizeof(CEDEVICE_POWER_STATE)) {
                SetLastError(ERROR_INVALID_PARAMETER);
                bReturn = FALSE;
            } else {
                *(PCEDEVICE_POWER_STATE) pBufOut = m_RequestedPowerState;
                if (pdwActualOut) {
                    *pdwActualOut = sizeof(CEDEVICE_POWER_STATE);
                }                
                bReturn = TRUE;
            }
            break;
        // Implemented IOCTL_BUS_SET_POWER_STATE in PDD layer, as the MDD
        // layer uses default bus driver with wrong parameters
		case IOCTL_BUS_SET_POWER_STATE:
			DEBUGMSG(ZONE_OTG_STATE,(TEXT("CAM3xxOTG::IOControl: IOCTL_BUS_SET_POWER_STATE\r\n")));
            if ((pBufIn == NULL) || (dwLenIn < sizeof(CE_BUS_POWER_STATE))) {
                SetLastError(ERROR_INVALID_PARAMETER);
                bReturn = FALSE;
			} else {
				CE_BUS_POWER_STATE* state = (CE_BUS_POWER_STATE *)pBufIn;
				CEDDK_BUS_POWER_STATE ceddkPowerState;

				ceddkPowerState.DevicePowerState = *(state->lpceDevicePowerState);
				ceddkPowerState.lpReserved = state->lpReserved;

				bReturn = DefaultBusDriver::IOControl(IOCTL_BUS_SET_POWER_STATE, 
											(PBYTE)state->lpDeviceBusName, sizeof(state->lpDeviceBusName), 
											(PBYTE)&ceddkPowerState, sizeof(CEDDK_BUS_POWER_STATE),
											pdwActualOut) ;
			}

			break;
		case 0x43210001:
			if ((pBufOut != NULL) && (dwLenOut >= (sizeof(DWORD)*(15))) && (pdwActualOut != 0)){
				UINT32 *pbo = (UINT32*)pBufOut;
				pbo[0] = m_UsbOtgInput.ul;
				pbo[1] = m_UsbOtgInternal.ul;
				pbo[2] = m_UsbOtgOutputValues.ul;
				pbo[3] = m_UsbOtgState;
				pbo[4] = m_UsbOtgMode;
				memcpy(&pbo[5], &m_UsbOtgTimers, sizeof(USBOTG_TIMERS));
				pbo[11]= m_bEnablePolling; 
				pbo[12]= m_bFunctionMode;
				pbo[13]= m_InFunctionModeFn;
				pbo[14]= m_bHostMode;
				*pdwActualOut = 15;
				bReturn = TRUE;
			}
			break;

			
        default:    
            bReturn = USBOTG::IOControl(dwCode,pBufIn,dwLenIn,pBufOut,dwLenOut,pdwActualOut);
            break;
    }

    DEBUGMSG(ZONE_OTG_FUNCTION,(TEXT("-CAM3xxOTG::IoControl(%d)\r\n"), bReturn));
    return bReturn;
}

// OTG PDD Functions
BOOL CAM3xxOTG::SessionRequest(BOOL fPulseLocConn, BOOL fPulseChrgVBus)
{
    DEBUGMSG(ZONE_OTG_FUNCTION,(TEXT("+CAM3xxOTG::SessionRequest(%d,%d)\r\n"), fPulseLocConn, fPulseChrgVBus));
 RETAILMSG(1,(TEXT("+CAM3xxOTG::SessionRequest(%d,%d)\r\n"), fPulseLocConn, fPulseChrgVBus));
    m_pUsbRegs->DEVCTL = (fPulseLocConn || fPulseChrgVBus) ?  BIT0 : 0;
    DEBUGMSG(ZONE_OTG_FUNCTION,(TEXT("-CAM3xxOTG::SessionRequest()\r\n")));
    return TRUE;
}

BOOL CAM3xxOTG::NewStateAction(USBOTG_STATES usbOtgState , USBOTG_OUTPUT usbOtgOutput) 
{
	UNREFERENCED_PARAMETER(usbOtgOutput);

    DEBUGMSG(ZONE_OTG_FUNCTION, (TEXT("+CAM3xxOTG::NewStateAction()\r\n")));
    DEBUGMSG(ZONE_OTG_FUNCTION, (TEXT("USBOTG state: %s\n\r"), g_cppOtgStateString[usbOtgState]));
//RETAILMSG(0, (TEXT("NewStateAction USBOTG[%d]%08x state: %s; b_conn %d\r\n"),
//	       m_Index, m_pUsbRegs->DEVCTL, g_cppOtgStateString[usbOtgState], m_UsbOtgInput.bit.b_conn));

    // Load correct driver for current state
    switch(usbOtgState)
    {        
        case USBOTG_a_host:
        // in host mode
        if (m_UsbOtgInput.bit.b_conn != 0)
        {
            HostMode(TRUE);            
        }
        break;
    case USBOTG_b_peripheral:
        // in function mode
        FunctionMode(TRUE);
        break;
    case USBOTG_a_wait_vfall:
        // Clear session bit
        m_pUsbRegs->DEVCTL &= ~CSL_USB_DEVCTL_SESSION_MASK;
        // Deliberate fallthrough
    default:
        // In neither host nor function,
        // unload both drivers
        HostMode(FALSE);
        FunctionMode(FALSE);
        break;
    }

    DEBUGMSG(ZONE_OTG_FUNCTION,(TEXT("-CAM3xxOTG::NewStateAction()\r\n")));

    // Always returning FALSE means the MDD progresses through the USBOTG state machine 
    // more slowly. This helps the case where a host cable is attached with no device.
    return FALSE;
}

BOOL CAM3xxOTG::IsSE0()
{
    DEBUGMSG(ZONE_OTG_FUNCTION,(TEXT("+CAM3xxOTG::IsSE0()\r\n")));
    DEBUGMSG(ZONE_OTG_FUNCTION,(TEXT("-CAM3xxOTG::IsSE0()\r\n")));
    return FALSE;
}

BOOL CAM3xxOTG::UpdateInput() 
{
    UINT8 devctl = 0;
    UINT8 vbus = 0;
    UINT32 statr = 0;

    DEBUGMSG(ZONE_OTG_FUNCTION,(TEXT("+CAM3xxOTG::UpdateInput()\r\n")));

    // read USB controller registers
    devctl = m_pUsbRegs->DEVCTL;
    statr = m_pUsbRegs->STATR;
//    DEBUGMSG(ZONE_OTG_FUNCTION,
//        (TEXT("DEVCTL 0x%x, CFGCHIP2 0x%x, STATR 0x%x\r\n"), devctl, cfgchip2, statr));
    DEBUGMSG(ZONE_OTG_FUNCTION,
        (TEXT("DEVCTL 0x%x, STATR 0x%x\r\n"), devctl, statr));

//VAREMOVE DEBUGMSG(1,(TEXT("DEVCTL 0x%x, STATR 0x%x\r\n"), devctl, statr));

    // fix some bits of MDD input to ensure
    // correct movement about the state machine
    m_UsbOtgInput.bit.a_bus_drop = 0;
    m_UsbOtgInput.bit.a_bus_req = 1;

    // check to see if a session is active
    if(devctl & CSL_USB_DEVCTL_SESSION_MASK)
    {
        // Session valid
        DEBUGMSG(ZONE_OTG_FUNCTION,(TEXT("    Session Valid\r\n")));
//VAREMOVE DEBUGMSG(1,(TEXT("    Session Valid\r\n")));

        // Determine A or B device
        if((devctl & CSL_USB_DEVCTL_BDEVICE_MASK) == 0)
        {
            // 'A' device
            DEBUGMSG(ZONE_OTG_FUNCTION,(TEXT("    'A' device\r\n")));
            m_UsbOtgInput.bit.id = 0;

            if((devctl & CSL_USB_DEVCTL_HOSTMODE_MASK) == 0)
            {
                // The controller has dropped host mode, 
                // so no device is attached
                m_UsbOtgInput.bit.b_conn = 0;
                DEBUGMSG(ZONE_OTG_FUNCTION,(TEXT("    host mode OFF\r\n")));
           }
            else
            {
                // Operating as host, B device connected
                m_UsbOtgInput.bit.b_conn = 1;
                DEBUGMSG(ZONE_OTG_FUNCTION,(TEXT("    host mode ON\r\n")));
            }
        }
        else
        {
            // 'B' device
            DEBUGMSG(ZONE_OTG_FUNCTION,(TEXT("    'B' device\r\n")));
            m_UsbOtgInput.bit.id = 1;
        }
    }
    else
    {
        DEBUGMSG(ZONE_OTG_FUNCTION,(TEXT("    Session End\r\n")));

        // no session active, set the session bit 
        // to identify USB mode
        m_pUsbRegs->DEVCTL |= CSL_USB_DEVCTL_SESSION_MASK;
        Sleep(100);
    }

    // Vbus
    m_UsbOtgInput.bit.a_vbus_vld = 0;
    m_UsbOtgInput.bit.b_sess_vld = 0;
    vbus = (devctl & CSL_USB_DEVCTL_VBUS_MASK) >> CSL_USB_DEVCTL_VBUS_SHIFT;
    switch(vbus)
    {
    case 0:
        DEBUGMSG(ZONE_OTG_FUNCTION,(TEXT("    VBus: Below Session End\r\n")));
        break;
    case 1:
        DEBUGMSG(ZONE_OTG_FUNCTION,(TEXT("    VBus: Above Session End, below AValid\r\n")));
        break;
    case 2:
        DEBUGMSG(ZONE_OTG_FUNCTION,(TEXT("    VBus: Above AValid, below VBusValid\r\n")));
        break;
    case 3:
        DEBUGMSG(ZONE_OTG_FUNCTION,(TEXT("    VBus: Above VBusValid\r\n")));
        // The bus voltage is good. 
        // In 'A' mode, this means the host is driving the bus OK (Vbus valid)
        // In 'B' mode, this means a connected device is driving the bus OK. 
        m_UsbOtgInput.bit.a_vbus_vld = 1;
        m_UsbOtgInput.bit.b_sess_vld = 1;
        break;
    default:
        DEBUGMSG(ZONE_OTG_FUNCTION,(TEXT("    VBus: ERROR, invalid state (%d)\r\n"), vbus));
        break;
    }

    DEBUGMSG(ZONE_OTG_FUNCTION, 
        (L"CAM3xxOTG::UpdateInput:m_UsbOtgInput.ul=%x\r\n",m_UsbOtgInput.ul));
    DEBUGMSG(ZONE_OTG_FUNCTION,(TEXT("-CAM3xxOTG::UpdateInput()\r\n")));
    return TRUE;
}

DWORD CAM3xxOTG::PollThread(void *data)
{
    BOOL bTerminate = FALSE;
    CAM3xxOTG *context = (CAM3xxOTG *)data;
    DEBUGMSG(ZONE_OTG_FUNCTION,(TEXT("+CAM3xxOTG::PollThread()\r\n")));
       
    if(context != NULL)
    {
        while (!bTerminate) 
        {
            switch (WaitForSingleObject(context->m_hPollThreadExitEvent, context->m_PollTimeout))
            {
            case WAIT_OBJECT_0:
                DEBUGMSG(ZONE_OTG_FUNCTION,(TEXT("CAM3xxOTG::PollThread(): Exit event signalled\r\n")));
                bTerminate = TRUE;
                break;
            case WAIT_TIMEOUT:
                DEBUGMSG(ZONE_OTG_FUNCTION,(TEXT("CAM3xxOTG::PollThread(): Polling...\r\n")));
//VAREMOVE DEBUGMSG(1,(L"CAM3xxOTG::PollThread(): Polling... %d\r\n", context->m_bEnablePolling));
                // Call MDD to query system state. 
                // (This is turn calls UpdateInput, which 
                // sets the bits of m_UsbOtgInput to expose
                // the USB state)
                if(context->m_bEnablePolling)
                {
                    context->UpdateInput();
                    context->EventNotification();
                }
                break;
            default:
                // error!
                bTerminate = TRUE;
                break;
            }
        }
    }
    else
        DEBUGMSG(ZONE_OTG_ERROR,(TEXT("CAM3xxOTG::PollThread(): ERROR: invalid context parameter\r\n")));

    DEBUGMSG(ZONE_OTG_FUNCTION,(TEXT("-CAM3xxOTG::PollThread()\r\n")));
RETAILMSG(1,(TEXT("-CAM3xxOTG::PollThread() %d\r\n"), context->m_Index));
    return 0;
}

VOID CAM3xxOTG::HostMode(BOOL start)
{
	BOOL bRes;
    // only need to load/unload if required
    // state is different from current one
    if(m_bHostMode != start)
    {
        DEBUGMSG(ZONE_OTG_FUNCTION,(TEXT("CAM3xxOTG::HostMode(): %s host mode\r\n"), start? (L"entering"):(L"leaving")));
RETAILMSG(1,(TEXT("CAM3xxOTG[%d]::HostMode(): %s host mode\r\n"),m_Index, start? (L"entering"):(L"leaving")));
		m_bHostMode = start;
        bRes = LoadUnloadHCD(start);
    }
}

VOID CAM3xxOTG::FunctionMode(BOOL start)
{    
    if (m_InFunctionModeFn)
    {
        DEBUGMSG(ZONE_OTG_FUNCTION,(TEXT("CAM3xxOTG::FunctionMode() recursion prevented\r\n")));
        return;
    }
    m_InFunctionModeFn = TRUE;
    // only need to load/unload if required
    // state is different from current one
    if(m_bFunctionMode != start)
    {
        DEBUGMSG(ZONE_OTG_FUNCTION,(TEXT("CAM3xxOTG::FunctionMode(): %s function mode\r\n"), start? (L"entering"):(L"leaving")));
RETAILMSG(TRUE,(TEXT("CAM3xxOTG[%d]::FunctionMode(): %s function mode\r\n"), m_Index, start? (L"entering"):(L"leaving")));
        m_bFunctionMode = start;
        LoadUnloadUSBFN(start);
    }
	m_InFunctionModeFn = FALSE;
}

// Class Factory.
USBOTG *CreateUSBOTGObject(LPTSTR lpActivePath)
{
    return new CAM3xxOTG(lpActivePath);
}
void DeleteUSBOTGObject(USBOTG *pUsbOtg)
{
    if (pUsbOtg)
        delete pUsbOtg;
}

