// All rights reserved ADENEO EMBEDDED 2010
// Copyright (c) 2007, 2008 BSQUARE Corporation. All rights reserved.

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
//  File:  oalrtc.c
//
//  This file implements OAL real time module. 
//
//
#include "omap.h"
#include "oalex.h"
#include <nkintr.h>
#include "sdk_i2c.h"
#include "s35390_rtc.h"


// I2C defines
#define RTC_I2C_DEV					OMAP_DEVICE_I2C1
#define I2C_ADDR_RTC				0x0030
#define I2C_BAUD_RTC				0

// Command defines
#define RTC_CMD_STATUS_1            (0x0)
#define RTC_CMD_STATUS_2            (0x1)
#define RTC_CMD_REAL_TIME_DATA_1    (0x2)
#define RTC_CMD_REAL_TIME_DATA_2    (0x3)
#define RTC_CMD_INT1                (0x4)
#define RTC_CMD_INT2                (0x5)
#define RTC_CMD_CLOCK_CORRECTION    (0x6)
#define RTC_CMD_FREE                (0x7)

// Bitmask defines for Status Register 1
#define RTC_RESET                   (0x80)
#define RTC_24_HOUR_FORMAT          (0x40)
#define RTC_BLD                     (0x02)
#define RTC_POC                     (0x01)

// Bitmask defines for Status Register 2
#define RTC_NO_INTERRUPTS           (0x00)
#define RTC_ALARM_INTERRUPT			(0x20)
#define RTC_TEST                    (0x01)

// Buffer offset defines for Real-Time Data 1
#define RTC_RTD1_YEAR               (0x0)
#define RTC_RTD1_MONTH              (0x1)
#define RTC_RTD1_DAY                (0x2)
#define RTC_RTD1_DAY_OF_WEEK        (0x3)
#define RTC_RTD1_HOUR               (0x4)
#define RTC_RTD1_MINUTE             (0x5)
#define RTC_RTD1_SECOND             (0x6)
#define RTC_AMPM_BIT                (1<<1)

// Buffer offset defines for Real-Time Data 2
#define RTC_RTD2_HOUR               (0x0)
#define RTC_RTD2_MINUTE             (0x1)
#define RTC_RTD2_SECOND             (0x2)

// Buffer offset defines for INT registers
#define RTC_INT_WEEK				(0x0)
#define RTC_INT_HOUR				(0x1)
#define RTC_INT_MINUTE				(0x2)

// Bitmask defines for INT registers
#define RTC_ALARM_ENABLE			(0x1)

// Bitmask defines for Clock Correction
#define RTC_NO_CLOCK_CORRECTION     (0x0)

// Base year
#define RTC_DEFAULT_BASE_YEAR		(2000)

// Max year
#define RTC_MAX_YEARS				(99)

#define SECS_IN_A_DAY              86400    // # of secs in a day



// Global variables
static void *g_hI2C = NULL;
static UINT16 g_devAddr;
static BOOL g_NeedReinit;
static UINT16 g_baseYear;


static BYTE reverse(BYTE b)
{
    int i;
    BYTE j;
    BYTE result=0;

    for (i=0x1,j=0x80;i<0x100;i<<=1,j>>=1)
    {
        if (b & i)
        {
            result |= j;
        }
    }
    return result;
}
static BYTE ReversedBCDtoBIN(BYTE bcd)
{    
    BYTE reversed = reverse(bcd);
    return (reversed >> 4) * 10 + (reversed & 0x0F);
}
static BYTE BINtoReversedBCD(BYTE binary)
{
    BYTE unit,deci;
    deci = binary / 10;
    unit = binary - (deci * 10);    
    return reverse((deci << 4) | (unit));
}

BOOL RTCI2CWrite(UINT16 subAddr, const VOID *pBuffer, UINT32 size)
{
    I2CSetSlaveAddress(g_hI2C, g_devAddr + subAddr);
    return (I2CWrite(g_hI2C, 0, pBuffer, size) == size) ? TRUE : FALSE;
}
BOOL RTCI2CRead(UINT16 subAddr, VOID *pBuffer, UINT32 size)
{
    I2CSetSlaveAddress(g_hI2C, g_devAddr + subAddr);
    return (I2CRead(g_hI2C, 0, pBuffer, size) == size) ? TRUE : FALSE;
}

//------------------------------------------------------------------------------

LPCWSTR
SystemTimeToString(
    SYSTEMTIME *pSystemTime,
    WCHAR * buffer,
    DWORD count
    )
{

    OALLogPrintf(
        buffer, count, L"%04d.%02d.%02d %02d:%02d:%02d.%03d",
        pSystemTime->wYear, pSystemTime->wMonth, pSystemTime->wDay,
        pSystemTime->wHour, pSystemTime->wMinute, pSystemTime->wSecond, 
        pSystemTime->wMilliseconds
        );     
    return buffer;
}

