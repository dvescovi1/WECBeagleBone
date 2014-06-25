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
//  File: prcm_priv.h
//
#ifndef _PRCM_PRIV_H_
#define _PRCM_PRIV_H_

#include "am33x_clocks.h"
#include "am33x_oal_prcm.h"
#include "am33x_config.h"

#pragma warning (push)
#pragma warning (disable:4200)

//-----------------------------------------------------------------------------
//#define MAX_IDLESTATUS_LOOP         (0x1000)
//#define cm_offset(field)            ((LONG)&(((OMAP_CM_REGS*)0)->field))
//#define prm_offset(field)           ((LONG)&(((OMAP_PRM_REGS*)0)->field))
//#define AUTOIDLE_DECL(a, x, y, z)   ClockOffsetInfo_3 a = {x, y, z}
//#define IDLESTAT_DECL(a, x, y)      ClockOffsetInfo_1 a = {x, y}
//#define ICLK_DECL(a, x, y, z)       ClockOffsetInfo_2 a = {x, y, z, FALSE}
//#define ICLK_VDECL(a, x, y, z)       ClockOffsetInfo_2 a = {x, y, z, TRUE}
//#define FCLK_DECL(a, x, y, z)       ClockOffsetInfo_2 a = {x, y, z, FALSE}
//#define FCLK_VDECL(a, x, y, z)       ClockOffsetInfo_2 a = {x, y, z, TRUE}
//#define WKEN_DECL(a, x, y)          ClockOffsetInfo_1 a = {x, y}

//-----------------------------------------------------------------------------
// common device structures

typedef struct {
    UINT                    mask;
    UINT                    offset;
} ClockOffsetInfo_1;

typedef struct {
    LONG                    refCount;    
    UINT                    mask;
    UINT                    offset;
    BOOL                    bVirtual;
} ClockOffsetInfo_2;

typedef struct {
    DWORD                   size;
    SourceClock_e           rgSourceClocks[5];    
} SourceDeviceClocks_t;

typedef struct {
    PowerDomain_e           powerDomain;
    SourceDeviceClocks_t    fclk;
    SourceDeviceClocks_t    iclk;
	LONG			        refCount;	 // reference count for device;
    LONG			        CLKCTRL_REG; // offset to the register on the AM33X_PRCM_REGS struct
    UINT                    idlestMask;
    UINT                    stbyMask;    
} DeviceLookupEntry;

typedef struct {
	PowerDomain_e	powerDomain;
	LONG			refCount;	// reference count for domain
	BOOL			ro;			// TRUE if register is RO 
	LONG			CLKSTCTRL_REG;// offset to the register on the AM33X_PRCM_REGS struct
} ClockDomainLookupEntry;


typedef struct {
    LONG                    refCount;
    UINT                    ffValidationMask;    
    UINT                    powerState;
    UINT                    logicState;
    LONG                    PWRSTCTRL_REG;
} PowerDomainInfo_t; 

typedef PowerDomainInfo_t   DomainMap[POWERDOMAIN_COUNT];


//------------------------------------------------------------------------------
// Voltage domain definition
typedef int                 VddRefCountTable[kVDD_COUNT];
//------------------------------------------------------------------------------
// DPLL domain definition
typedef struct {
    UINT                    dpllType; //ADPLLS or ADPLLJ    
    UINT                    lowPowerEnabled; //clkmode->dpll_lpmode_en;
    UINT                    rampOnRelock; //clkmode->DPLL_RELOCK_RAMP_EN
    UINT                    rampTime;//clkmode->dpll_ramp_rate
    UINT                    rampLevel;//clkmode->dpll_ramp_level
    UINT                    driftGuard;//clkmode->pll_driftguard_en
    UINT                    dpllMode; //clkmode->dpll_en             // off, bypass, locked
    UINT                    bypassSelect;//clksel->dpll_byp_clksel
    UINT                    multiplier;//clksel->dpll_mult
    UINT                    divisor;//clksel->dpll_div
    UINT                    dpllAutoidleState;//autoidle->auto_dpll_mode
    LONG			        CLKMODE_REG; // offset to the register on the AM33X_PRCM_REGS struct
    LONG			        CLKSEL_REG; // offset to the register on the AM33X_PRCM_REGS struct
    LONG			        AUTOIDLE_REG; // offset to the register on the AM33X_PRCM_REGS struct
} DpllState_t;

