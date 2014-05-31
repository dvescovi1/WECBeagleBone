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
//  File: tps65023.c
//
//

#include "omap.h"
#include "triton.h"
#include "twl.h"
#include "tps65023.h"

typedef enum {
    VERSION,
	PGOODZ,
	MASK,
	REG_CTRL,
	CON_CTRL,
	CON_CTRL2,
	DEFCORE,
	DEFSLEW,
	LDO_CTRL
    } TPS65023_REGISTERS;

#define PGOODZ_LDO1		(1<<1)
#define PGOODZ_LDO2		(1<<2)
#define PGOODZ_DCDC3	(1<<3)
#define PGOODZ_DCDC2	(1<<4)
#define PGOODZ_DCDC1	(1<<5)
#define PGOODZ_LOWBATT	(1<<6)
#define PGOODZ_PWRFAIL	(1<<7)

#define REG_CTRL_LDO1	 (1<<1)
#define REG_CTRL_LDO2	 (1<<2)
#define REG_CTRL_DCDC3   (1<<3)
#define REG_CTRL_DCDC2   (1<<4)
#define REG_CTRL_DCDC1   (1<<5)

#define CON_CTRL2_GO     (1<<7)


#define	VOLTAGE_FAILURE	0
#define	VOLTAGE_OK		1

#define VDCDC1_MIN_VOLTAGE	800
#define VDCDC1_MAX_VOLTAGE	1600
#define VDCDC1_VOLTAGE_STEP 25

#define VLDO2_MIN_VOLTAGE	1050
#define VLDO2_MAX_VOLTAGE	3300

#define VLDO1_MIN_VOLTAGE	1000
#define VLDO1_MAX_VOLTAGE	3150

UINT16 LDO2_table[]={
    1050,
    1200,
    1300,
    1800,
    2500,
    2800,
    3000,
    3300,
    0xFFFF
};
UINT16 LDO1_table[]={
    1000,
    1100,
    1300,
    1800,
    2200,
    2600,
    2800,
    3150,
    0xFFFF
};

static HANDLE g_hTwl = NULL;

BOOL ValidateHandle()
{
	if ( g_hTwl == NULL)
	{
		g_hTwl = TWLOpen();
	}
	return (g_hTwl != NULL);
}

