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
//  File:  opp_map.c
//
#include "bsp.h"
#include "oalex.h"
#include "am33x_oal_prcm.h"
#include "bsp_opp_map.h"

#include "omap_dvfs.h"
#include "tps65217.h"

#define MAX_VOLT_DOMAINS        (2)

//-----------------------------------------------------------------------------
static UINT _rgOppVdd[2];


//-----------------------------------------------------------------------------
void 
UpdateRetentionVoltages(IOCTL_RETENTION_VOLTAGES *pData)
{
    UNREFERENCED_PARAMETER(pData);
}

//-----------------------------------------------------------------------------
BOOL
SetFrequencyOpp(
    VddOppSetting_t        *pVddOppSetting
    )
{
    int i;
    BOOL rc = FALSE;

    // iterate through and set the dpll frequency settings    
    for (i = 0; i < pVddOppSetting->dpllCount; ++i)
    {
        PrcmClockSetDpllFrequency(
            pVddOppSetting->rgDpllFreqSettings[i].dpllId,
            pVddOppSetting->rgDpllFreqSettings[i].m,
            pVddOppSetting->rgDpllFreqSettings[i].n,
            0,
            0
            );
    }
    for (i=0; i < pVddOppSetting->dpllClkOutCount; ++i)
    {
        PrcmClockSetDpllClkOutDivisor(
            pVddOppSetting->rgDpllClkOutFreqSettings[i].dpllClkOutId,
            pVddOppSetting->rgDpllClkOutFreqSettings[i].divisor);
    }
    for (i=0; i<pVddOppSetting->clkCount; i++)
    {
        SourceClockInfo_t pClkInfo;
        PrcmClockGetParentClockInfo(pVddOppSetting->rgClkFreqSettings[i].clockId,1,&pClkInfo);            
        PrcmClockSetDivisor(pVddOppSetting->rgClkFreqSettings[i].clockId,
                            pClkInfo.clockId,
                            pVddOppSetting->rgClkFreqSettings[i].divisor);
    }

    rc = TRUE;

    return rc;
}

//-----------------------------------------------------------------------------
BOOL
SetVoltageOpp(
    VddOppSetting_t    *pVddOppSetting
    )
{
	OALMSG(OAL_INFO,(L"TWLSetOPVoltage: voltage %d, mv 0x%08x\r\n",pVddOppSetting->vpInfo.vp, pVddOppSetting->vpInfo.voltSelBitsVal ));
    return TWLSetOPVoltage(pVddOppSetting->vpInfo.vp,pVddOppSetting->vpInfo.voltSelBitsVal);
}


void Opp_init(void)
{
    _rgOppVdd[0] = Vdd1_init_val[BSP_OPM_SELECT];
    _rgOppVdd[1] = INITIAL_VDD2_OPP;
	
}

//-----------------------------------------------------------------------------
BOOL 
SetOpp(
    DWORD *rgDomains,
    DWORD *rgOpps,    
    DWORD  count
    )
{
    UINT                opp;
    UINT                i;
    int                 vdd = 0;
    VddOppSetting_t   **ppVoltDomain;

    // loop through and update all changing voltage domains
    //
    for (i = 0; i < count; ++i)
        {
        // select the Opp table to use
        switch (rgDomains[i])
            {
            case DVFS_MPU1_OPP:
                // validate parameters
                if (rgOpps[i] > MAX_VDD1_OPP) continue;                
                vdd = kVdd1;
                ppVoltDomain = _rgVdd1OppMap;
                break;
            
            case DVFS_CORE1_OPP:
                // validate parameters
                if (rgOpps[i] > MAX_VDD2_OPP) continue;                
                vdd = kVdd2;
                ppVoltDomain = _rgVdd2OppMap;
                break;

            default:
                continue;
            }

        // check if the operating point is actually changing
        opp = rgOpps[i];

        OALMSG(1, (L"SetOpp to %d from %d\r\n", opp, _rgOppVdd[vdd]));
        
//      if (_rgOppVdd[vdd] == opp) continue;

        // depending on which way the transition is occurring change
        // the frequency and voltage levels in the proper order
        if (opp > _rgOppVdd[vdd])
            {
            // transitioning to higher performance, change voltage first
            SetVoltageOpp(ppVoltDomain[opp]);
            SetFrequencyOpp(ppVoltDomain[opp]);         
            }
        else
            {
            // transitioning to lower performance, change frequency first
            SetFrequencyOpp(ppVoltDomain[opp]); 
            SetVoltageOpp(ppVoltDomain[opp]);         
            }
            
        // update opp for voltage domain
        _rgOppVdd[vdd] = opp;
    
        }

    // update latency table
    //OALWakeupLatency_UpdateOpp(rgDomains, rgOpps, count);

    return TRUE;    
}

DWORD GetOpp (DWORD domain)
{
    switch (domain)
    {
        case DVFS_MPU1_OPP:
            return _rgOppVdd[kVdd1];            
        break;
        
        case DVFS_CORE1_OPP:
            return _rgOppVdd[kVdd2];            
        break;

    }
    return kOppUndefined;
}

BOOL 
SmartReflex_EnableMonitor(
    UINT                    channel,
    BOOL                    bEnable
    )
{
    UNREFERENCED_PARAMETER(channel);
    UNREFERENCED_PARAMETER(bEnable);

    return FALSE;
}

BOOL 
IsSmartReflexMonitoringEnabled(UINT channel)
{   
    UNREFERENCED_PARAMETER(channel);
    return FALSE;
}



//-----------------------------------------------------------------------------


