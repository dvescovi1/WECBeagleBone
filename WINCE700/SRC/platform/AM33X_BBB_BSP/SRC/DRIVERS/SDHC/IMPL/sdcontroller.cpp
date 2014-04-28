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
//  File: sdcontroller.cpp
//
//

#include <windows.h>
#include <nkintr.h>
#include <oalex.h>
#include "SDController.h"


CSDIOController::CSDIOController()
    : CSDIOControllerBase()
{
    CDIntrEnabled = FALSE;
}

CSDIOController::~CSDIOController()
{
}


//-----------------------------------------------------------------------------
BOOL CSDIOController::InitializeInterrupt()
{
    return TRUE;
}


// Is this card write protected?
BOOL CSDIOController::IsWriteProtected()
{
    DWORD MMC_PSTATE = INREG32(&m_pbRegisters->MMCHS_PSTATE);

    return ( MMC_PSTATE & MMCHS_PSTATE_WP );
}

// Is the card present?
//          CDP
//       | 0   1
//---------------
//      0| F   T
// CINS  |
//      1| T   F
BOOL CSDIOController::SDCardDetect()
{
    DWORD MMC_CON    = INREG32(&m_pbRegisters->MMCHS_CON);
    DWORD MMC_PSTATE = INREG32(&m_pbRegisters->MMCHS_PSTATE);

	return ( ((MMC_CON & MMCHS_CON_CDP)?TRUE:FALSE) !=
		((MMC_PSTATE & MMCHS_PSTATE_CINS)?TRUE:FALSE) );
}

//-----------------------------------------------------------------------------
BOOL CSDIOController::InitializeHardware()
{
    DWORD               dwCountStart;

    // Manually enable clock here. 
    EnableDeviceClocks(SOCGetSDHCDeviceBySlot(m_dwSlot), TRUE);

    // Reset the controller
    OUTREG32(&m_pbRegisters->MMCHS_SYSCONFIG, MMCHS_SYSCONFIG_SOFTRESET);

    // calculate timeout conditions
    dwCountStart = GetTickCount();

    // Verify that reset has completed.
    while (!(INREG32(&m_pbRegisters->MMCHS_SYSSTATUS) & MMCHS_SYSSTATUS_RESETDONE))
    {
        Sleep(0);

        // check for timeout
        if (GetTickCount() - dwCountStart > m_dwMaxTimeout)
        {
            DEBUGMSG(ZONE_ENABLE_ERROR, (TEXT("InitializeModule() - exit: TIMEOUT.\r\n")));
            goto cleanUp;
        }
    }
    return TRUE;

cleanUp:
    RETAILMSG(1,  (L"-InitializeHardware: Error\r\n")); 
    return FALSE;
}

void CSDIOController::DeinitializeHardware()
{
    EnableDeviceClocks(SOCGetSDHCDeviceBySlot(m_dwSlot), FALSE);
}

//-----------------------------------------------------------------------------
//  SDHCCardDetectIstThreadImpl - card detect IST thread for driver
//  Input:  lpParameter - pController - controller instance
//  Output:
//  Return: Thread exit status
DWORD CSDIOController::SDHCCardDetectIstThreadImpl()
{
    DWORD   dataSz = 0;
	DWORD   data;
	DWORD	slot;

    // check for the card already inserted when the driver is loaded
	Sleep(100);
    if (!KernelIoControl(IOCTL_HAL_GET_BOOT_INFO,
                         NULL, 0, &data, sizeof(DWORD), &dataSz))
    {
        RETAILMSG( TRUE,(TEXT("Failed to read boot info from Boot Args\r\n")));        
    }
	slot = data>>16;
	if (slot != m_dwSlot)
	{
		// if not the boot device delay so boot device mounts first.
		// First mounted device will get the persistent registry.
		Sleep(3000);
	}

	if(SDCardDetect())
    {
        CardInterrupt(TRUE);
    }

    return ERROR_SUCCESS;
}

//-----------------------------------------------------------------------------
VOID CSDIOController::PreparePowerChange(CEDEVICE_POWER_STATE curPowerState, BOOL bInPowerHandler)
{
    UNREFERENCED_PARAMETER(bInPowerHandler);

}

VOID CSDIOController::PostPowerChange(CEDEVICE_POWER_STATE curPowerState, BOOL bInPowerHandler)
{
	UNREFERENCED_PARAMETER(bInPowerHandler);

}

VOID CSDIOController::TurnCardPowerOn()
{
    SetSlotPowerState( D0 );
}

VOID CSDIOController::TurnCardPowerOff()
{
    SetSlotPowerState( D4 );
}

CSDIOControllerBase *CreateSDIOController()
{
    return new CSDIOController();
}

//-----------------------------------------------------------------------------
// Enable SDHC Card Detect Interrupts.
VOID CSDIOController::EnableSDHCCDInterrupts()
{
    EnterCriticalSection( &m_critSec );
    SETREG32(&m_pbRegisters->MMCHS_ISE, (MMCHS_IE_CREM|MMCHS_IE_CINS));
    SETREG32(&m_pbRegisters->MMCHS_IE,  (MMCHS_IE_CREM|MMCHS_IE_CINS));
    CDIntrEnabled = TRUE;
    LeaveCriticalSection( &m_critSec );
}

//-----------------------------------------------------------------------------
// Disable SDHC Card Detect Interrupts.
void CSDIOController::DisableSDHCCDInterrupts()
{
    EnterCriticalSection( &m_critSec );
    CLRREG32(&m_pbRegisters->MMCHS_ISE, (MMCHS_IE_CREM|MMCHS_IE_CINS));
    CLRREG32(&m_pbRegisters->MMCHS_IE,  (MMCHS_IE_CREM|MMCHS_IE_CINS));
    CDIntrEnabled = FALSE;
    LeaveCriticalSection( &m_critSec );
}

void CSDIOController::Write_MMC_CD_STAT( DWORD dwVal )
{
    EnterCriticalSection( &m_critSec );
    dwVal &= (MMCHS_STAT_CREM|MMCHS_STAT_CINS);
    OUTREG32(&m_pbRegisters->MMCHS_STAT, dwVal);
    LeaveCriticalSection( &m_critSec );
}