static BYTE FindVoltageIndex(UINT16* table,UINT mv)
{
    BYTE i=0;    
    while (table[i] != 0xFFFF)    
    {
        if (mv < table[i])
        {            
            return i;            
        }
        i++;
    }
    return i-1;
}
//------------------------------------------------------------------------------
//
//  Function:  
//
BOOL TWLSetVoltage(UINT voltage,UINT32 mv)
{
	BYTE Buffer;
	ValidateHandle();
	
	switch(voltage)
	{
	case VLDO1:
		if( mv < VLDO1_MIN_VOLTAGE || mv > VLDO1_MAX_VOLTAGE)
			return FALSE;
		Buffer = FindVoltageIndex(LDO1_table,mv);
		TWLSetValue(g_hTwl, LDO_CTRL, Buffer, 0x7);
		break;

	case VLDO2:
		if( mv < VLDO2_MIN_VOLTAGE || mv > VLDO2_MAX_VOLTAGE)
			return FALSE;
		Buffer = FindVoltageIndex(LDO2_table,mv);		
		TWLSetValue(g_hTwl, LDO_CTRL, Buffer << 4, 0x7 << 4);
		break;

	case VDCDC1:
		if( mv < VDCDC1_MIN_VOLTAGE || mv > VDCDC1_MAX_VOLTAGE)
			return FALSE;
        
        if (mv > 1575)  //special case (0x1F that should be 1575mv is actually 1600mv)
            mv = 1575;

		Buffer = (BYTE) ((mv - VDCDC1_MIN_VOLTAGE)/VDCDC1_VOLTAGE_STEP);        
		TWLWriteRegs(g_hTwl, DEFCORE, &Buffer, 1);
        TWLSetValue(g_hTwl, CON_CTRL2, CON_CTRL2_GO, CON_CTRL2_GO);
        
		break;

	case VDCDC2:
		return FALSE;

	case VDCDC3:
		return FALSE;

	default :
		return FALSE;
	}
	return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  
//
BOOL TWLGetVoltage(UINT voltage,UINT32* pmv)
{
	BYTE Buffer;
	ValidateHandle();

	switch(voltage)
	{
	case VLDO1:
		TWLReadRegs(g_hTwl, LDO_CTRL, &Buffer, 1);		
		*pmv = LDO1_table[Buffer & 0x7];
		break;

	case VLDO2:
		TWLReadRegs(g_hTwl, LDO_CTRL, &Buffer, 1);
		*pmv = LDO2_table[(Buffer >> 4) & 0x7];
		break;

	case VDCDC1:
		TWLReadRegs(g_hTwl, DEFCORE, &Buffer, 1);        
		*pmv = (Buffer * VDCDC1_VOLTAGE_STEP) + VDCDC1_MIN_VOLTAGE;
        if (*pmv == 1575) //special case
            *pmv = 1600; 
		break;

	case VDCDC2:
		return FALSE;
		break;

	case VDCDC3:
		return FALSE;
		break;

	default :
		return FALSE;
	}
	return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  
//
BOOL TWLGetVoltageStatus(UINT voltage, UINT32 *status)
{
	BYTE Buffer;
	*status = VOLTAGE_OK;
	ValidateHandle();
	if(!TWLReadRegs(g_hTwl, PGOODZ, &Buffer, 1))
		return FALSE;
	switch(voltage)
	{
	case VLDO1:
		if(Buffer & PGOODZ_LDO1)
			*status = VOLTAGE_FAILURE;
		break;

	case VLDO2:
		if(Buffer & PGOODZ_LDO2)
			*status = VOLTAGE_FAILURE;
		break;

	case VDCDC1:
		if(Buffer & PGOODZ_DCDC1)
			*status = VOLTAGE_FAILURE;
		break;

	case VDCDC2:
		if(Buffer & PGOODZ_DCDC2)
			*status = VOLTAGE_FAILURE;
		break;

	case VDCDC3:
		if(Buffer & PGOODZ_DCDC1)
			*status = VOLTAGE_FAILURE;
		break;

	case LOWBATT:
		if(Buffer & PGOODZ_LOWBATT)
			*status = VOLTAGE_FAILURE;
		break;

	case PWRFAIL:
		if(Buffer & PGOODZ_PWRFAIL)
			*status = VOLTAGE_FAILURE;
		break;

	default :
		return FALSE;
	}
	return TRUE;
}
//------------------------------------------------------------------------------
//
//  Function:  
//
BOOL TWLSetVoltageSlewRate(UINT voltage,UINT32 rate)
{
    if (voltage != VDCDC1)
    {
        return FALSE;
    }
    UNREFERENCED_PARAMETER(rate);
    return FALSE;
}
//------------------------------------------------------------------------------
//
//  Function:  
//
BOOL TWLGetVoltageSlewRate(UINT voltage,UINT32* rate)
{
    if (voltage != VDCDC1)
    {
        return FALSE;
    }
    UNREFERENCED_PARAMETER(rate);
    return FALSE;
}
//------------------------------------------------------------------------------
//
//  Function:  
//
BOOL TWLEnableVoltage(UINT voltage, BOOL fOn)
{
    BYTE mask = 0;
    switch (voltage)
    {
    case VDCDC1:
        mask = REG_CTRL_DCDC1;
        break;
    case VDCDC2:
        mask = REG_CTRL_DCDC2;
        break;
    case VDCDC3:
        mask = REG_CTRL_DCDC3;
        break;
    case VLDO1:
        mask = REG_CTRL_LDO1;
        break;
    case VLDO2:
        mask = REG_CTRL_LDO2;
        break;
    default: return FALSE;
    }
    return TWLSetValue(g_hTwl, REG_CTRL, fOn ? mask : 0, mask);
}


//------------------------------------------------------------------------------
//
//  Function:  TWLReadIDCode
//
//   Return the ID code of tthe triton
//
UINT8 TWLReadIDCode(void* hTwl)
{    
    UINT8 version;
    if (TWLReadRegs(hTwl, VERSION, &version, sizeof(version)))
    {
        return version;
    }
    else
    {
        return (UINT8) -1;
    }
}