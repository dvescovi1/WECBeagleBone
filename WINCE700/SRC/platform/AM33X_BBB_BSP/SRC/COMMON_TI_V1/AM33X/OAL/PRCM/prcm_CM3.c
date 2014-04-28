
#include "windows.h"
#include <nkintr.h>

#include "am33x.h"
#include "am33x_oal_prcm.h"
#include "am33x_config.h"
#include "am33x_interrupt_struct.h"

#include "cm3_ipc.h"
#include "mailbox.h"

/* M3_TXEV_EOI */
#define CONTROL_M3_TXEV_EOI_M3_TXEV_EOI   (0x00000001u)
#define CONTROL_M3_TXEV_EOI_M3_TXEV_EOI_SHIFT   (0x00000000u)

#define CONTROL_M3_TXEV_EOI_SMA3   (0xFFFFFFFEu)
#define CONTROL_M3_TXEV_EOI_SMA3_SHIFT   (0x00000001u)

/*	Flag to indicate M3 event is received	*/
volatile unsigned int isM3IntReceived = 0;

/******************************************************************************
**              EXTERN DEFINITIONS
******************************************************************************/
extern AM33X_SYSC_MISC2_REGS   *g_pSyscMisc2Regs;
extern BOOL RegisterPrcmCM3InterruptHandler();

//-----------------------------------------------------------------------------
//  Global:  g_pIntr
//  Reference to all interrupt related registers. Initialized in OALIntrInit
extern AM33X_INTR_CONTEXT const *g_pIntr;

//------------------------------------------------------------------------------
//  This variable is defined in interrupt module and it is used in interrupt
//  handler to distinguish M3 interrupt.
extern UINT32           g_oalM3Irq;




/******************************************************************************
**              FUNCTION DEFINITIONS
******************************************************************************/
unsigned short readCmdStatus(void);

/******************************************************************************
**              DATA STRUCTURE DEFINITIONS
******************************************************************************/

BOOL PrcmCM3FillDSData(deepSleepData * dsData, UINT32 cmdID)
{
    BOOL ret = TRUE;
    if (dsData == NULL) return FALSE;

    memset(dsData,0,sizeof(deepSleepData));

    switch (cmdID) {
        case PM_CMD_DS0_MODE:
            dsData->dsDataBits.cmdID = PM_CMD_DS0_MODE;
        	dsData->dsDataBits.resumeAddr = (UINT)fnRomRestoreLoc;
        	dsData->dsDataBits.moscState = PM_MOSC_STATE_OFF;
        	dsData->dsDataBits.deepSleepCount = 0;
        	dsData->dsDataBits.pdMpuState = PM_MPU_POWERSTATE_RET;
        	dsData->dsDataBits.pdMpuRamRetState = PM_MPU_RAM_RETSTATE_ON;
        	dsData->dsDataBits.pdMpul1RetState = 0;
        	dsData->dsDataBits.pdMpul2RetState = 0;
        	dsData->dsDataBits.pdPerState = PM_PER_POWERSTATE_RET;
        	dsData->dsDataBits.pdPerIcssMemRetState = PM_PER_ICSS_RAM_RETSTATE_OFF;
        	dsData->dsDataBits.pdPerMemRetState = PM_PER_MEM_RETSTATE_OFF;
        	dsData->dsDataBits.pdPerOcmcRetState = PM_PER_OCMC_RAM_RETSTATE_RET;
        	dsData->dsDataBits.wakeSources = WAKE_SOURCES_ALL;
        break;

        case PM_CMD_DS1_MODE:
            dsData->dsDataBits.cmdID = PM_CMD_DS1_MODE;
            dsData->dsDataBits.resumeAddr = (UINT)fnRomRestoreLoc;
        	dsData->dsDataBits.moscState = PM_MOSC_STATE_OFF;
        	dsData->dsDataBits.deepSleepCount = 0;
        	dsData->dsDataBits.pdMpuState = PM_MPU_POWERSTATE_RET;
        	dsData->dsDataBits.pdMpuRamRetState = PM_MPU_RAM_RETSTATE_ON;
        	dsData->dsDataBits.pdMpul1RetState = 0;
        	dsData->dsDataBits.pdMpul2RetState = 0;
        	dsData->dsDataBits.pdPerState = PM_PER_POWERSTATE_ON;
        	dsData->dsDataBits.pdPerIcssMemOnState = PM_PER_ICSS_RAM_ONSTATE_ON;
        	dsData->dsDataBits.pdPerMemOnState = PM_PER_MEM_ONSTATE_ON;
        	dsData->dsDataBits.pdPerOcmcOnState = (UINT)PM_PER_OCMC_RAM_ONSTATE_ON;
        	dsData->dsDataBits.wakeSources = WAKE_SOURCES_ALL;
        break;

        case PM_CMD_DS2_MODE:            
            dsData->dsDataBits.cmdID = PM_CMD_DS2_MODE;
            dsData->dsDataBits.resumeAddr = (UINT)fnRomRestoreLoc;
            dsData->dsDataBits.moscState = PM_MOSC_STATE_OFF;
            dsData->dsDataBits.deepSleepCount = 0;
            dsData->dsDataBits.pdMpuState = PM_MPU_POWERSTATE_ON;
            dsData->dsDataBits.pdMpuRamRetState = 0;
            dsData->dsDataBits.pdMpul1RetState = 0;
            dsData->dsDataBits.pdMpul2RetState = 0;
            dsData->dsDataBits.pdMpuRamOnState = PM_MPU_RAM_ONSTATE_ON;
            dsData->dsDataBits.pdPerState = PM_PER_POWERSTATE_ON;
            dsData->dsDataBits.pdPerIcssMemOnState = PM_PER_ICSS_RAM_ONSTATE_ON;
            dsData->dsDataBits.pdPerMemOnState = PM_PER_MEM_ONSTATE_ON;
            dsData->dsDataBits.pdPerOcmcOnState = (UINT)PM_PER_OCMC_RAM_ONSTATE_ON;
            dsData->dsDataBits.wakeSources = WAKE_SOURCES_ALL;
        break;

        case PM_CMD_RTC_FAST_MODE:
            dsData->dsDataBits.cmdID = PM_CMD_RTC_FAST_MODE;
	        dsData->dsDataBits.rtcTimeoutVal = 2;
        break;

        case PM_CMD_RTC_MODE:
            ret=FALSE;
        break; 

        default:
            ret=FALSE;
        break;
    }        
    return ret;
}


