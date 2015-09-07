/*===============================================================================
*             Texas Instruments OMAP(TM) Platform Software
* (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
*
* Use of this software is controlled by the terms and conditions found
* in the license agreement under which this software has been supplied.
*
===============================================================================
*/
//
//  File: oem_pm.c
//
#include "bsp.h"
#include "oalex.h"
#include <nkintr.h>
#include "am33x_oal_prcm.h"
#include "omap_dvfs.h"
#include "bsp_opp_map.h"
#include "am33x_irq.h"
#include "oal_clock.h"
#include "am33x_interrupt_struct.h"
#include "cm3_ipc.h"

extern DWORD GetOpp (DWORD domain);

extern BOOL SetOpp(DWORD *rgDomains,DWORD *rgOpps,DWORD  count);

extern BOOL SetVoltageOpp(VddOppSetting_t    *pVddOppSetting);

extern BOOL OEMSetAlarm2OffTime(int secFromNow);

//-----------------------------------------------------------------------------
// Global : g_pIntr
//  pointer to interrupt structure.
//
extern AM33X_INTR_CONTEXT const    *g_pIntr;

extern UINT32 g_oalM3SysIntr;


//-----------------------------------------------------------------------------
//
//  Global:  g_PrcmDebugSuspendResume
//
//  Enables suspend/resume debug code in platform\common\src\soc tree
//
BOOL g_PrcmDebugSuspendResume = FALSE;


//-----------------------------------------------------------------------------
//
//  Function:  OALIoCtlOppRequest
//
//  updates the current operating point
//
BOOL 
OALIoCtlOppRequest(
    UINT32 code, 
    VOID *pInBuffer,
    UINT32 inSize, 
    VOID *pOutBuffer, 
    UINT32 outSize, 
    UINT32 *pOutSize
    )
{
    BOOL rc = FALSE;
    IOCTL_OPP_REQUEST_IN *pOppRequest;
    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"+OALIoCtlOppRequest\r\n"));

    UNREFERENCED_PARAMETER(outSize);
    UNREFERENCED_PARAMETER(pOutSize);
    UNREFERENCED_PARAMETER(pOutBuffer);
    UNREFERENCED_PARAMETER(code);

    if (pInBuffer == NULL || inSize != sizeof(IOCTL_OPP_REQUEST_IN)) goto cleanUp;
    
    pOppRequest = (IOCTL_OPP_REQUEST_IN*)pInBuffer;
    if (pOppRequest->dwCount > MAX_DVFS_DOMAINS) goto cleanUp;
    
    rc = SetOpp(pOppRequest->rgDomains, 
            pOppRequest->rgOpps, 
            pOppRequest->dwCount
            );

cleanUp:    
    OALMSG(OAL_INTR&&OAL_FUNC, (L"-OALIoCtlOppRequest(rc = %d)\r\n", rc));
    return rc;
}


//-----------------------------------------------------------------------------
//
//  Function:  OALIoCtlPrcmDeviceGetSourceClockInfo
//
//  returns information about a devices clock
//
BOOL OALIoCtlPrcmDeviceGetSourceClockInfo(
    UINT32 code, 
    VOID  *pInBuffer,
    UINT32 inSize, 
    VOID  *pOutBuffer, 
    UINT32 outSize, 
    UINT32*pOutSize
    )
{
    BOOL rc = FALSE;
    IOCTL_PRCM_DEVICE_GET_SOURCECLOCKINFO_OUT *pOut;

    UNREFERENCED_PARAMETER(code);

    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"+OALIoCtlPrcmDeviceGetSourceClockInfo\r\n"));
    
    if (pInBuffer == NULL || inSize != sizeof(UINT) ||
        pOutBuffer == NULL || outSize != sizeof(IOCTL_PRCM_DEVICE_GET_SOURCECLOCKINFO_OUT))
        {
        goto cleanUp;
        }

    // update info and call appropriate routine
    //
    if (pOutSize != NULL) *pOutSize = 0;
    pOut = (IOCTL_PRCM_DEVICE_GET_SOURCECLOCKINFO_OUT*)(pOutBuffer);        
    rc = PrcmDeviceGetSourceClockInfo(*(UINT*)pInBuffer, pOut);

cleanUp:
    OALMSG(OAL_INTR&&OAL_FUNC, (L"-OALIoCtlPrcmDeviceGetSourceClockInfo(rc = %d)\r\n", rc));
    return rc;
}


