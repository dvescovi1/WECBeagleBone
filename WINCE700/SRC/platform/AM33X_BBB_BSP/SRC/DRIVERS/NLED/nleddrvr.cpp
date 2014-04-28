// Copyright (c) David Vescovi.  All rights reserved.
// Copyright (c) Microsoft Corporation.  All rights reserved.
//===================================================================
//
//	Module Name:	NLED.DLL
//
//	File Name:		nleddrv.c
//
//	Description:	Control of the notification LED(s)
//
//===================================================================
#pragma warning(push)
#pragma warning(disable : 4115 4214)
#include <windows.h>
#include <winuser.h>
#include <winuserm.h>
#include <ceddk.h>
#include <ceddkex.h>
#include <nled.h>
#include <led_drvr.h>
#include <initguid.h>
#pragma warning(pop)

#include "sdk_gpio.h"
#include "oalex.h"

#include "bsp_padcfg.h"
#include "sdk_padcfg.h"
#include "bsp.h"



//------------------------------------------------------------------------------
// External Functions

//------------------------------------------------------------------------------
// External Variables 
int NLedCpuFamily = -1;

//------------------------------------------------------------------------------
// Defines 
#define NLED_MAX_LED				4	// Total number of notification LEDs supported
#define NLED_MAX_OFFONBLINK 		2 	// Max OffOnBlink value

//------------------------------------------------------------------------------
// Types
struct NLedPddParameters
{
	HANDLE	hNLedStopThreadEvent;
	HANDLE	hNLedThreadStoppedEvent;
	BOOL	bNledState;
	BOOL	bNLedDriverSetDeviceThreadState;
	NLED_SETTINGS_INFO NLedSettingsInfo;
	NLED_SUPPORTS_INFO NLedSupportsInfo;
	DWORD	GPIOId;
	DWORD	GPIOActiveState;
};

//------------------------------------------------------------------------------
// Global Variables 
static HANDLE g_hGPIO;
static NLedPddParameters g_NLedPddParam[NLED_MAX_LED];

static const PAD_INFO NLedPads[]    = {NLED_PADS END_OF_PAD_ARRAY};

//------------------------------------------------------------------------------
// Local Functions
DWORD WINAPI NLedDriverSetDeviceThread(LPVOID lParam);

