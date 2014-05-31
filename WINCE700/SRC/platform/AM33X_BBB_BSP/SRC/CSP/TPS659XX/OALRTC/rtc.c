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
//  File:  rtc.c
//
//  This file implements OAL real time module. 
//
//  Implementation uses system tick to calculate realtime clock. Tritons
//  chip driver should call IOCTL_HAL_INIT_RTC periodically to update
//  internal RTC time with real one.
//
//
#include "omap.h"
#include "bsp_cfg.h"
#include "ceddkex.h"
#include "oalex.h"
#include <oal_clock.h>
#include "soc_cfg.h"
#include "omap_gpio_regs.h"
#include "sdk_gpio.h"
#include "gpio_ioctls.h"
#ifdef OAL
#include "oal_alloc.h"
#endif
#include <nkintr.h>

#include "twl.h"
#include "tps659xx.h"
#include "tps659xx_internals.h"
//------------------------------------------------------------------------------
//
//  Define:  RTC_BASE_YEAR
//
//  Delta from which RTC counts years
//  Resolution of RTC years is from 2000 to 2099
//
#define RTC_BASE_YEAR_MIN       2000    // must be divisible by 4
#define RTC_BASE_YEAR_MAX       2099


//------------------------------------------------------------------------------

#define BCD2BIN(b)              (((b) >> 4)*10 + ((b)&0xF))
#define BIN2BCD(b)              ((((UINT8)(b)/10) << 4)|((UINT8)(b)%10))

//------------------------------------------------------------------------------

static struct {

    BOOL       initialized;     // Is RTC subsystem intialized?
    
    CRITICAL_SECTION cs;

    void*      hTWL;            // Triton driver
    
    ULONGLONG  baseFiletime;    // Base filetime 
    ULONGLONG  baseOffset;      // Secure time offset from base filetime
    DWORD      baseTickCount;   // Tick count from base filetime

    ULONGLONG  alarmFiletime;   // Alarm filetime 

} s_rtc = { FALSE };

    
//------------------------------------------------------------------------------

UINT32
OEMGetTickCount(
    );

BOOL
FiletimeToHWTime(
    ULONGLONG fileTime, 
    UCHAR bcdTime[6]
    );

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


//------------------------------------------------------------------------------

LPCWSTR
HWTimeToString(
    UCHAR bcdTime[6]
    )
{
    static WCHAR buffer[64];

    OALLogPrintf(
        buffer, 64, L"%04d.%02d.%02d %02d:%02d:%02d",
        BCD2BIN(bcdTime[5]) + RTC_BASE_YEAR_MIN, 
        BCD2BIN(bcdTime[4]), 
        BCD2BIN(bcdTime[3]),
        BCD2BIN(bcdTime[2]), 
        BCD2BIN(bcdTime[1]), 
        BCD2BIN(bcdTime[0])
        );        
    return buffer;
}

//------------------------------------------------------------------------------

VOID
ReadBaseOffset(
    ULONGLONG   *pOffset
    )
{
    UCHAR   val;

    // Read backup registers for secure time offset
    *pOffset = 0;

    TWLReadByteReg(s_rtc.hTWL, TWL_BACKUP_REG_H, &val);
    *pOffset = (*pOffset << 8) | val;

    TWLReadByteReg(s_rtc.hTWL, TWL_BACKUP_REG_G, &val);
    *pOffset = (*pOffset << 8) | val;

    TWLReadByteReg(s_rtc.hTWL, TWL_BACKUP_REG_F, &val);
    *pOffset = (*pOffset << 8) | val;

    TWLReadByteReg(s_rtc.hTWL, TWL_BACKUP_REG_E, &val);
    *pOffset = (*pOffset << 8) | val;

    TWLReadByteReg(s_rtc.hTWL, TWL_BACKUP_REG_D, &val);
    *pOffset = (*pOffset << 8) | val;

    TWLReadByteReg(s_rtc.hTWL, TWL_BACKUP_REG_C, &val);
    *pOffset = (*pOffset << 8) | val;

    TWLReadByteReg(s_rtc.hTWL, TWL_BACKUP_REG_B, &val);
    *pOffset = (*pOffset << 8) | val;

    TWLReadByteReg(s_rtc.hTWL, TWL_BACKUP_REG_A, &val);
    *pOffset = (*pOffset << 8) | val;
}

