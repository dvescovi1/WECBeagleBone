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
//  File: intr.c
//
//  This file implement major part of interrupt module for OMAP7xx SoC.
//
#include "omap.h"
#include <oal.h>
#include "oalex.h"
#include "oal_clock.h"
#include <nkintr.h>
#include "soc_cfg.h"
#include "bsp_cfg.h"
#include "intr.h"
#include "omap_gpio_regs.h"
#include "omap_intc_regs.h"
#include "omap_prcm_regs.h"
#include "interrupt_struct.h"
#include "oal_intrex.h"
#ifdef OAL
#include "oal_alloc.h"
#endif
#include <cmnintrin.h>


//------------------------------------------------------------------------------
//
//  Global: g_oalTimerIrq 
//
UINT32 g_oalTimerIrq = (UINT32) OAL_INTR_IRQ_UNDEFINED;

//------------------------------------------------------------------------------
//
//  Global: g_oalPrcmIrq 
//
UINT32 g_oalPrcmIrq = (UINT32) OAL_INTR_IRQ_UNDEFINED;

//------------------------------------------------------------------------------
//
//  Global: g_oalSmartReflex1 
//
UINT32 g_oalSmartReflex1 = (UINT32) OAL_INTR_IRQ_UNDEFINED;

//------------------------------------------------------------------------------
//
//  Global: g_oalSmartReflex2 
//
UINT32 g_oalSmartReflex2 = (UINT32) OAL_INTR_IRQ_UNDEFINED;


//------------------------------------------------------------------------------
//
//  Static: s_intr
//
//  This value contains virtual uncached address of interrupt controller
//  unit registers.
//
static OMAP_INTR_CONTEXT  s_intr;

//------------------------------------------------------------------------------
//
//  Static: g_pIntr
//
//  exposes pointer to interrupt structure.
//
OMAP_INTR_CONTEXT const *g_pIntr = &s_intr;

//------------------------------------------------------------------------------
//// g_BSP_icL1Level contains the priority and levl configuration for the interrupt
extern const UINT32 g_BSP_icL1Level[];

INTR_GPIO_CTXT* GetGPIOCtxtByIrq(DWORD irq)
{
    DWORD i;
    for (i=0;i<s_intr.nbGpioBank;i++)
    {        
        if ((s_intr.pGpioCtxt[i].irq_start <= irq) && (irq < (s_intr.pGpioCtxt[i].irq_start+32)))
        {
            return &s_intr.pGpioCtxt[i];
        }
    }
    return NULL;
}
static _inline DWORD OALGPIOIntrHandler(DWORD irq)
{    
    DWORD i;
    for (i=0;i<s_intr.nbGpioBank;i++)
    {
        if (irq == s_intr.pGpioCtxt[i].bank_irq)
        {
            register OMAP_GPIO_REGS* pRegs = s_intr.pGpioCtxt[i].pRegs;
            register DWORD status;
            register DWORD index;
            register DWORD mask;
            status = INREG32(&pRegs->IRQSTATUS1);
            status &= INREG32(&pRegs->IRQENABLE1);
            index = 31 - _CountLeadingZeros(status);
            mask = 1 << (index);
            OUTPORT32(&pRegs->IRQSTATUS1, mask);
            OUTPORT32(&pRegs->IRQSTATUS2, mask);
            OUTPORT32(&pRegs->CLEARIRQENABLE1, mask);
            OUTPORT32(&pRegs->CLEARWAKEUPENA, mask);

            irq = s_intr.pGpioCtxt[i].irq_start + index;
            OEMEnableIOPadWakeup((irq - BSPGetGpioIrq(0)), FALSE);

        }
    }
    return irq;
}
static _inline void OALGPIOEnableIRQ(DWORD irq)
{
    register INTR_GPIO_CTXT* ctxt = GetGPIOCtxtByIrq(irq);
    if (ctxt)
    {
//        EnableDeviceClocks(ctxt->device, TRUE);  // This function uses a reference counter.
		if (ctxt->bank_irq < 32)
		{
	        OUTREG32(&s_intr.pICLRegs->INTC_MIR_CLEAR0, 1 << (ctxt->bank_irq % 32));
		}
		else if (ctxt->bank_irq < 64)
		{
	        OUTREG32(&s_intr.pICLRegs->INTC_MIR_CLEAR1, 1 << (ctxt->bank_irq % 32));
		}
		else
		{
	        OUTREG32(&s_intr.pICLRegs->INTC_MIR_CLEAR2, 1 << (ctxt->bank_irq % 32));
		}

        OUTREG32(&ctxt->pRegs->SETIRQENABLE1, 1<<(irq - ctxt->irq_start));   // unmask IRQ
        OUTREG32(&ctxt->pRegs->SETWAKEUPENA,  1<<(irq - ctxt->irq_start));        
        OEMEnableIOPadWakeup((irq - BSPGetGpioIrq(0)), TRUE);
    }
}
static _inline void OALGPIODisableIRQ(DWORD irq)
{
    register INTR_GPIO_CTXT* ctxt = GetGPIOCtxtByIrq(irq);
    if (ctxt)
    {
        OUTREG32(&ctxt->pRegs->CLEARIRQENABLE1, 1<<(irq - ctxt->irq_start));   // mask IRQ
        OUTREG32(&ctxt->pRegs->CLEARWAKEUPENA,  1<<(irq - ctxt->irq_start));        
        OEMEnableIOPadWakeup((irq - BSPGetGpioIrq(0)), FALSE);

//        EnableDeviceClocks(ctxt->device, FALSE); // This function uses a reference counter.
    }
}

