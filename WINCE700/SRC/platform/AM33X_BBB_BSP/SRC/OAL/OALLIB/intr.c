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
//  File:  intr.c
//
//  Interrupt BSP callback functions. OMAP SoC has one external interrupt
//  pin. In general solution it can be used for cascade interrupt controller.
//  However in most cases it will be used as simple interrupt input. To avoid
//  unnecessary penalty not all BSPIntrXXXX functions are called by default
//  interrupt module implementation.
//
//
#include <bsp.h>
#include "am33x_base_regs.h"
#include "am33x_irq.h"
//#include <oal_prcm.h>
//#include <oal_sr.h>

//------------------------------------------------------------------------------
//  This variable is defined in interrupt module and it is used in interrupt
//  handler to distinguish prcm interrupt.
extern UINT32                   g_oalPrcmIrq;
extern UINT32                   g_oalM3SysIntr;

//------------------------------------------------------------------------------
//  This variable is defined in interrupt module and it is used in interrupt
//  handler to distinguish SmartReflex1 interrupt.
extern UINT32                   g_oalSmartReflex1;

//------------------------------------------------------------------------------
//  This variable is defined in interrupt module and it is used in interrupt
//  handler to distinguish SmartReflex2 interrupt.
extern UINT32                   g_oalSmartReflex2;

extern UINT32 g_oalM3Irq;

#if 0
//------------------------------------------------------------------------------
//
//  Function:  RegisterSmartReflex1InterruptHandler
//
//  This function registers with the interrupt handler for the 
//  SmartReflex1 interrupt.
//
BOOL
RegisterSmartReflex1InterruptHandler()
{
    UINT32 sysIntr;
    BOOL rc = FALSE;
    
    // enable PRCM iterrupts
    g_oalSmartReflex1 = IRQ_SR1;

    // clear status
    SmartReflex_ClearInterruptStatus(kSmartReflex_Channel1, 
        ERRCONFIG_INTR_SR_MASK
        );
    
    // Request SYSINTR for timer IRQ, it is done to reserve it...
    sysIntr = OALIntrRequestSysIntr(1, &g_oalSmartReflex1, OAL_INTR_FORCE_STATIC);

    // Enable System Tick interrupt
    if (!OEMInterruptEnable(sysIntr, NULL, 0))
        {
        OALMSG(OAL_ERROR, (
            L"ERROR: RegisterSmartReflex1InterruptHandler: "
            L"Interrupt enable for SmartReflex1 failed"
            ));
        goto cleanUp;
        }

    // enable smartreflex interrupts
    SmartReflex_EnableInterrupts(kSmartReflex_Channel1,
        ERRCONFIG_VP_BOUNDINT_EN,
        TRUE
        );
        
    rc = TRUE;

cleanUp:
    return rc;
}


//------------------------------------------------------------------------------
//
//  Function:  RegisterSmartReflex2InterruptHandler
//
//  This function registers with the interrupt handler for the 
//  SmartReflex2 interrupt.
//
BOOL
RegisterSmartReflex2InterruptHandler()
{
    UINT32 sysIntr;
    BOOL rc = FALSE;
    
    // enable PRCM iterrupts
    g_oalSmartReflex2 = IRQ_SR2;

    // clear status
    SmartReflex_ClearInterruptStatus(kSmartReflex_Channel2, 
        ERRCONFIG_INTR_SR_MASK
        );
    
    // Request SYSINTR for timer IRQ, it is done to reserve it...
    sysIntr = OALIntrRequestSysIntr(1, &g_oalSmartReflex2, OAL_INTR_FORCE_STATIC);

    // Enable System Tick interrupt
    if (!OEMInterruptEnable(sysIntr, NULL, 0))
        {
        OALMSG(OAL_ERROR, (
            L"ERROR: RegisterSmartReflex2InterruptHandler: "
            L"Interrupt enable for SmartReflex2 failed"
            ));
        goto cleanUp;
        }

    SmartReflex_EnableInterrupts(kSmartReflex_Channel2,
        ERRCONFIG_VP_BOUNDINT_EN,
        TRUE
        );

    rc = TRUE;

cleanUp:
    return rc;
}
#endif
#if 0
//------------------------------------------------------------------------------
//
//  Function:  RegisterPrcmInterruptHandler
//
//  This function registers with the interrupt handler for the prcm interrupt.
//
BOOL
RegisterPrcmInterruptHandler()
{
    UINT32 sysIntr;
    BOOL rc = FALSE;
    
    // enable PRCM iterrupts
    g_oalPrcmIrq = IRQ_PRCM_MPU;

    // clear status
    PrcmInterruptClearStatus(0xFFFFFFFF);

    // Request SYSINTR for timer IRQ, it is done to reserve it...
    sysIntr = OALIntrRequestSysIntr(1, &g_oalPrcmIrq, OAL_INTR_FORCE_STATIC);

    // Enable System Tick interrupt
    if (!OEMInterruptEnable(sysIntr, NULL, 0))
        {
        OALMSG(OAL_ERROR, (
            L"ERROR: RegisterPrcmInterruptHandler: "
            L"Interrupt enable for PRCM failed"
            ));
        goto cleanUp;
        }

    // enable some prcm interrupts, others should be enabled as needed
    PrcmInterruptEnable(
        //PRM_IRQENABLE_WKUP_EN |
        PRM_IRQENABLE_MPU_DPLL_RECAL_EN |
        PRM_IRQENABLE_CORE_DPLL_RECAL_EN |
        //PRM_IRQENABLE_PERIPH_DPLL_RECAL_EN |
        //PRM_IRQENABLE_IVA2_DPLL_RECAL_EN_MPU |
        //PRM_IRQENABLE_SND_PERIPH_DPLL_RECAL_EN |
        PRM_IRQENABLE_VP1_OPPCHANGEDONE_EN |
        PRM_IRQENABLE_VP2_OPPCHANGEDONE_EN |
        PRM_IRQENABLE_VP1_NOSMPSACK_EN |
        PRM_IRQENABLE_VP2_NOSMPSACK_EN |
        PRM_IRQENABLE_VC_SAERR_EN |
        PRM_IRQENABLE_VC_RAERR_EN |
        PRM_IRQENABLE_VC_TIMEOUTERR_EN |
        PRM_IRQENABLE_IO_EN ,
        TRUE
        );
   
    rc = TRUE;

cleanUp:
    return rc;
}
#endif