//-----------------------------------------------------------------------------
//
// Function: NLedDriverInitialize
//
// The NLED MDD calls this routine to initialize the underlying NLED hardware.
//
// Parameters:
//		None
//
// Returns:  
// 		This routine should return TRUE if successful. If there's a problem
// 		it should return FALSE
//
//-----------------------------------------------------------------------------
BOOL WINAPI
NLedDriverInitialize(VOID)
{

	DEBUGMSG(ZONE_PDD, (_T("+NLedDriverInitialize\r\n")));

	if (!RequestAndConfigurePadArray(NLedPads))
    {
        ERRORMSG(1,(TEXT("Unable to request PAD configuration for NLED driver\r\n")));
        return FALSE;
    }


	// Open gpio driver
    g_hGPIO = GPIOOpen();
    if (g_hGPIO == NULL)
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: NLedDriverInitialize: Failed to open Gpio driver!\r\n"));
        return FALSE;
    }

	for(int i=0; i<NLED_MAX_LED; i++)
	{
		g_NLedPddParam[i].bNledState = FALSE;
		g_NLedPddParam[i].bNLedDriverSetDeviceThreadState = FALSE;
		g_NLedPddParam[i].hNLedStopThreadEvent = NULL;
		g_NLedPddParam[i].hNLedThreadStoppedEvent = NULL;
		
		g_NLedPddParam[i].NLedSettingsInfo.LedNum = 0;
		g_NLedPddParam[i].NLedSettingsInfo.MetaCycleOff = 0;
		g_NLedPddParam[i].NLedSettingsInfo.MetaCycleOn = 0;
		g_NLedPddParam[i].NLedSettingsInfo.OffOnBlink = 0;
		g_NLedPddParam[i].NLedSettingsInfo.OffTime = 0;
		g_NLedPddParam[i].NLedSettingsInfo.OnTime = 0;
		g_NLedPddParam[i].NLedSettingsInfo.TotalCycleTime = 0;	

		g_NLedPddParam[i].NLedSupportsInfo.LedNum = 0;
		g_NLedPddParam[i].NLedSupportsInfo.fAdjustOffTime = TRUE;
		g_NLedPddParam[i].NLedSupportsInfo.fAdjustOnTime = TRUE;
		g_NLedPddParam[i].NLedSupportsInfo.fAdjustTotalCycleTime = FALSE;
		g_NLedPddParam[i].NLedSupportsInfo.lCycleAdjust = 1000;
		g_NLedPddParam[i].NLedSupportsInfo.fMetaCycleOff = FALSE;
		g_NLedPddParam[i].NLedSupportsInfo.fMetaCycleOn = FALSE;
		g_NLedPddParam[i].GPIOActiveState = 1;
		switch (i) {
			case 0:
				g_NLedPddParam[i].GPIOId = NOTIFICATION_LED0_GPIO;
				break;
			case 1:
				g_NLedPddParam[i].GPIOId = NOTIFICATION_LED1_GPIO;
				break;
			case 2:
				g_NLedPddParam[i].GPIOId = NOTIFICATION_LED2_GPIO;
				break;
			case 3:
				g_NLedPddParam[i].GPIOId = NOTIFICATION_LED3_GPIO;
				break;
			default:
				break;
		}
		GPIOSetMode(g_hGPIO, g_NLedPddParam[i].GPIOId, GPIO_DIR_OUTPUT);
		// Leds initially off
		if (g_NLedPddParam[i].GPIOActiveState == 0)
			GPIOSetBit(g_hGPIO, g_NLedPddParam[i].GPIOId);
		else
			GPIOClrBit(g_hGPIO, g_NLedPddParam[i].GPIOId);
	}
	
	for(int i=0; i<NLED_MAX_LED; i++)
	{		
		if((g_NLedPddParam[i].hNLedStopThreadEvent = CreateEvent(NULL, FALSE, FALSE, NULL)) == NULL) 
		{
			while(i)
			{
				i--;
				CloseHandle(g_NLedPddParam[i].hNLedStopThreadEvent);				
			}
			
			DEBUGMSG(ZONE_PDD, (_T("NLedDriverInitialize: CreateEvent() hNLedStopThreadEvent Failed\r\n")));
			return FALSE;
		}
	}

	for(int i=0; i<NLED_MAX_LED; i++)
	{		
		if((g_NLedPddParam[i].hNLedThreadStoppedEvent = CreateEvent(NULL, FALSE, FALSE, NULL)) == NULL)
		{
			
			while(i)
			{
				i--;
				CloseHandle(g_NLedPddParam[i].hNLedThreadStoppedEvent);				
			}
			
			DEBUGMSG(ZONE_PDD, (_T("NLedDriverInitialize: CreateEvent() hNLedThreadStoppedEvent Failed\r\n")));
			return FALSE;
		}
	}
	
	DEBUGMSG(ZONE_PDD, (_T("-NLedDriverInitialize\r\n")));

	return (TRUE);
  }

//-----------------------------------------------------------------------------
//
// Function: NLedDriverDeInitialize
//
// This routine deinitializes the underlying NLED hardware as the NLED driver is unloaded.
//
// Parameters:
//		None
//
// Returns:  
//      This routine returns TRUE if successful, or FALSE if there's a problem
//
//-----------------------------------------------------------------------------
BOOL WINAPI
NLedDriverDeInitialize(VOID)
{
    DEBUGMSG(ZONE_PDD, (_T("+NLedDriverDeInitialize\r\n")));

    ReleasePadArray(NLedPads);

	for(int i=0; i<NLED_MAX_LED; i++)		// Stop all threads
		SetEvent(g_NLedPddParam[i].hNLedStopThreadEvent);

	
	for(int i=0; i<NLED_MAX_LED; i++)		// Wait for all threads to stop
	{
		if(g_NLedPddParam[i].bNLedDriverSetDeviceThreadState)
			WaitForSingleObject(g_NLedPddParam[i].hNLedThreadStoppedEvent, INFINITE);
	}	
	
	for(int i=0; i<NLED_MAX_LED; i++)
	{
		CloseHandle(g_NLedPddParam[i].hNLedThreadStoppedEvent);
		CloseHandle(g_NLedPddParam[i].hNLedStopThreadEvent);
	}
	
    GPIOClose(g_hGPIO);

	DEBUGMSG(ZONE_PDD, (_T("-NLedDriverDeInitialize\r\n")));

	return (TRUE);
}