//-----------------------------------------------------------------------------
//
//  Function:  OALIoCtlPrcmClockGetSourceClockInfo
//
//  returns information about a source clock
//
BOOL OALIoCtlPrcmClockGetSourceClockInfo(
    UINT32 code, 
    VOID  *pInBuffer,
    UINT32 inSize, 
    VOID  *pOutBuffer, 
    UINT32 outSize, 
    UINT32*pOutSize
    )
{
    BOOL rc = FALSE;
    SourceClockInfo_t info;
    IOCTL_PRCM_CLOCK_GET_SOURCECLOCKINFO_IN *pIn;
    IOCTL_PRCM_CLOCK_GET_SOURCECLOCKINFO_OUT *pOut;

    UNREFERENCED_PARAMETER(code);

    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"+OALIoCtlPrcmClockGetSourceClockInfo\r\n"));
    
    if (pInBuffer == NULL || inSize != sizeof(IOCTL_PRCM_CLOCK_GET_SOURCECLOCKINFO_IN) ||
        pOutBuffer == NULL || outSize != sizeof(IOCTL_PRCM_CLOCK_GET_SOURCECLOCKINFO_OUT))
        {
        goto cleanUp;
        }

    // update info and call appropriate routine
    //
    if (pOutSize != NULL) *pOutSize = 0;
    pIn = (IOCTL_PRCM_CLOCK_GET_SOURCECLOCKINFO_IN*)pInBuffer;
    pOut = (IOCTL_PRCM_CLOCK_GET_SOURCECLOCKINFO_OUT*)pOutBuffer;

    memset(pOut, 0, sizeof(IOCTL_PRCM_CLOCK_GET_SOURCECLOCKINFO_OUT));
    rc = PrcmClockGetParentClockRefcount(pIn->clockId, pIn->clockLevel, &pOut->refCount);
    if (PrcmClockGetParentClockInfo(pIn->clockId, pIn->clockLevel, &info))
    {        
        pOut->parentId = info.clockId;
        pOut->parentLevel = info.nLevel;
        pOut->parentRefCount = info.refCount;
    }

cleanUp:
    OALMSG(OAL_INTR&&OAL_FUNC, (L"-OALIoCtlPrcmClockGetSourceClockInfo(rc = %d)\r\n", rc));
    return rc;
}



//-----------------------------------------------------------------------------
//
//  Function:  OALIoCtlPrcmClockSetSourceClockDivisor
//
//  sets the source clock divisor for a given functional clock
//
BOOL OALIoCtlPrcmClockSetSourceClockDivisor(
    UINT32 code, 
    VOID  *pInBuffer,
    UINT32 inSize, 
    VOID  *pOutBuffer, 
    UINT32 outSize, 
    UINT32*pOutSize
    )
{
    BOOL rc = FALSE;
    IOCTL_PRCM_CLOCK_SET_SOURCECLOCKDIVISOR_IN *pIn;

    UNREFERENCED_PARAMETER(outSize);
    UNREFERENCED_PARAMETER(pOutBuffer);
    UNREFERENCED_PARAMETER(code);

    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"+OALIoCtlPrcmClockSetSourceClockDivisor\r\n"));
    
    if (pInBuffer == NULL || inSize != sizeof(IOCTL_PRCM_CLOCK_SET_SOURCECLOCKDIVISOR_IN))
        {
        goto cleanUp;
        }

    // update info and call appropriate routine
    //
    if (pOutSize != NULL) *pOutSize = 0;
    pIn = (IOCTL_PRCM_CLOCK_SET_SOURCECLOCKDIVISOR_IN*)pInBuffer;
    rc = PrcmClockSetDivisor(pIn->clkId, pIn->parentClkId, pIn->divisor);

cleanUp:
    OALMSG(OAL_INTR&&OAL_FUNC, (L"-OALIoCtlPrcmClockSetSourceClockDivisor(rc = %d)\r\n", rc));
    return rc;
}



