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
//  File:  power.c
//

#pragma warning(push)
#pragma warning(disable: 4214 4201 4115)
#include <windows.h>
#include <nkintr.h>
#include <oal.h>
#pragma warning(pop)

#include <omap.h>
#include <oalex.h>
#include <oal_clock.h>
#include <omap_gpio_regs.h>
#include <omap_intc_regs.h>
#include <interrupt_struct.h>

//------------------------------------------------------------------------------
//
//  External functions
//
extern DWORD SOCGetGPIODeviceByBank(DWORD index);
extern BOOL OALIntrIsIrqPending(UINT32 irq);
extern INTR_GPIO_CTXT* GetGPIOCtxtByIrq(DWORD irq);

//------------------------------------------------------------------------------
//
//  External:  g_pIntr
//
//  contains gpio and interrupt information.  Initialized in OALIntrInit()
//
extern OMAP_INTR_CONTEXT const *g_pIntr;

//------------------------------------------------------------------------------
//
//  Function:  OEMInterruptPending
//
//  This function returns true when given sysIntr interrupt is pending.
//
BOOL OEMInterruptPending(DWORD sysIntr)
{
    BOOL pending = FALSE;
    const UINT32 *pIrqs;
    UINT32 ix, count;

    if (OALIntrTranslateSysIntr(sysIntr, &count, &pIrqs))
    {
        for (ix = 0; ix < count && !pending; ix++)
        {
            pending = OALIntrIsIrqPending(pIrqs[ix]);
        }            
    }
        
    return pending;
}

//------------------------------------------------------------------------------
//
//  Function:  OEMPowerOff
//
//  Called when the system is to transition to it's lowest power mode (off)
//
void OEMPowerOff()
{
#if 0

	DWORD  i		  = 0;
	UINT32 sysIntr;
    UINT32 intr[3];
	UINT32 Count;
	UINT32* pIrqs;
	DWORD  deviceID;
	INTR_GPIO_CTXT* ctxt;

    // Give chance to do board specific stuff
    BSPPowerOff();

	// Save all interrupts masks
    intr[0] = INREG32(&g_pIntr->pICLRegs->INTC_MIR0);
    intr[1] = INREG32(&g_pIntr->pICLRegs->INTC_MIR1);
    intr[2] = INREG32(&g_pIntr->pICLRegs->INTC_MIR2);

	// Disable all interrupts
    OUTREG32(&g_pIntr->pICLRegs->INTC_MIR0, OMAP_MPUIC_MASKALL);
    OUTREG32(&g_pIntr->pICLRegs->INTC_MIR1, OMAP_MPUIC_MASKALL);
    OUTREG32(&g_pIntr->pICLRegs->INTC_MIR2, OMAP_MPUIC_MASKALL);

	// Save state then mask all GPIO interrupts
	for (i=0; i<g_pIntr->nbGpioBank; i++)
    {
		INTR_GPIO_CTXT* pCurrGpioCtxt = &g_pIntr->pGpioCtxt[i];		

		// Enable Bank clocks first
		EnableDeviceClocks(pCurrGpioCtxt->device, TRUE);

		// Save current state
		pCurrGpioCtxt->restoreCtxt.IRQENABLE1 = INREG32(&pCurrGpioCtxt->pRegs->IRQENABLE1);
		pCurrGpioCtxt->restoreCtxt.WAKEUPENABLE = INREG32(&pCurrGpioCtxt->pRegs->WAKEUPENABLE);

		// Disable all GPIO interrupts in the bank
        OUTREG32(&pCurrGpioCtxt->pRegs->IRQENABLE1, 0);
        OUTREG32(&pCurrGpioCtxt->pRegs->WAKEUPENABLE, 0);

		// Disable Bank clocks
		EnableDeviceClocksNoRefCount(pCurrGpioCtxt->device, FALSE);
	}

    // Enable wakeup sources interrupts
    for (sysIntr = SYSINTR_DEVICES; sysIntr < SYSINTR_MAXIMUM; sysIntr++)
    {
        // Skip if sysIntr isn't allowed as wake source
        if (!OALPowerWakeSource(sysIntr)) continue;

		// Retrieve IRQs
		OALIntrTranslateSysIntr(sysIntr, &Count, &pIrqs);

		// Loop into the IRQs
		for (i=0; i<Count; i++)
		{
			// We must handle the special case of the abstracted GPIO interrupts
			ctxt = GetGPIOCtxtByIrq(pIrqs[i]);
			if (ctxt)
			{
				// This is a GPIO interrupt, we need to enable its bank interrupt
				OALIntrEnableIrqs(1, &ctxt->bank_irq);

				// Store its bank device ID for enabling its clocks
				deviceID = ctxt->device;
			}
			else
			{
				// This is NOT a GPIO interrupt, so we deduce the deviceid directly from its irq
				deviceID = GetDeviceByIrq(pIrqs[i]);
			}

			// Enable Clocks for this device
			EnableDeviceClocksNoRefCount(deviceID, TRUE);
		}

        // Enable it as interrupt
        OEMInterruptEnable(sysIntr, NULL, 0);
  }

	// Go to suspend mode
	//OALGoToSuspendMode();

	/*
		Sleeping .... until waking up
	*/

	// Find out about the wake up source
    for (sysIntr = SYSINTR_DEVICES; sysIntr < SYSINTR_MAXIMUM; sysIntr++)
    {            
		// Skip if sysIntr isn't allowed as wake source
		if (!OALPowerWakeSource(sysIntr)) continue;

		// When this sysIntr is pending we find wake source
		if (OEMInterruptPending(sysIntr))
        {
			g_oalWakeSource = sysIntr;
			break;
        }
    }

	// Put GPIO interrupt state back to the way it was before suspend
	for (i=0; i<g_pIntr->nbGpioBank; i++)
    {		
		INTR_GPIO_CTXT* pCurrGpioCtxt = &g_pIntr->pGpioCtxt[i];

		// Enable Bank clocks first
		EnableDeviceClocksNoRefCount(pCurrGpioCtxt->device, TRUE);

		// Write registers with the previously saved values
		OUTREG32(&pCurrGpioCtxt->pRegs->IRQENABLE1, pCurrGpioCtxt->restoreCtxt.IRQENABLE1);
		OUTREG32(&pCurrGpioCtxt->pRegs->WAKEUPENABLE, pCurrGpioCtxt->restoreCtxt.WAKEUPENABLE);

		// Disable Bank clocks
		EnableDeviceClocks(pCurrGpioCtxt->device, FALSE);
	}

	// restore inetrrupt masks
    OUTREG32(&g_pIntr->pICLRegs->INTC_MIR0, intr[0]);
    OUTREG32(&g_pIntr->pICLRegs->INTC_MIR1, intr[1]);
    OUTREG32(&g_pIntr->pICLRegs->INTC_MIR2, intr[2]);  

    // Allow the BSP to perform board specific processing
    BSPPowerOn();  
#endif
}