static _inline void OALGPIODoneIRQ(DWORD irq)
{
    register INTR_GPIO_CTXT* ctxt = GetGPIOCtxtByIrq(irq);
    if (ctxt)
    {
        OUTREG32(&ctxt->pRegs->SETIRQENABLE1, 1<<(irq - ctxt->irq_start));   // unmask IRQ
        OUTREG32(&ctxt->pRegs->SETWAKEUPENA,  1<<(irq - ctxt->irq_start));
        OEMEnableIOPadWakeup((irq - BSPGetGpioIrq(0)), TRUE);
    }
}

static _inline BOOL OALGPIOIsIrqPending(DWORD irq)
{
    BOOL rc = FALSE;
    register INTR_GPIO_CTXT* ctxt = GetGPIOCtxtByIrq(irq);
    if (ctxt)
    {
        rc = INREG32(&ctxt->pRegs->IRQSTATUS1) & (1 << (irq - ctxt->irq_start));           
    }
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  OALIntrInit
//
//  This function initialize OMAP interrupt subsystem. Implementation must
//  use its own mapping structure because general implementation limits
//  number of IRQ to 64 but OMAP35XX has 320 IRQs.
//
BOOL OALIntrInit()
{
    BOOL rc = FALSE;
    UINT32 i, mask;    

    OALMSG(OAL_INTR&&OAL_FUNC, (L"+OALInterruptInit\r\n"));

    // Initialize interrupt mapping
    OALIntrMapInit();

    // Get interrupt controller and GPIO registers' virtual uncached addresses
    memset(&s_intr,0,sizeof(s_intr));
    s_intr.pICLRegs = OALPAtoUA(SOCGetIntCtrlAddr());
    i=1;
    while (SOCGetGPIODeviceByBank(i++) != OMAP_DEVICE_NONE)
    {        
        s_intr.nbGpioBank++; 
    }
    s_intr.pGpioCtxt = OALLocalAlloc(LPTR,sizeof(INTR_GPIO_CTXT)*s_intr.nbGpioBank);
    if (s_intr.pGpioCtxt == NULL)
    {
        ERRORMSG(1,(TEXT("unable to allocate interrupt for GPIO structure\r\n")));
        return FALSE;
    }
    for (i=0;i<s_intr.nbGpioBank;i++)
    {        
        OMAP_DEVICE device = SOCGetGPIODeviceByBank(i+1);
        s_intr.pGpioCtxt[i].pRegs = OALPAtoUA(GetAddressByDevice(device));
        s_intr.pGpioCtxt[i].device = device;
        s_intr.pGpioCtxt[i].irq_start = BSPGetGpioIrq(i*32);
        s_intr.pGpioCtxt[i].bank_irq = GetIrqByDevice(device,NULL);
        s_intr.pGpioCtxt[i].padWakeupEvent = 0;
    }

    //Reset the MPU INTC and wait until reset is complete
    SETREG32(&s_intr.pICLRegs->INTC_SYSCONFIG, SYSCONFIG_SOFTRESET);
    while ((INREG32(&s_intr.pICLRegs->INTC_SYSSTATUS) & SYSSTATUS_RESETDONE) == 0);

    // Disable auto-idle for the interrupt controller
    CLRREG32(&s_intr.pICLRegs->INTC_SYSCONFIG, SYSCONFIG_AUTOIDLE);

    //Disable all interrupts and clear the ISR - for all for GPIO banks, too
    OUTREG32(&s_intr.pICLRegs->INTC_MIR_SET0, OMAP_MPUIC_MASKALL);
    OUTREG32(&s_intr.pICLRegs->INTC_MIR_SET1, OMAP_MPUIC_MASKALL);
    OUTREG32(&s_intr.pICLRegs->INTC_MIR_SET2, OMAP_MPUIC_MASKALL);

    // we supose that the GPIO locks are are always running
    //// enable gpio clocks
    //for (i=0;i<s_intr.nbGpioBank;i++)
    //{        
    //    EnableDeviceClocks(s_intr.pGpioCtxt[i].device,TRUE);
    //}

    //Reset and Disable interrupt/wakeup for all GPIOs
    for (i=0;i<s_intr.nbGpioBank;i++)
    {
        //Disable interrupt/wakeup
        OUTREG32(&s_intr.pGpioCtxt[i].pRegs->IRQENABLE1, 0x00000000);
        OUTREG32(&s_intr.pGpioCtxt[i].pRegs->IRQENABLE2, 0x00000000);
        OUTREG32(&s_intr.pGpioCtxt[i].pRegs->WAKEUPENABLE, 0x00000000);

        // clear irq status bits
        mask = INREG32(&s_intr.pGpioCtxt[i].pRegs->IRQSTATUS1);
        OUTREG32(&s_intr.pGpioCtxt[i].pRegs->IRQSTATUS1, mask);

        mask = INREG32(&s_intr.pGpioCtxt[i].pRegs->IRQSTATUS2);
        OUTREG32(&s_intr.pGpioCtxt[i].pRegs->IRQSTATUS2, mask);

        //Enable smart and auto idle for GPIO 
        //(We don't need to set INTC since INTC is always in smart mode)
        // Why is the interrupt subsystem controlling the GPIO subsystem clocks?
        OUTREG32(&s_intr.pGpioCtxt[i].pRegs->SYSCONFIG, 
            SYSCONFIG_AUTOIDLE| SYSCONFIG_ENAWAKEUP | SYSCONFIG_SMARTIDLE
            );       
    }
    // clear any possible pending interrupts
    INREG32(&s_intr.pICLRegs->INTC_SIR_IRQ);
    INREG32(&s_intr.pICLRegs->INTC_SIR_FIQ);
    //Initialize interrupt routing, level and priority
    for (i = 0; i < 96; i++)
    {
        OUTREG32(&s_intr.pICLRegs->INTC_ILR[i], g_BSP_icL1Level[i]);
    }
    //Call board specific initializatrion
    rc = BSPIntrInit();

    //// disable gpio clocks
    //for (i=0;i<s_intr.nbGpioBank;i++)
    //{        
    //    EnableDeviceClocks(s_intr.pGpioCtxt[i].device,FALSE);
    //}

    // Finally enable the interrupts for the GPIO controllers
    for (i=0;i<s_intr.nbGpioBank;i++)
    {        
        OALIntrEnableIrqs(1,&s_intr.pGpioCtxt[i].bank_irq);
    }
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
BOOL
OALIntrRequestIrqs(
                   DEVICE_LOCATION *pDevLoc,
                   UINT32 *pCount,
                   UINT32 *pIrqs
                   )
{
    BOOL rc = FALSE;

    OALMSG(OAL_INTR&&OAL_FUNC, (
        L"+OALIntrRequestIrqs(0x%08x->%d/%d/0x%08x/%d, 0x%08x, 0x%08x)\r\n",
        pDevLoc, pDevLoc->IfcType, pDevLoc->BusNumber, pDevLoc->LogicalLoc,
        pDevLoc->Pin, pCount, pIrqs
        ));

    // This shouldn't happen
    if (*pCount < 1) goto cleanUp;

    switch (pDevLoc->IfcType)
    {
    case Internal:
        /*
        switch ((ULONG)pDevLoc->LogicalLoc)
        {
        case OMAP35XX_MCSPI1_REGS_PA:
        *pCount = 1;
        pIrqs[0] = IRQ_SPI1;
        rc = TRUE;
        break;
        case OMAP35XX_MCSPI2_REGS_PA:
        *pCount = 1;
        pIrqs[0] = IRQ_SPI2;
        rc = TRUE;
        break;
        case OMAP35XX_UART1_REGS_PA:
        *pCount = 1;
        pIrqs[0] = IRQ_UART1;
        rc = TRUE;
        break;
        case OMAP35XX_UART2_REGS_PA:
        *pCount = 1;
        pIrqs[0] = IRQ_UART2;
        rc = TRUE;
        break;
        case OMAP35XX_UART3_REGS_PA:
        *pCount = 1;
        pIrqs[0] = IRQ_UART3;
        rc = TRUE;
        break;
        }
        */
        break;
    }

    if (!rc) rc = BSPIntrRequestIrqs(pDevLoc, pCount, pIrqs);

cleanUp:
    OALMSG(OAL_INTR&&OAL_FUNC, (L"-OALIntrRequestIrqs(rc = %d)\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  OALIntrSetDataIrqs
//
void
OALIntrSetDataIrqs(
    UINT32 count,
    const UINT32 *pIrqs, 
    LPVOID pvData, 
    DWORD cbData
    )
{
    UINT32 i;

    OALMSG(OAL_INTR&&OAL_VERBOSE, (
        L"+OALIntrSetDataIrqs(%d, 0x%08x, 0x%08X, %d)\r\n", 
        count, pIrqs, pvData, cbData
        ));

    for (i = 0; i < count; i++)
        {
        if (96 < pIrqs[i] && pIrqs[i] < IRQ_SW_RESERVED_MAX)
            {
            // call software irq handler
            OALSWIntrSetDataIrq(pIrqs[i], pvData, cbData);
            }
        }

    OALMSG(OAL_INTR&&OAL_VERBOSE, (L"-OALIntrSetDataIrqs()\r\n"));
}

//------------------------------------------------------------------------------
//
//  Function:  OALIntrEnableIrqs
//
BOOL
OALIntrEnableIrqs(
                  UINT32 count,
                  const UINT32 *pIrqs
                  )
{
    BOOL rc = FALSE;
    UINT32 irq, i;

    OALMSG(OAL_INTR&&OAL_VERBOSE, (
        L"+OALntrEnableIrqs(%d, 0x%08x)\r\n", count, pIrqs
        ));

    for (i = 0; i < count; i++)
    {
        irq = pIrqs[i];

        if (irq != OAL_INTR_IRQ_UNDEFINED)
        {
            if (irq < 32)
            {
                OUTREG32(&s_intr.pICLRegs->INTC_MIR_CLEAR0, 1 << irq);
            }
            else if (irq < 64)
            {
                OUTREG32(&s_intr.pICLRegs->INTC_MIR_CLEAR1, 1 << (irq - 32));
            }
            else if (irq < 96)
            {
                OUTREG32(&s_intr.pICLRegs->INTC_MIR_CLEAR2, 1 << (irq - 64));
            }
            else if (irq < IRQ_SW_RESERVED_MAX )
            {
                // call software irq handler
                OALSWIntrEnableIrq(irq);
            }
            else 
            {
                OALGPIOEnableIRQ(irq);
            }
            rc = TRUE;
        }

    }

    OALMSG(OAL_INTR&&OAL_VERBOSE, (L"-OALIntrEnableIrqs(rc = %d)\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  OALIntrDisableIrqs
//
VOID
OALIntrDisableIrqs(
                   UINT32 count,
                   const UINT32 *pIrqs
                   )
{
    UINT32 irq, i;

    OALMSG(OAL_INTR&&OAL_VERBOSE, (
        L"+OALIntrDisableIrqs(%d, 0x%08x)\r\n", count, pIrqs
        ));

    for (i = 0; i < count; i++)
    {
        irq = pIrqs[i];

        if (irq != OAL_INTR_IRQ_UNDEFINED)
        {            
            if (irq < 32)
            {
                OUTREG32(&s_intr.pICLRegs->INTC_MIR_SET0, 1 << irq);
            }
            else if (irq < 64)
            {
                OUTREG32(&s_intr.pICLRegs->INTC_MIR_SET1, 1 << (irq - 32));
            }
            else if (irq < 96)
            {
                OUTREG32(&s_intr.pICLRegs->INTC_MIR_SET2, 1 << (irq - 64));
            }
            else if (irq < IRQ_SW_RESERVED_MAX )
            {
                // call software irq handler
                OALSWIntrDisableIrq(irq);
            }
            else 
            {
                OALGPIODisableIRQ(irq);
            }
        }
    }

    OALMSG(OAL_INTR&&OAL_VERBOSE, (L"-OALIntrDisableIrqs\r\n"));
}


//------------------------------------------------------------------------------
//
//  Function:  OALIntrDoneIrqs
//
VOID
OALIntrDoneIrqs(
                UINT32 count,
                const UINT32 *pIrqs
                )
{
    BOOL rc = FALSE;
    UINT32 irq, i;

    OALMSG(OAL_INTR&&OAL_VERBOSE, (
        L"+OALIntrDoneIrqs(%d, 0x%08x)\r\n", count, pIrqs
        ));

    for (i = 0; i < count; i++)
    {
        irq = pIrqs[i];

        if (irq != OAL_INTR_IRQ_UNDEFINED)
        {  
            if (irq < 32)
            {
                OUTREG32(&s_intr.pICLRegs->INTC_MIR_CLEAR0, 1 << irq);
            }
            else if (irq < 64)
            {
                OUTREG32(&s_intr.pICLRegs->INTC_MIR_CLEAR1, 1 << (irq - 32));
            }
            else if (irq < 96)
            {
                OUTREG32(&s_intr.pICLRegs->INTC_MIR_CLEAR2, 1 << (irq - 64));
            }
            else if (irq < IRQ_SW_RESERVED_MAX )
            {
                // call software irq handler
                OALSWIntrDoneIrq(irq);
            }
            else 
            {
                OALGPIODoneIRQ(irq);
            }
            rc = TRUE;
        }
    }

    OALMSG(OAL_INTR&&OAL_VERBOSE, (L"-OALIntrDoneIrqs\r\n"));
}


//------------------------------------------------------------------------------
//
//  Function:  OALIntrIsIrqPending
//
//  This function checks if the given interrupt is pending.
//
BOOL
OALIntrIsIrqPending(
                    UINT32 irq
                    )
{
    BOOL rc = FALSE;

    if (irq < 32)
    {
        rc = INREG32(&s_intr.pICLRegs->INTC_ITR0) & (1 << irq);
    }
    else if (irq < 64)
    {
        rc = INREG32(&s_intr.pICLRegs->INTC_ITR1) & (1 << (irq - 32));
    }
    else if (irq < 96)
    {
        rc = INREG32(&s_intr.pICLRegs->INTC_ITR2) & (1 << (irq - 64));
    }
    else if (irq < IRQ_SW_RESERVED_MAX )
    {
        //  SW triggered interrupts only - none will be pending
        rc = FALSE;
    }
    else 
    {
        rc = OALGPIOIsIrqPending(irq);
    }

    return (rc != 0);
}

//------------------------------------------------------------------------------
//
//  Function:  OEMInterruptHandler
//
//  This is interrupt handler implementation.
//
UINT32
OEMInterruptHandler(
                    UINT32 ra
                    )
{
    UINT32 irq = (UINT32) OAL_INTR_IRQ_UNDEFINED;
    UINT32 sysIntr = SYSINTR_NOP;

    // Get pending interrupt
    irq = INREG32(&s_intr.pICLRegs->INTC_SIR_IRQ);

#ifdef OAL_ILTIMING
        if (g_oalILT.active) 
            g_oalILT.interrupts ++;        
#endif

    OALMSG(OAL_INTR, (L"OEMInterruptHandler(Irq %d)\r\n", irq));

    irq = OALGPIOIntrHandler(irq); //Check if this is a GPIO irq. In so, then translate the irq number
    if (irq < 32) 
    {
        SETPORT32(&s_intr.pICLRegs->INTC_MIR0, 1 << irq);
    } 
    else if (irq < 64) 
    {
        SETPORT32(&s_intr.pICLRegs->INTC_MIR1, 1 << (irq - 32));
    }
    else if (irq < 96) 
    {
        SETPORT32(&s_intr.pICLRegs->INTC_MIR2, 1 << (irq - 64));
    }
    else
    {
        //...
    }

    // Acknowledge interrupt 
    OUTREG32(&s_intr.pICLRegs->INTC_CONTROL, IC_CNTL_NEW_IRQ);

    // Check if this is profiler IRQ
    if (irq == g_oalPerfTimerIrq && g_oalProfilerEnabled == TRUE)
    {
        OALProfileTimerHit(ra);
    }

    // Check if this is timer IRQ
    if (irq == g_oalTimerIrq)
    {        

        if (g_oalILT.active)
        {
            g_oalILT.interrupts--;
        }
        // Call timer interrupt handler
        sysIntr = OALTimerIntrHandler();

        // re-enable interrupts
        OALIntrDoneIrqs(1, &irq);
    }
    else if (irq == g_oalPrcmIrq)
        {
        // call prcm interrupt handler
        sysIntr = OALPrcmIntrHandler();

        if (sysIntr != SYSINTR_NOP)
            {
            // sysIntr is a the GPIO irq number for which IO PAD
            // event occured, get the sysIntr for the irq
            sysIntr = OALIntrTranslateIrq(sysIntr);
            }

        OALIntrDoneIrqs(1, &irq);
     }
    else if (irq == g_oalSmartReflex1)
    {
        // call prcm interrupt handler
        sysIntr = OALSmartReflex1Intr();
    }
    else if (irq == g_oalSmartReflex2)
    {
        // call prcm interrupt handler
        sysIntr = OALSmartReflex2Intr();
    }
    else if (irq != OAL_INTR_IRQ_UNDEFINED)
    {
        // We don't assume IRQ sharing, use static mapping
		// If needed, implement BSP level interrupt management here
		sysIntr = OALIntrTranslateIrq(irq);
    }
    // re-enable interrupts
    if (sysIntr == SYSINTR_NOP)
    {
        OALIntrDoneIrqs(1, &irq);
    }

    return sysIntr;
}

//------------------------------------------------------------------------------