//-----------------------------------------------------------------------------
//
//  Function:  OALIoCtlPrcmClockSetDpllClkOutState
//
//  updates the current dpll clock out settings
//
BOOL 
OALIoCtlPrcmClockSetDpllClkOutState(
    UINT32 code, 
    VOID *pInBuffer,
    UINT32 inSize, 
    VOID *pOutBuffer, 
    UINT32 outSize, 
    UINT32 *pOutSize
    )
{
    BOOL rc = FALSE;
    IOCTL_PRCM_CLOCK_SET_DPLLCLKOUTSTATE_IN *pIn;

    UNREFERENCED_PARAMETER(outSize);
    UNREFERENCED_PARAMETER(pOutBuffer);
    UNREFERENCED_PARAMETER(code);

    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"+OALIoCtlPrcmClockSetDpllState\r\n"));
    
    if (pInBuffer == NULL || inSize != sizeof(IOCTL_PRCM_CLOCK_SET_DPLLCLKOUTSTATE_IN))
        {
        goto cleanUp;
        }

    // update info and call appropriate routine
    //
    if (pOutSize != NULL) *pOutSize = 0;
    pIn = (IOCTL_PRCM_CLOCK_SET_DPLLCLKOUTSTATE_IN*)pInBuffer;
    rc = PrcmClockSetDpllClkOutState(pIn);

cleanUp:
    OALMSG(OAL_INTR&&OAL_FUNC, (L"-OALIoCtlPrcmClockSetDpllState(rc = %d)\r\n", rc));
    return rc;
}


UINT32 OALPrcmIntrHandler()
{

#if 0
    const UINT clear_mask = PRM_IRQENABLE_VP1_NOSMPSACK_EN |
                            PRM_IRQENABLE_VP2_NOSMPSACK_EN |
                            PRM_IRQENABLE_VC_SAERR_EN |
                            PRM_IRQENABLE_VC_RAERR_EN |
                            PRM_IRQENABLE_VC_TIMEOUTERR_EN |
                            PRM_IRQENABLE_WKUP_EN |
                            PRM_IRQENABLE_TRANSITION_EN |
                            PRM_IRQENABLE_MPU_DPLL_RECAL_EN |
                            PRM_IRQENABLE_CORE_DPLL_RECAL_EN |
                            PRM_IRQENABLE_VP1_OPPCHANGEDONE_EN |
                            PRM_IRQENABLE_VP2_OPPCHANGEDONE_EN |
                            PRM_IRQENABLE_IO_EN ;
#endif 
	UINT sysIntr = SYSINTR_NOP;

    OALMSG(1/*OAL_FUNC*/, (L"+OALPrcmIntrHandler\r\n"));

    // get cause of interrupt
//    sysIntr = PrcmInterruptProcess(clear_mask);

    OALMSG(OAL_FUNC, (L"-OALPrcmIntrHandler\r\n"));
    return sysIntr;
}

extern void PrcmCM3Init();


VOID OALPowerPostInit() 
{
    
    DWORD rgDomain = DVFS_MPU1_OPP;
    DWORD * rgOpp = OALArgsQuery(OAL_ARGS_QUERY_OPP_MODE);  

    OALMSG(OAL_FUNC, (L"+OALPowerPostInit\r\n"));
    
	PrcmPostInit();
    PrcmCM3Init();
    Opp_init();    

    SetOpp(&rgDomain,rgOpp,1);   

    
#ifdef DEBUG_PRCM_SUSPEND_RESUME
    g_PrcmDebugSuspendResume = TRUE;
#endif

    OALMSG(OAL_FUNC, (L"-OALPowerPostInit\r\n"));
}

//-----------------------------------------------------------------------------
//
//  Function:  OALContextSavePerfTimer
//
//  Saves the PerfTimer Registers
//
VOID
OALContextSavePerfTimer()
{
    /* TODO: Save registers for Perf Timer since their context will be lost when going into DS0 */
}

//-----------------------------------------------------------------------------
//
//  Function:  OALContextRestorePerfTimer
//
//  Restores the PerfTimer Registers from shadow register
//
VOID
OALContextRestorePerfTimer()
{
    /* TODO: Restore registers for Perf Timer since their context is lost when going into DS0 */
}


typedef struct _GPIO_INTR_CTXT{
    DWORD irq0;
    DWORD irq1;
} GPIO_INTR_CTXT;


