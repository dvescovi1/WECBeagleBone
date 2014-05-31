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
//  File: prcm_reset.c
//

#include "omap.h"
#include "omap_prof.h"
#include "am33x.h"
#include <nkintr.h>
#include <pkfuncs.h>
#include "oalex.h"
#include "prcm_priv.h"

//------------------------------------------------------------------------------
BOOL ResetInitialize()
{	
    // clear the reset flags for all the power domains
	OUTREG32(&g_pPrcmRegs->RM_PER_RSTST, RSTST_PCI_LRST ); // Not sure we need it for Subarctic
    
    OUTREG32(&g_pPrcmRegs->RM_WKUP_RSTST, RSTST_WKUP_M3_LRST );
    
    OUTREG32(&g_pPrcmRegs->PRM_RSTST, RSTST_EXTERNAL_WARM_RST | RSTST_WDT1_RST | RSTST_WDT0_RST				
                 | RSTST_MPU_SECURITY_VIOL_RST | RSTST_GLOBAL_WARM_SW_RST | RSTST_GLOBAL_COLD_RST );		
		
    OUTREG32(&g_pPrcmRegs->RM_GFX_RSTST, GFX_RST );

	return TRUE;
}

//------------------------------------------------------------------------------
BOOL PrcmDomainResetStatus(UINT powerDomain, UINT *pResetStatus, BOOL    bClear)
{
	BOOL rc = FALSE;
	UINT resetStatus;

	if (pResetStatus == NULL) goto cleanUp;
	
	rc = TRUE;
	switch (powerDomain){
		case POWERDOMAIN_PER:
			resetStatus = INREG32(&g_pPrcmRegs->RM_PER_RSTST);
			if (bClear) OUTREG32(&g_pPrcmRegs->RM_PER_RSTST, resetStatus);
			break;
			
		case POWERDOMAIN_WKUP:
			resetStatus = INREG32(&g_pPrcmRegs->RM_WKUP_RSTST);
			if (bClear) OUTREG32(&g_pPrcmRegs->RM_WKUP_RSTST, resetStatus);
			break;
			
		case POWERDOMAIN_GFX:
			resetStatus = INREG32(&g_pPrcmRegs->RM_GFX_RSTST );
			if (bClear) OUTREG32(&g_pPrcmRegs->RM_GFX_RSTST, resetStatus);
			break;

		default:
			rc = FALSE;
	}		
		
cleanUp:
	return rc;
}

//------------------------------------------------------------------------------
BOOL PrcmDomainReset(UINT powerDomain, UINT resetMask)
{
	BOOL			rc = TRUE;


	switch (powerDomain){
		case POWERDOMAIN_PER:
			resetMask &= PCI_LRST | ICSS_LRST ;
			OUTREG32(&g_pPrcmRegs->RM_PER_RSTCTRL, 
				 INREG32(&g_pPrcmRegs->RM_PER_RSTCTRL) | resetMask);
			break;
			
		case POWERDOMAIN_WKUP:
			resetMask &= WKUP_M3_LRST;
			OUTREG32(&g_pPrcmRegs->RM_WKUP_RSTCTRL, 
				INREG32(&g_pPrcmRegs->RM_WKUP_RSTCTRL) | resetMask);
			break;
			
		case POWERDOMAIN_GFX:
			resetMask &= GFX_RST;
			OUTREG32(&g_pPrcmRegs->RM_GFX_RSTCTRL, 
				INREG32(&g_pPrcmRegs->RM_GFX_RSTCTRL) | resetMask);
			break;

		default:
			rc = FALSE;
	}		
		
	return rc;
}

//------------------------------------------------------------------------------
BOOL PrcmDomainResetRelease(UINT powerDomain, UINT resetMask)
{
	BOOL	rc = TRUE;
	UINT32	resetStatus=0;
	volatile UINT32  *rstctrl = NULL;
	volatile UINT32  *rstst   = NULL;

    OALMSG(OAL_INFO,(L"PrcmDomainResetRelease DMN %d; mask %d\r\n",powerDomain,  resetMask));

    switch (powerDomain){
		case POWERDOMAIN_PER:
			resetMask &= PCI_LRST | ICSS_LRST ;
            rstctrl = &g_pPrcmRegs->RM_PER_RSTCTRL;
            rstst   = &g_pPrcmRegs->RM_PER_RSTST;
            if (resetMask&PCI_LRST) resetStatus = PCI_LRSTST;            
			break;
			
		case POWERDOMAIN_WKUP:
			resetMask &= WKUP_M3_LRST;
			rstctrl = &g_pPrcmRegs->RM_WKUP_RSTCTRL;
            rstst   = &g_pPrcmRegs->RM_WKUP_RSTST;
            if (resetMask&WKUP_M3_LRST) resetStatus = WKUP_M3_LRSTST;
			break;
			
		case POWERDOMAIN_GFX:
			resetMask &= GFX_RST;
			rstctrl = &g_pPrcmRegs->RM_GFX_RSTCTRL;
			rstst   = &g_pPrcmRegs->RM_GFX_RSTST;
//			resetStatus = resetMask;
			resetStatus = 0;		/* Temp because SGX rstst is failing the status check below */
			break;

		default:
			rc = FALSE;
	}		

	if (rc == TRUE && rstctrl != NULL){
		OUTREG32(rstctrl, INREG32(rstctrl) & ~resetMask);

		if (resetStatus) {
			volatile UINT32	resetTmp=0;

			resetTmp=resetStatus;
			while (resetTmp != 0)
				resetTmp &= ~INREG32(rstst);

			// clear the status
			OUTREG32(rstst, resetMask);
		}
	}

	return rc;
}

//------------------------------------------------------------------------------
void
PrcmGlobalReset(
    )
{
	OUTREG32(&g_pPrcmRegs->PRM_RSTCTRL, RST_GLOBAL_WARM_SW);
}