/*
** Clear CM3 event and re-enable the event
*/
void clearM3Events(void)
{
	/*	Clear M3_TXEV event	*/
	SETREG32(&g_pSyscMisc2Regs->M3_TXEV_EOI,CONTROL_M3_TXEV_EOI_M3_TXEV_EOI);

    /*	Re-arm M3_TXEV event	*/
    CLRREG32(&g_pSyscMisc2Regs->M3_TXEV_EOI,CONTROL_M3_TXEV_EOI_M3_TXEV_EOI);

}

/*
** Initialize M3 interrupts
*/
void initCM3Events(void)
{
    RegisterPrcmCM3InterruptHandler();
    
	/*	Clear M3 events if any	*/
	clearM3Events();
}


/*
** Wait for ACK from CM3
*/
void PrcmCM3WaitForTxevent(void)
{
	/*	wait until CM3 TX Event is generated	*/
	while(!isM3IntReceived);
		
	while(PM_IN_PROGRESS == readCmdStatus())
	{
		clearM3Events();	
	}
	
	switch(readCmdStatus())
	{
		case PM_CMD_PASS:
		case PM_WAIT4OK:
				break;
		case PM_CMD_FAIL:
		default:
				/*	Command failed or invalid status	*/
				OALMSG(1,(L"\n\n PrcmCM3WaitForTxevent: ACK Failed \r\n", -1));
				//while(1);	
	}
	clearM3Events();
	
	/*	Reset interrupt flag	*/
	isM3IntReceived = 0;
}


/********************** PRCM APIs ***************************/

void PrcmCM3Init()
{
	OALMSG(OAL_INFO, (L"+PrcmCM3Init: init M3 events and mailbox\r\n"));
	/*	Initialize M3 events	*/
	initCM3Events();
    /* initialize mailbox */
    MBInitializeMailbox();
    
    OALMSG(OAL_INFO, (L"-PrcmCM3Init: done with M3 \r\n"));  
}


/*
** CM3 ISR handler
*/
int PrcmCM3Isr()
{
	isM3IntReceived = 1;
	clearM3Events();
    OALMSG(OAL_INFO,(L"Cm3ISR: M3 int recvd\r\n"));
    return SYSINTR_NOP;
}

void PrcmCM3ResetAndHandshake()
{
    OALMSG(OAL_INFO,(L"PrcmCM3ResetAndHandshake: isM3IntReceived: %d\r\n",isM3IntReceived));
    
    /*	Release CM3 from reset	 and handshake*/
    PrcmDomainResetRelease(POWERDOMAIN_WKUP,WKUP_M3_LRST);

    /*	Wait for ACK from CM3 - M3_TXEV	*/
	PrcmCM3WaitForTxevent();
}

/*************************************************/

#define STATUS_SHIFT			16


/**************************************************************************
  API FUNCTION DEFINITIONS
***************************************************************************/


/**
 *  \brief   This function configures the deep sleep data in to IPC registers
 *
 * \param     pmDsDataVar	structure variable containing deep sleep data
 *
 * \return 	  None
 */
  