//-----------------------------------------------------------------------------
//
// Function: NLedDriverGetDeviceInfo
//
// This routine retrieves information about the NLED device(s) that
// this driver supports.  The nInfoId parameter indicates what specific
// information is being queried and pOutput is a buffer to be filled in.
// The size of pOutput depends on the type of data being requested.
//
// Parameters:
//      InputParm 
//          [in] INT     nInfoId
//
//      OutputParm 
//          [out] PVOID   pOutput
//
// Returns:  
//      This routine returns TRUE if successful, or FALSE if there's a problem
//
//-----------------------------------------------------------------------------
BOOL WINAPI
NLedDriverGetDeviceInfo(
                       INT     nInfoId,
                       PVOID   pOutput
                       )
{
	DEBUGMSG(ZONE_PDD, (_T("+NLedDriverGetDeviceInfo\r\n")));

	BOOL fOk = TRUE;	

	if(nInfoId == NLED_COUNT_INFO_ID)
	{
		struct NLED_COUNT_INFO  *p = (struct NLED_COUNT_INFO*)pOutput;						
        	p -> cLeds = NLED_MAX_LED;
	}			
	else if (nInfoId == 	NLED_SUPPORTS_INFO_ID)
	{
		struct NLED_SUPPORTS_INFO  *p = (struct NLED_SUPPORTS_INFO*)pOutput;

		if(p->LedNum >= NLED_MAX_LED)			
			fOk = FALSE;
		else
		{		
			p->fAdjustOffTime			= g_NLedPddParam[p->LedNum].NLedSupportsInfo.fAdjustOffTime;
			p->fAdjustOnTime 			= g_NLedPddParam[p->LedNum].NLedSupportsInfo.fAdjustOnTime;
			p->lCycleAdjust 			= g_NLedPddParam[p->LedNum].NLedSupportsInfo.lCycleAdjust;
			p->fMetaCycleOff 			= g_NLedPddParam[p->LedNum].NLedSupportsInfo.fMetaCycleOff;
			p->fMetaCycleOn 			= g_NLedPddParam[p->LedNum].NLedSupportsInfo.fMetaCycleOn;
			p->fAdjustTotalCycleTime	= g_NLedPddParam[p->LedNum].NLedSupportsInfo.fAdjustTotalCycleTime;
		}
	}	
	else if (nInfoId ==	NLED_SETTINGS_INFO_ID)
	{
		struct NLED_SETTINGS_INFO  *p = (struct NLED_SETTINGS_INFO*)pOutput;

		if(p->LedNum >= NLED_MAX_LED)			
			fOk = FALSE;
		else
		{		
			p->MetaCycleOff 	= g_NLedPddParam[p->LedNum].NLedSettingsInfo.MetaCycleOff;
			p->MetaCycleOn 		= g_NLedPddParam[p->LedNum].NLedSettingsInfo.MetaCycleOn;
			p->OffOnBlink 		= g_NLedPddParam[p->LedNum].NLedSettingsInfo.OffOnBlink;
			p->OffTime 			= g_NLedPddParam[p->LedNum].NLedSettingsInfo.OffTime;
			p->OnTime 			= g_NLedPddParam[p->LedNum].NLedSettingsInfo.OnTime;
			p->TotalCycleTime 	= g_NLedPddParam[p->LedNum].NLedSettingsInfo.TotalCycleTime;
		}	
	}
	else
	{
		fOk = FALSE;
	}
			
   	DEBUGMSG(ZONE_PDD, (_T("-NLedDriverGetDeviceInfo\r\n")));
	
   	return (fOk);
}