//------------------------------------------------------------------------------
//
//  Function:  RegisterPrcmInterruptHandler
//
//  This function registers with the interrupt handler for the prcm interrupt.
//
BOOL
RegisterPrcmCM3InterruptHandler()
{
    BOOL rc = FALSE;
    
    // enable PRCM iterrupts
    g_oalM3Irq = IRQ_M3_TXEV;

    // Request SYSINTR for M3 IRQ, it is done to reserve it...
    g_oalM3SysIntr = OALIntrRequestSysIntr(1, &g_oalM3Irq, OAL_INTR_FORCE_STATIC);

    // Enable M3 interrupt
    if (!OEMInterruptEnable(g_oalM3SysIntr, NULL, 0))
        {
        OALMSG(OAL_ERROR, (
            L"ERROR: RegisterPrcmCM3InterruptHandler: "
            L"Interrupt enable for M3 failed"
            ));
        goto cleanUp;
        }

    rc = TRUE;

cleanUp:
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  BSPIntrInit
//
//  This function is called from OALIntrInit to initialize on-board secondary
//  interrupt controller if exists. As long as GPIO interrupt edge registers
//  are initialized in startup.s code function is stub only.
//
BOOL BSPIntrInit()
{    
    return TRUE;
}


//------------------------------------------------------------------------------
//
//  Function:  BSPIntrRequestIrqs
//
//  This function is called from OALIntrRequestIrq to obtain IRQ for on-board
//  devices if exists.
//
BOOL
BSPIntrRequestIrqs(
    DEVICE_LOCATION *pDevLoc, 
    UINT32 *pCount, 
    UINT32 *pIrq
    )
{
    BOOL rc = FALSE;

    OALMSG(OAL_INTR&&OAL_FUNC, (
        L"+BSPIntrRequestIrq(0x%08x->%d/%d/0x%08x/%d, 0x%08x)\r\n", pDevLoc, 
        pDevLoc->IfcType, pDevLoc->BusNumber, pDevLoc->LogicalLoc,
        pDevLoc->Pin, pIrq
        ));

    switch (pDevLoc->IfcType)
        {
        case Internal:
            switch ((ULONG)pDevLoc->LogicalLoc){
                case AM33X_EMACSW_REGS_PA:
					OALMSG(OAL_INTR&&OAL_FUNC, (L"CODDING ERROR: BSPIntrRequestIrq - intr.c \r\n", rc));
                    *pCount = 1;
//                    *pIrq = IRQ_MACRXINT0;
                    rc = FALSE/*TRUE*/;
                    break;
            }
            break;
        }

    OALMSG(OAL_INTR&&OAL_FUNC, (L"-BSPIntrRequestIrq(rc = %d)\r\n", rc));

    return rc;
}

//------------------------------------------------------------------------------