BOOL RTC_GetTime(LPSYSTEMTIME time)
{
    UCHAR receiveBuffer[7];
    WCHAR buffer1[64];
   
#ifdef SHIP_BUILD    
        UNREFERENCED_PARAMETER(buffer1);
#endif  
   
    OALMSG(OAL_TIMER && OAL_FUNC, (L"+OEMGetRealTime()\r\n"));

	if (g_hI2C == NULL)
		return FALSE;

    if (RTCI2CRead(RTC_CMD_REAL_TIME_DATA_1, receiveBuffer, 7) == FALSE)
    {
        OALMSG(OAL_ERROR, (L"RTC_GetTime(): Failed to read the date/time from the RTC.\r\n"));
		return FALSE;
	}

	time->wYear = ReversedBCDtoBIN(receiveBuffer[RTC_RTD1_YEAR]) + g_baseYear;
	time->wMonth = ReversedBCDtoBIN(receiveBuffer[RTC_RTD1_MONTH]);
	time->wDay = ReversedBCDtoBIN(receiveBuffer[RTC_RTD1_DAY]);
	time->wHour = ReversedBCDtoBIN(receiveBuffer[RTC_RTD1_HOUR] & ~(RTC_AMPM_BIT));
	time->wMinute = ReversedBCDtoBIN(receiveBuffer[RTC_RTD1_MINUTE]);
	time->wSecond = ReversedBCDtoBIN(receiveBuffer[RTC_RTD1_SECOND]);
	time->wDayOfWeek = ReversedBCDtoBIN(receiveBuffer[RTC_RTD1_DAY_OF_WEEK]);
	time->wMilliseconds = 0;

    
    OALMSG(OAL_TIMER && OAL_FUNC, (L"-OEMGetRealTime() = %s\r\n", SystemTimeToString(time,buffer1,64)));

	return TRUE;
}

BOOL RTC_SetTime(LPSYSTEMTIME time)
{
    UCHAR cmdBuffer[7];
    WCHAR buffer1[64];

#ifdef SHIP_BUILD    
        UNREFERENCED_PARAMETER(buffer1);        
#endif  
    
    OALMSG(OAL_TIMER && OAL_FUNC, (L"+OEMSetRealTime(%s)\r\n", SystemTimeToString(time,buffer1,64)));

	if((time->wYear < g_baseYear) || (time->wYear > g_baseYear + RTC_MAX_YEARS))
	{
		// warning
		OALMSG(OAL_WARN, (TEXT("RTC_SetTime(): Due to hardware limitation, setting a year out of the %u-%u range may cause unexpected behavior after reset.\r\n")
							,RTC_DEFAULT_BASE_YEAR,RTC_DEFAULT_BASE_YEAR+RTC_MAX_YEARS));
		g_baseYear = time->wYear;
	}

	if (g_hI2C == NULL)
		return FALSE;

	cmdBuffer[RTC_RTD1_YEAR] = BINtoReversedBCD((UINT8)(time->wYear - g_baseYear));
	cmdBuffer[RTC_RTD1_MONTH] = BINtoReversedBCD((UINT8)time->wMonth);
	cmdBuffer[RTC_RTD1_DAY] = BINtoReversedBCD((UINT8)time->wDay);
	cmdBuffer[RTC_RTD1_HOUR] = BINtoReversedBCD((UINT8)time->wHour);
	cmdBuffer[RTC_RTD1_MINUTE] = BINtoReversedBCD((UINT8)time->wMinute);
	cmdBuffer[RTC_RTD1_SECOND] = BINtoReversedBCD((UINT8)time->wSecond);
	cmdBuffer[RTC_RTD1_DAY_OF_WEEK] = BINtoReversedBCD((UINT8)time->wDayOfWeek);

	if (RTCI2CWrite(RTC_CMD_REAL_TIME_DATA_1,cmdBuffer,7)==FALSE)
	{
		OALMSG(OAL_ERROR, (L"RTC_SetTime(): Failed to write the date/time to the RTC.\r\n"));
		return FALSE;
	}
    
    OALMSG(OAL_TIMER && OAL_FUNC, (L"-OEMSetRealTime\r\n"));

	return TRUE;
}

