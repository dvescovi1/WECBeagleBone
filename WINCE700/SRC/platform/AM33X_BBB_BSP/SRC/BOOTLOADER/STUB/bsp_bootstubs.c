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
//  File:  bsp_bootstubs.c
// stub routines which exist in full images but not in bootloader images

#include "bsp.h"
#include "bsp_cfg.h"
#include "am33x_prcm.h"
#include "am33x_clocks.h"
#include "am33x_base_regs.h"

BOOL INTERRUPTS_STATUS() { return FALSE; }
BOOL CloseHandle(HANDLE hObject){
    UNREFERENCED_PARAMETER(hObject);
    return TRUE;
}

void WINAPI EnterCriticalSection(LPCRITICAL_SECTION lpcs) {UNREFERENCED_PARAMETER(lpcs);}
void WINAPI LeaveCriticalSection(LPCRITICAL_SECTION lpcs) {UNREFERENCED_PARAMETER(lpcs);}
void WINAPI InitializeCriticalSection( LPCRITICAL_SECTION lpcs){UNREFERENCED_PARAMETER(lpcs);}
void DeleteCriticalSection(LPCRITICAL_SECTION lpCriticalSection){UNREFERENCED_PARAMETER(lpCriticalSection);}

HANDLE WINAPI SC_CreateMutex(LPSECURITY_ATTRIBUTES lpsa,BOOL bInitialOwner,LPCTSTR lpName){
    UNREFERENCED_PARAMETER(lpsa);
    UNREFERENCED_PARAMETER(bInitialOwner);
    UNREFERENCED_PARAMETER(lpName);
    return NULL;
}

DWORD WINAPI SC_WaitForMultiple(DWORD cObjects, CONST HANDLE *lphObjects, 
								BOOL fWaitAll, DWORD dwTimeout) {
    UNREFERENCED_PARAMETER(cObjects);
    UNREFERENCED_PARAMETER(lphObjects);
    UNREFERENCED_PARAMETER(fWaitAll);
    UNREFERENCED_PARAMETER(dwTimeout);
    return 0;
}

BOOL WINAPI SC_ReleaseMutex( HANDLE hMutex) {
    UNREFERENCED_PARAMETER(hMutex);
    return TRUE;
}