typedef struct {
    int                     vddDomain;
    LONG                    refCount;
    DpllState_t             *pDpllInfo; 
} Dpll_Entry_t;

typedef Dpll_Entry_t        DpllMap[kDPLL_COUNT];
//------------------------------------------------------------------------------
// Root clk src definition
//------------------------------------------------------------------------------
// DPLL clk out definition
typedef struct {
    UINT                    clkOutDivType;
    UINT                    powerDownEn;
    UINT                    autoGateEn;
    UINT                    divisor;
    LONG			        CLKOUT_REG; // offset to the register on the AM33X_PRCM_REGS struct
} DpllClockOutState_t;

typedef struct {
    int                     dpllDomain;
    LONG                    refCount;
    DpllClockOutState_t     *pDpllClkOutInfo;
} DpllClkOut_Entry_t;

typedef DpllClkOut_Entry_t  DpllClkOutMap[kDPLL_CLKOUT_COUNT];
typedef enum { NONE, SEL_PRCM, DIV_PRCM} SelRegs_T;

//------------------------------------------------------------------------------
typedef struct {
    UINT                    id;
    UINT                    divisor;
} SrcClockDivisor_Entry_t;

typedef struct {
    UINT                    count;
    SrcClockDivisor_Entry_t SourceClock[];
} SrcClockDivisorTable_t;

typedef struct {
    int                 thisClk;
    int                 parentClk;
    LONG                refCount;
    BOOL                bIsDpllSrcClk;
    ClockDomain_e       clockDomain;
    SrcClockDivisorTable_t  *pDivisors;
    UINT                clkActMask;
} SrcClock_Entry_t;
typedef SrcClock_Entry_t    SrcClockMap[kSOURCE_CLOCK_COUNT];

//------------------------------------------------------------------------------
//  Reference to all PRCM-registers. Initialized in OALPowerInit
extern AM33X_PRCM_REGS    *g_pPrcmRegs;
extern AM33X_SECnFUSE_KEY_REGS *g_pSecnFuseRegs;
extern AM33X_SUPPL_DEVICE_CTRL_REGS *g_pSupplDeviceCtrlRegs;
//------------------------------------------------------------------------------
//
//  Global:  g_PrcmPostInit
//
//  Indicates if prcm library has been fully initialized. 
//  Initialized in OALPowerPostInit
//
extern BOOL                 g_PrcmPostInit;

//------------------------------------------------------------------------------
//
//  Global:  g_PrcmMutex
//
//  Contains a list of CRITICAL_SECTIONS used for synchronized access to 
//  PRCM registers
//
typedef enum
{
    Mutex_DeviceClock = 0,
    Mutex_Clock,
    Mutex_Domain,
    Mutex_Voltage,
    Mutex_Power,
    Mutex_Intr,
    Mutex_Count
} PRCMMutex_Id;

extern CRITICAL_SECTION     g_rgPrcmMutex[Mutex_Count];

//------------------------------------------------------------------------------
//
//  Global:  g_bSingleThreaded
//
//  Indicated that the OAL PRCM functions are being called while the kernel is 
//  single threaded (OEMIdle, OEMPoweroff)
//
extern BOOL g_bSingleThreaded;

//------------------------------------------------------------------------------
// internal/helper routines

BOOL ClockInitialize();
BOOL ClockUpdateParentClock( int srcClkId, BOOL bEnable );
BOOL ClockEnableClkDomain(ClockDomain_e clkDomain, BOOL bEnable);
BOOL DomainInitialize();
BOOL ResetInitialize();
BOOL DeviceInitialize();
BOOL DomainGetDeviceContextState(UINT powerDomain, ClockOffsetInfo_2 *pInfo, BOOL bSet );
BOOL PrcmClockEnableClockDomain(UINT clockId,BOOL bEnable);


//------------------------------------------------------------------------------
// synchronization routines
__inline void Lock(PRCMMutex_Id mutexId)
{
    if (g_PrcmPostInit && !g_bSingleThreaded) 
        EnterCriticalSection(&g_rgPrcmMutex[mutexId]);
}

__inline void Unlock(PRCMMutex_Id mutexId)
{
    if (g_PrcmPostInit && !g_bSingleThreaded)
        LeaveCriticalSection(&g_rgPrcmMutex[mutexId]);
}

#pragma warning (pop)
//------------------------------------------------------------------------------
#endif // _PRCM_PRIV_H_
