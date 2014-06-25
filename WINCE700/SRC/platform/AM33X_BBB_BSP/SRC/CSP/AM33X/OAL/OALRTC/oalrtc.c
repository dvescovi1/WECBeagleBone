// All rights reserved Texas Instruments, Inc. 2011
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
//#include "am33x.h"
#pragma warning(push)
#pragma warning(disable: 4115 4201 4214)
#include <windows.h>
#include <oal.h>
#pragma warning(pop)

#include "oalex.h"
#include <nkintr.h>

typedef volatile struct
{
	REG32	SECONDS_REG;		// 0h Seconds Register
	REG32	MINUTES_REG;		// 4h  Minutes Register
	REG32	HOURS_REG;			// 8h  Hours Register
	REG32	DAYS_REG;			// Ch  Day of the Month Register
	REG32	MONTHS_REG;			// 10h  Month Register
	REG32	YEARS_REG;			// 14h  Year Register
	REG32	WEEK_REG;			// 18h  Day of the Week Register
	UINT32	res_1c;
	REG32	ALARM_SECONDS_REG;	// 20h  Alarm Seconds Register
	REG32	ALARM_MINUTES_REG;	// 24h  Alarm Minutes Register
	REG32	ALARM_HOURS_REG;	// 28h  Alarm Hours Register
	REG32	ALARM_DAYS_REG;		// 2Ch  Alarm Days Register
	REG32	ALARM_MONTHS_REG;	// 30h  Alarm Months Register
	REG32	ALARM_YEARS_REG;	// 34h  Alarm Years Register
	UINT32	res_38_3c[2];
	REG32	RTC_CTRL_REG;		// 40h  Control Register
	REG32	RTC_STATUS_REG;		// 44h  Status Register
	REG32	RTC_INTERRUPTS_REG;	// 48h  Interrupt Enable Register
	REG32	RTC_COMP_LSB_REG;	// 4Ch  Compensation (LSB) Register
	REG32	RTC_COMP_MSB_REG;	// 50h  Compensation (MSB) Register
	REG32	RTC_OSC_REG;		// 54h  Oscillator Register
	UINT32	res_58_5c[2];
	REG32	RTC_SCRATCH0_REG;	// 60h  Scratch 0 Register (General-Purpose)
	REG32	RTC_SCRATCH1_REG;	// 64h  Scratch 1 Register (General-Purpose)
	REG32	RTC_SCRATCH2_REG;	// 68h  Scratch 2 Register (General-Purpose)
	REG32	KICK0R;				// 6Ch  Kick 0 Register (Write Protect)
	REG32	KICK1R;				// 70h  Kick 1 Register (Write Protect)
	REG32	RTC_REVISION;		// 74h  Revision Register
	REG32	RTC_SYSCONFIG;		// 78h  System Configuration Register
	REG32	RTC_IRQWAKEEN_0;	// 7Ch  Wakeup Enable Register
} AM3XX_RTC_REGS;

#define RTC_K_UNLOCK0 0x83E70B13
#define RTC_K_UNLOCK1 0x95A4F1E0

#define RTC_RUNNING   0x1 
#define BASE_YEAR     2000 
#define RTC_MAX_YEARS 99
#define RTC_STATUS_BUSY 0x1


static AM3XX_RTC_REGS * rtc_regs = NULL;

static UINT8 BCDtoBIN(UINT8 bt)
{
	return (bt & 0xf) + (((bt & 0xf0) >> 4)* 10);	
}

static UINT8 BINtoBCD(UINT8 bt)
{
	return (((bt/10) << 4) | (bt % 10));
}

BOOL RTC_GetTime(LPSYSTEMTIME time)
{
	// read the seconds first; that updates all hidden registers that will be actually read after that
	time->wSecond       = BCDtoBIN((UINT8)(rtc_regs->SECONDS_REG & 0x7F));
	time->wYear         = BCDtoBIN((UINT8)(rtc_regs->YEARS_REG)) + BASE_YEAR;
	time->wMonth        = BCDtoBIN((UINT8)(rtc_regs->MONTHS_REG  & 0x1F));
	time->wDay          = BCDtoBIN((UINT8)(rtc_regs->DAYS_REG    & 0x3F));
	time->wHour         = BCDtoBIN((UINT8)(rtc_regs->HOURS_REG   & 0x3F));
	time->wMinute       = BCDtoBIN((UINT8)(rtc_regs->MINUTES_REG & 0x7F));
	time->wDayOfWeek    = BCDtoBIN((UINT8)(rtc_regs->WEEK_REG    & 0x7F));
	time->wMilliseconds = 0;

	return TRUE;
}

