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
//  File: prcm_domain.c
//
#include "omap.h"
#include "omap_prof.h"
#include "am33x.h"
#include <nkintr.h>
#include <pkfuncs.h>
#include "oalex.h"
#include "prcm_priv.h"

//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
//DomainMap s_DomainTable = {};
DomainMap s_DomainTable = {
    {//    POWERDOMAIN_WKUP,
        0,
        DOMAIN_UPDATE_LOGICSTATE,
        0,
        LOGICRETSTATE_WKUP_PER_LOGICRET_DOMAINRET,
        PRCM_OFS(PM_WKUP_PWRSTCTRL)
    },    
    {   //POWERDOMAIN_PER,
        0,
        DOMAIN_UPDATE_ALL,
        POWERSTATE_ON >> POWERSTATE_SHIFT,
        LOGICRETSTATE_WKUP_PER_LOGICRET_DOMAINRET,
        PRCM_OFS(PM_PER_PWRSTCTRL)
    },
    {   //POWERDOMAIN_GFX,        
        0,
        DOMAIN_UPDATE_ALL,
        POWERSTATE_ON >> POWERSTATE_SHIFT,
        LOGICRETSTATE_OTHER_LOGICRET_DOMAINRET,
        PRCM_OFS(PM_GFX_PWRSTCTRL)
    },
    {   //POWERDOMAIN_MPU,
        0,
        DOMAIN_UPDATE_ALL,
        POWERSTATE_ON >> POWERSTATE_SHIFT,
        LOGICRETSTATE_OTHER_LOGICRET_DOMAINRET,
        PRCM_OFS(PM_MPU_PWRSTCTRL)
    },
    {   //POWERDOMAIN_RTC,    
        0,
        DOMAIN_UPDATE_LOGICSTATE,
        0,
        LOGICRETSTATE_OTHER_LOGICRET_DOMAINRET,
        PRCM_OFS(PM_RTC_PWRSTCTRL)
    },
    {   //POWERDOMAIN_CEFUSE,
        0,
        DOMAIN_UPDATE_POWERSTATE,
        POWERSTATE_ON >> POWERSTATE_SHIFT,
        0,
        PRCM_OFS(PM_CEFUSE_PWRSTCTRL)
    },
    {   //POWERDOMAIN_EFUSE,
        0,
        0,
        0,
        0,
    }        
};
        


//-----------------------------------------------------------------------------
static
BOOL
_PrcmDomainHwUpdate(
    UINT powerDomain,
    UINT ffMask
    )
{
    BOOL rc = TRUE;
    UINT pm_pwstctrl;
    volatile unsigned int *PM_PWRSTCTRL_REG;    
    
    
    // PM_PWSTCTRL_xxx
    if (ffMask & DOMAIN_UPDATE_POWERSTATE)
    {
        PM_PWRSTCTRL_REG = (volatile unsigned int*)((UCHAR*)g_pPrcmRegs +
    		                          s_DomainTable[powerDomain].PWRSTCTRL_REG);            
        pm_pwstctrl = INREG32(PM_PWRSTCTRL_REG) & ~(POWERSTATE_MASK);
        pm_pwstctrl |= s_DomainTable[powerDomain].powerState << POWERSTATE_SHIFT;        
        OUTREG32(PM_PWRSTCTRL_REG, pm_pwstctrl);
    }
    if (ffMask & DOMAIN_UPDATE_LOGICSTATE)
    {
        PM_PWRSTCTRL_REG = (volatile unsigned int*)((UCHAR*)g_pPrcmRegs +
    		                          s_DomainTable[powerDomain].PWRSTCTRL_REG);            
        pm_pwstctrl = INREG32(PM_PWRSTCTRL_REG) & ~(LOGICRETSTATE_MASK(powerDomain));
        pm_pwstctrl |= s_DomainTable[powerDomain].powerState << LOGICRETSTATE_SHIFT(powerDomain);        
        OUTREG32(PM_PWRSTCTRL_REG, pm_pwstctrl);
    } 
    return rc;
}

//-----------------------------------------------------------------------------
BOOL DomainGetDeviceContextState(
    UINT                powerDomain,
    ClockOffsetInfo_2  *pInfo,
    BOOL                bSet
    )
{
	UNREFERENCED_PARAMETER(pInfo);
    OALMSG(1, (L"DomainGetDeviceContextState %d %d\r\n",
		powerDomain, bSet));
	return FALSE;
}

//-----------------------------------------------------------------------------
BOOL DomainInitialize()
{
    UINT            i;
    BOOL            rc = TRUE;
    volatile unsigned int *PM_PWRSTCTRL_REG;    
    
    OALMSG(OAL_FUNC, (L"+DomainInitialize()\r\n"));

    for (i = 0; i < POWERDOMAIN_COUNT; ++i)
    {    
        if (s_DomainTable[i].ffValidationMask & DOMAIN_UPDATE_POWERSTATE)
        {
            PM_PWRSTCTRL_REG = (volatile unsigned int*)((UCHAR*)g_pPrcmRegs +
    		                          s_DomainTable[i].PWRSTCTRL_REG);
            s_DomainTable[i].powerState = (INREG32(PM_PWRSTCTRL_REG)& POWERSTATE_MASK) >> POWERSTATE_SHIFT;
        }
        if (s_DomainTable[i].ffValidationMask & DOMAIN_UPDATE_LOGICSTATE)
        {
            PM_PWRSTCTRL_REG = (volatile unsigned int*)((UCHAR*)g_pPrcmRegs +
    		                          s_DomainTable[i].PWRSTCTRL_REG);
            s_DomainTable[i].logicState = (INREG32(PM_PWRSTCTRL_REG)& LOGICRETSTATE_MASK(i));
        }
    }

    OALMSG(OAL_FUNC, (L"-DomainInitialize()=%d\r\n", rc));
    return rc;    
}


