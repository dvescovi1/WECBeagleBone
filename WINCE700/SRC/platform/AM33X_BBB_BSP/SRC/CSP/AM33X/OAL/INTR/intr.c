//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this sample source code is subject to the terms of the Microsoft
// license agreement under which you licensed this sample source code. If
// you did not accept the terms of the license agreement, you are not
// authorized to use this sample source code. For the terms of the license,
// please see the license agreement between you and Microsoft or, if applicable,
// see the LICENSE.RTF on your install media or the root of your tools installation.
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
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
//  File: intr.c
//
//  This file implement major part of interrupt module for OMAP7xx SoC.
//
#include <windows.h>
#include <ceddk.h>
#include <nkintr.h>
#include <oal.h>
#include <oalex.h>
#include <am33x.h>
//#include <bus.h>
//#include <oal_prcm.h>
#include <am33x_interrupt_struct.h>
#include <am33x_oal_prcm.h>

//------------------------------------------------------------------------------
//
//  Global: g_oalTimerIrq 
//
UINT32 g_oalTimerIrq = (UINT32)OAL_INTR_IRQ_UNDEFINED;

//------------------------------------------------------------------------------
//
//  Global: g_oalPrcmIrq 
//
UINT32 g_oalPrcmIrq = (UINT32)OAL_INTR_IRQ_UNDEFINED;

//------------------------------------------------------------------------------
//
//  Global: g_oalPrcmIrq 
//
UINT32 g_oalM3Irq = (UINT32)OAL_INTR_IRQ_UNDEFINED;
UINT32 g_oalM3SysIntr = (UINT32)SYSINTR_UNDEFINED;


//------------------------------------------------------------------------------
//
//  Global: g_oalSmartReflex1 
//
UINT32 g_oalSmartReflex1 = (UINT32)OAL_INTR_IRQ_UNDEFINED;

//------------------------------------------------------------------------------
//
//  Global: g_oalSmartReflex2 
//
UINT32 g_oalSmartReflex2 = (UINT32)OAL_INTR_IRQ_UNDEFINED;

//------------------------------------------------------------------------------
//
//  Static: s_intr
//
//  This value contains virtual uncached address of interrupt controller
//  unit registers.
//
static AM33X_INTR_CONTEXT  s_intr;

//------------------------------------------------------------------------------
//
//  Static: g_pIntr
//
//  exposes pointer to interrupt structure.
//
AM33X_INTR_CONTEXT const *g_pIntr = &s_intr;


//------------------------------------------------------------------------------
//
//  Static: g_intcLxLevel
//
//  Following arrays contain interrupt routing, level and priority
//  initialization values for ILR interrupt controller registers.
//
static UINT32 s_icL1Level[] = {
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 0
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 2
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 4
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 6
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 8
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 10
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 12
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 14
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 16
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 18
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 20
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 22
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 24
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 26
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 28
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 30

    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 32/0
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 34/2
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 36/4 
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 38/6
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 40/8
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 42/10
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 44/12
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 46/14
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 48/16
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 50/18
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 52/20
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 54/22
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 56/24
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 58/26
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 60/28
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 62/30

    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 64/0
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI1,    // 66/2
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 68/4
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 70/6
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 72/8
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 74/10
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 76/12
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 78/14
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 80/16
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 82/18
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 84/20
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 86/22
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 88/24
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 90/26
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 92/28
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 94/30

    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 96/0
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 98/2
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 100/4
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 102/6
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 104/8
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 106/10
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 108/12
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 110/14
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 112/16
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 114/18
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 116/20
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 118/22
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 120/24
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 122/26
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 124/28
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16    // 126/30
 };

//------------------------------------------------------------------------------
UINT32 g_IrqCnt[MAX_IRQ_COUNT];     // IRQ counters for statistic purpose

void OALGetIrqCounters(UINT32 *outBuf)
{
	if (outBuf)
		memcpy(outBuf, g_IrqCnt, sizeof(g_IrqCnt));
}