//-----------------------------------------------------------------------------
//
// Function: NLedDriverSetDevice
//
// This routine changes the configuration of an LED.  The nInfoId parameter
// indicates what kind of configuration information is being changed.  
// Currently only the NLED_SETTINGS_INFO_ID value is supported.  The pInput
// parameter points to a buffer containing the data to be updated.  The size
// of the buffer depends on the value of nInfoId.
//
// Parameters:
//      InputParm 
//          [in] INT   	nInfoId
//          [in] PVOID 	pInput
//
// Returns:  
//      This routine returns TRUE if successful, or FALSE if there's a problem
//
//-----------------------------------------------------------------------------
BOOL WINAPI
NLedDriverSetDevice(
                   INT     nInfoId,
                   PVOID   pInput
                   )
{
   	DEBUGMSG(ZONE_PDD, (_T("+NLedDriverSetDevice\r\n")));
			
	HANDLE hThread = NULL;	
	struct NLED_SETTINGS_INFO *pNledSettingsInfo = (struct NLED_SETTINGS_INFO*)pInput;

	if(nInfoId != NLED_SETTINGS_INFO_ID)
	{
   		DEBUGMSG(ZONE_PDD, (_T("NLedDriverSetDevice: Invalid nInfoId\r\n")));	
		return FALSE;
	}
	
	if(pNledSettingsInfo->LedNum >= NLED_MAX_LED)	// Check LedNum validity
	{
   		DEBUGMSG(ZONE_PDD, (_T("NLedDriverSetDevice: Invalid LedNum\r\n")));	
		return FALSE;
	}

	if(pNledSettingsInfo->OffOnBlink > NLED_MAX_OFFONBLINK)	// Check OffOnBlink validity
	{
   		DEBUGMSG(ZONE_PDD, (_T("NLedDriverSetDevice: Invalid OffOnBlink\r\n")));
		return FALSE;
	}
	
	if(g_NLedPddParam[pNledSettingsInfo->LedNum].bNLedDriverSetDeviceThreadState)
	{
		SetEvent(g_NLedPddParam[pNledSettingsInfo->LedNum].hNLedStopThreadEvent);
		DEBUGMSG(ZONE_PDD, (_T("NLedDriverSetDevice: WaitForSingleObject - hNLedThreadStoppedEvent\r\n")));
		WaitForSingleObject(g_NLedPddParam[pNledSettingsInfo->LedNum].hNLedThreadStoppedEvent, INFINITE);
	}

	g_NLedPddParam[pNledSettingsInfo->LedNum].NLedSettingsInfo.LedNum = pNledSettingsInfo->LedNum;	
	g_NLedPddParam[pNledSettingsInfo->LedNum].NLedSettingsInfo.OffOnBlink = pNledSettingsInfo->OffOnBlink;
	
	if(pNledSettingsInfo->OffOnBlink == 0)	// OffOnBlink setting - 0 OFF
	{
		if (g_NLedPddParam[pNledSettingsInfo->LedNum].GPIOActiveState == 0)
			GPIOSetBit(g_hGPIO, g_NLedPddParam[pNledSettingsInfo->LedNum].GPIOId);
		else
			GPIOClrBit(g_hGPIO, g_NLedPddParam[pNledSettingsInfo->LedNum].GPIOId);
		g_NLedPddParam[pNledSettingsInfo->LedNum].bNledState = FALSE;			
	}	
	else if(pNledSettingsInfo->OffOnBlink == 1)  // OffOnBlink setting - 1 ON
	{	
		if (g_NLedPddParam[pNledSettingsInfo->LedNum].GPIOActiveState == 0)
			GPIOClrBit(g_hGPIO, g_NLedPddParam[pNledSettingsInfo->LedNum].GPIOId);
		else
			GPIOSetBit(g_hGPIO, g_NLedPddParam[pNledSettingsInfo->LedNum].GPIOId);
		g_NLedPddParam[pNledSettingsInfo->LedNum].bNledState = TRUE;
	}
	else // OffOnBlink setting - 2 BLINK
	{
		UINT Index = pNledSettingsInfo->LedNum;
			
		g_NLedPddParam[Index].bNLedDriverSetDeviceThreadState = TRUE;

		g_NLedPddParam[Index].NLedSettingsInfo = *(pNledSettingsInfo);

		// Round up OnTime and OffTime values 100 millisecond at least
		if(g_NLedPddParam[Index].NLedSettingsInfo.OnTime < 100000)
			g_NLedPddParam[Index].NLedSettingsInfo.OnTime = 100000;
	
		if(g_NLedPddParam[Index].NLedSettingsInfo.OffTime < 100000)
			g_NLedPddParam[Index].NLedSettingsInfo.OffTime = 100000;	

		hThread = CreateThread(
 	        		NULL,
      				0,   
					NLedDriverSetDeviceThread,
      				&g_NLedPddParam[Index].NLedSettingsInfo,           		// Start routine parameter
      				0,
      				NULL
      			);

		if (hThread == NULL)
    		{
			g_NLedPddParam[Index].bNLedDriverSetDeviceThreadState = FALSE;
			return FALSE;
    		}
		
		CloseHandle(hThread);
	}
				
   	DEBUGMSG(ZONE_PDD, (_T("-NLedDriverSetDevice\r\n")));

	return TRUE;
}
	

