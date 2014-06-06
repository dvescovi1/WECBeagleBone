// All rights reserved ADENEO EMBEDDED 2010
/*
===============================================================================
*             Texas Instruments OMAP(TM) Platform Software
* (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
*
* Use of this software is controlled by the terms and conditions found
* in the license agreement under which this software has been supplied.
*
===============================================================================
*/
//
//  File:  omap_clocks.h
//
#ifndef __OMAP_CLOCKS_H
#define __OMAP_CLOCKS_H

#ifdef __cplusplus
extern "C" {
#endif

#pragma warning(push)
#pragma warning(disable: 4201) //allow namelesss struct/union

//------------------------------------------------------------------------------

#define OMAP_CLOCK_NONE			((OMAP_CLOCK)-1)
#define MAX_IDLE_WAIT_LOOP		10000

//------------------------------------------------------------------------------

typedef enum {  
	NONE = 0,
	WAKEUP,
	CORE,
	PERIPHERAL,
	USBHOST,
	EMU,
	MPU,
	DSS,
	NEON,
	SGX,
	GLOBAL,
	OCP_SYSTEM,
	CLOCK_CONTROL

} OMAP_CLOCK_DOMAIN;

//------------------------------------------------------------------------------

#define	MAX_CLK_NUM					2

#define reg_offset(type, field)		((LONG)&(((type*)0)->field))
#define cm_offset(field)            reg_offset(OMAP_CM_REGS, field)
#define pm_offset(field)            reg_offset(OMAP_PRM_REGS, field)
#define AUTOIDLE_REG(r, x, y)		static const PRCM_REG r = {x, y}
#define	WAKEUP_REG(r, x, y)			static const PRCM_REG r = {x, y}
#define	MPU_WAKEUP_REG(r, x, y)		static const PRCM_REG r = {x, y}

typedef struct {
    union	{
		DWORD               size;
		DWORD				extClkFreq;
	};
	OMAP_CLOCK			clocks[MAX_CLK_NUM];

} CLOCK_ARRAY;

typedef struct {
	DWORD				domain;
	DWORD				mask;
	DWORD				offset;

} CLOCK_DIV_REG;

typedef struct {
	DWORD						fixedDivider;
	const CLOCK_DIV_REG*		divider;
	const CLOCK_DIV_REG*		multiplier;
	const CLOCK_DIV_REG*		extraDivider;

} CLOCK_DIV;

typedef struct {
    DWORD               mask;
    DWORD               offset;

} PRCM_REG;

typedef struct {
	DWORD				domain;
    DWORD               mask;
    DWORD               offset;
	DWORD				value[MAX_CLK_NUM];
} PRCM_REG_CLK_SEL;

typedef struct {
    DWORD               mask;
    DWORD               offset;
	LONG				refCount;
	OMAP_CLOCK			srcClock;

} PRCM_REG_ICLK;

typedef struct {
    DWORD				mask;
    DWORD				offset;
	LONG				refCount;

} PRCM_REG_FCLK;

typedef struct {
    DWORD				size;
	PRCM_REG_FCLK		clock[MAX_CLK_NUM];
	CLOCK_ARRAY*		pSrcClocks;

} FCLK_ARRAY;

//------------------------------------------------------------------------------

#define OMAP_IDLE_MODE_MASK		0xF
#define	OMAP_FORCE_IDLE_FLAG	(1 << 0)
#define	OMAP_NO_IDLE_FLAG		(1 << 1)
#define	OMAP_SMART_IDLE_FLAG	(1 << 2)
#define	OMAP_NO_AUTO_FLAG		(1 << 3)

#define	OMAP_SIDLE_MASK			(3 << 3)
#define	OMAP_FORCE_IDLE_MASK	(0 << 3)
#define	OMAP_NO_IDLE_MASK		(1 << 3)
#define	OMAP_SMART_IDLE_MASK	(2 << 3)

#define SYSCONFIG_REG(r, x, y)		static const SYSCONFIG_REG_OFFSET r = {x, y}

typedef struct {
    DWORD               offset;
	DWORD				supportedModes;

} SYSCONFIG_REG_OFFSET;

//------------------------------------------------------------------------------

typedef struct {    
	DWORD						DeviceID;
	const PRCM_REG*				AutoIdleReg;
	const SYSCONFIG_REG_OFFSET*	SysConfReg;
	const PRCM_REG*				WakeUpReg;
	const PRCM_REG*				MPUWakeUpReg;

} DEVICE_CLOCK_IDLE_CONFIG;

//------------------------------------------------------------------------------

#define ICLKEN_REG(r, x, y, z)	static PRCM_REG_ICLK	r = {x, y, 0, z}
#define IDLEST_REG(r, x, y)		static const PRCM_REG	r = {x, y}

typedef struct {    
	OMAP_DEVICE				DeviceID;
	FCLK_ARRAY*				FClkReg;
	PRCM_REG_ICLK*			IClkReg;
	const PRCM_REG*			IdleStReg;

} DEVICE_CLOCK_CONFIG;

//------------------------------------------------------------------------------

typedef struct {    
	OMAP_CLOCK				ClockID;
	CLOCK_ARRAY*			pSrcClocks;
	const PRCM_REG_CLK_SEL*	ClkSelReg;
	CLOCK_DIV*				srcDivMul;

} CLOCK_SOURCE_CONFIG;

//------------------------------------------------------------------------------

typedef struct {    
	DWORD					DeviceID;
	DWORD					ClockDomain;

} DEVICE_CLOCK_DOMAIN_CONFIG;

//------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#pragma warning(pop)

#endif