BOOL RTC_SetTime(LPSYSTEMTIME time)
{
	BOOL fWasEnabled;

	if((time->wYear < BASE_YEAR) || (time->wYear > BASE_YEAR + RTC_MAX_YEARS)){
		OALMSG(OAL_WARN,(TEXT("RTC_SetTime(): HW RTC supports setting a year in the %u-%u range\r\n")
							,BASE_YEAR,BASE_YEAR+RTC_MAX_YEARS));
		return FALSE;
	}

	//OALMSG(1,(L"+++RTC_SetTime %02d/%02d/%04d %d:%d:%d\r\n",
	//		time->wMonth, time->wDay, time->wYear, 	 
	//		time->wHour, time->wMinute, time->wSecond ));

	while (rtc_regs->RTC_STATUS_REG & RTC_STATUS_BUSY );
	fWasEnabled = INTERRUPTS_ENABLE (FALSE);

	rtc_regs->KICK0R = RTC_K_UNLOCK0;
	rtc_regs->KICK1R = RTC_K_UNLOCK1;

	rtc_regs->SECONDS_REG = (UINT32)BINtoBCD((UINT8)time->wSecond);
	rtc_regs->YEARS_REG   = (UINT32)BINtoBCD((UINT8)(time->wYear - BASE_YEAR));         
	rtc_regs->MONTHS_REG  = (UINT32)BINtoBCD((UINT8)time->wMonth);        
	rtc_regs->DAYS_REG    = (UINT32)BINtoBCD((UINT8)time->wDay);          
	rtc_regs->HOURS_REG   = (UINT32)BINtoBCD((UINT8)time->wHour);         
	rtc_regs->MINUTES_REG = (UINT32)BINtoBCD((UINT8)time->wMinute);       
	rtc_regs->WEEK_REG    = (UINT32)BINtoBCD((UINT8)time->wDayOfWeek);    

	//OALMSG(1,(L"---RTC_SetTime %02X/%02X/%02X %02X:%02X:%02X\r\n",
	//	rtc_regs->MONTHS_REG,rtc_regs->DAYS_REG, rtc_regs->YEARS_REG,
	//	rtc_regs->HOURS_REG, rtc_regs->MINUTES_REG, rtc_regs->SECONDS_REG));


	rtc_regs->KICK0R = 0;

	INTERRUPTS_ENABLE (fWasEnabled);

	return TRUE;
}

BOOL RTC_SetAlarmTime(LPSYSTEMTIME time)
{
	BOOL fWasEnabled;

	if((time->wYear < BASE_YEAR) || (time->wYear > BASE_YEAR + RTC_MAX_YEARS)){
		OALMSG(OAL_WARN,(TEXT("RTC_SetTime(): HW RTC supports setting a year in the %u-%u range\r\n")
							,BASE_YEAR,BASE_YEAR+RTC_MAX_YEARS));
		return FALSE;
	}

	//OALMSG(1,(L"+++RTC_SetAlarmTime %02d/%02d/%04d %d:%d:%d\r\n",
    //			time->wMonth, time->wDay, time->wYear, 	 
    //			time->wHour, time->wMinute, time->wSecond ));

	while (rtc_regs->RTC_STATUS_REG & RTC_STATUS_BUSY );
	fWasEnabled = INTERRUPTS_ENABLE (FALSE);

	rtc_regs->KICK0R = RTC_K_UNLOCK0;
	rtc_regs->KICK1R = RTC_K_UNLOCK1;

	rtc_regs->RTC_INTERRUPTS_REG = 0x0; // disable all interrupts

	rtc_regs->ALARM_SECONDS_REG = (UINT32)BINtoBCD((UINT8)time->wSecond);
	rtc_regs->ALARM_YEARS_REG   = (UINT32)BINtoBCD((UINT8)(time->wYear - BASE_YEAR));         
	rtc_regs->ALARM_MONTHS_REG  = (UINT32)BINtoBCD((UINT8)time->wMonth);        
	rtc_regs->ALARM_DAYS_REG    = (UINT32)BINtoBCD((UINT8)time->wDay);          
	rtc_regs->ALARM_HOURS_REG   = (UINT32)BINtoBCD((UINT8)time->wHour);         
	rtc_regs->ALARM_MINUTES_REG = (UINT32)BINtoBCD((UINT8)time->wMinute);       

	//OALMSG(1,(L"---RTC_SetAlarmTime %02X/%02X/%02X %02X:%02X:%02X\r\n",
	//	rtc_regs->ALARM_MONTHS_REG,rtc_regs->ALARM_DAYS_REG, rtc_regs->ALARM_YEARS_REG,
	//	rtc_regs->ALARM_HOURS_REG, rtc_regs->ALARM_MINUTES_REG, rtc_regs->ALARM_SECONDS_REG));

	rtc_regs->RTC_STATUS_REG     = 1 << 6; // clear pending interrupt
	rtc_regs->RTC_INTERRUPTS_REG = 1 << 3; // enable alarm interrupt

	rtc_regs->KICK0R = 0;

	INTERRUPTS_ENABLE (fWasEnabled);

	return TRUE;
}