BOOL OALS35390RTCInit(OMAP_DEVICE i2cdev,DWORD i2cBaudIndex,UINT16 slaveAddress,BOOL fEnableAsWakeUpSource)
{    
    UCHAR cmd;
    UCHAR status;

    g_NeedReinit = FALSE;

	g_baseYear = RTC_DEFAULT_BASE_YEAR;

    // Open I2C instance
    g_hI2C = I2COpen(i2cdev);
    if (g_hI2C == NULL)
    {
        return FALSE;
    }
    g_devAddr = slaveAddress;
    // Set baud rate
    I2CSetBaudIndex(g_hI2C, i2cBaudIndex);	
    // Set sub address mode
	I2CSetSubAddressMode(g_hI2C, I2C_SUBADDRESS_MODE_0);

    // Get the BLD and POC flags that will tell if the circuit muste be reinitialized    
	// Read status register 1	
	if (RTCI2CRead(RTC_CMD_STATUS_1, &status, 1) == FALSE)
    {
        OALMSG(OAL_ERROR, (L"OALIoCtlHalInitRTC(): Failed to read status register 1 after reset.\r\n"));
        return FALSE;
    }
    if (status & (RTC_POC | RTC_BLD))
    {
        g_NeedReinit = TRUE;
        OALMSG(OAL_INFO | OAL_WARN, (L"OALIoCtlHalInitRTC():RTC need a reinit because either RTC had been powered off aor the RTC has dropped below the allowed level (status %x).\r\n",status));
    }

    if (g_NeedReinit)
    {
        // Reset the RTC
        cmd = RTC_RESET;
	    if (RTCI2CWrite(RTC_CMD_STATUS_1, &cmd, 1) == FALSE)
	    {
		    OALMSG(OAL_ERROR, (L"OALIoCtlHalInitRTC(): Failed to reset RTC.\r\n"));
		    return FALSE;
	    }

        // Set 24 hour time format
        if (RTCI2CRead(RTC_CMD_STATUS_1,&status,1) == FALSE)
        {
            OALMSG(OAL_ERROR, (L"OALIoCtlHalInitRTC(): Failed to set 24 hour time format.\r\n"));
            return FALSE;
        }
        status |= RTC_24_HOUR_FORMAT;
        if (RTCI2CWrite(RTC_CMD_STATUS_1,&status,1) == FALSE)
        {
            OALMSG(OAL_ERROR, (L"OALIoCtlHalInitRTC(): Failed to set 24 hour time format.\r\n"));
            return FALSE;
        }
        // Disable RTC interrupts
        status = RTC_NO_INTERRUPTS;
        if (RTCI2CWrite(RTC_CMD_STATUS_2,&status,1) == FALSE)
        {
            OALMSG(OAL_ERROR, (L"OALIoCtlHalInitRTC(): Failed to disable RTC interrupts.\r\n"));
            return FALSE;
        }
        // Turn off clock correction        
        cmd = RTC_NO_CLOCK_CORRECTION;
        if (RTCI2CWrite(RTC_CMD_CLOCK_CORRECTION,&cmd,1) == FALSE)
        {
            OALMSG(OAL_ERROR, (L"OALIoCtlHalInitRTC(): Failed to turn off clock correction.\r\n"));
            return FALSE;
        }
    }
    OALIntrStaticTranslate(SYSINTR_RTC_ALARM, BSPGetRTCGpioIrq());
    OEMInterruptEnable(SYSINTR_RTC_ALARM,NULL,0);
    
    if (fEnableAsWakeUpSource)
    {
        DWORD dwSysintr = SYSINTR_RTC_ALARM;
        OALIoCtlHalEnableWake(0,&dwSysintr,sizeof(dwSysintr),NULL,0,NULL);
    }

    return TRUE;
}
//------------------------------------------------------------------------------
//
//  Function:  OALIoCtlHalInitRTC
//
//  This function is called by WinCE OS to initialize the time after boot. 
//  Input buffer contains SYSTEMTIME structure with default time value.
//
//
BOOL
OALIoCtlHalInitRTC(
    UINT32 code, 
    VOID *pInBuffer, 
    UINT32 inSize, 
    VOID *pOutBuffer, 
    UINT32 outSize, 
    UINT32 *pOutSize
    )
{
	SYSTEMTIME *pGivenTime = (LPSYSTEMTIME)pInBuffer;

	UNREFERENCED_PARAMETER(code);
	UNREFERENCED_PARAMETER(inSize);
	UNREFERENCED_PARAMETER(pOutBuffer);
	UNREFERENCED_PARAMETER(outSize);
	UNREFERENCED_PARAMETER(pOutSize);

	RETAILMSG(1, (L"Initializing RTC\r\n"));

	// Initialize the time if needed
	if (g_NeedReinit && pGivenTime != NULL)
	{
        g_NeedReinit = FALSE;
		RTC_SetTime(pGivenTime);
	}

	return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  OEMGetRealTime
//
//  This function is called by the kernel to retrieve the time from
//  the real-time clock.
//
BOOL
OEMGetRealTime(
    SYSTEMTIME *pSystemTime
    ) 
{
    if (g_NeedReinit)
    {
        return FALSE;
    }
	return RTC_GetTime(pSystemTime);
}

//------------------------------------------------------------------------------
//
//  Function:  OEMSetRealTime
//
//  This function is called by the kernel to set the real-time clock. A secure
//  timer requirement means that the time change is noted in baseOffset and
//  used to compute the time delta from the non-alterable RTC in T2
//
BOOL
OEMSetRealTime(
    SYSTEMTIME *pSystemTime
    ) 
{
    BOOL fResult = RTC_SetTime(pSystemTime);

    if (fResult && g_NeedReinit)
    {
        g_NeedReinit = FALSE;
    }
	return fResult;
}

//------------------------------------------------------------------------------
//
//  Function:  OEMSetAlarmTime
//
//  This function is called by the kernel to set the real-time clock alarm.
//
BOOL
OEMSetAlarmTime(
    SYSTEMTIME *pSystemTime
    ) 
{
	UCHAR cmdBuffer[3];
	UCHAR status;
    LONGLONG alarmFiletime;
    SYSTEMTIME getSystemTime;
    LONGLONG getFiletime;
    WCHAR buffer1[64], buffer2[64];
    LONGLONG diffTime = 0;
    
#ifdef SHIP_BUILD    
    UNREFERENCED_PARAMETER(buffer1);
    UNREFERENCED_PARAMETER(buffer2);
#endif  
    
    OALMSG(OAL_TIMER && OAL_FUNC, (L"+OEMSetAlarmTime(%s)\r\n", SystemTimeToString(pSystemTime,buffer1,64)));

    if (g_NeedReinit)
    {
        return FALSE;
    }

	if (g_hI2C == NULL)
	{
		return FALSE;
	}

    NKSystemTimeToFileTime(pSystemTime,(FILETIME *)&alarmFiletime);
    OEMGetRealTime(&getSystemTime);
    NKSystemTimeToFileTime(&getSystemTime,(FILETIME *)&getFiletime);
    if (alarmFiletime<getFiletime) {
        OALMSG(1,(L"OEMSetAlarmTime: alarmtime (%s) is behind current time (%s)\r\n",SystemTimeToString(pSystemTime,buffer1,64),SystemTimeToString(&getSystemTime,buffer2,64)));
        return FALSE;
    }
    diffTime = (alarmFiletime-getFiletime)/10000; // in ms
    diffTime = diffTime/1000; // in secs
    diffTime = diffTime/SECS_IN_A_DAY; //in days
    if (diffTime>7) {
        OALMSG(1,(L"OEMSetAlarmTime: alarmtime (%s) is more than 1 week ahead of current time (%s). This RTC chip doesnt support this.\r\n",SystemTimeToString(pSystemTime,buffer1,64),SystemTimeToString(&getSystemTime,buffer2,64)));
        return FALSE;
    }
    

	// Deactivate RTC alarm interrupt first
    status = RTC_NO_INTERRUPTS;
    if (RTCI2CWrite(RTC_CMD_STATUS_2,&status,1) == FALSE)
    {
        OALMSG(OAL_ERROR, (L"OEMSetAlarmTime(): Failed to enable RTC alarm interrupt.\r\n"));
        return FALSE;
    }
    if (RTCI2CRead(RTC_CMD_STATUS_1,&status,1) == FALSE)
    {
        OALMSG(OAL_ERROR, (L"OEMSetAlarmTime(): Failed to set 24 hour time format.\r\n"));
        return FALSE;
    }

	// Activate RTC alarm interrupt first (for access to the INT1 alarm registers)
    status = RTC_ALARM_INTERRUPT;
    if (RTCI2CWrite(RTC_CMD_STATUS_2,&status,1) == FALSE)
    {
        OALMSG(OAL_ERROR, (L"OEMSetAlarmTime(): Failed to enable RTC alarm interrupt.\r\n"));
        return FALSE;
    }

	// Set alarm
	cmdBuffer[RTC_INT_WEEK] = RTC_ALARM_ENABLE | BINtoReversedBCD((UINT8)pSystemTime->wDayOfWeek);
    cmdBuffer[RTC_INT_HOUR] = RTC_ALARM_ENABLE | BINtoReversedBCD((UINT8)pSystemTime->wHour) | ((pSystemTime->wHour>=12) ? (1<<1) : 0);
	cmdBuffer[RTC_INT_MINUTE] = RTC_ALARM_ENABLE | BINtoReversedBCD((UINT8)pSystemTime->wMinute);

	if (RTCI2CWrite(RTC_CMD_INT1,cmdBuffer,3)==FALSE)
	{
		OALMSG(OAL_ERROR, (L"OEMSetAlarmTime(): Failed to write the date/time to the RTC.\r\n"));
		return FALSE;
	}

    OEMInterruptDone(SYSINTR_RTC_ALARM);

	return TRUE;
}
