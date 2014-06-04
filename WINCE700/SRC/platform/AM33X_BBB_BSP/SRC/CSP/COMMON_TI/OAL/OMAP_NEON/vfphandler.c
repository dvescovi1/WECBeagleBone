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
//  File:  vfphandler.c
//
//
#include "omap.h"
#include <oalex.h>

#define SaveRestoreTagMN 0x1A2B3C4D

#if (_WINCEOSVER==600)

IOCTL_HAL_GET_NEON_STAT_S g_oalNeonStat = {0,0,0,0,0,0,NULL,NULL};
//------------------------------------------------------------------------------
//
//  VFP STATISTICS
//
//  


//------------------------------------------------------------------------------
//
//  Function:  OALHandleVFPException
//
//  Handles exceptions from the VFP co-processor
//  Passes the exceptions on to the OS
//
BOOL OALHandleVFPException(
  EXCEPTION_RECORD* er,
  PCONTEXT pctx
)
{
    UNREFERENCED_PARAMETER(pctx);
    UNREFERENCED_PARAMETER(er);
    //  All VPF exceptions handed off to OS kernel
    return FALSE;
}

//------------------------------------------------------------------------------
//
//  Function:  OALSaveVFPCtrlRegs
//
//  Saves of extra VFP registers on a context switch.
//  OMAP35XX does not have any "extra" VFP registers;
//  WinCE kernel saves off all OMAP35XX VFP registers
//  on context switches
//
void OALSaveVFPCtrlRegs(
  LPDWORD lpExtra,
  int nMaxRegs
)
{
    UNREFERENCED_PARAMETER(nMaxRegs);
    UNREFERENCED_PARAMETER(lpExtra);
}

//------------------------------------------------------------------------------
//
//  Function:  OALRestoreVFPCtrlRegs
//
//  Restores extra VFP registers on a context switch.
//  OMAP35XX does not have any "extra" VFP registers;
//  WinCE kernel restores off all OMAP35XX VFP registers
//
void OALRestoreVFPCtrlRegs(
  LPDWORD lpExtra,
  int nMaxRegs
)
{
    UNREFERENCED_PARAMETER(nMaxRegs);
    UNREFERENCED_PARAMETER(lpExtra);
}

//------------------------------------------------------------------------------
//
//  Function:  OALInitNeonSaveArea
//
//  Initializes the same area for Neon registers saved off during a thread
//  context switch.  Zeros out the memory area
//
void OALInitNeonSaveArea(
  LPBYTE pArea
)
{
    UNREFERENCED_PARAMETER(pArea);
}

// Reservce space for 32 64 bit NEON registers, 32 bit FPSCR and 32 bit Tag. 
#define NEON_SAVEAREA_SIZE      (33 * 8)

//------------------------------------------------------------------------------
//
//  Function: OALSaveNeonRegistersEx 
//
//  Save the current thread's NEON registers. Some basic checks are done to
//  make sure that the pArea that was restored last is indeed the one that is getting
//  saved now. If not, do some error correction if possible and log them as well. We have 
//  seen the error conditions being hit when interrupt happens while a thread switch is in progress
//  The error condition exists due to a bug in the kernel. 
//
//  One example of error scenario is:
//  Thread Switch A to B->hence Save A and Restore B Neon Context->Interrupt Happened->because
//  of ISR Thread C is woken up and is high priority than B->Due to interrupt, thread switch A to B 
//  was not completed, so A is still the current thread->Interrupt has now caused thread switch from 
//  A to C->hence save A and Restore C======> Oops error!!! As far as NEON is concerned, we saved
//  A context here, restored B context, then saved A again, but its actually B's values that are being
//  saved to A's context==> Corruption!!! 
// 
// This function acheives error correction with the help of history (last Restored pArea) and tag 
// information that is added to the last Restored pArea.
//
void OALSaveNeonRegistersEx(LPBYTE pArea)
{
    LPBYTE pAreaToSave = NULL;     
       
    /* check pArea magicNumber */
    if (*((DWORD *)(pArea+(32*8)+4)) == SaveRestoreTagMN) 
    {
        pAreaToSave = pArea;    
        if (pArea != g_oalNeonStat.pLastRestoredCoProcArea)
            g_oalNeonStat.dwErrCond4++; /* Kernel Screwed up bad */        
        else
            g_oalNeonStat.dwNumCorrectSave++; /* This is what should happen always ! */
        
    }
    /* oops - error conditions - lets try to restore */
    else if (*((DWORD *)(g_oalNeonStat.pLastRestoredCoProcArea +(32*8)+4)) == SaveRestoreTagMN)
    {
        pAreaToSave = g_oalNeonStat.pLastRestoredCoProcArea;       
        /* get some stats on type of Kernel error */
        if (pArea == g_oalNeonStat.pLastSavedCoProcArea) /* Back-to-Back Thread Switch Scenario */
            g_oalNeonStat.dwErrCond1++;
        else {               
            g_oalNeonStat.dwErrCond2++; /* Multiple Thread Switch Scenario */
        }
    }
    /* oops oops - dont know what to do - lets log and skip */
    else
    {
        g_oalNeonStat.dwErrCond3++;
    }

    /* if valid Area to Save */
    if (pAreaToSave)
    {
        OALSaveNeonRegisters(pAreaToSave);        
        *((DWORD *)(pAreaToSave+(32*8)+4)) = 0;
        g_oalNeonStat.pLastSavedCoProcArea = pAreaToSave;        
    }    

    
    return;
}