//-----------------------------------------------------------------------------
// WARNING: This function is called from OEMPowerOff - no system calls, critical 
// sections, OALMSG, etc., may be used by this function or any function that it calls.
//------------------------------------------------------------------------------
BOOL PrcmRestoreDomain(UINT powerDomain)
{
    BOOL rc = TRUE;
 	OALMSG(1/*OAL_FUNC*/, (L"+PrcmRestoreDomain (powerDomain=%d)\r\n", powerDomain));
    return rc;
}

//-----------------------------------------------------------------------------
BOOL PrcmDomainSetPowerState(UINT powerDomain,
    UINT        powerState, UINT        logicState
    )
{
    BOOL rc = FALSE;
    UINT mask=0;

    OALMSG(1, (L"+PrcmDomainSetPowerState"
        L"(powerDomain=%d, powerState=0x%08X, logicState=%d)\r\n", 
        powerDomain, powerState, logicState));


    if (powerDomain >= POWERDOMAIN_COUNT) goto cleanUp;
    mask=(s_DomainTable[powerDomain].ffValidationMask & (DOMAIN_UPDATE_POWERSTATE|DOMAIN_UPDATE_LOGICSTATE));
    if (mask == 0) goto cleanUp;
    
    
    Lock(Mutex_Domain);
    if (mask&DOMAIN_UPDATE_POWERSTATE)
        s_DomainTable[powerDomain].powerState = (powerState & POWERSTATE_MASK)>>POWERSTATE_SHIFT;
    if (mask&DOMAIN_UPDATE_LOGICSTATE)
        s_DomainTable[powerDomain].logicState = (logicState & LOGICRETSTATE_MASK(powerDomain));
    rc = _PrcmDomainHwUpdate(powerDomain, mask);
    Unlock(Mutex_Domain);

cleanUp:    
    return rc;
}

//-----------------------------------------------------------------------------
BOOL PrcmDomainSetMemoryState(UINT powerDomain,
    UINT memoryState, UINT memoryStateMask )
{
    UINT val;    
    BOOL rc = FALSE;
    volatile unsigned int * PM_PWRSTCTRL_REG;
    OALMSG(OAL_FUNC, (L"+PrcmDomainSetMemoryState"
        L"(powerDomain=%d, memoryState=0x%08X, memoryStateMask=0x%08X)\r\n", 
        powerDomain, memoryState, memoryStateMask));

    if (powerDomain>=POWERDOMAIN_COUNT) goto cleanUp;

    Lock(Mutex_Domain);
    // update cached logic state
    if (memoryStateMask & LOGICRETSTATE(powerDomain) & s_DomainTable[powerDomain].ffValidationMask)
    {
        s_DomainTable[powerDomain].logicState = memoryState & LOGICRETSTATE_MASK(powerDomain);        
    }
    else
    {
        memoryStateMask &= ~ LOGICRETSTATE(powerDomain);
    }

    PM_PWRSTCTRL_REG = (volatile unsigned int*)((UCHAR*)g_pPrcmRegs +
    		                          s_DomainTable[powerDomain].PWRSTCTRL_REG);  
    
    memoryState &= memoryStateMask;
    val = INREG32(PM_PWRSTCTRL_REG) & ~memoryStateMask;
    val |= memoryState;
    OUTREG32(PM_PWRSTCTRL_REG, val);
    Unlock(Mutex_Domain);
    
    rc = TRUE;
    
cleanUp:        
    OALMSG(OAL_FUNC, (L"-PrcmDomainSetMemoryState()=%d\r\n", rc));
    return rc;
}

//-----------------------------------------------------------------------------
void PrcmDomainUpdateRefCount(UINT powerDomain, UINT bEnable )
{ 
    // update refcount
    if (bEnable)
    {
        InterlockedIncrement((LONG*)&s_DomainTable[powerDomain].refCount);
    }
    else
    {
        InterlockedDecrement((LONG*)&s_DomainTable[powerDomain].refCount);
    }
}

//-----------------------------------------------------------------------------
void PrcmProcessPostMpuWakeup()
{
    // to be implemented later
}

//-----------------------------------------------------------------------------
VOID PrcmDomainClearReset()
{       
    // clear the reset flags for all the power domains
	OUTREG32(&g_pPrcmRegs->RM_PER_RSTST, INREG32(&g_pPrcmRegs->RM_PER_RSTST) ); 
    
    OUTREG32(&g_pPrcmRegs->RM_WKUP_RSTST, INREG32(&g_pPrcmRegs->RM_WKUP_RSTST) );
		
    OUTREG32(&g_pPrcmRegs->RM_GFX_RSTST, INREG32(&g_pPrcmRegs->RM_GFX_RSTST) );
}