void PrcmCM3ConfigIPCRegs(deepSleepData *pmDsDataVar)
{

	/*	Command ID	*/
	OUTREG32(&g_pSyscMisc2Regs->IPC_MSG_REG1,pmDsDataVar->readDsData.short1);
		
	if((PM_CMD_RTC_MODE == pmDsDataVar->dsDataBits.cmdID) || 
			(PM_CMD_RTC_FAST_MODE == pmDsDataVar->dsDataBits.cmdID))
	{
		/*	RTC time out value	*/
		OUTREG32(&g_pSyscMisc2Regs->IPC_MSG_REG2,pmDsDataVar->readDsData.byte1) ;
	}
	else
	{
		/*	Resume address	*/
		OUTREG32(&g_pSyscMisc2Regs->IPC_MSG_REG0,pmDsDataVar->readDsData.word0);
		/*	deep sleep data	*/
		OUTREG32(&g_pSyscMisc2Regs->IPC_MSG_REG2,pmDsDataVar->readDsData.word1);
		/*	deep sleep data	*/
		OUTREG32(&g_pSyscMisc2Regs->IPC_MSG_REG3,pmDsDataVar->readDsData.word2);	
	}
}

void PrcmCM3ConfigDeepSleep(DWORD suspendMode, deepSleepData * dsData)
{
    volatile UINT32 baseAddMB; 
    
    // set the deep sleep state here
    PrcmCM3FillDSData(dsData,suspendMode);    
    OALMSG(OAL_INFO,(L"PrcmSuspend: Done filling DS Data\r\n"));
    PrcmCM3ConfigIPCRegs(dsData);   
    OALMSG(OAL_INFO,(L"PrcmSuspend: Done config IPC regs with DS data\r\n"));
    PrcmCM3DumpIPCRegs();
    baseAddMB = (UINT32)OALPAtoUA(AM33X_MBOX_REGS_PA);
    OALMSG(OAL_INFO,(L"PrcmSuspend: baseAddMB=0x%x\r\n",baseAddMB));    
    MBGenerateMailboxInt(baseAddMB);
    OALMSG(OAL_INFO,(L"PrcmSuspend: Done generating MB interrupt\r\n"));       
    while (INREG32(&g_pIntr->pICLRegs->INTC_SIR_IRQ) != g_oalM3Irq) {
            OUTREG32(&g_pIntr->pICLRegs->INTC_CONTROL, IC_CNTL_NEW_IRQ);    
    }    
    OALMSG(OAL_INFO,(L"PrcmCM3ConfigDeepSleep: ITR 0x%x 0x%x 0x%x 0x%x\r\n",
                    INREG32(&g_pIntr->pICLRegs->INTC_ITR0),
                    INREG32(&g_pIntr->pICLRegs->INTC_ITR1),
                    INREG32(&g_pIntr->pICLRegs->INTC_ITR2),
                    INREG32(&g_pIntr->pICLRegs->INTC_ITR3)));      
    OALMSG(OAL_INFO,(L"PrcmSuspend: Got M3 interrupt\r\n"));   
    PrcmCM3Isr();
    OUTREG32(&g_pIntr->pICLRegs->INTC_CONTROL, IC_CNTL_NEW_IRQ);  
    PrcmCM3DumpIPCRegs();
    PrcmCM3WaitForTxevent();
    OALMSG(OAL_INFO,(L"PrcmSuspend: Recvd ack from M3\r\n"));    
    MBClearMailboxMsg(baseAddMB);
    OALMSG(OAL_INFO,(L"PrcmSuspend: Done sending message to M3\r\n"));
}

void PrcmCM3DumpIPCRegs()
{
    OALMSG(1,(L"IPC REGS 0x%x 0x%x 0x%x 0x%x \r\n",
                INREG32(&g_pSyscMisc2Regs->IPC_MSG_REG0),
                INREG32(&g_pSyscMisc2Regs->IPC_MSG_REG1),
                INREG32(&g_pSyscMisc2Regs->IPC_MSG_REG2),
                INREG32(&g_pSyscMisc2Regs->IPC_MSG_REG3)));
}

/**
 *  \brief   This function reads teh trace data from CM3
 *
 * \param     None
 *
 * \return 	  trace		trace data indicating the state of CM3
 */
 
unsigned int readCM3Trace(void)
{
	return INREG32(&g_pSyscMisc2Regs->IPC_MSG_REG4);
}


/* ipc */
/**
 *  \brief   This function returns the status of the last sent command
 *
 * \param     None
 *
 * \return 	  status 	status of the last sent command
 */
 
unsigned short readCmdStatus(void)
{
	return ((INREG32(&g_pSyscMisc2Regs->IPC_MSG_REG1)) >> STATUS_SHIFT);
}