BOOL OAL3XX_RTCInit(UINT32 rtc_base_pa, DWORD rtc_irq)
{    
    DWORD dwSysintr = SYSINTR_RTC_ALARM;

	rtc_regs = OALPAtoUA(rtc_base_pa /*AM33X_RTCSS_REGS_PA*/);
	//OALMSG(1,(L"OAL3XX_RTCInit rtc_regs 0x%08x\r\n", rtc_regs));

	rtc_regs->KICK0R = RTC_K_UNLOCK0;
	rtc_regs->KICK1R = RTC_K_UNLOCK1;
	rtc_regs->RTC_OSC_REG |= (1 << 6);
	rtc_regs->RTC_INTERRUPTS_REG = 0; // disable all interrupts for now
	rtc_regs->RTC_CTRL_REG       = RTC_RUNNING; 
	rtc_regs->KICK0R = 0;
    rtc_regs->RTC_SYSCONFIG = 0x3;
    rtc_regs->RTC_IRQWAKEEN_0 =  0x2;
    

	OALIntrStaticTranslate(SYSINTR_RTC_ALARM, rtc_irq /*IRQ_RTCALARM*/);
    OEMInterruptEnable(SYSINTR_RTC_ALARM,NULL,0);
    
    OALIoCtlHalEnableWake(0,&dwSysintr,sizeof(dwSysintr),NULL,0,NULL);

    return TRUE;
}

//------------------------------------------------------------------------------
BOOL OALIoCtlHalInitRTC( UINT32 code, 
    VOID *pInBuffer, UINT32 inSize, VOID *pOutBuffer, UINT32 outSize, UINT32 *pOutSize)
//  This function is called by WinCE OS to initialize the time after boot. 
//  Input buffer contains SYSTEMTIME structure with default time value.
{
	SYSTEMTIME *pGivenTime = (LPSYSTEMTIME)pInBuffer;

	UNREFERENCED_PARAMETER(code);
	UNREFERENCED_PARAMETER(inSize);
	UNREFERENCED_PARAMETER(pOutBuffer);
	UNREFERENCED_PARAMETER(outSize);
	UNREFERENCED_PARAMETER(pOutSize);

	//RETAILMSG(1, (L"Initializing RTC\r\n"));

	if (pGivenTime != NULL)	{
		RTC_SetTime(pGivenTime);
	}

	return TRUE;
}

//------------------------------------------------------------------------------
BOOL OEMGetRealTime(SYSTEMTIME *pSystemTime) 
//  This function is called by the kernel to retrieve the time from
//  the real-time clock.
{
	return RTC_GetTime(pSystemTime);
}

//------------------------------------------------------------------------------
BOOL OEMSetRealTime( SYSTEMTIME *pSystemTime ) 
//  This function is called by the kernel to set the real-time clock. A secure
//  timer requirement means that the time change is noted in baseOffset and
//  used to compute the time delta from the non-alterable RTC in T2
{
    BOOL fResult = RTC_SetTime(pSystemTime);

	return fResult;
}

//------------------------------------------------------------------------------
BOOL OEMSetAlarmTime(SYSTEMTIME *pSystemTime ) 
//  This function is called by the kernel to set the real-time clock alarm.
{
	BOOL fResult = RTC_SetAlarmTime(pSystemTime);

    OEMInterruptDone(SYSINTR_RTC_ALARM);

	return fResult;
}