//------------------------------------------------------------------------------
//
//  Function:  OALIntrInit
//
//  This function initialize AM33X interrupt subsystem. Implementation must
//  use its own mapping structure because general implementation limits
//  number of IRQ to 64 but AM33X has 128 IRQs + 128 GPIOs 
//
BOOL OALIntrInit()
{
    BOOL rc = FALSE;
    UINT32 i, mask;

    OALMSG(OAL_INTR&&OAL_FUNC, (L"+OALInterruptInit\r\n"));

	memset(g_IrqCnt, 0, sizeof(g_IrqCnt));

    // Initialize interrupt mapping
    OALIntrMapInit();

    // Get interrupt controller and GPIO registers' virtual uncached addresses
    s_intr.pICLRegs = OALPAtoUA(AM33X_INTC_MPU_REGS_PA);
    s_intr.pGPIORegs[0] = OALPAtoUA(AM33X_GPIO0_REGS_PA);
    s_intr.pGPIORegs[1] = OALPAtoUA(AM33X_GPIO1_REGS_PA);
    s_intr.pGPIORegs[2] = OALPAtoUA(AM33X_GPIO2_REGS_PA);
    s_intr.pGPIORegs[3] = OALPAtoUA(AM33X_GPIO3_REGS_PA);

    //Reset the MPU INTC and wait until reset is complete
    SETREG32(&s_intr.pICLRegs->INTC_SYSCONFIG, SYSCONFIG_SOFTRESET);
    while ((INREG32(&s_intr.pICLRegs->INTC_SYSSTATUS) & SYSSTATUS_RESETDONE) == 0);

    //auto-idle the interface clock for the interrupt controller
    SETREG32(&s_intr.pICLRegs->INTC_SYSCONFIG, SYSCONFIG_AUTOIDLE);

    //Disable all interrupts and clear the ISR - for all for GPIO banks, too
    OUTREG32(&s_intr.pICLRegs->INTC_MIR_SET0, OMAP_MPUIC_MASKALL);
    OUTREG32(&s_intr.pICLRegs->INTC_MIR_SET1, OMAP_MPUIC_MASKALL);
    OUTREG32(&s_intr.pICLRegs->INTC_MIR_SET2, OMAP_MPUIC_MASKALL);
    OUTREG32(&s_intr.pICLRegs->INTC_MIR_SET3, OMAP_MPUIC_MASKALL);

    // enable gpio clocks
    PrcmDeviceEnableClocks(AM_DEVICE_GPIO0, TRUE);
    PrcmDeviceEnableClocks(AM_DEVICE_GPIO1, TRUE);
    PrcmDeviceEnableClocks(AM_DEVICE_GPIO2, TRUE);
    PrcmDeviceEnableClocks(AM_DEVICE_GPIO3, TRUE);

    //Reset and Disable interrupt/wakeup for all GPIOs
    for (i = 0; i < AM33X_GPIO_BANK_COUNT; i++)
        {
        //Disable interrupt/wakeup
        OUTREG32(&s_intr.pGPIORegs[i]->IRQSTATUS_CLR_0, 0xffffffff);
        OUTREG32(&s_intr.pGPIORegs[i]->IRQSTATUS_CLR_1, 0xffffffff);

        // clear irq status bits
        mask = INREG32(&s_intr.pGPIORegs[i]->IRQSTATUS_0);
        OUTREG32(&s_intr.pGPIORegs[i]->IRQSTATUS_0, mask);

        mask = INREG32(&s_intr.pGPIORegs[i]->IRQSTATUS_1);
        OUTREG32(&s_intr.pGPIORegs[i]->IRQSTATUS_1, mask);

        //Enable smart and auto idle for GPIO 
        //(We don't need to set INTC since INTC is always in smart mode)
        // Why is the interrupt subsystem controlling the GPIO subsystem clocks?
        OUTREG32(&s_intr.pGPIORegs[i]->SYSCONFIG, 
            SYSCONFIG_AUTOIDLE| SYSCONFIG_ENAWAKEUP | SYSCONFIG_SMARTIDLE
            );
        
        }

    // clear any possible pending interrupts
    INREG32(&s_intr.pICLRegs->INTC_SIR_IRQ);
    INREG32(&s_intr.pICLRegs->INTC_SIR_FIQ);
   
    //Initialize interrupt routing, level and priority
    for (i = 0; i < 128; i++) {
        OUTREG32(&s_intr.pICLRegs->INTC_ILR[i], s_icL1Level[i]);
    }
    
    //Call board specific initializatrion
    rc = BSPIntrInit();

     // Finally enable the interrupts for the GPIO controllers
	//OALIntrEnableIrqs(1,&irqGPIO0A);
	//OALIntrEnableIrqs(1,&irqGPIO0B);
	//OALIntrEnableIrqs(1,&irqGPIO1A);
	//OALIntrEnableIrqs(1,&irqGPIO1B);
	//OALIntrEnableIrqs(1,&irqGPIO2A);
	//OALIntrEnableIrqs(1,&irqGPIO2B);
	//OALIntrEnableIrqs(1,&irqGPIO3A);
	//OALIntrEnableIrqs(1,&irqGPIO3B);

	// disable gpio clocks
	PrcmDeviceEnableClocks(AM_DEVICE_GPIO0, FALSE);
	PrcmDeviceEnableClocks(AM_DEVICE_GPIO1, FALSE);
	PrcmDeviceEnableClocks(AM_DEVICE_GPIO2, FALSE);
	PrcmDeviceEnableClocks(AM_DEVICE_GPIO3, FALSE);
    
	OALMSG(OAL_INTR&&OAL_FUNC, (L"-OALInterruptInit(rc = %d)\r\n", rc));
    return rc;

}