//-----------------------------------------------------------------------------
//
// Function: NLedDriverPowerDown
//
// This routine is invoked by the driver MDD when the system suspends or
// resumes.  The power_down flag indicates whether the system is powering 
// up or powering down.
//
// Parameters:
//      InputParm 
//          [in] BOOL power_down
//
// Returns:  
//      	None
//
//-----------------------------------------------------------------------------
VOID WINAPI
NLedDriverPowerDown(
                   BOOL power_down
                   )
{
    DEBUGMSG(ZONE_PDD, (_T("+NLedDriverPowerDown\r\n")));
	UINT Index;

	if ( power_down )
	{
		// shut off all NLEDs
		for (Index = 0; Index < NLED_MAX_LED; Index++)
		{
			if (g_NLedPddParam[Index].GPIOActiveState == 0)
				GPIOSetBit(g_hGPIO, g_NLedPddParam[Index].GPIOId);
			else
				GPIOClrBit(g_hGPIO, g_NLedPddParam[Index].GPIOId);

			g_NLedPddParam[Index].bNledState = FALSE;
		}
	}
	else
	{
		for (Index = 0; Index < NLED_MAX_LED; Index++)
		{
			// On Power Up (Resume) turn on any LEDs that should be "ON".
			//	the individual LED control threads will put the Blinking
			//	LEDs back in their proper pre suspend state while "OFF" LEDs
			//	will stay off.
			if ( g_NLedPddParam[Index].NLedSettingsInfo.OffOnBlink == 1 )
			{
				if (g_NLedPddParam[Index].GPIOActiveState == 0)
					GPIOClrBit(g_hGPIO, g_NLedPddParam[Index].GPIOId);
				else
					GPIOSetBit(g_hGPIO, g_NLedPddParam[Index].GPIOId);

				g_NLedPddParam[Index].bNledState = TRUE;
			}
		}
	}
    DEBUGMSG(ZONE_PDD, (_T("-NLedDriverPowerDown\r\n")));
}

//-----------------------------------------------------------------------------
//
// Function: NLedDriverSetDeviceThread
//
// This routine is invoked by CreateThread() function in NLedDriverSetDevice when
// caller attempts to change the configuration of a notification LED to 'Blink' mode
//
// Parameters:
//      InputParm 
//          [in] LPVOID lParam
//
// Returns:  
//      This routine returns 0
//
//-----------------------------------------------------------------------------
DWORD WINAPI NLedDriverSetDeviceThread(LPVOID lParam)
{	
   	DEBUGMSG(ZONE_PDD, (_T("+NLedDriverSetDeviceThread\r\n")));

	NLED_SETTINGS_INFO *pNledSettingsInfo = (struct NLED_SETTINGS_INFO*)lParam;
	UINT Index = pNledSettingsInfo->LedNum;
	DWORD OnTimeOut = (DWORD)(pNledSettingsInfo->OnTime/1000);		// to convert micro to milli-seconds 
	DWORD OffTimeOut = (DWORD)(pNledSettingsInfo->OffTime/1000);	// to convert micro to milli-seconds 
		
	for(;;)
	{	// on	
		if (g_NLedPddParam[Index].GPIOActiveState == 0)
			GPIOClrBit(g_hGPIO, g_NLedPddParam[Index].GPIOId);
		else
			GPIOSetBit(g_hGPIO, g_NLedPddParam[Index].GPIOId);
			
		g_NLedPddParam[Index].bNledState = TRUE;

		if(WaitForSingleObject(g_NLedPddParam[Index].hNLedStopThreadEvent, OnTimeOut) == WAIT_OBJECT_0)		
			break;

		// off
		if (g_NLedPddParam[Index].GPIOActiveState == 0)
			GPIOSetBit(g_hGPIO, g_NLedPddParam[Index].GPIOId);
		else
			GPIOClrBit(g_hGPIO, g_NLedPddParam[Index].GPIOId);

		g_NLedPddParam[Index].bNledState = FALSE;

		if(WaitForSingleObject(g_NLedPddParam[Index].hNLedStopThreadEvent, OffTimeOut) == WAIT_OBJECT_0)			
			break;

	}
	
	pNledSettingsInfo->LedNum = 0;
	pNledSettingsInfo->MetaCycleOff = 0;
	pNledSettingsInfo->MetaCycleOn = 0;
	pNledSettingsInfo->OffOnBlink = 0;
	pNledSettingsInfo->OffTime = 0;
	pNledSettingsInfo->OnTime = 0;
	pNledSettingsInfo->TotalCycleTime = 0;
	
	SetEvent(g_NLedPddParam[Index].hNLedThreadStoppedEvent);

	g_NLedPddParam[Index].bNLedDriverSetDeviceThreadState = FALSE;

   	DEBUGMSG(ZONE_PDD, (_T("-NLedDriverSetDeviceThread\r\n")));

	return 0;
}