VOID OEMPowerOff() 
{
    DWORD i;
    UINT intr[4];
    BOOL bPowerOn;
    UINT irq = 0;
    GPIO_INTR_CTXT gpio_intr[AM33X_GPIO_BANK_COUNT];
    
    DWORD rgDomain = DVFS_MPU1_OPP;
    DWORD rgOPP = kOpp1;
    DWORD prevOpp[2];
    
    OALMSG(OAL_FUNC,(L"+OEMPowerOff\r\n"));

//	if (OEMSetAlarm2OffTime(2))
//		OALMSG(1,(L"OEMPowerOff: alarm set!\r\n"));
//Loop:
//    OALMSG(1,(L"OEMPowerOff: wait..!\r\n"));
//    goto Loop;

    // Disable hardware watchdog
    OALWatchdogEnable(FALSE);    
    
    // Make sure that KITL is powered off
    bPowerOn = FALSE;
    KITLIoctl(IOCTL_KITL_POWER_CALL, &bPowerOn, sizeof(bPowerOn), NULL, 0, NULL);    
    
    //Save Perf Timer
    OALContextSavePerfTimer();
    // Disable GPTimer (used for high perf/monte carlo profiling)
    EnableDeviceClocks(BSPGetGPTPerfDevice(), FALSE);
    

    OALTimerStop();
    OALMSG(OAL_INFO,(L"OEMPowerOff: Stop System Timer Clock\r\n"));

    // Give chance to do board specific stuff
    BSPPowerOff();
   
    OALMSG(OAL_INFO,(L"OEMPowerOff: Done with BSPPowerOff\r\n"));   
   
    
    //----------------------------------------------
    // capture all enabled interrupts and disable interrupts
    intr[0] = INREG32(&g_pIntr->pICLRegs->INTC_MIR0);
    intr[1] = INREG32(&g_pIntr->pICLRegs->INTC_MIR1);
    intr[2] = INREG32(&g_pIntr->pICLRegs->INTC_MIR2);
    intr[3] = INREG32(&g_pIntr->pICLRegs->INTC_MIR3);

    OUTREG32(&g_pIntr->pICLRegs->INTC_MIR_SET0, OMAP_MPUIC_MASKALL);
    OUTREG32(&g_pIntr->pICLRegs->INTC_MIR_SET1, OMAP_MPUIC_MASKALL);
    OUTREG32(&g_pIntr->pICLRegs->INTC_MIR_SET2, OMAP_MPUIC_MASKALL);    
    OUTREG32(&g_pIntr->pICLRegs->INTC_MIR_SET3, OMAP_MPUIC_MASKALL);

    //----------------------------------------------
    // Context Save/Restore       
	// Save state then mask all GPIO interrupts
	EnableDeviceClocks(AM_DEVICE_GPIO0,TRUE);
	EnableDeviceClocks(AM_DEVICE_GPIO1,TRUE);
	EnableDeviceClocks(AM_DEVICE_GPIO2,TRUE);
	EnableDeviceClocks(AM_DEVICE_GPIO3,TRUE);
	
	for (i=0; i<AM33X_GPIO_BANK_COUNT; i++)
    {		
		// Save current state
		gpio_intr[i].irq0 = INREG32(&g_pIntr->pGPIORegs[i]->IRQSTATUS_SET_0);
        gpio_intr[i].irq1 = INREG32(&g_pIntr->pGPIORegs[i]->IRQSTATUS_SET_1);

		// Disable all GPIO interrupts in the bank
        OUTREG32(&g_pIntr->pGPIORegs[i]->IRQSTATUS_SET_0, 0x00000000);
        OUTREG32(&g_pIntr->pGPIORegs[i]->IRQSTATUS_SET_1, 0x00000000);        
		
	}

	EnableDeviceClocks(AM_DEVICE_GPIO0,FALSE);
	EnableDeviceClocks(AM_DEVICE_GPIO1,FALSE);
	EnableDeviceClocks(AM_DEVICE_GPIO2,FALSE);
	EnableDeviceClocks(AM_DEVICE_GPIO3,FALSE);

    //Enable M3 interrupt for handshake protocol while setting up DSx state
    OEMInterruptEnable(g_oalM3SysIntr, NULL, 0);

    // TODO - Use wakeup sysintr to set the wake source in M3 message

    //Reduce VDD1 and VDD2; 
    prevOpp[0] = GetOpp(DVFS_MPU1_OPP);
    prevOpp[1] = GetOpp(DVFS_CORE1_OPP);
    SetOpp(&rgDomain,&rgOPP,1);
    // Cannot drop Core voltage to OPP 50 as it cause the device to hang. This needs to be debugged.
//    OALMSG(1,(L"OEMPowerOff: Pre Reduce Voltage\r\n"));
//    SetVoltageOpp(&vdd2Opp1Info);

    OALMSG(OAL_INFO,(L"OEMPowerOff: Done with Reduce Voltage\r\n"));
    
    // enter full retention
    PrcmSuspend();

    OALMSG(OAL_INFO,(L"OEMPowerOff: Done with PrcmSuspend\r\n"));
    
    SetOpp(&rgDomain,&prevOpp[0],1);
    SetVoltageOpp(_rgVdd2OppMap[prevOpp[1]]);

    OALMSG(OAL_INFO,(L"OEMPowerOff: Done with Recover Voltage\r\n"));
    
    
#if 0
    //----------------------------------------------
    // Find wakeup source
    for (sysIntr = SYSINTR_DEVICES; sysIntr < SYSINTR_MAXIMUM; sysIntr++)
    {            
        // Skip if sysIntr isn't allowed as wake source
        if (!OALPowerWakeSource(sysIntr)) 
		    continue;

        // When this sysIntr is pending we find wake source
        if (OEMInterruptPending(sysIntr))
            {
            g_oalWakeSource = sysIntr;
            break;
            }
    }
    
#endif

    
    //----------------------------------------------
    // Context Save/Restore
    // Put GPIO interrupt state back to the way it was before suspend
    EnableDeviceClocks(AM_DEVICE_GPIO0,TRUE);
	EnableDeviceClocks(AM_DEVICE_GPIO1,TRUE);
	EnableDeviceClocks(AM_DEVICE_GPIO2,TRUE);
	EnableDeviceClocks(AM_DEVICE_GPIO3,TRUE);
	
    for (i=0; i<AM33X_GPIO_BANK_COUNT; i++)
    {
        // Write registers with the previously saved values
        OUTREG32(&g_pIntr->pGPIORegs[i]->IRQSTATUS_SET_0, gpio_intr[i].irq0);
        OUTREG32(&g_pIntr->pGPIORegs[i]->IRQSTATUS_SET_1, gpio_intr[i].irq1);

    }

    EnableDeviceClocks(AM_DEVICE_GPIO0,FALSE);
	EnableDeviceClocks(AM_DEVICE_GPIO1,FALSE);
	EnableDeviceClocks(AM_DEVICE_GPIO2,FALSE);
	EnableDeviceClocks(AM_DEVICE_GPIO3,FALSE);
    

    //----------------------------------------------
    // Re-enable interrupts    
    OUTREG32(&g_pIntr->pICLRegs->INTC_MIR_CLEAR0, ~intr[0]);
    OUTREG32(&g_pIntr->pICLRegs->INTC_MIR_CLEAR1, ~intr[1]);
    OUTREG32(&g_pIntr->pICLRegs->INTC_MIR_CLEAR2, ~intr[2]);  
    OUTREG32(&g_pIntr->pICLRegs->INTC_MIR_CLEAR3, ~intr[3]);  

    OALMSG(OAL_INFO,(L"OEMPowerOff: Re-enable Interrupts 0x%x 0x%x 0x%x 0x%x\r\n",
                        INREG32(&g_pIntr->pICLRegs->INTC_MIR0),
                        INREG32(&g_pIntr->pICLRegs->INTC_MIR1),
                        INREG32(&g_pIntr->pICLRegs->INTC_MIR2),
                        INREG32(&g_pIntr->pICLRegs->INTC_MIR3)));

    
    //----------------------------------------------
    // Do board specific stuff    
    BSPPowerOn();   
        
    //Sync to Hardware RTC after suspend\resume
    //OALIoCtlHalRtcTime( 0,  NULL, 0, NULL, 0, NULL);    // Do we need this?? [TODO]

    OALTimerStart();	
	
    // Enable GPTimer (used for high perf/monte carlo profiling)
    EnableDeviceClocks(BSPGetGPTPerfDevice(), TRUE);	
    //Restore Perf Timer
    OALContextRestorePerfTimer();
		
    // Reinitialize KITL
    bPowerOn = TRUE;
    KITLIoctl(IOCTL_KITL_POWER_CALL, &bPowerOn, sizeof(bPowerOn), NULL, 0, NULL);    

    // Enable hardware watchdog
    OALWatchdogEnable(TRUE);

	OALMSG(OAL_FUNC,(L"-OEMPowerOff\r\n"));
}