// assumption is that clock domains (L3, l3s, L4fw, L4ls, wkup_l4) are enabled in xldr
// do not touch common clk domains here as we dont want to disable the clock domain here since we dont refcount them in eboot.
// since LCDC, CPSW are dedicated clk domains, its ok to enable and disable in eboot.
BOOL EnableDeviceClocks( UINT devId, BOOL bEnable)
{
    AM33X_PRCM_REGS* pPrcmRegs = OALPAtoUA(AM33X_PRCM_REGS_PA);
    volatile unsigned int *pModReg = NULL;
    volatile unsigned int *pDomainReg = NULL;
    UINT32 val = 0;   

    switch (devId)
    {
        case AM_DEVICE_DEBUGSS:
            pModReg = &pPrcmRegs->CM_WKUP_DEBUGSS_CLKCTRL;            
            break;	        
        case AM_DEVICE_GPIO0:
            pModReg = &pPrcmRegs->CM_WKUP_GPIO0_CLKCTRL;            
            break;
        case AM_DEVICE_GPIO1:
            pModReg = &pPrcmRegs->CM_PER_GPIO1_CLKCTRL;            
            break;
        case AM_DEVICE_GPIO2:
            pModReg = &pPrcmRegs->CM_PER_GPIO2_CLKCTRL;            
            break;
        case AM_DEVICE_GPIO3:
            pModReg = &pPrcmRegs->CM_PER_GPIO3_CLKCTRL;            
            break;
        case AM_DEVICE_I2C0:
            pModReg = &pPrcmRegs->CM_WKUP_I2C0_CLKCTRL;            
            break;
        case AM_DEVICE_I2C1:
            pModReg = &pPrcmRegs->CM_PER_I2C1_CLKCTRL;            
            break;
        case AM_DEVICE_I2C2:
            pModReg = &pPrcmRegs->CM_PER_I2C2_CLKCTRL;            
            break;
        case AM_DEVICE_TIMER0:
            pModReg = &pPrcmRegs->CM_WKUP_TIMER0_CLKCTRL;            
            break;	        
        case AM_DEVICE_UART0:
            pModReg = &pPrcmRegs->CM_WKUP_UART0_CLKCTRL;            
            break;
        case AM_DEVICE_WDT0:
            pModReg = &pPrcmRegs->CM_WKUP_WDT0_CLKCTRL;            
            break;
        case AM_DEVICE_WDT1:
            pModReg = &pPrcmRegs->CM_WKUP_WDT1_CLKCTRL;            
            break;	
        case AM_DEVICE_CPGMAC0:
            pModReg = &pPrcmRegs->CM_PER_CPGMAC0_CLKCTRL;
            pDomainReg = &pPrcmRegs->CM_PER_CPSW_CLKSTCTRL;
            break;
        case AM_DEVICE_ELM:
            pModReg = &pPrcmRegs->CM_PER_ELM_CLKCTRL;            
            break;	
        case AM_DEVICE_GPMC:
            pModReg = &pPrcmRegs->CM_PER_GPMC_CLKCTRL;            
            break;          
        case AM_DEVICE_IEEE5000:
            pModReg = &pPrcmRegs->CM_PER_IEEE5000_CLKCTRL;            
            break;  
        case AM_DEVICE_MMCHS0:
            pModReg = &pPrcmRegs->CM_PER_MMC0_CLKCTRL;            
            break;
        case AM_DEVICE_MMCHS1:
            pModReg = &pPrcmRegs->CM_PER_MMC1_CLKCTRL;            
            break;
        case AM_DEVICE_TIMER2:
            pModReg = &pPrcmRegs->CM_PER_TIMER2_CLKCTRL;            
            break;
        case AM_DEVICE_LCDC:
            pModReg = &pPrcmRegs->CM_PER_LCDC_CLKCTRL;            
            pDomainReg = &pPrcmRegs->CM_PER_LCDC_CLKSTCTRL;
            break;        
        case AM_DEVICE_EMIF:
            pModReg = &pPrcmRegs->CM_PER_EMIF_CLKCTRL;            
            break;        
        case AM_DEVICE_EMIF_FW:
            pModReg = &pPrcmRegs->CM_PER_EMIF_FW_CLKCTRL;            
            break;
        case AM_DEVICE_CONTROL:
            pModReg = &pPrcmRegs->CM_WKUP_CONTROL_CLKCTRL;
            break;
        case AM_DEVICE_L3:
            pModReg = &pPrcmRegs->CM_PER_L3_CLKCTRL;
            break;
        case AM_DEVICE_L4LS:
            pModReg = &pPrcmRegs->CM_PER_L4LS_CLKCTRL;
            break;
        case AM_DEVICE_L4FW:
            pModReg = &pPrcmRegs->CM_PER_L4FW_CLKCTRL;
            break;
        case AM_DEVICE_L4WKUP:
            pModReg = &pPrcmRegs->CM_WKUP_L4WKUP_CLKCTRL;
            break;
        case AM_DEVICE_L3_INSTR:
            pModReg = &pPrcmRegs->CM_PER_L3_INSTR_CLKCTRL;
            break;
        case AM_DEVICE_L4_HS:
            pModReg = &pPrcmRegs->CM_PER_L4HS_CLKCTRL;
            break;            
    }
    
    if (pModReg || pDomainReg)
    {   
        if (bEnable) 
        {
            //OALLog(L"\r\n****Eboot:EnableDeviceClocks: %d %d \r\n", devId, bEnable);  
            if (pDomainReg) {
                val = INREG32(pDomainReg) & ~CLKSTCTRL_CLKTRCTRL_MASK;
                OUTREG32(pDomainReg, val|CLKSTCTRL_CLKTRCTRL_SW_WKUP); 
            }
            if (pModReg) 
            {
                val = INREG32(pModReg) & ~ClKCTRL_MODULEMODE_MASK;
                OUTREG32(pModReg, val|ClKCTRL_MODULEMODE_EN);
                while ((INREG32(pModReg) & ClKCTRL_IDLEST_DIS)==ClKCTRL_IDLEST_DIS);
            }
        } else {
            if (pModReg) 
            {
                val = INREG32(pModReg) & ~ClKCTRL_MODULEMODE_MASK;
                OUTREG32(pModReg, val|ClKCTRL_MODULEMODE_DIS);
            }
            if (pDomainReg) {
                val = INREG32(pDomainReg) & ~CLKSTCTRL_CLKTRCTRL_MASK;
                OUTREG32(pDomainReg, val|CLKSTCTRL_CLKTRCTRL_SW_SLEEP);             
            }
        }            
    }
    return TRUE;
}