//------------------------------------------------------------------------------
//
//  Function:  OALIntrRequestIrq
//
//  This function returns IRQs for CPU/SoC devices based on their
//  physical address.
//
BOOL OALIntrRequestIrqs( DEVICE_LOCATION *pDevLoc, UINT32 *pCount, UINT32 *pIrqs )
{
    BOOL rc = FALSE;

    OALMSG(OAL_INTR&&OAL_FUNC, (
        L"+OALIntrRequestIrqs(0x%08x->%d/%d/0x%08x/%d, 0x%08x, 0x%08x)\r\n",
        pDevLoc, pDevLoc->IfcType, pDevLoc->BusNumber, pDevLoc->LogicalLoc,
        pDevLoc->Pin, pCount, pIrqs
        ));

    // This shouldn't happen
    if (*pCount < 1) goto cleanUp;

    //switch (pDevLoc->IfcType) {
    //    case Internal:
    //        break;
    //    }

    if (!rc) rc = BSPIntrRequestIrqs(pDevLoc, pCount, pIrqs);

cleanUp:
    OALMSG(OAL_INTR&&OAL_FUNC, (L"-OALIntrRequestIrqs %d   %d (rc = %d)\r\n", *pCount, pIrqs[0], rc));
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  OALIntrEnableIrqs
//
BOOL OALIntrEnableIrqs( UINT32 count, const UINT32 *pIrqs )
{
    BOOL rc = FALSE;
    UINT32 irq, i;

    OALMSG(OAL_INTR&&OAL_FUNC, (L"+OALIntrEnableIrqs(%d, 0x%08x)\r\n", count, pIrqs ));

    for (i = 0; i < count; i++){
        irq = pIrqs[i];
        if (irq != (UINT32)OAL_INTR_IRQ_UNDEFINED) {
			if (irq < 32){
				OUTREG32(&s_intr.pICLRegs->INTC_MIR_CLEAR0, 1 << irq);
			} else if (irq < 64) {
				OUTREG32(&s_intr.pICLRegs->INTC_MIR_CLEAR1, 1 << (irq - 32));
			} else if (irq < 96) {
				OUTREG32(&s_intr.pICLRegs->INTC_MIR_CLEAR2, 1 << (irq - 64));
			} else if (irq < MAX_IRQ_COUNT) {
				OUTREG32(&s_intr.pICLRegs->INTC_MIR_CLEAR3, 1 << (irq - 96));
            } else if (irq <= IRQ_GPIO_31) {
                PrcmDeviceEnableClocks(AM_DEVICE_GPIO0, TRUE);           
                OUTREG32(&s_intr.pICLRegs->INTC_MIR_CLEAR3, 1 << (IRQ_GPIO0A - 96));
                OUTREG32(&s_intr.pICLRegs->INTC_MIR_CLEAR3, 1 << (IRQ_GPIO0B - 96));

                OUTREG32(&s_intr.pGPIORegs[0]->IRQSTATUS_SET_0, 1<<(irq - IRQ_GPIO_0));   // unmask IRQ

                OEMEnableIOPadWakeup((irq - IRQ_GPIO_0), TRUE);
            } else if (irq <= IRQ_GPIO_63) {
                PrcmDeviceEnableClocks(AM_DEVICE_GPIO1, TRUE);
                OUTREG32(&s_intr.pICLRegs->INTC_MIR_CLEAR3, 1<< (IRQ_GPIO1A - 96));
                OUTREG32(&s_intr.pICLRegs->INTC_MIR_CLEAR3, 1<< (IRQ_GPIO1B - 96));

                OUTREG32(&s_intr.pGPIORegs[1]->IRQSTATUS_SET_0, 1<<(irq - IRQ_GPIO_32));  // unmask IRQ

                OEMEnableIOPadWakeup((irq - IRQ_GPIO_0), TRUE);
            } else if (irq <= IRQ_GPIO_95) {
                PrcmDeviceEnableClocks(AM_DEVICE_GPIO2, TRUE);
                OUTREG32(&s_intr.pICLRegs->INTC_MIR_CLEAR1, 1<< (IRQ_GPIO2A - 32));
                OUTREG32(&s_intr.pICLRegs->INTC_MIR_CLEAR1, 1<< (IRQ_GPIO2B - 32));

                OUTREG32(&s_intr.pGPIORegs[2]->IRQSTATUS_SET_0, 1<<(irq - IRQ_GPIO_64));  // unmask IRQ

                OEMEnableIOPadWakeup((irq - IRQ_GPIO_0), TRUE);
            } else if (irq <= IRQ_GPIO_127) {
                PrcmDeviceEnableClocks(AM_DEVICE_GPIO3, TRUE);
                OUTREG32(&s_intr.pICLRegs->INTC_MIR_CLEAR1, 1<< (IRQ_GPIO3A - 32));
                OUTREG32(&s_intr.pICLRegs->INTC_MIR_CLEAR1, 1<< (IRQ_GPIO3B - 32));
               
                OUTREG32(&s_intr.pGPIORegs[3]->IRQSTATUS_SET_0, 1<<(irq - IRQ_GPIO_96));  // unmask IRQ

                OEMEnableIOPadWakeup((irq - IRQ_GPIO_0), TRUE);
            } else if (irq < IRQ_SW_RESERVED_MAX ) {
							//  SW triggered interrupts only - nothing to enable in HW
			}
			rc = TRUE;
		}
	}
    OALMSG(OAL_INTR&&OAL_FUNC, (L"-OALIntrEnableIrqs(%d, 0x%08x)\r\n", count, pIrqs ));
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  OALIntrDisableIrqs
//
VOID OALIntrDisableIrqs( UINT32 count, const UINT32 *pIrqs )
{
    UINT32 irq, i;

    OALMSG(OAL_INTR&&OAL_FUNC, (L"+OALIntrDisableIrqs(%d, 0x%08x)\r\n", count, pIrqs ));

    for (i = 0; i < count; i++) {
        irq = pIrqs[i];

        if (irq != (UINT32)OAL_INTR_IRQ_UNDEFINED) {            
			if (irq < 32){
				OUTREG32(&s_intr.pICLRegs->INTC_MIR_SET0, 1 << irq);
			} else if (irq < 64) {
				OUTREG32(&s_intr.pICLRegs->INTC_MIR_SET1, 1 << (irq - 32));
			} else if (irq < 96) {
				OUTREG32(&s_intr.pICLRegs->INTC_MIR_SET2, 1 << (irq - 64));
			} else if (irq < MAX_IRQ_COUNT) {
				OUTREG32(&s_intr.pICLRegs->INTC_MIR_SET3, 1 << (irq - 96));
            } else if (irq <= IRQ_GPIO_31) {
                OUTREG32(&s_intr.pGPIORegs[0]->IRQSTATUS_CLR_0, 1<<(irq - IRQ_GPIO_0));   // Disable IRQ
                OEMEnableIOPadWakeup((irq - IRQ_GPIO_0), FALSE);
                PrcmDeviceEnableClocks(AM_DEVICE_GPIO0, FALSE);
            } else if (irq <= IRQ_GPIO_63) {
                OUTREG32(&s_intr.pGPIORegs[1]->IRQSTATUS_CLR_0, 1<<(irq - IRQ_GPIO_32));  // Disable IRQ
                OEMEnableIOPadWakeup((irq - IRQ_GPIO_0), FALSE);
                PrcmDeviceEnableClocks(AM_DEVICE_GPIO1, FALSE);
            } else if (irq <= IRQ_GPIO_95) {
                OUTREG32(&s_intr.pGPIORegs[2]->IRQSTATUS_CLR_0, 1<<(irq - IRQ_GPIO_64));  // Disable IRQ
                OEMEnableIOPadWakeup((irq - IRQ_GPIO_0), FALSE);
                PrcmDeviceEnableClocks(AM_DEVICE_GPIO1, FALSE);
            } else if (irq <= IRQ_GPIO_127) {
                OUTREG32(&s_intr.pGPIORegs[3]->IRQSTATUS_CLR_0, 1<<(irq - IRQ_GPIO_96));  // Disable IRQ
                OEMEnableIOPadWakeup((irq - IRQ_GPIO_0), FALSE);
                PrcmDeviceEnableClocks(AM_DEVICE_GPIO1, FALSE);
            } else if (irq < IRQ_SW_RESERVED_MAX ) {
                            //  SW triggered interrupts only - nothing to disable in HW
            }
		}
    }

    OALMSG(OAL_INTR&&OAL_FUNC, (L"-OALIntrDisableIrqs\r\n"));
}
    
//------------------------------------------------------------------------------
VOID OALIntrDoneIrqs( UINT32 count, const UINT32 *pIrqs )
{
    BOOL rc = FALSE;
    UINT32 irq, i;

    OALMSG(OAL_INTR&&OAL_FUNC, ( L"+OALIntrDoneIrqs(%d, 0x%08x)\r\n", count, pIrqs ));

    for (i = 0; i < count; i++) {
        irq = pIrqs[i];

        if (irq != (UINT32)OAL_INTR_IRQ_UNDEFINED) {  
			if (irq < 32){
				OUTREG32(&s_intr.pICLRegs->INTC_MIR_CLEAR0, 1 << irq);
			} else if (irq < 64) {
				OUTREG32(&s_intr.pICLRegs->INTC_MIR_CLEAR1, 1 << (irq - 32));
			} else if (irq < 96) {
				OUTREG32(&s_intr.pICLRegs->INTC_MIR_CLEAR2, 1 << (irq - 64));
			} else if (irq < MAX_IRQ_COUNT) {
				OUTREG32(&s_intr.pICLRegs->INTC_MIR_CLEAR3, 1 << (irq - 96));
			} else if (irq <= IRQ_GPIO_31) {
				OUTREG32(&s_intr.pGPIORegs[0]->IRQSTATUS_0, 1<<(irq - IRQ_GPIO_0));   // unmask IRQ
				OEMEnableIOPadWakeup((irq - IRQ_GPIO_0), TRUE);
			} else if (irq <= IRQ_GPIO_63){
				OUTREG32(&s_intr.pGPIORegs[1]->IRQSTATUS_0, 1<<(irq - IRQ_GPIO_32));  // unmask IRQ
				OEMEnableIOPadWakeup((irq - IRQ_GPIO_0), TRUE);
			} else if (irq <= IRQ_GPIO_95){
				OUTREG32(&s_intr.pGPIORegs[2]->IRQSTATUS_0, 1<<(irq - IRQ_GPIO_64));  // unmask IRQ
				OEMEnableIOPadWakeup((irq - IRQ_GPIO_0), TRUE);
			} else if (irq <= IRQ_GPIO_127){
				OUTREG32(&s_intr.pGPIORegs[3]->IRQSTATUS_0, 1<<(irq - IRQ_GPIO_96));  // unmask IRQ
				OEMEnableIOPadWakeup((irq - IRQ_GPIO_0), TRUE);
			} else if (irq < IRQ_SW_RESERVED_MAX ) {
						  //  SW triggered interrupts only - nothing to finish in HW
			}
            rc = TRUE;
        }
	}

    OALMSG(OAL_INTR&&OAL_FUNC, (L"-OALIntrDoneIrqs\r\n"));
}


BOOL OALIntrIsIrqPending( UINT32 irq )
//  This function checks if the given interrupt is pending.
{
    BOOL rc = FALSE;

    if (irq < 32){
        rc = INREG32(&s_intr.pICLRegs->INTC_ITR0) & (1 << irq);
    } else if (irq < 64) {
        rc = INREG32(&s_intr.pICLRegs->INTC_ITR1) & (1 << (irq - 32));
    } else if (irq < 96) {
        rc = INREG32(&s_intr.pICLRegs->INTC_ITR2) & (1 << (irq - 64));
    } else if (irq < MAX_IRQ_COUNT) {
        rc = INREG32(&s_intr.pICLRegs->INTC_ITR3) & (1 << (irq - 96));
    } else if (irq <= IRQ_GPIO_31) {
        rc = INREG32(&s_intr.pGPIORegs[0]->IRQSTATUS_0) & (1 << (irq - IRQ_GPIO_0));
    } else if (irq <= IRQ_GPIO_63) {
        rc = INREG32(&s_intr.pGPIORegs[1]->IRQSTATUS_0) & (1 << (irq - IRQ_GPIO_32));    
    } else if (irq <= IRQ_GPIO_95) {
        rc = INREG32(&s_intr.pGPIORegs[2]->IRQSTATUS_0) & (1 << (irq - IRQ_GPIO_64));    
    } else if (irq <= IRQ_GPIO_127) {
        rc = INREG32(&s_intr.pGPIORegs[3]->IRQSTATUS_0) & (1 << (irq - IRQ_GPIO_96));    
    } else if (irq < IRQ_SW_RESERVED_MAX ) {
        //  SW triggered interrupts only - none will be pending
        rc = FALSE;
    }
    
    return (rc != 0);
}


//------------------------------------------------------------------------------
//
//  Function:  OEMInterruptHandler
//
//  This is interrupt handler implementation.
//
UINT32 OEMInterruptHandler( UINT32 ra )
{
    UINT32 irq = (UINT32)OAL_INTR_IRQ_UNDEFINED;
    UINT32 sysIntr = SYSINTR_NOP;
    UINT32 mask,status;

    if (g_oalILT.active) g_oalILT.interrupts++;

    // Get pending interrupt
    irq = INREG32(&s_intr.pICLRegs->INTC_SIR_IRQ);

	if (irq < MAX_IRQ_COUNT)
		g_IrqCnt[irq]++;

    OALMSG(OAL_INTR&&OAL_FUNC, (L"+OEMInterruptHandler(Irq %d)\r\n", irq));

    if (irq == IRQ_GPIO0A){
        // mask status with irq enabled GPIO's to make sure
        // only interrupts which generated a new interrupt is 
        // handled
        status = INREG32(&s_intr.pGPIORegs[0]->IRQSTATUS_0);
        status &= INREG32(&s_intr.pGPIORegs[0]->IRQSTATUS_SET_0);
        for (irq = IRQ_GPIO_0, mask = 1; mask != 0; mask <<= 1, irq++){
            if ((mask & status) != 0) break;
        }
        OUTPORT32(&s_intr.pGPIORegs[0]->IRQSTATUS_0, mask);
        OUTPORT32(&s_intr.pGPIORegs[0]->IRQSTATUS_1, mask);
        OEMEnableIOPadWakeup((irq - IRQ_GPIO_0), FALSE);
    } else if (irq == IRQ_GPIO1A) {
        // mask status with irq enabled GPIO's to make sure
        // only interrupts which generated a new interrupt is 
        // handled
        status = INREG32(&s_intr.pGPIORegs[1]->IRQSTATUS_0);
        status &= INREG32(&s_intr.pGPIORegs[1]->IRQSTATUS_SET_0);
        for (irq = IRQ_GPIO_32, mask = 1; mask != 0; mask <<= 1, irq++){
            if ((mask & status) != 0) break;
        }
        OUTPORT32(&s_intr.pGPIORegs[1]->IRQSTATUS_0, mask);
        OUTPORT32(&s_intr.pGPIORegs[1]->IRQSTATUS_1, mask);
        OEMEnableIOPadWakeup((irq - IRQ_GPIO_0), FALSE);
    } else if (irq == IRQ_GPIO2A) {
        // mask status with irq enabled GPIO's to make sure
        // only interrupts which generated a new interrupt is 
        // handled
        status = INREG32(&s_intr.pGPIORegs[2]->IRQSTATUS_0);
        status &= INREG32(&s_intr.pGPIORegs[2]->IRQSTATUS_SET_0);
        for (irq = IRQ_GPIO_64, mask = 1; mask != 0; mask <<= 1, irq++){
            if ((mask & status) != 0) break;
        }
        OUTPORT32(&s_intr.pGPIORegs[2]->IRQSTATUS_0, mask);
        OUTPORT32(&s_intr.pGPIORegs[2]->IRQSTATUS_1, mask);
        OEMEnableIOPadWakeup((irq - IRQ_GPIO_0), FALSE);
    } else if (irq == IRQ_GPIO3A) {
        // mask status with irq enabled GPIO's to make sure
        // only interrupts which generated a new interrupt is 
        // handled
        status = INREG32(&s_intr.pGPIORegs[3]->IRQSTATUS_0);
        status &= INREG32(&s_intr.pGPIORegs[3]->IRQSTATUS_SET_0);
        for (irq = IRQ_GPIO_96, mask = 1; mask != 0; mask <<= 1, irq++){
            if ((mask & status) != 0) break;
        }
        OUTPORT32(&s_intr.pGPIORegs[3]->IRQSTATUS_0, mask);
        OUTPORT32(&s_intr.pGPIORegs[3]->IRQSTATUS_1, mask);
        OEMEnableIOPadWakeup((irq - IRQ_GPIO_0), FALSE);
    } else if (irq < 32) {
        SETPORT32(&s_intr.pICLRegs->INTC_MIR0, 1 << irq);
    } else if (irq < 64) {
        SETPORT32(&s_intr.pICLRegs->INTC_MIR1, 1 << (irq - 32));
    } else if (irq < 96) {
        SETPORT32(&s_intr.pICLRegs->INTC_MIR2, 1 << (irq - 64));
    } else if (irq < MAX_IRQ_COUNT) {
        SETPORT32(&s_intr.pICLRegs->INTC_MIR3, 1 << (irq - 96));
    }
    // Acknowledge interrupt 
    OUTREG32(&s_intr.pICLRegs->INTC_CONTROL, IC_CNTL_NEW_IRQ);
    
    // Check if this is profiler IRQ
    if (irq == g_oalPerfTimerIrq && g_oalProfilerEnabled == TRUE){
        OALProfileTimerHit(ra);
    }
    // Check if this is timer IRQ
    if (irq == g_oalTimerIrq){
        if (g_oalILT.active) g_oalILT.interrupts--;
        // Call timer interrupt handler
        sysIntr = OALTimerIntrHandler();
        // re-enable interrupts
        OALIntrDoneIrqs(1, &irq);
    } else if (irq == g_oalPrcmIrq){
        sysIntr = OALPrcmIntrHandler();
        // re-enable interrupts
        if (sysIntr == SYSINTR_NOP) OALIntrDoneIrqs(1, &irq);
    } else if (irq == g_oalSmartReflex1) {
//        sysIntr = OALSmartReflex1Intr();
        // re-enable interrupts
        if (sysIntr == SYSINTR_NOP) OALIntrDoneIrqs(1, &irq);
    } else if (irq == g_oalSmartReflex2) {
//        sysIntr = OALSmartReflex2Intr();
        // re-enable interrupts
        if (sysIntr == SYSINTR_NOP) OALIntrDoneIrqs(1, &irq);
    } else if (irq == g_oalM3Irq) {
        sysIntr = PrcmCM3Isr();
        // re-enable interrupts
        if (sysIntr == SYSINTR_NOP) OALIntrDoneIrqs(1, &irq);
    } else if (irq != OAL_INTR_IRQ_UNDEFINED) {
        // We don't assume IRQ sharing, use static mapping
        sysIntr = OALIntrTranslateIrq(irq);
    }

    return sysIntr;
}

//------------------------------------------------------------------------------