//------------------------------------------------------------------------------
//
//  Function: OALRestoreNeonRegistersEx 
//
//  Restore the Next thread's (that is about to be scheduled) NEON registers. Tag the pArea
//  to use it when the next thread switch happens and kernel asks us to save this thread's context
//
void OALRestoreNeonRegistersEx(LPBYTE pArea)
{    
    DWORD * lastSaveRestoreStatus = (DWORD *)(pArea+(32*8)+4);      
    OALRestoreNeonRegisters(pArea);    
    *lastSaveRestoreStatus = SaveRestoreTagMN;        
    g_oalNeonStat.pLastRestoredCoProcArea = pArea;
    g_oalNeonStat.dwNumRestore++;     
    return;
}

//------------------------------------------------------------------------------
//
//  Function:  OALVFPInitialize
//
//  Initializes the VFP and NEON co-processors
//
void 
OALVFPInitialize(OEMGLOBAL* pOemGlobal)
{
    //  Enable the VFP    
    OALVFPEnable();

    // Vector Floating Point support
    pOemGlobal->pfnHandleVFPExcp      = OALHandleVFPException;
    pOemGlobal->pfnSaveVFPCtrlRegs    = OALSaveVFPCtrlRegs;
    pOemGlobal->pfnRestoreVFPCtrlRegs = OALRestoreVFPCtrlRegs;

    // Note: VFP and NEON share the same registers. Although CE6 kernel saves/restores
    //       VFPv2 regs (16 64 bit regs) but not on all thread switches, 
    //       so need to save/restore all VFPv3 VFP/NEON regs at every thread switch.
    pOemGlobal->pfnInitCoProcRegs    = OALInitNeonSaveArea;
    pOemGlobal->pfnSaveCoProcRegs    = OALSaveNeonRegisters; //use Extended functions
    pOemGlobal->pfnRestoreCoProcRegs = OALRestoreNeonRegisters; // Use Extended funtions
    pOemGlobal->cbCoProcRegSize      = NEON_SAVEAREA_SIZE;
    pOemGlobal->fSaveCoProcReg       = TRUE;
}

#endif
//------------------------------------------------------------------------------
#if (_WINCEOSVER==700)


void VfpOemInit(OEMGLOBAL* pOemGlobal, DWORD dwFPSID);

//------------------------------------------------------------------------------
//
//  Function:  OALHandleVFPException
//
//  Handles exceptions from the VFP co-processor
//  Passes the exceptions on to the OS
//
/*BOOL OALHandleVFPException(
    ULONG fpexc, 
    EXCEPTION_RECORD* pExr, 
    CONTEXT* pContext, 
    DWORD* pdwExcpId, 
    BOOL fInKMode
)
{
    //  All VPF exceptions handed off to OS kernel
    return FALSE;
}*/

BOOL OALHandleVFPException(
  EXCEPTION_RECORD* er,
  PCONTEXT pctx
)
{
    UNREFERENCED_PARAMETER(pctx);
    UNREFERENCED_PARAMETER(er);
    //  All VPF exceptions handed off to OS kernel
    return FALSE;
}

//------------------------------------------------------------------------------
//
//  Function:  OALSaveVFPCtrlRegs
//
//  Saves of extra VFP registers on a context switch.
//  OMAP3430 does not have any "extra" VFP registers;
//  WinCE kernel saves off all OMAP3430 VFP registers
//  on context switches
//
void OALSaveVFPCtrlRegs(
  LPDWORD lpExtra,
  int nMaxRegs
)
{
}

//------------------------------------------------------------------------------
//
//  Function:  OALRestoreVFPCtrlRegs
//
//  Restores extra VFP registers on a context switch.
//  OMAP3430 does not have any "extra" VFP registers;
//  WinCE kernel restores off all OMAP3430 VFP registers
//
void OALRestoreVFPCtrlRegs(
  LPDWORD lpExtra,
  int nMaxRegs
)
{
}

//------------------------------------------------------------------------------
//
//  Function:  OALVFPInitialize
//
//  Initializes the VFP and NEON co-processors
//
void 
OALVFPInitialize(OEMGLOBAL* pOemGlobal)
{
    //  Enable the VFP    
    OALVFPEnable();
    
    //  Initialize the kernel VFP, init should be called from OEMInit()
    //VfpOemInit( pOemGlobal, 0 );
    
    pOemGlobal->pfnSaveVFPCtrlRegs    = OALSaveVFPCtrlRegs;
    pOemGlobal->pfnRestoreVFPCtrlRegs = OALRestoreVFPCtrlRegs;
    
}

#endif