//------------------------------------------------------------------------------

VOID
WriteBaseOffset(
    ULONGLONG   *pOffset
    )
{
    UCHAR   val;

    // Write backup registers with secure time offset
    val = (UCHAR)(*pOffset >> 0);
    TWLWriteByteReg(s_rtc.hTWL, TWL_BACKUP_REG_A, val);

    val = (UCHAR)(*pOffset >> 8);
    TWLWriteByteReg(s_rtc.hTWL, TWL_BACKUP_REG_B, val);

    val = (UCHAR)(*pOffset >> 16);
    TWLWriteByteReg(s_rtc.hTWL, TWL_BACKUP_REG_C, val);

    val = (UCHAR)(*pOffset >> 24);
    TWLWriteByteReg(s_rtc.hTWL, TWL_BACKUP_REG_D, val);

    val = (UCHAR)(*pOffset >> 32);
    TWLWriteByteReg(s_rtc.hTWL, TWL_BACKUP_REG_E, val);

    val = (UCHAR)(*pOffset >> 40);
    TWLWriteByteReg(s_rtc.hTWL, TWL_BACKUP_REG_F, val);

    val = (UCHAR)(*pOffset >> 48);
    TWLWriteByteReg(s_rtc.hTWL, TWL_BACKUP_REG_G, val);

    val = (UCHAR)(*pOffset >> 56);
    TWLWriteByteReg(s_rtc.hTWL, TWL_BACKUP_REG_H, val);
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
    BOOL            rc = FALSE;
    SYSTEMTIME      *pGivenTime = (LPSYSTEMTIME) pInBuffer;
    UCHAR           bcdTime[6];
    UCHAR           status;
    UCHAR           secure;

    UNREFERENCED_PARAMETER(pOutSize);
    UNREFERENCED_PARAMETER(outSize);
    UNREFERENCED_PARAMETER(pOutBuffer);
    UNREFERENCED_PARAMETER(inSize);
    UNREFERENCED_PARAMETER(code);

    OALMSG(OAL_TIMER && OAL_FUNC, (L"+OALIoCtlHalInitRTC()\r\n"));

   
    // Initialize RTC critical section
    InitializeCriticalSection(&s_rtc.cs);

    // Set CPU GPIO_64 (T2 MSECURE) to be output/high (unsecure)
    // This allows write access to the T2 RTC calendar/time registers
    // OMAP35XX GP only
    if( dwOEMHighSecurity == OEM_HIGH_SECURITY_GP )
    {
        BSPSetT2MSECURE(TRUE);
    }

    // First read RTC status from Triton 
    s_rtc.hTWL = TWLOpen();
    if (s_rtc.hTWL == NULL)
        {
        OALMSG(OAL_ERROR, (L" OALIoCtlHalInitRTC(): Failed to open Triton\r\n"));
        goto cleanUp;
        }

    // Read secure registers for secure hash
    status = 0;

    TWLReadByteReg(s_rtc.hTWL, TWL_SECURED_REG_A, &secure);
    status |= secure;

    TWLReadByteReg(s_rtc.hTWL, TWL_SECURED_REG_B, &secure);
    status |= secure;

    TWLReadByteReg(s_rtc.hTWL, TWL_SECURED_REG_C, &secure);
    status |= secure;

    TWLReadByteReg(s_rtc.hTWL, TWL_SECURED_REG_D, &secure);
    status |= secure;


    OALMSG(OAL_TIMER && OAL_FUNC, (L" OALIoCtlHalInitRTC():  RTC TWL_SECURED_REG_= 0x%x\r\n", status));

#if 0
    // Not needed for CE embedded, only need to reset RTC if TWL/TPS PMIC is reset
    // Check for a clean boot of device - if so, reset date/time to system default (LTK2026)
    pColdBoot = OALArgsQuery(OAL_ARGS_QUERY_COLDBOOT);
    if ((pColdBoot != NULL) && *pColdBoot)
        {
        OALMSG(OAL_TIMER && OAL_FUNC, (L" OALIoCtlHalInitRTC():  Clean boot, reset date time\r\n"));
        status = 0;
        }
#endif
		
    // Start RTC when it isn't running
    if (status == 0 && pGivenTime != NULL)
        {
        OALMSG(OAL_TIMER && OAL_FUNC, (L" OALIoCtlHalInitRTC():  Resetting RTC\r\n"));

        // Write power_up and alarm bits to clear power up flag (and any interrupt flag)
        TWLWriteByteReg(s_rtc.hTWL, TWL_RTC_STATUS_REG, TWL_RTC_STATUS_POWER_UP|TWL_RTC_STATUS_ALARM);

        //  Convert system time to BCD
        bcdTime[5] = BIN2BCD(pGivenTime->wYear - RTC_BASE_YEAR_MIN);
        bcdTime[4] = BIN2BCD(pGivenTime->wMonth);
        bcdTime[3] = BIN2BCD(pGivenTime->wDay);
        bcdTime[2] = BIN2BCD(pGivenTime->wHour);
        bcdTime[1] = BIN2BCD(pGivenTime->wMinute);
        bcdTime[0] = BIN2BCD(pGivenTime->wSecond);

        //  Initialize RTC with given values
        TWLWriteByteReg(s_rtc.hTWL, TWL_YEARS_REG, bcdTime[5]);
        TWLWriteByteReg(s_rtc.hTWL, TWL_MONTHS_REG, bcdTime[4]);
        TWLWriteByteReg(s_rtc.hTWL, TWL_DAYS_REG, bcdTime[3]);
        TWLWriteByteReg(s_rtc.hTWL, TWL_HOURS_REG, bcdTime[2]);
        TWLWriteByteReg(s_rtc.hTWL, TWL_MINUTES_REG, bcdTime[1]);
        TWLWriteByteReg(s_rtc.hTWL, TWL_SECONDS_REG, bcdTime[0]);

        //  Enable RTC
        TWLWriteByteReg(s_rtc.hTWL, TWL_RTC_CTRL_REG, TWL_RTC_CTRL_RUN);

        //  Write fake hash to secure regs
        TWLWriteByteReg(s_rtc.hTWL, TWL_SECURED_REG_A, 0xAA);
        TWLWriteByteReg(s_rtc.hTWL, TWL_SECURED_REG_B, 0xBB);
        TWLWriteByteReg(s_rtc.hTWL, TWL_SECURED_REG_C, 0xCC);
        TWLWriteByteReg(s_rtc.hTWL, TWL_SECURED_REG_D, 0xDD);

        //  Convert given time initialization date/time to FILETIME
        NKSystemTimeToFileTime(pGivenTime, (FILETIME*)&s_rtc.baseFiletime);

        //  Set a default value for base offset
        s_rtc.baseOffset = 0;

        //  Save off base offset to the backup regs
        WriteBaseOffset( &s_rtc.baseOffset ); 
        }
    else
        {
        SYSTEMTIME  baseSystemTime;

        OALMSG(OAL_TIMER && OAL_FUNC, (L" OALIoCtlHalInitRTC():  Getting RTC\r\n"));

        //  Set get time flag            
        TWLReadByteReg(s_rtc.hTWL, TWL_RTC_CTRL_REG, &status);

        status |= TWL_RTC_CTRL_RUN | TWL_RTC_CTRL_GET_TIME;
        TWLWriteByteReg(s_rtc.hTWL, TWL_RTC_CTRL_REG, status);

        //  Get date and time from RTC
        TWLReadByteReg(s_rtc.hTWL, TWL_YEARS_REG, &bcdTime[5]);
        TWLReadByteReg(s_rtc.hTWL, TWL_MONTHS_REG, &bcdTime[4]);
        TWLReadByteReg(s_rtc.hTWL, TWL_DAYS_REG, &bcdTime[3]);
        TWLReadByteReg(s_rtc.hTWL, TWL_HOURS_REG, &bcdTime[2]);
        TWLReadByteReg(s_rtc.hTWL, TWL_MINUTES_REG, &bcdTime[1]);
        TWLReadByteReg(s_rtc.hTWL, TWL_SECONDS_REG, &bcdTime[0]);

        //  Convert current RTC date/time to FILETIME
        baseSystemTime.wYear    = BCD2BIN(bcdTime[5]) + RTC_BASE_YEAR_MIN;
        baseSystemTime.wMonth   = BCD2BIN(bcdTime[4]);
        baseSystemTime.wDay     = BCD2BIN(bcdTime[3]);
        baseSystemTime.wHour    = BCD2BIN(bcdTime[2]);
        baseSystemTime.wMinute  = BCD2BIN(bcdTime[1]);
        baseSystemTime.wSecond  = BCD2BIN(bcdTime[0]);
        baseSystemTime.wMilliseconds = 0;

        NKSystemTimeToFileTime(&baseSystemTime, (FILETIME*)&s_rtc.baseFiletime);

        //  Read the offset from the backup regs
        ReadBaseOffset( &s_rtc.baseOffset ); 
        }        


    OALMSG(OAL_TIMER && OAL_FUNC, (L" OALIoCtlHalInitRTC():  RTC = %s\r\n", HWTimeToString(bcdTime)));


    // Now update RTC state values
    s_rtc.initialized   = TRUE;
    s_rtc.baseTickCount = OEMGetTickCount();


    //  Success
    rc = TRUE;


cleanUp:
    OALMSG(OAL_TIMER && OAL_FUNC, (L"-OALIoCtlHalInitRTC() rc = %d\r\n", rc));
    return rc;
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
    DWORD       delta;
    ULONGLONG   time;
    WCHAR buffer1[64];
    
#ifdef SHIP_BUILD    
    UNREFERENCED_PARAMETER(buffer1);        
#endif  
    

    OALMSG(OAL_TIMER && OAL_FUNC, (L"+OEMGetRealTime()\r\n"));

    if (!s_rtc.initialized)
        {
        // Return default time if RTC isn't initialized
        pSystemTime->wYear   = RTC_BASE_YEAR_MIN;
        pSystemTime->wMonth  = 1;
        pSystemTime->wDay    = 1;
        pSystemTime->wHour   = 0;
        pSystemTime->wMinute = 0;
        pSystemTime->wSecond = 0;
        pSystemTime->wDayOfWeek    = 0;
        pSystemTime->wMilliseconds = 0;
        }
    else
        {
        EnterCriticalSection(&s_rtc.cs);
        if (g_ResumeRTC)
    		{
            // suspend/resume occured, sync RTC
            OALIoCtlHalRtcTime(0, NULL, 0, NULL, 0, NULL);
            g_ResumeRTC = FALSE;
	    	}
        delta = OEMGetTickCount() - s_rtc.baseTickCount;
        time = s_rtc.baseFiletime + s_rtc.baseOffset + ((ULONGLONG)delta) * 10000;
        NKFileTimeToSystemTime((FILETIME*)&time, pSystemTime);
        pSystemTime->wMilliseconds = 0;
        LeaveCriticalSection(&s_rtc.cs);
        }

    OALMSG(OAL_TIMER && OAL_FUNC, (L"-OEMGetRealTime() = %s\r\n", SystemTimeToString(pSystemTime,buffer1,64)));

    return TRUE;
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
    BOOL        rc = FALSE;
    ULONGLONG   fileTime;
    DWORD       tickDelta;
    WCHAR buffer1[64];
    
#ifdef SHIP_BUILD    
    UNREFERENCED_PARAMETER(buffer1);        
#endif     

    OALMSG(OAL_TIMER && OAL_FUNC, (L"+OEMSetRealTime(%s)\r\n", SystemTimeToString(pSystemTime,buffer1,64)));

    if (s_rtc.initialized)
        {
        // Save time to global structure
        EnterCriticalSection(&s_rtc.cs);

        if (g_ResumeRTC)
    		{
            OALIoCtlHalRtcTime(0, NULL, 0, NULL, 0, NULL);
            g_ResumeRTC = FALSE;
	    	}
		
        // Round to seconds
        pSystemTime->wMilliseconds = 0;

        // Convert to filetime
        if (NKSystemTimeToFileTime(pSystemTime, (FILETIME*)&fileTime))
            {
            // Compute the tick delta (indicates the time in the RTC)
            tickDelta = OEMGetTickCount() - s_rtc.baseTickCount;
            
            // Update all the parameters
            s_rtc.baseFiletime  = s_rtc.baseFiletime + ((ULONGLONG)tickDelta)*10000;
            s_rtc.baseOffset    = fileTime - s_rtc.baseFiletime;
            s_rtc.baseTickCount = OEMGetTickCount();

            //  Save off base offset to the backup regs
            WriteBaseOffset( &s_rtc.baseOffset ); 

            // Done
            rc = TRUE;
            }

        LeaveCriticalSection(&s_rtc.cs);
        }
    
    OALMSG(OAL_TIMER && OAL_FUNC, (L"-OEMSetRealTime\r\n"));

    return rc;
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
    BOOL rc = FALSE;
    WCHAR buffer1[64];
    
#ifdef SHIP_BUILD    
    UNREFERENCED_PARAMETER(buffer1);        
#endif      

    OALMSG(OAL_TIMER && OAL_FUNC, (L"+OEMSetAlarmTime(%s)\r\n", SystemTimeToString(pSystemTime,buffer1,64)));

    if (s_rtc.initialized)
        {
        // Save time to global structure
        EnterCriticalSection(&s_rtc.cs);

        if (g_ResumeRTC)
    		{
            OALIoCtlHalRtcTime(0, NULL, 0, NULL, 0, NULL);
            g_ResumeRTC = FALSE;
	    	}

        // Round to seconds
        pSystemTime->wMilliseconds = 0;

        // Convert to filetime
        if (NKSystemTimeToFileTime(pSystemTime, (FILETIME*)&s_rtc.alarmFiletime))
            {
            UCHAR   status;
            UCHAR   bcdTime[6];

            //  Adjust alarm time by secure offset
            s_rtc.alarmFiletime  = s_rtc.alarmFiletime - s_rtc.baseOffset;

            //  Convert to BCD time format
            FiletimeToHWTime( s_rtc.alarmFiletime, bcdTime );

            //  Write alarm registers
            TWLWriteByteReg(s_rtc.hTWL, TWL_ALARM_YEARS_REG, bcdTime[5]);
            TWLWriteByteReg(s_rtc.hTWL, TWL_ALARM_MONTHS_REG, bcdTime[4]);
            TWLWriteByteReg(s_rtc.hTWL, TWL_ALARM_DAYS_REG, bcdTime[3]);
            TWLWriteByteReg(s_rtc.hTWL, TWL_ALARM_HOURS_REG, bcdTime[2]);
            TWLWriteByteReg(s_rtc.hTWL, TWL_ALARM_MINUTES_REG, bcdTime[1]);
            TWLWriteByteReg(s_rtc.hTWL, TWL_ALARM_SECONDS_REG, bcdTime[0]);

            //  Set toggle bit to latch alarm registers
            TWLReadByteReg(s_rtc.hTWL, TWL_RTC_CTRL_REG, &status);

            status |= TWL_RTC_CTRL_RUN | TWL_RTC_CTRL_GET_TIME;
            TWLWriteByteReg(s_rtc.hTWL, TWL_RTC_CTRL_REG, status);

            // Done
            rc = TRUE;
            }

        LeaveCriticalSection(&s_rtc.cs);
        }
    
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  OALIoCtlHalRtcTime
//
//  This function is called by RTC driver when time event interrupt
//  occurs.
//
BOOL
OALIoCtlHalRtcTime(
    UINT32 code, 
    VOID *pInBuffer, 
    UINT32 inSize, 
    VOID *pOutBuffer, 
    UINT32 outSize, 
    UINT32 *pOutSize
    )
{
    SYSTEMTIME  baseSystemTime;
    UCHAR       status;
    UCHAR       bcdTime[6];

    UNREFERENCED_PARAMETER(pOutSize);
    UNREFERENCED_PARAMETER(outSize);
    UNREFERENCED_PARAMETER(pOutBuffer);
    UNREFERENCED_PARAMETER(inSize);
    UNREFERENCED_PARAMETER(pInBuffer);
    UNREFERENCED_PARAMETER(code);

    OALMSG(OAL_TIMER && OAL_FUNC, (L"+OALIoCtlHalRtcTime()\r\n"));

    //  The RTC in Triton2 is set to periodically sync with the kernel time
    //  to ensure there is no clock drift.  When a sync event is triggered,
    //  the T2 RTC is used to set the base time in the kernel.

    EnterCriticalSection(&s_rtc.cs);

    //  Set get time flag            
    TWLReadByteReg(s_rtc.hTWL, TWL_RTC_CTRL_REG, &status);

    status |= TWL_RTC_CTRL_RUN | TWL_RTC_CTRL_GET_TIME;
    TWLWriteByteReg(s_rtc.hTWL, TWL_RTC_CTRL_REG, status);

    //  Get date and time from RTC
    TWLReadByteReg(s_rtc.hTWL, TWL_YEARS_REG, &bcdTime[5]);
    TWLReadByteReg(s_rtc.hTWL, TWL_MONTHS_REG, &bcdTime[4]);
    TWLReadByteReg(s_rtc.hTWL, TWL_DAYS_REG, &bcdTime[3]);
    TWLReadByteReg(s_rtc.hTWL, TWL_HOURS_REG, &bcdTime[2]);
    TWLReadByteReg(s_rtc.hTWL, TWL_MINUTES_REG, &bcdTime[1]);
    TWLReadByteReg(s_rtc.hTWL, TWL_SECONDS_REG, &bcdTime[0]);

    //  Convert current RTC date/time to FILETIME
    baseSystemTime.wYear    = BCD2BIN(bcdTime[5]) + RTC_BASE_YEAR_MIN;
    baseSystemTime.wMonth   = BCD2BIN(bcdTime[4]);
    baseSystemTime.wDay     = BCD2BIN(bcdTime[3]);
    baseSystemTime.wHour    = BCD2BIN(bcdTime[2]);
    baseSystemTime.wMinute  = BCD2BIN(bcdTime[1]);
    baseSystemTime.wSecond  = BCD2BIN(bcdTime[0]);
    baseSystemTime.wMilliseconds = 0;

    //  Update the base filetime to match RTC
    NKSystemTimeToFileTime(&baseSystemTime, (FILETIME*)&s_rtc.baseFiletime);

    //  Reset the tick count
    s_rtc.baseTickCount = OEMGetTickCount();
    
    LeaveCriticalSection(&s_rtc.cs);

    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  OALIoCtlHalRtcAlarm
//
//  This function is called by RTC driver when alarm interrupt
//  occurs.
//
BOOL
OALIoCtlHalRtcAlarm(
    UINT32 code, 
    VOID *pInBuffer, 
    UINT32 inSize, 
    VOID *pOutBuffer, 
    UINT32 outSize, 
    UINT32 *pOutSize
    )
{
    OALMSG(OAL_TIMER && OAL_FUNC, (L"+OALIoCtlHalRtcAlarm()\r\n"));

    UNREFERENCED_PARAMETER(pOutSize);
    UNREFERENCED_PARAMETER(outSize);
    UNREFERENCED_PARAMETER(pOutBuffer);
    UNREFERENCED_PARAMETER(inSize);
    UNREFERENCED_PARAMETER(pInBuffer);
    UNREFERENCED_PARAMETER(code);

    //  Alarm has been triggered by RTC driver.
    NKSetInterruptEvent(SYSINTR_RTC_ALARM);
    return TRUE;
}

//------------------------------------------------------------------------------

BOOL
FiletimeToHWTime(
    ULONGLONG fileTime, 
    UCHAR bcdTime[6]
    )
{
    SYSTEMTIME systemTime;

    //  Convert filetime to RTC HW time format
    NKFileTimeToSystemTime((FILETIME*)&fileTime, &systemTime);

    //  Limit RTC year range
    if( systemTime.wYear < RTC_BASE_YEAR_MIN )
        systemTime.wYear = RTC_BASE_YEAR_MIN;

    if( systemTime.wYear > RTC_BASE_YEAR_MAX )
        systemTime.wYear = RTC_BASE_YEAR_MAX;

    bcdTime[5] = BIN2BCD(systemTime.wYear - RTC_BASE_YEAR_MIN);
    bcdTime[4] = BIN2BCD(systemTime.wMonth);
    bcdTime[3] = BIN2BCD(systemTime.wDay);
    bcdTime[2] = BIN2BCD(systemTime.wHour);
    bcdTime[1] = BIN2BCD(systemTime.wMinute);
    bcdTime[0] = BIN2BCD(systemTime.wSecond);
        
    return TRUE;
}

//------------------------------------------------------------------------------