/* PrcmClockGetClockRate only supports LCD_PCLK for now */
UINT32 PrcmClockGetClockRate(OMAP_CLOCKID clock_id)
{
    DWORD freq=0;
    DWORD sys_clk = 24;
    DWORD val;
    DWORD val2;
    DWORD freqSel;
    AM33X_PRCM_REGS* pPrcmRegs = OALPAtoUA(AM33X_PRCM_REGS_PA);

    switch(clock_id)
    {
    	case LCD_PCLK:
        	freqSel = (INREG32(&pPrcmRegs->CLKSEL_LCDC_PIXEL_CLK)&&CLKSEL_LCDC_PIXEL_CLK_MASK)>>CLKSEL_LCDC_PIXEL_CLK_SHIFT;
        	
        	switch (freqSel)
        	{
        		case 0: //disp DPLL M2
        			val = INREG32(&pPrcmRegs->CM_CLKSEL_DPLL_DISP);
        			val2 = INREG32(&pPrcmRegs->CM_DIV_M2_DPLL_DISP);
        			
        			// fdpll/2
        			freq = ((DWORD)((val & DPLL_MULT_MASK)>>DPLL_MULT_SHIFT) * sys_clk) /
        						((DWORD)((val & DPLL_DIV_MASK) >> DPLL_DIV_SHIFT) + 1);
        			// fdll/(2*M2)
        			freq = freq / ((DWORD)((val2 & DPLL_ADPLLS_CLKOUT_DIV_MASK) >> DPLL_ADPLLS_CLKOUT_DIV_SHIFT));  
        		break;
        
        		case 1: //CORE DPLL M5->SYSCLK2 				   
        			val = INREG32(&pPrcmRegs->CM_CLKSEL_DPLL_CORE);
        			val2 = INREG32(&pPrcmRegs->CM_DIV_M5_DPLL_CORE);
        			//fdpll
        			freq = 2 * ((DWORD)((val & DPLL_MULT_MASK)>>DPLL_MULT_SHIFT) * sys_clk) /
        						((DWORD)((val & DPLL_DIV_MASK) >> DPLL_DIV_SHIFT) + 1);
        			//fdpll/(M5)
        			freq = freq / ((DWORD)((val2 & DPLL_ADPLLS_CLKOUT_DIV_MASK) >> DPLL_ADPLLS_CLKOUT_DIV_SHIFT));					  
        		break;
        		
        		case 2: //PER DPLL M2 -> 192MHz 				   
        			val = INREG32(&pPrcmRegs->CM_CLKSEL_DPLL_PERIPH);
        			val2 = INREG32(&pPrcmRegs->CM_DIV_M2_DPLL_PER);
        			// fdpll
        			freq = ((DWORD)((val & DPLL_MULT_MASK)>>DPLL_MULT_SHIFT) * sys_clk) /
        						((DWORD)((val & DPLL_DIV_MASK) >> DPLL_DIV_SHIFT) + 1);
        			//fdpll/M2
        			freq = freq / ((DWORD)((val2 & DPLL_ADPLLJ_CLKOUT_DIV_MASK) >> DPLL_ADPLLJ_CLKOUT_DIV_SHIFT) + 1);					 
        		break;
        
        	}
        break;

        default:
            freq = 0;
        break;
		
	}		
    return (UINT32)freq;
}

BOOL PrcmDeviceGetContextState( UINT devId, BOOL bSet){
    UNREFERENCED_PARAMETER(devId);
    UNREFERENCED_PARAMETER(bSet);
    return TRUE;
}

BOOL EnableDeviceClocksNoRefCount( UINT devId, BOOL bEnable)
{
	return EnableDeviceClocks( devId, bEnable );
}

BOOL BusClockRelease(HANDLE hBus, UINT id)
{
    UNREFERENCED_PARAMETER(hBus);
    return EnableDeviceClocks(id,FALSE);
}

BOOL BusClockRequest(HANDLE hBus, UINT id){
    UNREFERENCED_PARAMETER(hBus);
    return EnableDeviceClocks(id,TRUE);
}

VOID MmUnmapIoSpace(PVOID BaseAddress, ULONG NumberOfBytes){
    UNREFERENCED_PARAMETER(BaseAddress);
    UNREFERENCED_PARAMETER(NumberOfBytes);
}

PVOID MmMapIoSpace( PHYSICAL_ADDRESS PhysicalAddress,ULONG NumberOfBytes, 
					BOOLEAN CacheEnable )
{
    UNREFERENCED_PARAMETER(NumberOfBytes);
    return OALPAtoVA(PhysicalAddress.LowPart,CacheEnable);
}

HANDLE CreateBusAccessHandle (LPCTSTR lpActiveRegPath){    
    UNREFERENCED_PARAMETER(lpActiveRegPath);
    return (HANDLE) 0xAA;
}

void HalContextUpdateDirtyRegister(UINT32 ffRegister){UNREFERENCED_PARAMETER(ffRegister);}
// 383
