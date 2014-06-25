// All rights reserved ADENEO EMBEDDED 2010
//========================================================================
// Copyright (c) Texas Instruments Incorporated 2010. All Rights Reserved.
// Copyright (c) MPC Data Limited 2007.  All rights reserved.
//
//------------------------------------------------------------------------------
//
//  File:  chw.cpp
//
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this source code is subject to the terms of the Microsoft end-user
// license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
// If you did not accept the terms of the EULA, you are not authorized to use
// this source code. For a copy of the EULA, please see the LICENSE.RTF on your
// install media.
//
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Module Name:
//     CHW.cpp
// Abstract:
//     This file implements the HW specific register routines
//
// Notes:
//
//

#pragma warning(push)
#pragma warning(disable: 4201 4510 4512 4610)
#include <windows.h>
#include <nkintr.h>

#include "chw.hpp"
#include "cpipe.hpp"
#include "cohcd.hpp"
#include "am33x.h"
#include "drvcommon.h"

#pragma warning(pop)

#ifndef _PREFAST_
#pragma warning(disable: 4068)
#endif

// Macros to enable extra logging
#define CTRLTCMSG //RETAILMSG
#define INTTCMSG //RETAILMSG
#define BULKTCMSG //RETAILMSG
#define ISOTCMSG //RETAILMSG


// Port status bits
#define PORT_STATUS_CON_CHANGED             0x01    // Connection changed
#define PORT_STATUS_DEVICE_ATTACHED         0x02    // Device attched

#ifdef MUSB_USEDMA
#define PROCESSING_EVENT_TIMEOUT            1000     // Validate DMA transfer state once a second
                                                     // to detect stalled etc transfers
#else // MUSB_USEDMA
#define PROCESSING_EVENT_TIMEOUT            INFINITE // No timeout if DMA not used
#endif // MUSB_USEDMA

void 
memdump(unsigned char * data,   unsigned short num_bytes, unsigned short offset    )
{    
    unsigned short i,j,l;
    unsigned char tmp_str[100];
    unsigned char tmp_str1[10];
    for (i = 0; i < num_bytes; i += 16){
        unsigned short n ;
        tmp_str[0]='\0';
        n = i+offset ;
        for (j=0; j<4; j++) {
            l=n%16;
            if (l>=10)
            tmp_str[3-j]=(unsigned char)('A'+l-10);
            else
            tmp_str[3-j]=(unsigned char)(l+'0');
            n >>= 4 ;
        }
        tmp_str[4]='\0';
        strcat_s ( (char *)tmp_str, _countof(tmp_str), ": ");
        /*          Output the hex bytes        */
        for (j = i; j < (i+16); j++) {
            int m ;
            if (j < num_bytes)  {
                m=((unsigned int)((unsigned char)*(data+j)))/16 ;
                if (m>=10)
                    tmp_str1[0]='A'+(unsigned char)m-10;
                else
                    tmp_str1[0]=(unsigned char)m+'0';
                m=((unsigned int)((unsigned char)*(data+j)))%16 ;
                if (m>=10)
                    tmp_str1[1]='A'+(unsigned char)m-10;
                else
                    tmp_str1[1]=(unsigned char)m+'0';
                tmp_str1[2]='\0';
                strcat_s ((char *)tmp_str, _countof(tmp_str), (char *)tmp_str1);
                strcat_s ((char *)tmp_str, _countof(tmp_str), " ");
            }
            else {
                strcat_s((char *)tmp_str,_countof(tmp_str), "   ");
            }
        }
        strcat_s((char *)tmp_str, _countof(tmp_str), "  ");
        l=(unsigned short)strlen((char *)tmp_str);

        /*         * Output the ASCII bytes        */
        for (j = i; j < (i+16); j++){
            if (j < num_bytes){
                char c = *(data+j);
                if (c < ' ' || c > 'z')
                    c = '.';
                tmp_str[l++]=c;
            }
            else
                tmp_str[l++]=' ';
        }
        tmp_str[l++]='\r';        tmp_str[l++]='\n';        tmp_str[l++]='\0';
        RETAILMSG(1, (L"%S", tmp_str));    
    }
}

BOOL ReadFIFO(DWORD* pFifoBase, void *pData, DWORD size)
{
    DWORD total  = size / 4;
    DWORD remain = size % 4;
    DWORD i		 = 0;

    DWORD* pDword = (DWORD*)pData;

    volatile ULONG *pReg = (volatile ULONG*)pFifoBase;

    // this is 32-bit align
    for (i = 0; i < total; i++)
    {
        *pDword++ = INREG32(pReg);
    }
        
    // Set the pByte equal to the last bytes of data being transferred
    if (remain != 0)
    {
        UCHAR* pUCHAR = (UCHAR*) pDword;
        DWORD dwTemp = INREG32(pReg);
        
        while (remain--)
        {
            *pUCHAR++ = (UCHAR) (dwTemp & 0xFF);
            dwTemp>>=8;
        }
    }
/*
    RETAILMSG(1,(TEXT("Read fifo\r\n")));
    memdump((UCHAR*)pData,(USHORT)size,0);
    RETAILMSG(1,(TEXT("\r\n")));
*/
    return TRUE;
}

BOOL WriteFIFO(DWORD* pFifoBase, void *pData, DWORD size)
{
    DWORD total					= size / 4;
    DWORD remain				= size % 4;
    DWORD i						= 0;
    DWORD* pDword				= (DWORD*)pData;

    volatile ULONG *pReg = (volatile ULONG*)pFifoBase;

    // Critical section would be handled outside
    DEBUGMSG(ZONE_VERBOSE, (TEXT("WriteFIFO: total (0x%x), remain (0x%x), size(0x%x)\r\n"), total, remain, size));    
/*
    memdump((UCHAR*)pData,(USHORT)size,0);
    RETAILMSG(1,(TEXT("\r\n")));
*/
    // this is 32-bit align
    for (i = 0; i < total; i++)
    {
        OUTREG32(pReg, *pDword++);
    }

    // Set the pByte equal to the last bytes of data being transferred
    if (remain != 0)
    {
        // Pointer to the first byte of data
        USHORT *pWORD =(USHORT *)pDword;

        // Finally if there is remain
        if (remain & 0x2)  // either 2 or 3
        {
            // Write 2 bytes to there
            OUTREG16(pReg, *pWORD++);        
        }

        if (remain & 0x1)
        {
            // Write 1 byte to there
            OUTREG8(pReg, *((UCHAR*)pWORD));
        }
    }
    return TRUE;
}

// USB processing event

//HANDLE g_hUsbProcessingEvent = NULL;

#ifdef MUSB_USEDMA
CHW* CHW::m_pChw[2] = {NULL, NULL};
#endif

const TCHAR CHW::m_s_cpszName[8] = L"MGHC";

CHW::CHW( IN /*const*/ CSL_UsbRegs * portBase,
          IN /*const*/ CSL_CppiRegs * cppiBase,
          IN const DWORD dwSysIntr,
          IN const DWORD dwDescriptorCount,
          IN CPhysMem *pCPhysMem,
          IN LPVOID pvOhcdPddObject)
:m_portBase(portBase)
,m_cppiBase(cppiBase)
,m_pMem(pCPhysMem)
,m_pPddContext(pvOhcdPddObject)
,m_dwSysIntr(dwSysIntr)
,m_dwDescriptorCount(dwDescriptorCount)
#ifdef MUSB_USEDMA
,m_dmaCrtl(m_dwDescriptorCount)
#endif
{
    // definitions for static variables
    m_pHCCA = 0;
    m_wFrameHigh = 0;
    m_hUsbInterruptEvent = NULL;
    m_hUsbHubChangeEvent = NULL;
    m_hUsbInterruptThread = NULL;
    m_fUsbInterruptThreadClosing = FALSE;
    m_pControlHead = 0;
    m_pBulkInHead = 0;
    m_pBulkOutHead = 0;
    m_pIntInHead = 0;
    m_pIntOutHead = 0;
    m_fConstructionStatus = TRUE;

	m_NumOfPort = 1;

	m_pFreeED = NULL;
	m_pFreeTD = NULL;
	m_EDUsage = 0;
	m_TDUsage = 0;

#ifdef MUSB_USEDMA
#ifdef MUSB_USEDMA_FOR_ISO
    m_pIsoInHead = NULL;
    m_pIsoOutHead = NULL;

#endif // MUSB_USEDMA_FOR_ISO
#endif // MUSB_USEDMA

    m_dwCap = 0;
    m_portStatus = 0;
    m_wFifoOffset = 0;
    lastFn = 0;
    m_fHighSpeed = FALSE;
    m_fPowerUpFlag = FALSE;
    m_fPowerResuming = FALSE;

    m_pProcessEDControl = NULL;

	m_TDs = new USBTD[NO_TD];
	m_EDs = new USBED[NO_ED];
	
    memset(m_pProcessEDIn, 0, sizeof(m_pProcessEDIn));
    memset(m_pProcessEDOut, 0, sizeof(m_pProcessEDOut));

    //CLEAR_INTERRUPTS();
    InitializeCriticalSection( &m_csFrameCounter );
    InitializeCriticalSection( &m_csUsbProcLock );

    //WRITE_PORT_UCHAR((m_portBase + USB_DEVCTL_REG_OFFSET), 0);
    
	m_portBase->EOIR = 0;

    CreateDescriptors();

    /* Creation of the EP Sempahore objects. */
    m_Ep0ProtectSem = CreateSemaphore(NULL, 1, 1, NULL);
    for (UINT8 i = 0; i < (MGC_MAX_USB_ENDS - 1); i ++)
    {
        m_EpInProtectSem[i] = CreateSemaphore(NULL, 1, 1, NULL);
        m_EpOutProtectSem[i] = CreateSemaphore(NULL, 1, 1, NULL);
    }

    // Start with a clean configuration
    memset(m_EpInConfig, 0, sizeof(m_EpInConfig));
    memset(m_EpOutConfig, 0, sizeof(m_EpOutConfig));

    if (!LoadEnpointConfiguration())
    {
        DEBUGMSG( ZONE_ERROR, (TEXT("-CHW::CHW - failed to read EP configuration\n")));
        m_fConstructionStatus = FALSE;
    }
}

CHW::~CHW()
{
    DeInitialize();
    DeleteCriticalSection( &m_csFrameCounter );
    DeleteCriticalSection( &m_csUsbProcLock );

    if (m_Ep0ProtectSem)
        CloseHandle(m_Ep0ProtectSem);
    for (UINT8 i = 0; i < (MGC_MAX_USB_ENDS - 1); i ++)
    {
        if (m_EpInProtectSem[i])
            CloseHandle(m_EpInProtectSem[i]);
        if (m_EpOutProtectSem[i])
            CloseHandle(m_EpOutProtectSem[i]);
    }

	delete [] m_EDs;
	delete [] m_TDs;
}
// ******************************************************************
BOOL CHW::Initialize(void)
//
// Purpose: Reset and Configure the Host Controller with the schedule.
//
// Parameters: portBase - base address for host controller registers
//
//             dwSysIntr - system interrupt number to use for USB
//                         interrupts from host controller
//
//             frameListPhysAddr - physical address of frame list index
//                                 maintained by CPipe class
//
//             pvOhcdPddObject - PDD specific structure used during suspend/resume
//
// Returns: TRUE if initialization succeeded, else FALSE
//
// Notes: This function is only called from the COhcd::Initialize routine.
//
//        This function is static
// ******************************************************************
{
    DEBUGMSG( ZONE_INIT, (TEXT("+CHW::Initialize base=0x%x, intr=0x%x\n"), m_portBase, m_dwSysIntr));

#ifdef DEBUG
    dwTickCountLastTime = GetTickCount();
#endif

    DEBUGCHK( m_wFrameHigh == 0 );

    if (!m_fConstructionStatus)
        return FALSE;

    if ( m_portBase == 0 ) {
        DEBUGMSG( ZONE_ERROR, (TEXT("-CHW::Initialize - zero Register Base\n")));
        return FALSE;
    }

    m_pMem->ReInit();

    InitialiseFIFOs();
    m_bHostEndPointUseageCount = 1;
    m_bulkEpNum = 0;
    m_bulkEpUseCount = 0;
    m_bulkEpConfigured = FALSE;

#ifdef MUSB_USEDMA
    m_pChw[((SOhcdPdd*)m_pPddContext)->dwIndex - 1] = this;

    // Initialize DMA controller
    if (!m_dmaCrtl.Initialize(
		                      (((SOhcdPdd*)m_pPddContext)->dwIndex == 1 )? AM33X_USB0_REGS_PA : AM33X_USB1_REGS_PA,
		                       m_portBase,
							   m_cppiBase,
							   (UINT8)((SOhcdPdd*)m_pPddContext)->dwIndex - 1
							  )
		)
    {
        DEBUGMSG(ZONE_ERROR, (TEXT("-CHW::Initialize. Failed to initialize DMA\n")));
        return FALSE;
    }

#endif // MUSB_USEDMA

    DEBUGCHK( m_hUsbInterruptEvent == NULL );
    m_hUsbInterruptEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
    m_hUsbHubChangeEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
    if ( m_hUsbInterruptEvent == NULL || m_hUsbHubChangeEvent==NULL ){
        DEBUGMSG(ZONE_ERROR, (TEXT("-CHW::Initialize. Error creating USBInterrupt or USBHubEvent event\n")));
        return FALSE;
    }

    InterruptDisable( m_dwSysIntr ); // Just to make sure this is really ours.
    // Initialize Interrupt. When interrupt id # m_sysIntr is triggered,
    // m_hUsbInterruptEvent will be signaled. Last 2 params must be NULL
    if ( !InterruptInitialize( m_dwSysIntr, m_hUsbInterruptEvent, NULL, NULL) ) {
        DEBUGMSG(ZONE_ERROR, (TEXT("-CHW::Initialize. Error on InterruptInitialize\r\n")));
        return FALSE;
    }

    // Start up our IST - the parameter passed to the thread
    // is unused for now
    DEBUGCHK( m_hUsbInterruptThread == NULL &&
              m_fUsbInterruptThreadClosing == FALSE );
    m_hUsbInterruptThread = CreateThread( 0, 0, UsbInterruptThreadStub, this, 0, NULL );
    if ( m_hUsbInterruptThread == NULL ) {
        DEBUGMSG(ZONE_ERROR, (TEXT("-CHW::Initialize. Error creating IST\n")));
        return FALSE;
    }

    CeSetThreadPriority( m_hUsbInterruptThread, g_IstThreadPriority );

    m_hUsbProcessingEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
    m_hUsbProcessingThread = CreateThread( 0, 0, UsbProcessingThreadStub, this, 0, NULL );
    if ( m_hUsbProcessingThread == NULL ) {
        DEBUGMSG(ZONE_ERROR, (TEXT("-CHW::Initialize. Error creating IST\n")));
        return FALSE;
    }

    CeSetThreadPriority( m_hUsbProcessingThread, g_IstThreadPriority + 1 );

    DEBUGMSG(ZONE_INIT, (TEXT("-CHW::Initialize, success!\n")));

    return TRUE;
}

// ******************************************************************
void CHW::DeInitialize( void )
//
// Purpose: Delete any resources associated with static members
//
// Parameters: none
//
// Returns: nothing
//
// Notes: This function is only called from the ~COhcd() routine.
//
//        This function is static
// ******************************************************************
{
    m_fUsbInterruptThreadClosing = TRUE; // tell USBInterruptThread that we are closing
    DEBUGMSG( ZONE_INIT, (TEXT("+CHW::DeInitialize\r\n")));

    // Wake up the interrupt thread and give it time to die.
    if ( m_hUsbInterruptEvent ) {

        SetEvent(m_hUsbInterruptEvent);
        if ( m_hUsbInterruptThread ) {
            DWORD dwWaitReturn = WaitForSingleObject(m_hUsbInterruptThread, 5000);
            if ( dwWaitReturn != WAIT_OBJECT_0 ) {
                DEBUGMSG( ZONE_ERROR, (TEXT("CHW::DeInitialize: m_hUsbInterruptThread didn't exit cleanly\r\n")));
                DEBUGCHK( 0 );
#pragma prefast(suppress:258, "If we reach this code proper thread termination was attempted, but failed. So clean up by force is the last resort.")
#pragma warning(push)
#pragma warning(disable: 6258)
                TerminateThread(m_hUsbInterruptThread, DWORD(-1));
#pragma warning(pop)
            }
            CloseHandle(m_hUsbInterruptThread);
            m_hUsbInterruptThread = NULL;
        }

        // we have to close our interrupt before closing the event!
        InterruptDisable( m_dwSysIntr );
        DEBUGMSG( ZONE_INIT, (TEXT("CHW::DeInitialize: closing event handle %x\r\n"), m_hUsbInterruptEvent));
        CloseHandle(m_hUsbInterruptEvent);
        m_hUsbInterruptEvent = NULL;
    } else {
        InterruptDisable( m_dwSysIntr );
    }

    if ( m_hUsbHubChangeEvent) {
        SetEvent(m_hUsbHubChangeEvent);
        CloseHandle (m_hUsbHubChangeEvent);
        m_hUsbHubChangeEvent = NULL;
    }

    if (m_hUsbProcessingThread)
    {
        SetEvent(m_hUsbProcessingEvent);
        DWORD dwWaitReturn = WaitForSingleObject(m_hUsbProcessingThread, 5000);
        if ( dwWaitReturn != WAIT_OBJECT_0 )
        {
            DEBUGCHK( 0 );
#pragma prefast(suppress:258, "If we reach this code proper thread termination was attempted, but failed. So clean up by force is the last resort.")
#pragma warning(push)
#pragma warning(disable: 6258)
            TerminateThread(m_hUsbProcessingThread, DWORD(-1));
#pragma warning(pop)
        }

        CloseHandle(m_hUsbProcessingThread);
        m_hUsbProcessingThread = NULL;
        CloseHandle(m_hUsbProcessingEvent);
        m_hUsbProcessingEvent = NULL;
    }

    m_fUsbInterruptThreadClosing = FALSE;
    m_wFrameHigh = 0;

#ifdef MUSB_USEDMA

    // Deinitialize CPPI controller
    m_dmaCrtl.Deinitialize();

#endif // MUSB_USEDMA

    DEBUGMSG( ZONE_INIT, (TEXT("-CHW::DeInitialize\r\n")));
}

// ******************************************************************
void CHW::EnterOperationalState( void )
//
// Purpose: Signal the host controller to start processing the schedule
//
// Parameters: None
//
// Returns: Nothing.
//
// Notes: This function is only called from the COhcd::Initialize routine.
//        It assumes that CPipe::Initialize and CHW::Initialize
//        have already been called.
//
//        This function is static
// ******************************************************************
{
    DEBUGMSG( ZONE_INIT, (TEXT("+CHW::EnterOperationalState\n")));

    // Enable non-iso interrupts in CRTL register.
    //WRITE_PORT_UCHAR( (m_portBase+USB_CTRL_REG_OFFSET), BIT3);

    
	m_portBase->INTRUSBE = 0xf7;        // Enable interrupts in USBINTE (apart from SOF)
	m_portBase->TESTMODE = 0;
#ifdef NO_HIGHSPEED
    m_portBase->POWER    = BIT0;        // no HS
#else
 	m_portBase->POWER    = BIT0 | BIT5; // Enable HS
#endif
    USBHPDD_PowerVBUS(TRUE);            // VBUS power on

#if 0 // this code moved down after setting session bin in the devctl
	// Wait for VBUS power to settle - DEVCTL[3-4] bits
    DWORD dwTimeout = GetTickCount() + 2000;
    for(;;) {
		if ((m_portBase->DEVCTL & (BIT3 | BIT4))== (BIT3 | BIT4))
            break;

        if (GetTickCount() > dwTimeout) {
            RETAILMSG(1, (L"USBH: DEVCTL VBUS ready timeout!\r\n"));
            break;
        }
        Sleep(10);
    }
#endif

    // Clear any VBUS error due to power up
    // WRITE_PORT_ULONG( (m_portBase+USB_INTCLRR_REG_OFFSET), 0x80 << CSL_USB_INTMSKR_USB_SHIFT);

    // Enable interrupts
    m_portBase->IRQ_EN_SET_0 = USB_OTG_TXINT_MASK | USB_OTG_RXINT_MASK;
	m_portBase->IRQ_EN_SET_1 = USB_OTG_USBINT_MASK;

    m_portBase->IRQ_EN_CLR_1 = CSL_USB_INTRUSB_SOF_MASK; // SOF is enabled only when required

    m_portBase->DEVCTL = BIT0;					// start session in dev control register

    // Wait for VBUS power to settle - DEVCTL[3-4] bits
	UINT8 r_devctl;
    DWORD dwTimeout = GetTickCount() + 2000;
    for(;;) {
		r_devctl = m_portBase->DEVCTL;
		if ((r_devctl/*m_portBase->DEVCTL*/ & (BIT3 | BIT4))== (BIT3 | BIT4))
            break;

        if (GetTickCount() > dwTimeout) {
            RETAILMSG(1, (L"USBH: DEVCTL VBUS ready timeout!\r\n"));
            break;
        }
        Sleep(10);
    }

	
	DEBUGMSG( ZONE_INIT, (TEXT("-CHW::EnterOperationalState\n")));
}

// ******************************************************************
void CHW::StopHostController( void )
//
// Purpose: Signal the host controller to stop processing the schedule
//
// Parameters: None
//
// Returns: Nothing.
//
// Notes: This function can be called from the power handler callbacks and must
//        therefore abide by the restrictions. No system calls, no blocking.
//        Hence no DEBUGMSG's either.
//        This function is static
// ******************************************************************
{
    DEBUGMSG( ZONE_INIT, (TEXT("+CHW::StopHostController\n")));

    m_portBase->INTRUSBE     = 0;             // Disable interrupts in USBINTE
	m_portBase->IRQ_EN_CLR_0 = USB_OTG_TXINT_MASK | USB_OTG_RXINT_MASK;// Disable interrupts
	m_portBase->IRQ_EN_CLR_1 = USB_OTG_USBINT_MASK;
	
    USBHPDD_PowerVBUS(FALSE);  // Remove power

    DEBUGMSG( ZONE_INIT, (TEXT("-CHW::StopHostController\n")));
}

// ******************************************************************
VOID CHW::PowerMgmtCallback( IN BOOL fOff )
//
// Purpose: System power handler - called when device goes into/out of
//          suspend.
//
// Parameters:  fOff - if TRUE indicates that we're entering suspend,
//                     else signifies resume
//
// Returns: Nothing
//
// Notes: This needs to be implemented for HCDI
// ******************************************************************
{
    DEBUGMSG(ZONE_INIT && ZONE_VERBOSE, (TEXT("+CHW::PowerMgmtCallback(%d)\n\r"), fOff));

    if ( fOff )
    {   
		// suspending...
		m_fPowerResuming = FALSE;
        StopHostController();
    }
    else
    {   // resuming...
        m_fPowerUpFlag = TRUE;
        if (!m_fPowerResuming)
        {
            // can't use member data while `this' is invalid
            DEBUGMSG(ZONE_INIT && ZONE_VERBOSE, (TEXT("CHW::PowerMgmtCallback: can't use member data while `this' is invalid\n\r")));
            SetInterruptEvent(m_dwSysIntr);
        }
    }

    DEBUGMSG(ZONE_INIT && ZONE_VERBOSE, (TEXT("-CHW::PowerMgmtCallback\n\r")));
    return;
}

// ******************************************************************
DWORD CHW::UsbInterruptThreadStub( IN PVOID context )
{
    return ((CHW *)context)->UsbInterruptThread(context);
}

// ******************************************************************
DWORD CHW::UsbInterruptThread( IN PVOID context )
//
// Purpose: Main IST to handle interrupts from the USB host controller
//
// Parameters: context - parameter passed in when starting thread,
//                       (currently unused)
//
// Returns: 0 on thread exit.
//
// Notes:
//
//        This function is private
// ******************************************************************
{
    UINT16 IntrUsbValue;
    UINT8  IntrTxValue;
    UINT8  IntrRxValue;
    DWORD  CoreIntStatus;
	DWORD  EpIntStatus;
    UINT16 _HOST_CSR0, _HOST_RXCSR, _HOST_TXCSR, _RXCOUNT, _COUNT0;
    DWORD CopyCount;
    UINT8 *pPkt, *pData, RequestType, Reg, bShift;
    USBTD *pTD;
    USBED *pED;

	UNREFERENCED_PARAMETER(context);

    DEBUGMSG(ZONE_INIT && ZONE_VERBOSE, (TEXT("+CHW::USBInterruptThread\n\r")));
    while ( !m_fUsbInterruptThreadClosing ) {
        WaitForSingleObject(m_hUsbInterruptEvent, INFINITE);
        if ( m_fUsbInterruptThreadClosing ) {
            break;
        }

        // Power management handling.
        // This flag gets cleared in the resume thread.
        if(m_fPowerUpFlag)
        {
            if (m_fPowerResuming) {
                // this means we've restarted an IST and it's taken an early interrupt;
                // just pretend it didn't happen for now because we're about to be told to exit again.
                continue;
            }
            HcdPdd_InitiatePowerUp((DWORD) m_pPddContext);
            HANDLE ht;

            while ((ht = CreateThread(NULL, 0, CeResumeThreadStub, this, 0, NULL)) == NULL) {
                RETAILMSG(1, (TEXT("CHW::UsbInterruptThread: Cannot start thread for resume; sleeping.\n")));
                Sleep(15000);  // 15 seconds later, maybe it'll work.
            }
            CeSetThreadPriority( ht, g_IstThreadPriority );
            CloseHandle(ht);

            // The CE resume thread will force this IST to exit so we'll be cooperative proactively.
            break;
        }

        LockProcessingThread();

        // Inspect USB Interrupts
        CoreIntStatus = m_portBase->IRQ_STATUS_1;  //USB_CORE_INTMASKEDR_REG_OFFSET
//		m_portBase->IRQ_EN_CLR_1 = CoreIntStatus;
		m_portBase->IRQ_STATUS_1 = CoreIntStatus;
		EpIntStatus   = m_portBase->IRQ_STATUS_0;
		m_portBase->IRQ_STATUS_0 = EpIntStatus;
//		m_portBase->IRQ_EN_CLR_0 = EpIntStatus;

        IntrUsbValue = (UINT16)(CoreIntStatus & USB_OTG_USBINT_MASK);
        IntrRxValue = (UINT8)((EpIntStatus & USB_OTG_RXINT_MASK) >> USB_OTG_RXINT_SHIFT);
        IntrTxValue = (UINT8)(EpIntStatus & USB_OTG_TXINT_MASK) >> USB_OTG_TXINT_SHIFT;
//		((SOhcdPdd*)m_pPddContext)->pSysConfReg->CONTROL_LVL_INTR_CLEAR |= (1 << 4);

        DEBUGMSG(ZONE_VERBOSE, (TEXT("USBInterruptThread: IntrUsbValue=0x%x, IntrRxValue=0x%x, IntrTxValue=0x%x\n\r"), 
            IntrUsbValue, IntrRxValue, IntrTxValue));

        if(IntrUsbValue)
        {
            if (IntrUsbValue & CSL_USB_INTRUSB_SOF_MASK)
            {
                // SOFs are handled by the processing thread
            }

            if (IntrUsbValue & CSL_USB_INTRUSB_VBUSERR_MASK)
            {
                // session is stopped due to VBUS error
                RETAILMSG(1, (L"USBH: VBUS error detected - resetting controller\r\n"));
                USBHPDD_PowerVBUS(FALSE);
                WRITE_PORT_ULONG( (m_portBase+USB_CTRL_REG_OFFSET), BIT0);  // soft reset of USB module
                InitialiseFIFOs();
                EnterOperationalState();
            }
            else
            {
                if (IntrUsbValue & CSL_USB_INTRUSB_CONN_MASK)
                {
                    //device connected
                    m_portStatus = m_portStatus | (PORT_STATUS_CON_CHANGED | PORT_STATUS_DEVICE_ATTACHED);
                    SetEvent(m_hUsbHubChangeEvent);
                }
                else if (IntrUsbValue & CSL_USB_INTRUSB_DISCON_MASK)
                {
                    //device disconnected
                    m_portStatus = (m_portStatus & ~PORT_STATUS_DEVICE_ATTACHED) | PORT_STATUS_CON_CHANGED;
                    SetEvent(m_hUsbHubChangeEvent);
                }

#if 0
                if (IntrUsbValue & CSL_USB_INTRUSB_DRVVBUSCHG_MASK)
                {
                    RETAILMSG(1, (L"USBH: DRVVBUS changed\r\n"));
                    UINT32 drvvbus = READ_PORT_ULONG( (m_portBase+USB_STATR_REG_OFFSET));
                    if (drvvbus)
                        USBHPDD_PowerVBUS(TRUE);
                }
#endif
            }

            // RETAILMSG(FALSE,(TEXT("DevCtl = 0x%x"),READ_PORT_UCHAR( (m_portBase+USB_DEVCTL_REG_OFFSET))));
        }


        if (IntrTxValue & BIT0)
        {
            /*EP0 interrupt*/
            pED = (USBED*)m_pProcessEDControl;
            if (!pED)
            {
                // EP0 transfer aborted
                goto _skipControl;
            }
            pTD = (USBTD*)pED->HeadTD;

            // Skip if no TDs or only the NULL TD
            if (!pTD || pED->HeadTD == pED->TailTD)
            {
                goto _skipControl;
            }

            pPkt = (UINT8 *)pTD->sTransfer.lpvClientBuffer;

            // Check for an error condition (can occur at any stage)
            _HOST_CSR0 = m_portBase->EPCSR[0].TXCSR;
            if (_HOST_CSR0 & (MGC_M_CSR0_H_ERROR | MGC_M_CSR0_H_RXSTALL | MGC_M_CSR0_H_NAKTIMEOUT))
            {
                if (_HOST_CSR0 & MGC_M_CSR0_H_ERROR) {
                    _HOST_CSR0 &= (~MGC_M_CSR0_H_ERROR);
                    *pTD->sTransfer.lpfComplete = TRUE;
                    *pTD->sTransfer.lpdwError = USB_DEVICE_NOT_RESPONDING_ERROR;
                }
                if(_HOST_CSR0 & MGC_M_CSR0_H_RXSTALL) {
                    _HOST_CSR0 &= (~MGC_M_CSR0_H_RXSTALL);
                    *pTD->sTransfer.lpfComplete = TRUE;
                    *pTD->sTransfer.lpdwError = USB_STALL_ERROR;
                }
                if(_HOST_CSR0 & MGC_M_CSR0_H_NAKTIMEOUT) {
                    _HOST_CSR0 &= (~(MGC_M_CSR0_H_NAKTIMEOUT|MGC_M_CSR0_H_REQPKT|MGC_M_CSR0_TXPKTRDY));
                    *pTD->sTransfer.lpfComplete = TRUE;
                    *pTD->sTransfer.lpdwError = USB_DEVICE_NOT_RESPONDING_ERROR;
                }
                _HOST_CSR0 |= MGC_M_CSR0_FLUSHFIFO;
				m_portBase->EPCSR[0].TXCSR = _HOST_CSR0;
                pED->TransferStatus = STATUS_COMPLETE;
                goto _skipControl;
            }

            switch(pTD->TransferStage){
                /*case current stage*/
                case STAGE_SETUP:
                    _HOST_CSR0 = m_portBase->EPCSR[0].TXCSR;
                    if(!(_HOST_CSR0 & MGC_M_CSR0_TXPKTRDY)){
                        /*TX successful*/
                        if(pTD->BytesToTransfer){
                            /*data stage required*/
                            pPkt = (UINT8 *)pTD->sTransfer.lpvControlHeader;
                            RequestType = *pPkt;
                            pTD->TransferStage = (RequestType & 0x80)? STAGE_DATAIN : STAGE_DATAOUT ;
                        } else {
                            pTD->TransferStage = STAGE_STATUSIN; /*no data stage*/ 
                        }
                    } else{
                        RETAILMSG(FALSE,(TEXT("STAGE_SETUP:_HOST_CSR0 = 0x%x"),_HOST_CSR0));
                        RETAILMSG(1, (L"INCTL-NRDY\r\n"));
                    }
                    pED->TransferStatus = STATUS_IDLE;
                    break;
                case STAGE_DATAIN: /*Device to host*/
                    _HOST_CSR0 = m_portBase->EPCSR[0].TXCSR;
                    /*check for RXPKTRDY*/
                    if (_HOST_CSR0 & MGC_M_CSR0_RXPKTRDY) {
                        pPkt = (UINT8*)pTD->sTransfer.lpvClientBuffer;
                        CopyCount = _COUNT0 = m_portBase->EPCSR[0].RXCOUNT;
                        pData = pPkt;
                        pData += pTD->BytesTransferred;
						
						ReadFIFO((DWORD*)&(m_portBase->FIFO[0]), pData, CopyCount);
						pTD->BytesToTransfer -= CopyCount;
						pTD->BytesTransferred += CopyCount;
                        _HOST_CSR0 &= (~MGC_M_CSR0_RXPKTRDY);
						m_portBase->EPCSR[0].TXCSR = _HOST_CSR0;

						pTD->TransferStage = (!pTD->BytesToTransfer || (_COUNT0 < pED->bfMaxPacketSize))?
                                             STAGE_STATUSOUT : /*data phase complete*/ 
                                             STAGE_DATAIN;      /* data phase to continue*/;
			            pED->TransferStatus = STATUS_IDLE;
                    } else {
                        RETAILMSG(FALSE,(TEXT("STAGE_DATAIN:_HOST_CSR0 = 0x%x"),_HOST_CSR0));
                        RETAILMSG(0, (L"1CTL_%08X\r\n", pTD));
                    }
                    break;
                case STAGE_DATAOUT:
                    /*Host to device*/
                    _HOST_CSR0 = m_portBase->EPCSR[0].TXCSR;
                    if(!(_HOST_CSR0 & MGC_M_CSR0_TXPKTRDY)){
                        /*TX successful*/
                        pTD->TransferStage = (!pTD->BytesToTransfer) ? STAGE_STATUSIN : STAGE_DATAOUT;
                        pED->TransferStatus = STATUS_IDLE;
                    } else {
                        RETAILMSG(FALSE,(TEXT("STAGE_DATAOUT:_HOST_CSR0 = 0x%x"),_HOST_CSR0));
                        RETAILMSG(0, (L"2CTL_%08X\r\n", pTD));
                    }
                    break;
                case STAGE_STATUSIN:
                    _HOST_CSR0 = m_portBase->EPCSR[0].TXCSR;
                    if (_HOST_CSR0 & MGC_M_CSR0_RXPKTRDY) {
                        _HOST_CSR0 &= (~MGC_M_CSR0_RXPKTRDY);
                        m_portBase->EPCSR[0].TXCSR = _HOST_CSR0;
                    } else{
                        RETAILMSG(FALSE,(TEXT("STAGE_STATUSIN:_HOST_CSR0 = 0x%x"),_HOST_CSR0));
                    }
                    *pTD->sTransfer.lpdwBytesTransferred = pTD->BytesTransferred;
                    *pTD->sTransfer.lpfComplete = TRUE;
                    *pTD->sTransfer.lpdwError = USB_NO_ERROR;
                    pED->TransferStatus = STATUS_COMPLETE;
                    RETAILMSG(0, (L"3CTL_%08X\r\n", pTD));
                    break;
                case STAGE_STATUSOUT:
                    _HOST_CSR0 = m_portBase->EPCSR[0].TXCSR;
                    if(!(_HOST_CSR0 & MGC_M_CSR0_TXPKTRDY)){
                        /*TX successful*/
                    } else {
                        RETAILMSG(FALSE,(TEXT("STAGE_STATUSOUT:_HOST_CSR0 = 0x%x"),_HOST_CSR0));
                    }
                    *pTD->sTransfer.lpdwBytesTransferred = pTD->BytesTransferred;
                    *pTD->sTransfer.lpfComplete = TRUE;
                    *pTD->sTransfer.lpdwError = USB_NO_ERROR;
                    pED->TransferStatus = STATUS_COMPLETE;
                    RETAILMSG(0, (L"4CTL_%08X\r\n", pTD));
                    break;
            }
        }

_skipControl:

        /*RX interrupt*/
        Reg = IntrRxValue;
        bShift = 1;
        Reg >>= 1;
        while (Reg) {
            if (Reg & 1) {
                pED = (USBED*)m_pProcessEDIn[bShift - 1];
                if(pED){
                    pTD = (USBTD*)pED->HeadTD;

                    // Skip if no TDs or only the NULL TD
                    if (!pTD || pED->HeadTD == pED->TailTD)
                        goto _skipRx;

                    _HOST_RXCSR = m_portBase->EPCSR[bShift].RXCSR;

                    /*check for RXPKTRDY*/
                    if (_HOST_RXCSR & MGC_M_RXCSR_RXPKTRDY) {
                        //pED->bfToggleCarry = (!pED->bfToggleCarry);
                        pED->bfToggleCarry = (_HOST_RXCSR & (0x1<<9));
                        pPkt = (UINT8*)pTD->sTransfer.lpvClientBuffer;
                        CopyCount = _RXCOUNT = m_portBase->EPCSR[bShift].RXCOUNT;
						if(pTD->BytesToTransfer < CopyCount) {
                            /*data OVERRUN*/
                            UINT8 temp;
                            while (CopyCount) {
                                temp = READ_PORT_UCHAR(&m_portBase->FIFO[bShift]);
                                CopyCount--;
                            }
                            _HOST_RXCSR &= (~MGC_M_RXCSR_RXPKTRDY);
                            m_portBase->EPCSR[bShift].RXCSR = _HOST_RXCSR/*|MGC_M_RXCSR_FLUSHFIFO*/;
                            *pTD->sTransfer.lpdwBytesTransferred = 0;
                            *pTD->sTransfer.lpfComplete = TRUE;
                            *pTD->sTransfer.lpdwError = USB_DATA_OVERRUN_ERROR;
                            pED->bfHalted = TRUE;
                            pED->TransferStatus = STATUS_COMPLETE;
                        } else{
                            pData = pPkt;
                            pData += pTD->BytesTransferred;

							ReadFIFO((DWORD*)&(m_portBase->FIFO[bShift]), pData, CopyCount);

                            pTD->BytesTransferred += _RXCOUNT;
                            pTD->BytesToTransfer -= _RXCOUNT;
                            _HOST_RXCSR &= (~MGC_M_RXCSR_RXPKTRDY);
                            m_portBase->EPCSR[bShift].RXCSR =_HOST_RXCSR;
    /*5.8.3 of USB 2.0*/
                            if(!pTD->BytesToTransfer){
                                *pTD->sTransfer.lpdwBytesTransferred = pTD->BytesTransferred;
                                *pTD->sTransfer.lpfComplete = TRUE;
                                *pTD->sTransfer.lpdwError = USB_NO_ERROR;
                                pED->TransferStatus = STATUS_COMPLETE;
                            }
                            else if(_RXCOUNT < pED->bfMaxPacketSize){
                                /*data UNDERRUN (pTD->BytesToTransfer is not 0) handling depending on USB_SHORT_TRANSFER_OK*/
                                if(pTD->sTransfer.dwFlags & USB_SHORT_TRANSFER_OK){
                                    *pTD->sTransfer.lpdwBytesTransferred = pTD->BytesTransferred;
                                    *pTD->sTransfer.lpfComplete = TRUE;
                                    *pTD->sTransfer.lpdwError = USB_NO_ERROR;
                                    pED->TransferStatus = STATUS_COMPLETE;
                                }
                                else{
                                    *pTD->sTransfer.lpdwBytesTransferred = pTD->BytesTransferred;
                                    *pTD->sTransfer.lpfComplete = TRUE;
                                    *pTD->sTransfer.lpdwError = USB_DATA_UNDERRUN_ERROR;
                                    pED->bfHalted = TRUE;
                                    pED->TransferStatus = STATUS_COMPLETE;
                                }
                            }
                            else
                                pED->TransferStatus = STATUS_IDLE;
                        }
                    }
                    else{
                        RETAILMSG(FALSE,(TEXT("hostEP= %d HOST_RXCSR = 0x%x"),pED->bHostEndPointNum,_HOST_RXCSR));

                        TransferStatus Status = STATUS_IDLE;
                        if(_HOST_RXCSR & MGC_M_RXCSR_H_ERROR){
                            _HOST_RXCSR &= (~(MGC_M_RXCSR_H_REQPKT|MGC_M_RXCSR_H_ERROR));
                            //*pTD->sTransfer.lpdwBytesTransferred = pTD->BytesTransferred;
                            //*pTD->sTransfer.lpfComplete = TRUE;
                            //*pTD->sTransfer.lpdwError = USB_DEVICE_NOT_RESPONDING_ERROR;
                            Status = STATUS_NO_RESPONSE;
                        }
                        if(_HOST_RXCSR & MGC_M_RXCSR_H_RXSTALL){
                            _HOST_RXCSR &= (~(MGC_M_RXCSR_H_REQPKT|MGC_M_RXCSR_H_RXSTALL));
                            //*pTD->sTransfer.lpdwBytesTransferred = pTD->BytesTransferred;
                            *pTD->sTransfer.lpfComplete = TRUE;
                            *pTD->sTransfer.lpdwError = USB_STALL_ERROR;
                            pED->bfHalted = TRUE;
                            Status = STATUS_COMPLETE;
                        }
                        if(_HOST_RXCSR & MGC_M_RXCSR_DATAERROR){
                            _HOST_RXCSR &= (~(MGC_M_RXCSR_H_REQPKT|MGC_M_RXCSR_DATAERROR));
                            //*pTD->sTransfer.lpfComplete = TRUE;
                            //*pTD->sTransfer.lpdwError = USB_DEVICE_NOT_RESPONDING_ERROR;
                            Status = STATUS_NAK;
                        }
                        m_portBase->EPCSR[bShift].RXCSR = _HOST_RXCSR;
                        pED->TransferStatus = Status;
                    }
                }
            }
            Reg >>= 1;
            bShift++;
        }

_skipRx:

        /*TX interrupt*/
        Reg = IntrTxValue;
        bShift = 1;
        Reg >>= 1;
        while (Reg) {
            if (Reg & 1) {
                pED = (USBED*)m_pProcessEDOut[bShift - 1];
                if(pED){
                    pTD = (USBTD*)pED->HeadTD;

                    // Skip if no TDs or only the NULL TD
                    if (!pTD || pED->HeadTD == pED->TailTD)
                        goto _skipTx;

                    _HOST_TXCSR = m_portBase->EPCSR[bShift].TXCSR;
                    if(!(_HOST_TXCSR & MGC_M_TXCSR_H_RXSTALL) &&
                       !(_HOST_TXCSR & MGC_M_TXCSR_H_NAKTIMEOUT) &&
                       !(_HOST_TXCSR & MGC_M_TXCSR_H_ERROR))
                    {
                        if (!(_HOST_TXCSR & MGC_M_TXCSR_TXPKTRDY))
                        {
                            /*TX successful*/
                            //pED->bfToggleCarry = (!pED->bfToggleCarry);
                            pED->bfToggleCarry = (_HOST_TXCSR & (0x1<<8));
                            pTD->BytesTransferred += pTD->TXCOUNT;
                            pTD->BytesToTransfer -= pTD->TXCOUNT;
                            pTD->TXCOUNT = 0;
                            if(pTD->BytesToTransfer){
                                pED->TransferStatus = STATUS_IDLE;
                            } else{
                                *pTD->sTransfer.lpdwBytesTransferred = pTD->BytesTransferred;
                                *pTD->sTransfer.lpfComplete = TRUE;
                                *pTD->sTransfer.lpdwError = USB_NO_ERROR;
                                pED->TransferStatus = STATUS_COMPLETE;
                            }
                        } else {
                            // TX not complete yet, wait for next interrupt
                            // RETAILMSG(1, (L"EP%d: TX not complete yet!\r\n", pED->bHostEndPointNum));
                        }
                    } else{
                        RETAILMSG(FALSE,(TEXT("_HOST_TXCSR = 0x%x"),_HOST_TXCSR));
                        TransferStatus Status = STATUS_COMPLETE;

                        if(_HOST_TXCSR & MGC_M_TXCSR_H_ERROR){
                            _HOST_TXCSR &= (~MGC_M_TXCSR_H_ERROR);

                            //*pTD->sTransfer.lpdwBytesTransferred = pTD->BytesTransferred;
                            //*pTD->sTransfer.lpfComplete = TRUE;
                            //*pTD->sTransfer.lpdwError = USB_DEVICE_NOT_RESPONDING_ERROR;
                            Status = STATUS_NO_RESPONSE;
                        }
                        if(_HOST_TXCSR & MGC_M_TXCSR_H_RXSTALL){
                            _HOST_TXCSR &= (~MGC_M_TXCSR_H_RXSTALL);
                            //*pTD->sTransfer.lpdwBytesTransferred = pTD->BytesTransferred;
                            *pTD->sTransfer.lpfComplete = TRUE;
                            *pTD->sTransfer.lpdwError = USB_STALL_ERROR;
                            pED->bfHalted = TRUE;
                            Status = STATUS_COMPLETE;
                        }
                        if(_HOST_TXCSR & MGC_M_TXCSR_H_NAKTIMEOUT){
                            _HOST_TXCSR &= (~(MGC_M_TXCSR_H_NAKTIMEOUT|MGC_M_TXCSR_TXPKTRDY));
                            //*pTD->sTransfer.lpdwBytesTransferred = pTD->BytesTransferred;
                            //*pTD->sTransfer.lpfComplete = TRUE;
                            //*pTD->sTransfer.lpdwError = USB_DEVICE_NOT_RESPONDING_ERROR;
                            _HOST_TXCSR |= MGC_M_TXCSR_FLUSHFIFO;
                            Status = STATUS_IDLE;
                        }
                        m_portBase->EPCSR[bShift].TXCSR = _HOST_TXCSR;
                        pED->TransferStatus = Status;
                    }
                }
            }
            Reg >>= 1;
            bShift++;
        }

_skipTx:

        UnlockProcessingThread();
        SetEvent(m_hUsbProcessingEvent);
		m_portBase->EOIR = 0;

        InterruptDone(m_dwSysIntr);
    }

    // Disable and clear interrupts
	m_portBase->IRQ_EN_CLR_0 = USB_OTG_TXINT_MASK | USB_OTG_RXINT_MASK;
	m_portBase->IRQ_EN_CLR_1 = USB_OTG_USBINT_MASK; 
	m_portBase->IRQ_STATUS_0 = USB_OTG_TXINT_MASK | USB_OTG_RXINT_MASK;
	m_portBase->IRQ_STATUS_1 = USB_OTG_USBINT_MASK;
	m_portBase->EOIR         = 0;

    InterruptDone(m_dwSysIntr);

    DEBUGMSG(ZONE_INIT && ZONE_VERBOSE, (TEXT("-CHW::USBInterruptThread\n")));
    return (0);
}

// ******************************************************************
void CHW::UpdateFrameCounter( void )
//
// Purpose: Updates our internal frame counter
//
// Parameters: None
//
// Returns: Nothing
//
// Notes: The frame number register is 16 bits long.
//        Thus, the counter will wrap approx. every 64 seconds.
//        We maintain an additional 16 bits internally so we
//        needn't wrap for 50 days.
//
//        This function should be called at least once a minute;
//        otherwise we could miss frames.
//
// ******************************************************************
{

    EnterCriticalSection( &m_csFrameCounter );

#if 1 // Frame counter is only 11-bit long!

#ifdef DEBUG
    // If this fails, we haven't been called in a long time,
    // so the frame number is no longer accurate
    if (GetTickCount() - dwTickCountLastTime >= 0x700 )
        DEBUGMSG(1, (TEXT("CHW::UpdateFrameCounter missed frame count;")
                     TEXT(" isoch packets may have been dropped.\r\n")));
    dwTickCountLastTime = GetTickCount();
#endif // DEBUG

#else  // Original code

#ifdef DEBUG
    // If this fails, we haven't been called in a long time,
    // so the frame number is no longer accurate
    if (GetTickCount() - dwTickCountLastTime >= 64000 )
        DEBUGMSG(1, (TEXT("CHW::UpdateFrameCounter missed frame count;")
                     TEXT(" isoch packets may have been dropped.\r\n")));
    dwTickCountLastTime = GetTickCount();
#endif // DEBUG

#endif

    //WORD fn = m_pHCCA->HccaFrameNumber;
    
    WORD fn = m_portBase->FRAME;

    if (fn < lastFn)
        ++m_wFrameHigh;
    lastFn = fn;

    LeaveCriticalSection( &m_csFrameCounter );
}

// ******************************************************************
BOOL CHW::GetFrameNumber(OUT LPDWORD lpdwFrameNumber )
//
// Purpose: Return the current frame number
//
// Parameters: None
//
// Returns: 32 bit current frame number
//
// Notes: See also comment in UpdateFrameCounter
// ******************************************************************
{
    UpdateFrameCounter();

#if 1 // Frame counter is only 11-bit long!

    *lpdwFrameNumber = ((DWORD) m_wFrameHigh << 11) | lastFn ;

#else // Original code

    *lpdwFrameNumber = ((DWORD) m_wFrameHigh << 16) | lastFn ;

#endif

    return TRUE;
}

// ******************************************************************
BOOL CHW::GetFrameLength(OUT LPUSHORT lpuFrameLength)
//
// Purpose: Return the current frame length in 12 MHz clocks
//          (i.e. 12000 = 1ms)
//
// Parameters: None
//
// Returns: frame length
//
// Notes:
// ******************************************************************
{
    *lpuFrameLength = 12000;
    return TRUE;
}

// ******************************************************************
BOOL CHW::SetFrameLength( IN HANDLE /*hEvent*/,
                          IN const USHORT /*uFrameLength*/ )
//
// Purpose: Set the Frame Length in 12 Mhz clocks. i.e. 12000 = 1ms
//
// Parameters:  hEvent - event to set when frame has reached required
//                       length
//
//              uFrameLength - new frame length
//
// Returns: TRUE if frame length changed, else FALSE
//
// Notes:
// ******************************************************************
{
    // Not supported
    return FALSE;
}

// ******************************************************************
BOOL CHW::DidPortStatusChange( IN const UCHAR port )
//
// Purpose: Determine whether the status of root hub port # "port" changed
//
// Parameters: port - 0 for the hub itself, otherwise the hub port number
//
// Returns: TRUE if status changed, else FALSE
//
// Notes:
// ******************************************************************
{
    USB_HUB_AND_PORT_STATUS s;
    CHW::GetPortStatus(port, s);
    return s.change.word ? TRUE : FALSE;
}

// ******************************************************************
BOOL CHW::GetPortStatus( IN const UCHAR port,
                         OUT USB_HUB_AND_PORT_STATUS& rStatus )
//
// Purpose: This function will return the current root hub port
//          status in a non-hardware specific format
//
// Parameters: port - 0 for the hub itself, otherwise the hub port number
//
//             rStatus - reference to USB_HUB_AND_PORT_STATUS to get the
//                       status
//
// Returns: TRUE
//
// Notes:
// ******************************************************************
{
    memset( &rStatus, 0, sizeof( USB_HUB_AND_PORT_STATUS ) );

    if ( port > 0 )
    {
        rStatus.change.port.ConnectStatusChange = (m_portStatus & PORT_STATUS_CON_CHANGED) ? 1 : 0;
        rStatus.change.port.PortEnableChange = 0;
        rStatus.change.port.OverCurrentChange = 0;
        rStatus.change.port.SuspendChange = 0;
        rStatus.change.port.ResetChange = 0;
        /* high speed - !PORT_LOW_SPEED && PORT_HIGH_SPEED
         * low speed  - PORT_LOW_SPEED
         * full speed - !PORT_LOW_SPEED && !PORT_HIGH_SPEED
         */
        rStatus.status.port.DeviceIsLowSpeed = (m_portBase->DEVCTL & (1<<5))?1:0;
        rStatus.status.port.DeviceIsHighSpeed = (m_portBase->DEVCTL & (1<<6))?1:0;
        rStatus.status.port.PortConnected = (m_portStatus & PORT_STATUS_DEVICE_ATTACHED) ? 1 : 0;
        rStatus.status.port.PortEnabled = 1;
        rStatus.status.port.PortOverCurrent = 0;
        // we assume root hub ports are always powered, but believe the HW.
        rStatus.status.port.PortPower = 1;
        rStatus.status.port.PortReset = 0;
        rStatus.status.port.PortSuspended = 0;
    }
#ifdef DEBUG // these are available in OHCI but this driver doesn't use them
    else {
        // request is to Hub. rStatus was already memset to 0 above.
        DEBUGCHK( port == 0 );
        // local power supply good
        DEBUGCHK( rStatus.status.hub.LocalPowerStatus == 0 );
        // no over current condition
        DEBUGCHK( rStatus.status.hub.OverCurrentIndicator == 0 );
        // no change in power supply status
        DEBUGCHK( rStatus.change.hub.LocalPowerChange == 0 );
        // no change in over current status
        DEBUGCHK( rStatus.change.hub.OverCurrentIndicatorChange == 0 );
    }
#endif // DEBUG

    return TRUE;
}

// ******************************************************************
void CHW::GetRootHubDescriptor( OUT USB_HUB_DESCRIPTOR &descriptor )
//
// Purpose: Calculate and return a hub descriptor for the root hub so
//          that the upper-level code can pretend it's an external hub.
//
// Parameters: a reference to a descriptor to fill in.
//
// Returns: nothing
//
// Notes: assumes the hardware is sufficiently initialized.
// ******************************************************************
{
    PREFAST_DEBUGCHK ( m_portBase != 0 );

    descriptor.bNumberOfPorts = 1;
    DEBUGCHK( descriptor.bNumberOfPorts >= 1 && descriptor.bNumberOfPorts <= 15 );

    descriptor.bDescriptorLength = USB_HUB_DESCRIPTOR_MINIMUM_SIZE + (1-1)*2;
    descriptor.bDescriptorType = USB_HUB_DESCRIPTOR_TYPE;
}

// ******************************************************************
BOOL CHW::RootHubFeature( IN const UCHAR port,
                          IN const UCHAR setOrClearFeature,
                          IN const USHORT feature )
//
// Purpose: This function clears all the status change bits associated with
//          the specified root hub port.
//
// Parameters: port - 0 for the hub itself, otherwise the hub port number
//
// Returns: TRUE iff the requested operation is valid, FALSE otherwise.
//
// Notes: Assume that caller has already verified the parameters from a USB
//        perspective. The HC hardware may only support a subset of that
//        (which is indeed the case for OHCI).
// ******************************************************************
{
    BOOL fResult = FALSE;

    DEBUGMSG(0, (L"!!! CHW::RootHubFeature: port %d, setorclear %d, feature %d\r\n",
        (UINT32)port, (UINT32)setOrClearFeature, (UINT32)feature));

    if (port > 0)
    {
        // Clear port port con status change feature ...

        if (setOrClearFeature == USB_REQUEST_CLEAR_FEATURE)
        {
            switch (feature)
            {
            case USB_HUB_FEATURE_C_PORT_CONNECTION:

                DEBUGMSG(0, (L"!!! CHW::RootHubFeature: port %d, setorclear USB_REQUEST_CLEAR_FEATURE, feature USB_HUB_FEATURE_C_PORT_CONNECTION\r\n",
                    (UINT32)port));

                m_portStatus &= ~PORT_STATUS_CON_CHANGED;
                break;
            }
        }

        fResult = TRUE;
    }

    DEBUGMSG(0, (L"!!! CHW::RootHubFeature: port %d, setorclear %d, feature %d, result %d\r\n",
        (UINT32)port, (UINT32)setOrClearFeature, (UINT32)feature, fResult));

    return fResult;
}

// ******************************************************************
BOOL CHW::ResetAndEnablePort( IN const UCHAR port )
//
// Purpose: reset/enable device on the given port so that when this
//          function completes, the device is listening on address 0
//
// Parameters: port - root hub port # to reset/enable
//
// Returns: TRUE if port reset and enabled, else FALSE
//
// Notes: This function takes approx 60 ms to complete, and assumes
//        that the caller is handling any critical section issues
//        so that two different ports (i.e. root hub or otherwise)
//        are not reset at the same time.
// ******************************************************************
{
    BOOL fSuccess = TRUE;
    UCHAR Power;
    if( port >0 ){
        // reset the host controller. It is recommended to wait for 50ms
        Power = m_portBase->POWER;
        m_portBase->POWER = Power | BIT3 ;
        Sleep(200);
		m_portBase->POWER = Power & (~BIT3);
        Power = m_portBase->POWER;
		
        RETAILMSG(FALSE,(TEXT("Power = 0x%x\n"),Power));
        Sleep(400);
        m_fHighSpeed = (Power & (1<<4)) ? TRUE : FALSE;
    }

    DEBUGMSG( ZONE_REGISTERS, (TEXT("Root hub, after reset & enable, port %d \n"), port) );
    return fSuccess;
}

// ******************************************************************
void CHW::DisablePort( IN const UCHAR port )
//
// Purpose: disable the given root hub port
//
// Parameters: port - port # to disable
//
// Returns: nothing
//
// Notes: This function will take about 10ms to complete
// ******************************************************************
{
    RootHubFeature(port, USB_REQUEST_CLEAR_FEATURE, USB_HUB_FEATURE_PORT_ENABLE);
}
BOOL CHW::WaitForPortStatusChange (HANDLE m_hHubChanged)
{
    if (m_hUsbHubChangeEvent)
        {
        if (m_hHubChanged!=NULL)
            {
            HANDLE hArray[2];
            hArray[0]=m_hHubChanged;
            hArray[1]=m_hUsbHubChangeEvent;
            DEBUGMSG(ZONE_INIT && ZONE_VERBOSE, (TEXT("-CHW::WaitForPortStatusChange - multiple\n")));
            WaitForMultipleObjects(2,hArray,FALSE,INFINITE);
            }
        else
            {
            DEBUGMSG(ZONE_INIT && ZONE_VERBOSE, (TEXT("-CHW::WaitForPortStatusChange - single\n")));
            WaitForSingleObject(m_hUsbHubChangeEvent,INFINITE);
            }
        return TRUE;
        }

    return FALSE;
}
#ifdef DEBUG
// ******************************************************************
void CHW::DumpAllRegisters( void )
//
// Purpose: Queries Host Controller for all registers, and prints
//          them to DEBUG output.
//
// Parameters: None
//
// Returns: Nothing
//
// Notes: used in DEBUG mode only
//
//        This function is static
// ******************************************************************
{
    DEBUGMSG(ZONE_REGISTERS, (TEXT("CHW - DUMP REGISTERS BEGIN\n")));
    for ( USHORT port = 1; port <= UHCD_NUM_ROOT_HUB_PORTS; port++ ) {
        ;
    }
    DEBUGMSG(ZONE_REGISTERS, (TEXT("CHW - DUMP REGISTERS DONE\n")));
}
#endif

DWORD CHW::UsbProcessingThreadStub( IN PVOID context )
{
    return ((CHW *)context)->UsbProcessingThread();
}

// ******************************************************************
DWORD CHW::UsbProcessingThread( )
//
// Purpose: Main IST to handle interrupts from the USB host controller
//
// Parameters: context - parameter passed in when starting thread,
//                       (currently unused)
//
// Returns: 0 on thread exit.
//
// Notes:
//
//        This function is private
// ******************************************************************
{
    USBED *pED;
    USBTD *pTD;
    BOOL fEnableSOFStateChanged = FALSE;
    DWORD dwWait;

#ifdef MUSB_USEDMA
    DWORD dwNow, dwLastStateCheck = 0;
#ifdef MUSB_USEDMA_FOR_ISO
    BOOL fEnableSOF = FALSE;
#endif
#endif

    for(;;)
    {
        dwWait = WaitForSingleObject(m_hUsbProcessingEvent, PROCESSING_EVENT_TIMEOUT);
        if (m_fUsbInterruptThreadClosing)
            break;

#ifdef MUSB_USEDMA

        // Validate DMA transfer state
        //
        // Notes: as far as error conditions (e.g. STALL) do not trigger DMA completion
        //        interrupt, the only possibilities to detect those errors are:
        //
        //        1) Enable/handle both EP and CPPI interrupts;
        //        2) Periodically poll transfer status checking for errors.
        //
        //        The first method is too expensive involving at least TWO interrupt
        //        events per USB packet! We will use the second one instead.

        if (((SOhcdPdd*)m_pPddContext)->CurrentDx == D0) // Do not fail transfers when powered down
        {
            dwNow = GetTickCount();
            if ((dwWait == WAIT_TIMEOUT) || ((INT32)(dwNow - dwLastStateCheck) >= PROCESSING_EVENT_TIMEOUT))
            {
                dwLastStateCheck = dwNow;
                if (m_dmaCrtl.ValidateTransferState() && (dwWait == WAIT_TIMEOUT))
                {
                    // Nothing to do, just continue sleeping
                    continue;
                }
            }
        }

#endif // MUSB_USEDMA
        LockProcessingThread();

        fEnableSOFStateChanged = FALSE;

        ///////////////////////////////////////////////////////////////////////////
        // Control requests

        pED = (USBED*)m_pControlHead;
        while(pED){
redoforsameEDCtl:
            if(pED->HeadTD && (pED->HeadTD != pED->TailTD)){
                pTD = (USBTD *)pED->HeadTD;
                switch(pED->TransferStatus){
                case STATUS_IDLE:
                    if(!pED->bfSkip){
                        m_pProcessEDControl = (PDWORD)pED;
                        pED->TransferStatus = STATUS_PROGRESS;
                        InitializeTransaction(m_portBase, pED, pTD);
                        RETAILMSG(0, (L"PCTL_%08X\r\n", pTD));
                    }
                    break;
                case STATUS_PROGRESS:
                    if ((m_portStatus & PORT_STATUS_DEVICE_ATTACHED) == 0)
                    {
                        // Device disconnected, need to fail the transfer
                        *pTD->sTransfer.lpfComplete = TRUE;
                        *pTD->sTransfer.lpdwError = USB_CANCELED_ERROR;
                        pED->TransferStatus = STATUS_COMPLETE;
                        goto redoforsameEDCtl;
                    }
                    break;
                case STATUS_COMPLETE:

                    RETAILMSG(0, (L"PCTL_%08X--\r\n", pTD));
                    if(RemoveElementFromList((ListHead**)&pED->HeadTD, (ListHead*)pTD)){
                        CTRLTCMSG(1, (L"TC: EP0 len %d", pTD->BytesTransferred));
                        if (pTD->sTransfer.lpfnCallback)
                        {
                            ( *pTD->sTransfer.lpfnCallback )( pTD->sTransfer.lpvCallbackParameter );
                        }
                        FreeTD(pTD);
                    }
                    else{
                        RETAILMSG(1, (TEXT("RemoveElementFromList Failed\n")));
                    }
                    m_pProcessEDControl = NULL;
                    pED->TransferStatus = STATUS_IDLE;
                    goto redoforsameEDCtl;
                }
            }
            pED = (USBED *)pED->NextED.next;
        }

        ///////////////////////////////////////////////////////////////////////////
        // Interrupt out requests

        pED = (USBED*)m_pIntOutHead;
        while(pED) {
redoforsameEDIntOut:
            if(pED->HeadTD && (pED->HeadTD != pED->TailTD)){
                pTD = (USBTD *)pED->HeadTD;
                switch(pED->TransferStatus){
                case STATUS_IDLE:
                    if(!pED->bfSkip){
                        m_pProcessEDOut[pED->bHostEndPointNum - 1] = (PDWORD)pED;
                        pED->TransferStatus = STATUS_PROGRESS;
                        InitializeTransaction(m_portBase, pED, pTD);
                    }
                    break;
                case STATUS_PROGRESS:
                    break;
                case STATUS_COMPLETE:
                    if(RemoveElementFromList((ListHead**)&pED->HeadTD, (ListHead*)pTD)){
                        INTTCMSG(1, (L"TC: EP%d INT OUT, len %d", pED->bHostEndPointNum, pTD->BytesTransferred));
                        if (pTD->sTransfer.lpfnCallback)
                        {
                            ( *pTD->sTransfer.lpfnCallback )( pTD->sTransfer.lpvCallbackParameter );
                        }
                        FreeTD(pTD);
                    }
                    else{
                        RETAILMSG(1,(TEXT("RemoveElementFromList Failed\n")));
                    }
                    m_pProcessEDOut[pED->bHostEndPointNum - 1] = NULL;
                    pED->TransferStatus = STATUS_IDLE;
                    goto redoforsameEDIntOut;
                }
            }
            pED = (USBED *)pED->NextED.next;
        }

        ///////////////////////////////////////////////////////////////////////////
        // Interrupt in requests

        pED = (USBED*)m_pIntInHead;
        while(pED) {
redoforsameEDIntIn:

            if(pED->HeadTD && (pED->HeadTD != pED->TailTD)){
                pTD = (USBTD *)pED->HeadTD;
                switch(pED->TransferStatus){
                case STATUS_IDLE:
                    if(!pED->bfSkip){
                        m_pProcessEDIn[pED->bHostEndPointNum - 1] = (PDWORD)pED;
                        pED->TransferStatus = STATUS_PROGRESS;
                        InitializeTransaction(m_portBase, pED, pTD);
                    }
                    break;
                case STATUS_PROGRESS:
                    break;
                case STATUS_COMPLETE:
                    if(RemoveElementFromList((ListHead**)&pED->HeadTD, (ListHead*)pTD)){
                        INTTCMSG(1, (L"TC: EP%d INT IN, len %d", pED->bHostEndPointNum, pTD->BytesTransferred));
                        if (pTD->sTransfer.lpfnCallback)
                        {
                            ( *pTD->sTransfer.lpfnCallback )( pTD->sTransfer.lpvCallbackParameter );
                        }
                        FreeTD(pTD);
                    }
                    else{
                        RETAILMSG(1,(TEXT("RemoveElementFromList Failed\n")));
                    }
                    m_pProcessEDIn[pED->bHostEndPointNum - 1] = NULL;
                    pED->TransferStatus = STATUS_IDLE;
                    goto redoforsameEDIntIn;
                }
            }
            pED = (USBED *)pED->NextED.next;
        }


#ifdef MUSB_USEDMA
#ifdef MUSB_USEDMA_FOR_ISO


        ///////////////////////////////////////////////////////////////////////////
        // Isochronous (out) requests

        pED = (USBED*)m_pIsoOutHead;
        while(pED) {
redoforsameIsoOutED:

            if(pED->HeadTD && (pED->HeadTD != pED->TailTD))
            {
                pTD = (USBTD *)pED->HeadTD;

                if (pTD->bAwaitingFrame)
                {
                    break;
                }

                switch(pED->TransferStatus)
                {
                    case STATUS_IDLE:
                        if(!pED->bfSkip){
                            if( pED->bSemaphoreOwner == TRUE)
                            {
                                m_pProcessEDOut[pED->bHostEndPointNum - 1] = (PDWORD)pED;
                                pED->TransferStatus = STATUS_PROGRESS;

                                //InitializeTransaction(m_portBase, pED, pTD);
                                pTD->bAwaitingFrame = TRUE;

                                //RETAILMSG(1, (L"TX ISO: wait1 TD %08X\r\n", pTD));

                                //RETAILMSG(FALSE,(TEXT("OUT take %d\n"),pED->bHostEndPointNum));
                                goto IsoOutDone;
                            }
                            else if(WaitForSingleObject( pED->hSemaphore, 0 ) == WAIT_OBJECT_0){
                                pED->bSemaphoreOwner = TRUE;

                                m_pProcessEDOut[pED->bHostEndPointNum - 1] = (PDWORD)pED;
                                pED->TransferStatus = STATUS_PROGRESS;

                                //InitializeTransaction(m_portBase, pED, pTD);
                                pTD->bAwaitingFrame = TRUE;

                                RETAILMSG(FALSE,(TEXT("OUT take %d\n"),pED->bHostEndPointNum));
                                goto IsoOutDone;
                            }
                        }
                        break;
                    case STATUS_PROGRESS:
                        break;
                    case STATUS_COMPLETE:

                        if(RemoveElementFromList((ListHead**)&pED->HeadTD, (ListHead*)pTD)){
                            ISOTCMSG(1, (L"TC: EP%d ISO OUT, len %d", pED->bHostEndPointNum, pTD->BytesTransferred));
                            if (pTD->sTransfer.lpfnCallback)
                            {
                                ( *pTD->sTransfer.lpfnCallback )( pTD->sTransfer.lpvCallbackParameter );
                            }
                            FreeTD(pTD);
                            ReleaseSemaphore(pED->hSemaphore, 1, NULL );
                            pED->bSemaphoreOwner = FALSE;
                            RETAILMSG(FALSE,(TEXT("OUT give %d\n"),pED->bHostEndPointNum));
                        }
                        else{
                            RETAILMSG(TRUE,(TEXT("RemoveElementFromList Failed\n")));
                        }
                        m_pProcessEDOut[pED->bHostEndPointNum - 1] = NULL;
                        pED->TransferStatus = STATUS_IDLE;
                        goto redoforsameIsoOutED;
                }
            }
            pED = (USBED *)pED->NextED.next;
        }

IsoOutDone: ; // NOP


        ///////////////////////////////////////////////////////////////////////////
        // Isochronous (in) requests

        pED = (USBED*)m_pIsoInHead;

        while(pED) {
redoforsameIsoInED:

            if(pED->HeadTD && (pED->HeadTD != pED->TailTD)){
                pTD = (USBTD *)pED->HeadTD;

                if (pTD->bAwaitingFrame)
                {
                    break;
                }

                switch(pED->TransferStatus)
                {
                    case STATUS_IDLE:
                        if(!pED->bfSkip){
                            if( pED->bSemaphoreOwner == TRUE)
                            {
                                m_pProcessEDIn[pED->bHostEndPointNum - 1] = (PDWORD)pED;
                                pED->TransferStatus = STATUS_PROGRESS;

                                //InitializeTransaction(m_portBase, pED, pTD);
                                pTD->bAwaitingFrame = TRUE;

                                //RETAILMSG(FALSE,(TEXT("IN take %d\n"),pED->bHostEndPointNum));
                                goto IsoInDone;
                            }
                            else if(WaitForSingleObject( pED->hSemaphore, 0 ) == WAIT_OBJECT_0){
                                pED->bSemaphoreOwner = TRUE;

                                m_pProcessEDIn[pED->bHostEndPointNum - 1] = (PDWORD)pED;
                                pED->TransferStatus = STATUS_PROGRESS;

                                //InitializeTransaction(m_portBase, pED, pTD);
                                pTD->bAwaitingFrame = TRUE;

                                RETAILMSG(FALSE,(TEXT("IN take %d\n"),pED->bHostEndPointNum));
                                goto IsoInDone;
                            }
                        }
                        break;
                    case STATUS_PROGRESS:
                        break;
                    case STATUS_COMPLETE:

                        if(RemoveElementFromList((ListHead**)&pED->HeadTD, (ListHead*)pTD)){
                            ISOTCMSG(1, (L"TC: EP%d ISO IN, len %d", pED->bHostEndPointNum, pTD->BytesTransferred));
                            if (pTD->sTransfer.lpfnCallback)
                            {
                                ( *pTD->sTransfer.lpfnCallback )( pTD->sTransfer.lpvCallbackParameter );
                            }
                            FreeTD(pTD);
                            ReleaseSemaphore(pED->hSemaphore, 1, NULL );
                            pED->bSemaphoreOwner = FALSE;
                            RETAILMSG(FALSE,(TEXT("IN give %d\n"),pED->bHostEndPointNum));
                        }
                        else{
                            RETAILMSG(TRUE,(TEXT("RemoveElementFromList Failed\n")));
                        }
                        m_pProcessEDIn[pED->bHostEndPointNum - 1] = NULL;
                        pED->TransferStatus = STATUS_IDLE;
                        goto redoforsameIsoInED;
                }
            }
            pED = (USBED *)pED->NextED.next;
        }
IsoInDone: ; // NOP

        // See if we have to trigger any ISO transfers

        {
            USBED *pIsoOutED = (USBED *)m_pIsoOutHead;
            USBED *pIsoInED = (USBED *)m_pIsoInHead;
            USBTD *pIsoOutTD = NULL;
            USBTD *pIsoInTD = NULL;

            if (pIsoOutED && pIsoOutED->HeadTD && (pIsoOutED->HeadTD != pIsoOutED->TailTD) &&
                ((USBTD *)pIsoOutED->HeadTD)->bAwaitingFrame)
            {
                pIsoOutTD = (USBTD *)pIsoOutED->HeadTD;
            }

            if (pIsoInED && pIsoInED->HeadTD && (pIsoInED->HeadTD != pIsoInED->TailTD) &&
                ((USBTD *)pIsoInED->HeadTD)->bAwaitingFrame)
            {
                pIsoInTD = (USBTD *)pIsoInED->HeadTD;
            }

            if (pIsoOutTD || pIsoInTD)
            {
                DWORD dwCurrentFrame;
                INT32 nDiff;

                GetFrameNumber(&dwCurrentFrame);

                fEnableSOF = FALSE;

                if (pIsoOutTD)
                {
                    nDiff = (INT32)(pIsoOutTD->sTransfer.dwStartingFrame - dwCurrentFrame);

                    if (nDiff + ISO_MAX_FRAME_ERROR < 0)
                    {
                        RETAILMSG(1, (L"TX ISO does not meet the schedule! CurFrame %08X, StartFrame %08X\r\n",
                            dwCurrentFrame, pIsoOutTD->sTransfer.dwStartingFrame));

                        // Do not currently fail this, just adjust start frames for subsequent transfers
                        DWORD dwAdjust = dwCurrentFrame - pIsoOutTD->sTransfer.dwStartingFrame;
                        USBTD *pTemp = pIsoOutTD;
                        while (pTemp->NextTD.next != pIsoOutED->TailTD)
                        {
                            pTemp = (USBTD *)pTemp->NextTD.next;
                            pTemp->sTransfer.dwStartingFrame += dwAdjust;
                        }
                    }

                    if (nDiff <= 0)
                    {
                        pIsoOutTD->bAwaitingFrame = FALSE;
                        InitializeTransaction(m_portBase, pIsoOutED, pIsoOutTD);
                    }
                    else
                    {
                        fEnableSOF = TRUE;
                    }
                }

                if (pIsoInTD)
                {
                    nDiff = (INT32)(pIsoInTD->sTransfer.dwStartingFrame - dwCurrentFrame);

                    if (nDiff + ISO_MAX_FRAME_ERROR < 0)
                    {
                        RETAILMSG(1, (L"RX ISO does not meet the schedule! CurFrame %08X, StartFrame %08X\r\n",
                            dwCurrentFrame, pIsoInTD->sTransfer.dwStartingFrame));

                        // Do not currently fail this, just adjust start frames for subsequent transfers
                        DWORD dwAdjust = dwCurrentFrame - pIsoInTD->sTransfer.dwStartingFrame;
                        USBTD *pTemp = pIsoInTD;
                        while (pTemp->NextTD.next != pIsoInED->TailTD)
                        {
                            pTemp = (USBTD *)pTemp->NextTD.next;
                            pTemp->sTransfer.dwStartingFrame += dwAdjust;
                        }
                    }

                    if (nDiff <= 0)
                    {
                        pIsoInTD->bAwaitingFrame = FALSE;
                        InitializeTransaction(m_portBase, pIsoInED, pIsoInTD);
                    }
                    else
                    {
                        fEnableSOF = TRUE;
                    }
                }

                fEnableSOFStateChanged = TRUE;
            }
        }

#endif // MUSB_USEDMA_FOR_ISO
#endif // MUSB_USEDMA

        ///////////////////////////////////////////////////////////////////////////
        // Bulk (out) requests
        pED = (USBED*)m_pBulkOutHead;
        while(pED) {
redoforsameOUTED:

            if(pED->HeadTD && (pED->HeadTD != pED->TailTD)){
                pTD = (USBTD *)pED->HeadTD;
                switch(pED->TransferStatus){
                case STATUS_IDLE:
                    if(!pED->bfSkip){
                        if( pED->bSemaphoreOwner == TRUE)
                        {
                            m_pProcessEDOut[pED->bHostEndPointNum - 1] = (PDWORD)pED;
                            pED->TransferStatus = STATUS_PROGRESS;

                            InitializeTransaction(m_portBase, pED, pTD);
                            //RETAILMSG(FALSE,(TEXT("OUT take %d\n"),pED->bHostEndPointNum));
                            goto bulkoutdone;
                        }
                        else if(WaitForSingleObject( pED->hSemaphore, 0 ) == WAIT_OBJECT_0){
                            pED->bSemaphoreOwner = TRUE;
                            m_pProcessEDOut[pED->bHostEndPointNum - 1] = (PDWORD)pED;
                            pED->TransferStatus = STATUS_PROGRESS;

                            InitializeTransaction(m_portBase, pED, pTD);
                            RETAILMSG(FALSE,(TEXT("OUT take %d\n"),pED->bHostEndPointNum));
                            goto bulkoutdone;
                        }
                    }
                    break;
                case STATUS_PROGRESS:
                    break;
                case STATUS_COMPLETE:
                    if(RemoveElementFromList((ListHead**)&pED->HeadTD, (ListHead*)pTD)){
                        BULKTCMSG(1, (L"TC: EP%d BULK OUT, len %d", pED->bHostEndPointNum, pTD->BytesTransferred));
                        if (pTD->sTransfer.lpfnCallback)
                        {
                            ( *pTD->sTransfer.lpfnCallback )( pTD->sTransfer.lpvCallbackParameter );
                        }
                        FreeTD(pTD);
                        ReleaseSemaphore(pED->hSemaphore, 1, NULL );
                        pED->bSemaphoreOwner = FALSE;
                        RETAILMSG(FALSE,(TEXT("OUT give %d\n"),pED->bHostEndPointNum));
                    }
                    else{
                        RETAILMSG(TRUE,(TEXT("RemoveElementFromList Failed\n")));
                    }
                    m_pProcessEDOut[pED->bHostEndPointNum - 1] = NULL;
                    pED->TransferStatus = STATUS_IDLE;
                    goto redoforsameOUTED;
                }
            }
            pED = (USBED *)pED->NextED.next;
        }
bulkoutdone: ; // NOP


        ///////////////////////////////////////////////////////////////////////////
        // Bulk (in) requests
        pED = (USBED*)m_pBulkInHead;
        while(pED) {
redoforsameINED:

            if(pED->HeadTD && (pED->HeadTD != pED->TailTD)){
                pTD = (USBTD *)pED->HeadTD;
                switch(pED->TransferStatus){
                case STATUS_IDLE:
                    if(!pED->bfSkip){
                        if( pED->bSemaphoreOwner == TRUE)
                        {
                            m_pProcessEDIn[pED->bHostEndPointNum - 1] = (PDWORD)pED;
                            pED->TransferStatus = STATUS_PROGRESS;

                            InitializeTransaction(m_portBase, pED, pTD);
                            //RETAILMSG(FALSE,(TEXT("IN take %d\n"),pED->bHostEndPointNum));
                            goto bulkindone;
                        }
                        else if(WaitForSingleObject( pED->hSemaphore, 0 ) == WAIT_OBJECT_0){
                            pED->bSemaphoreOwner = TRUE;
                            m_pProcessEDIn[pED->bHostEndPointNum - 1] = (PDWORD)pED;
                            pED->TransferStatus = STATUS_PROGRESS;

                            InitializeTransaction(m_portBase, pED, pTD);
                            RETAILMSG(FALSE,(TEXT("IN take %d\n"),pED->bHostEndPointNum));
                            goto bulkindone;
                        }
                    }
                    break;
                case STATUS_PROGRESS:
                    break;
                case STATUS_COMPLETE:
                    if(RemoveElementFromList((ListHead**)&pED->HeadTD, (ListHead*)pTD)){
                        BULKTCMSG(1, (L"TC: EP%d BULK IN, len %d", pED->bHostEndPointNum, pTD->BytesTransferred));
                        if (pTD->sTransfer.lpfnCallback)
                        {
                            ( *pTD->sTransfer.lpfnCallback )( pTD->sTransfer.lpvCallbackParameter );
                        }
                        FreeTD(pTD);
                        ReleaseSemaphore(pED->hSemaphore, 1, NULL );
                        pED->bSemaphoreOwner = FALSE;
                        RETAILMSG(FALSE,(TEXT("IN give %d\n"),pED->bHostEndPointNum));
                    }
                    else{
                        RETAILMSG(TRUE,(TEXT("RemoveElementFromList Failed\n")));
                    }
                    m_pProcessEDIn[pED->bHostEndPointNum - 1] = NULL;
                    pED->TransferStatus = STATUS_IDLE;
                    goto redoforsameINED;
                }
            }
            pED = (USBED *)pED->NextED.next;
        }
bulkindone: ; // NOP

#ifdef MUSB_USEDMA
#ifdef MUSB_USEDMA_FOR_ISO
        // Enable or disable SOF interrupts
        if (fEnableSOFStateChanged){
            if (fEnableSOF){
				m_portBase->IRQ_EN_SET_1 = CSL_USB_INTRUSB_SOF_MASK;
            } else {
				m_portBase->IRQ_EN_CLR_1 = CSL_USB_INTRUSB_SOF_MASK;
            }
        }

#endif // MUSB_USEDMA_FOR_ISO
#endif // MUSB_USEDMA
        UnlockProcessingThread();

    }

    return 0;
}

// ******************************************************************
INT32 CHW::AllocateHostEndPoint(UINT32 TransferType, DWORD MaxPktSize, BOOL IsDirectionIN )
{
    INT32 i = 0, EpNum = -1;

    // Use configurable static FIFO alloc

    if (TransferType == TYPE_CONTROL)
    {
        EpNum = 0;
        DEBUGMSG(ZONE_INIT, (TEXT("CHW::AllocateHostEndPoint: allocated EP0, type CONTROL, dir BIDIR, FIFO 64\n")));
    }
    else
    {
        EP_CONFIG *pConfig = IsDirectionIN ? m_EpInConfig : m_EpOutConfig;
        INT32 EpAlloc = -1;
        BOOL fStop = FALSE;

        if (TransferType == TYPE_BULK)
        {
            // Search for shared BULK EP
            for (i = 0; !fStop && (i < (MGC_MAX_USB_ENDS - 1)); i ++)
            {
                if ((pConfig[ i ].refCount > 0) &&
                    (pConfig[ i ].epTypeCurrent == TYPE_BULK) &&
                    pConfig[ i ].fSharedMode &&
                    (pConfig[ i ].fifoSize >= MaxPktSize))
                {
                    pConfig[ i ].refCount ++;
                    EpNum = i + 1;
                    fStop = TRUE;
                }
            }
        }

        if (!fStop)
        {
            for (i = 0; !fStop && (i < (MGC_MAX_USB_ENDS - 1)); i ++)
            {
                if (((pConfig[ i ].epType == (UINT8)TransferType) ||
                     (pConfig[ i ].epType == TYPE_ANY)) &&
                    (pConfig[ i ].fifoSize >= MaxPktSize) &&
                    (pConfig[ i ].refCount == 0))
                {
                    if (EpAlloc >= 0)
                    {
                        // Only accept endpoints with smaller FIFO allocation
                        if (pConfig[ i ].fifoSize < pConfig[ EpAlloc ].fifoSize)
                        {
                            EpAlloc = i;
                        }
                    }
                    else
                    {
                        EpAlloc = i;
                    }

                    // Stop searching if endpoint is hardcoded for
                    // this transfer type and keep looking if
                    // configured for "any" transfer type
                    if (pConfig[ i ].epType == (UINT8)TransferType)
                    {
                        fStop = TRUE;
                    }
                }
            }

            if (EpAlloc >= 0)
            {
                pConfig[ EpAlloc ].refCount = 1;
                pConfig[ EpAlloc ].epTypeCurrent = (UINT8)TransferType;
                EpNum = EpAlloc + 1;
            }
        }

        if (EpNum > 0)
        {
            DEBUGMSG(ZONE_INIT, (TEXT("CHW::AllocateHostEndPoint: allocated EP %u, type %d, dir %s, FIFO %d\n"),
                (INT32)EpNum,
                TransferType,
                IsDirectionIN ? L"IN" : L"OUT",
                (INT32)pConfig[ EpNum - 1 ].fifoSize));
        }
        else
        {
            DEBUGMSG(ZONE_ERROR, (TEXT("CHW::AllocateHostEndPoint: failed to alloc EP, type %d, dir %s\n"),
                TransferType,
                IsDirectionIN ? L"IN" : L"OUT"));
        }
    }

    RETAILMSG(FALSE,(TEXT("EPnum=%x\n"),EpNum));
    return EpNum;
}

void CHW::FreeHostEndPoint(UINT32 EndpointNum, BOOL IsDirectionIN)
{
    // Use configurable static FIFO alloc

    DEBUGCHK(EndpointNum < MGC_MAX_USB_ENDS);

    if ( (EndpointNum > 0) && (EndpointNum < MGC_MAX_USB_ENDS) )
    {
        EP_CONFIG *pConfig = IsDirectionIN ? m_EpInConfig : m_EpOutConfig;
        EndpointNum --;

        DEBUGCHK(pConfig[ EndpointNum ].refCount > 0);

        DEBUGMSG(ZONE_INIT, (TEXT("CHW::FreeHostEndPoint: releasing EP %u, type %d, dir %s, FIFO %d\n"),
            (INT32)(EndpointNum + 1),
            (INT32)pConfig[ EndpointNum ].epTypeCurrent,
            IsDirectionIN ? L"IN" : L"OUT",
            (INT32)pConfig[ EndpointNum ].fifoSize));

        switch (pConfig[ EndpointNum ].epTypeCurrent)
        {
        case TYPE_BULK:

            if (pConfig[ EndpointNum ].fSharedMode)
            {
                // Sharing mode enabled
                pConfig[ EndpointNum ].refCount --;

                if (pConfig[ EndpointNum ].refCount == 0)
                {
                    pConfig[ EndpointNum ].epTypeCurrent = TYPE_UNKNOWN;
                }

                break;
            }

            // Do not break here, just fall through
            __fallthrough;

        case TYPE_ISOCHRONOUS:
        case TYPE_INTERRUPT:

            if (pConfig[ EndpointNum ].refCount == 1)
            {
                pConfig[ EndpointNum ].refCount = 0;
            }
            else
            {
                DEBUGCHK(FALSE);
            }

            pConfig[ EndpointNum ].epTypeCurrent = TYPE_UNKNOWN;

            break;

        default:

            DEBUGCHK(FALSE);
        }
    }
    else
    {
        DEBUGMSG(ZONE_INIT, (TEXT("CHW::FreeHostEndPoint: releasing EP0, type CONTROL, dir BIDIR, FIFO 64\n")));
    }
}

void CHW::StopHostEndpoint(DWORD HostEndpointNum)
{
    UINT16 _HOST_CSR0, _HOST_TXCSR, _HOST_RXCSR;

    if(HostEndpointNum)
    {
    	_HOST_RXCSR = m_portBase->EPCSR[HostEndpointNum].RXCSR;
        if(_HOST_RXCSR & MGC_M_RXCSR_H_REQPKT)
            m_portBase->EPCSR[HostEndpointNum].RXCSR = _HOST_RXCSR & (~MGC_M_RXCSR_H_REQPKT);

        _HOST_TXCSR = m_portBase->EPCSR[HostEndpointNum].TXCSR;
        if(_HOST_TXCSR & MGC_M_TXCSR_TXPKTRDY)
            m_portBase->EPCSR[HostEndpointNum].TXCSR = _HOST_TXCSR & (~MGC_M_TXCSR_TXPKTRDY);
    }
    else
    {
        _HOST_CSR0 = m_portBase->EPCSR[0].TXCSR;
        m_portBase->EPCSR[0].TXCSR =  _HOST_CSR0 & (~(MGC_M_CSR0_TXPKTRDY|MGC_M_CSR0_H_REQPKT));
    }
}

#define FULL_HIGH_SPEED (pED->bfIsHighSpeed?(0x1<<6):(0x2<<6))
void CHW::ProgramHostEndpoint(UINT32 TransferType, void *pTemp){
    USBED *pED = (USBED *)pTemp;
    UINT32 EpNum = pED->bHostEndPointNum;

    if(EpNum < MGC_MAX_USB_ENDS){

        switch (TransferType)
        {
        case TYPE_CONTROL:
        case TYPE_BULK:

            pED->epType = (PIPE_TYPE)TransferType;
            // Note: Bulk and control are programmed in InitializeTransaction()
            break;

        case TYPE_INTERRUPT:
        case TYPE_ISOCHRONOUS:

            {
                UINT32 nXType = (TransferType == TYPE_INTERRUPT) ? 0x03 : 0x01;

                pED->epType = (PIPE_TYPE)TransferType;
				m_portBase->INDEX = (BYTE)EpNum;

                if(pED->bfDirection == TD_IN_PID) {
                	m_portBase->EPCSR[EpNum].RXTYPE = 
                        (BYTE)((pED->bfIsLowSpeed?(0x3<<6):FULL_HIGH_SPEED)|(nXType<<4)|(pED->bfEndpointNumber&0xf));
					m_portBase->EPCSR[EpNum].RXINTERVAL = pED->bInterval;
                } else {
                	m_portBase->EPCSR[EpNum].TXTYPE = 
                        (BYTE)((pED->bfIsLowSpeed?(0x3<<6):FULL_HIGH_SPEED)|(nXType<<4)|(pED->bfEndpointNumber&0xf));
					m_portBase->EPCSR[EpNum].TXINTERVAL = pED->bInterval;
                }

                if (m_fHighSpeed && !pED->bfIsHighSpeed && pED->bfHubAddress){
                	m_portBase->EPTRG[EpNum].TXHUBADDR = pED->bfHubAddress;
					m_portBase->EPTRG[EpNum].RXHUBADDR = pED->bfHubAddress;
					m_portBase->EPTRG[EpNum].TXHUBPORT = pED->bfHubPort;
					m_portBase->EPTRG[EpNum].RXHUBPORT = pED->bfHubPort;
                } else {
                	m_portBase->EPTRG[EpNum].TXHUBADDR = 0;
					m_portBase->EPTRG[EpNum].RXHUBADDR = 0;
					m_portBase->EPTRG[EpNum].TXHUBPORT = 0;
					m_portBase->EPTRG[EpNum].RXHUBPORT = 0;
                }
            }

            break;

        default:

            DEBUGCHK(FALSE);
            pED->epType = TYPE_UNKNOWN;
        }

        if (pED->bHostEndPointNum)
        {
            if(pED->bfDirection == TD_IN_PID)
            {
                pED->hSemaphore = m_EpInProtectSem[pED->bHostEndPointNum - 1];
            }
            else
            {
                pED->hSemaphore = m_EpOutProtectSem[pED->bHostEndPointNum - 1];
            }
        }
        else
        {
            pED->hSemaphore = m_Ep0ProtectSem;
        }
    }

    RETAILMSG(FALSE,(TEXT("ProgramHostEndpoint EP %u Type %d Direction%x\n"),
            EpNum, pED->epType, pED->bfDirection ));
}

// ******************************************************************
BOOL CHW::LoadEnpointConfiguration( void )
{
    const TCHAR *pszKeyFormat = L"%s\\CONFIG\\EP%u\\%s";
    HKEY hKey;
    EP_CONFIG *pConfig;
    DWORD i, j, dwSize, dwType, dwTemp;
    TCHAR *pszDir, szTemp[ 64 ];
    DWORD dwFifoUsage = 0;
    DWORD dwFifoEmpty = MGC_FIFO_RAM_SIZE - 64; // 64 bytes reserved for control EP0.

    // Loop through endpoints
    for (i = 0; i < (MGC_MAX_USB_ENDS - 1); i ++)
    {
        // Loop through directions
        for (j = 0; j < 2; j ++)
        {
            if (j)
            {
                pszDir = L"OUT";
                pConfig = m_EpOutConfig;
            }
            else
            {
                pszDir = L"IN";
                pConfig = m_EpInConfig;
            }

            // Open endpoint key
            if( SUCCEEDED( StringCbPrintf( szTemp, sizeof( szTemp ),
                               pszKeyFormat, ((SOhcdPdd*)m_pPddContext)->szDriverRegKey, i + 1, pszDir ) ) )
            {
                if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szTemp, 0, 0, &hKey) == ERROR_SUCCESS)
                {
                    // Get EP type
                    dwType = REG_SZ;
#pragma warning(push)
#pragma warning(disable: 6260 6386)
                    dwSize = sizeof(szTemp) * sizeof(TCHAR);

                    if (RegQueryValueEx(hKey, L"Type", NULL, &dwType, (LPBYTE)szTemp, &dwSize) != ERROR_SUCCESS)
                    {
                        szTemp[ 0 ] = L'\0';
                    }
                    else
                    {
                        // force zero-termination
                        szTemp[ (sizeof(szTemp) / sizeof(szTemp[0])) - 1 ] = L'\0';
                    }
#pragma warning(pop)
                    if (!wcsicmp(szTemp, L"bulk"))
                    {
                        pConfig[ i ].epType = TYPE_BULK;
                    }
                    else if (!wcsicmp(szTemp, L"isochronous"))
                    {
                        pConfig[ i ].epType = TYPE_ISOCHRONOUS;
                    }
                    else if (!wcsicmp(szTemp, L"interrupt"))
                    {
                        pConfig[ i ].epType = TYPE_INTERRUPT;
                    }
                    else if (!wcsicmp(szTemp, L"any"))
                    {
                        pConfig[ i ].epType = TYPE_ANY;
                    }
                    else
                    {
                        DEBUGMSG( ZONE_ERROR, (TEXT("CHW::LoadEnpointConfiguration: endpoint type invalid or unspecified, EP %u %s\n"),
                            i, pszDir));

                        RegCloseKey(hKey);
                        return FALSE;
                    }

                    // Get FIFO size
                    dwType = REG_DWORD;
                    dwSize = sizeof(dwTemp);
                    if (RegQueryValueEx(hKey, L"Fifo", NULL, &dwType, (LPBYTE)&dwTemp, &dwSize) != ERROR_SUCCESS)
                    {
                        dwTemp = 0;
                    }

                    if (!dwTemp || (dwTemp > 1024) || (dwTemp & 0x03))
                    {
                        DEBUGMSG( ZONE_ERROR, (TEXT("CHW::LoadEnpointConfiguration: endpoint FIFO size invalid or unspecified, EP %u %s %d\n"),
                            i, pszDir, dwTemp));

                        RegCloseKey(hKey);
                        return FALSE;
                    }

                    pConfig[ i ].fifoSize = (UINT16)dwTemp;

                    // Get double buffering mode flag
                    pConfig[ i ].fDBMode = 0;
                    dwType = REG_DWORD;
                    dwSize = sizeof(dwTemp);
                    if (RegQueryValueEx(hKey, L"Double", NULL, &dwType, (LPBYTE)&dwTemp, &dwSize) == ERROR_SUCCESS)
                    {
                        pConfig[ i ].fDBMode = (UINT8)dwTemp;
                    }

                    // Get sharing mode flag (applicable to bulk only) - default ON
                    pConfig[ i ].fSharedMode = 1;
                    if (pConfig[ i ].epType == TYPE_BULK || pConfig[ i ].epType == TYPE_ANY)
                    {
                        dwType = REG_DWORD;
                        dwSize = sizeof(dwTemp);
                        if (RegQueryValueEx(hKey, L"Shared", NULL, &dwType, (LPBYTE)&dwTemp, &dwSize) == ERROR_SUCCESS)
                        {
                            pConfig[ i ].fSharedMode = (UINT8)dwTemp;
                        }
                    }

                    if (pConfig[ i ].fDBMode)
                    {
                        dwFifoUsage += (pConfig[ i ].fifoSize << 1);
                    }
                    else
                    {
                        dwFifoUsage += pConfig[ i ].fifoSize;
                    }

                    RegCloseKey(hKey);
                }
                else
                {
                    // There is no way to completely skip unused endpoints. We still
                    // have to allocate at least 8 bytes (minimum).
                    dwFifoUsage += 8;
                }
            }
            else
            {
                // There is no way to completely skip unused endpoints. We still
                // have to allocate at least 8 bytes (minimum).
                dwFifoUsage += 8;
            }
        }
    }

    // Validate configuration
    if (!dwFifoUsage || (dwFifoUsage > dwFifoEmpty))
    {
        DEBUGMSG( ZONE_ERROR, (TEXT("CHW::LoadEnpointConfiguration: endpoint configuration not defined or invalid!\n")));
        return FALSE;
    }

    return TRUE;
}


// ******************************************************************
void CHW::InitialiseFIFOs(void)
//
// Purpose: Initialises the FIFO sizes for all endpoints.
//
// Parameters: None
//
// Returns: N/A
//
// ******************************************************************
{
    // Use configurable static FIFO alloc

    UINT8 fifoSize, epNum, DPB;
    UINT16 byteFifoSize;

    m_wFifoOffset = 0;

    // Configure control EP0
    byteFifoSize = 64; // 64 bytes
    fifoSize = Byte2FifoSize(byteFifoSize);
    DEBUGCHK(fifoSize != (UINT8)-1);

    // Index register
    m_portBase->INDEX = 0;
	// Adjust EP FIFO pointers
	m_portBase->TXFIFOADDR = m_wFifoOffset >> 3;
	m_portBase->RXFIFOADDR = m_wFifoOffset >> 3;

    // Assign EP FIFO size
	m_portBase->TXFIFOSZ   = fifoSize;
	m_portBase->RXFIFOSZ   = fifoSize;

    m_wFifoOffset = m_wFifoOffset + byteFifoSize;

    // Configure EP<1-4>
    for (epNum = 0; epNum < (MGC_MAX_USB_ENDS - 1); epNum ++)
    {
        m_portBase->INDEX = epNum + 1;

        ///////////////////////////////////////////////////////////////////////////
        // Configure in
        DPB = 0;
        if (m_EpInConfig[ epNum ].epType != TYPE_UNKNOWN){
            byteFifoSize = m_EpInConfig[ epNum ].fifoSize;

            if (m_EpInConfig[ epNum ].fDBMode){
                // Double buffered
                byteFifoSize <<= 1;
                DPB = (1 << 4);
            }
        } else {
            // 8 bytes minimum
            byteFifoSize = 8;
        }

        fifoSize = Byte2FifoSize(byteFifoSize);

        DEBUGCHK(fifoSize != (UINT8)-1);
        DEBUGCHK((m_wFifoOffset + byteFifoSize) <= MGC_FIFO_RAM_SIZE);

        // Adjust EP FIFO pointer
        m_portBase->RXFIFOADDR = m_wFifoOffset >> 3;
        m_wFifoOffset = m_wFifoOffset + byteFifoSize;

        // Assign EP FIFO size
        m_portBase->RXFIFOSZ = fifoSize | DPB;

        ///////////////////////////////////////////////////////////////////////////
        // Configure out
        DPB = 0;

        if (m_EpOutConfig[ epNum ].epType != TYPE_UNKNOWN){
            byteFifoSize = m_EpOutConfig[ epNum ].fifoSize;

            if (m_EpOutConfig[ epNum ].fDBMode){
                // Double buffered
                byteFifoSize <<= 1;
                DPB = (1 << 4);
            }
        } else {
            // 8 bytes minimum
            byteFifoSize = 8;
        }

        fifoSize = Byte2FifoSize(byteFifoSize);

        DEBUGCHK(fifoSize != (UINT8)-1);
        DEBUGCHK((m_wFifoOffset + byteFifoSize) <= MGC_FIFO_RAM_SIZE);

        // Adjust EP FIFO pointer
        m_portBase->TXFIFOADDR = m_wFifoOffset >> 3;
        m_wFifoOffset = m_wFifoOffset + byteFifoSize;

        // Assign EP FIFO size
        m_portBase->TXFIFOSZ = fifoSize | DPB;

		// Clean any previous setup
		m_portBase->EPCSR[epNum].RXTYPE     = 0x0;
		m_portBase->EPCSR[epNum].TXTYPE     = 0x0;
		m_portBase->EPCSR[epNum].RXINTERVAL = 0x0;
		m_portBase->EPCSR[epNum].RXCSR      = 0x0;
		m_portBase->EPCSR[epNum].TXINTERVAL = 0x0;
		m_portBase->EPCSR[epNum].TXCSR      = 0x0;

    }
}

//========================================================================
//!  \fn InitializeTransaction ()
//!  \brief USB Data Transfer routine which performs the bulk of the
//!         Transfer functions. This routine is invoked by the
//!         CHW::UsbProcessingThread context. It receives the
//!         EndPoint and the Transfer Descriptor Structures.
//!  \param UINT32 portbase - Base Address of the USB Controller.
//!         void * pEndDesc - EndPoint Descriptor Structure
//!         void * pTransDesc - Transfer Descriptor Structure.
//!  \return None
//========================================================================
void CHW::InitializeTransaction(CSL_UsbRegs * portBase, void *pEndDesc, void *pTransDesc)
{
    UINT8 *pPkt, *pData, epType;
    DWORD CopyCount;
    UINT16 HOST_CSR;
    USBED *pED = (USBED *)pEndDesc;
    USBTD *pTD = (USBTD *)pTransDesc;

    // Just in case
    DEBUGCHK(pED != NULL);
    DEBUGCHK(pTD != NULL);

RETAILMSG(0, (L"CHW::InitializeTransaction port %08X, pED %08X, TrDsc %08X\r\n", 
	           portBase, pEndDesc, pTransDesc ));

    // Grab EP type
    epType = pED->bfAttributes & USB_ENDPOINT_TYPE_MASK;
    CopyCount = MIN(pED->bfMaxPacketSize, pTD->BytesToTransfer);

    // Fail transfer immediately when disconnected
    if ((m_portStatus & PORT_STATUS_DEVICE_ATTACHED) == 0)
    {
        DEBUGMSG(ZONE_WARNING, (L"CHW::InitializeTransaction: Device disconnected - failing transfer\r\n"));
        *pTD->sTransfer.lpfComplete = TRUE;
        *pTD->sTransfer.lpdwError = USB_DEVICE_NOT_RESPONDING_ERROR;
        pED->bfHalted = TRUE;
        pED->TransferStatus = STATUS_COMPLETE;
        SetEvent(m_hUsbProcessingEvent);
        return;
    }

    // Cancel if powered down
    if (((SOhcdPdd*)m_pPddContext)->CurrentDx != D0)
    {
        DEBUGMSG(ZONE_WARNING, (L"CHW::InitializeTransaction: Device powered down - cancelling transfer\r\n"));
        *pTD->sTransfer.lpfComplete = TRUE;
        *pTD->sTransfer.lpdwError = USB_CANCELED_ERROR;
        pED->bfHalted = TRUE;
        pED->TransferStatus = STATUS_COMPLETE;
        SetEvent(m_hUsbProcessingEvent);
        return;
    }

	switch(pED->bfDirection){
    case TD_SETUP_PID:

		portBase->EPTRG[pED->bHostEndPointNum].TXFUNCADDR = pTD->sTransfer.address;
		portBase->EPTRG[pED->bHostEndPointNum].RXFUNCADDR = pTD->sTransfer.address;
//		portBase->FADDR = pTD->sTransfer.address;
		portBase->EPCSR[pED->bHostEndPointNum].TXMAXP     = pED->bfMaxPacketSize;
		portBase->EPCSR[pED->bHostEndPointNum].RXMAXP     = pED->bfMaxPacketSize;
		portBase->EPCSR[0].TXTYPE = pED->bfIsHighSpeed ? 
			                      (0x1<<6) : (pED->bfIsLowSpeed?(0x3<<6):(0x2<<6));

        /*Host controller in High Speed
         *Device is not High speed
         *Device connected after hub
         */
#if 1
		if(m_fHighSpeed && !pED->bfIsHighSpeed && pED->bfHubAddress){
			portBase->EPTRG[0].TXHUBADDR = pED->bfHubAddress;
			portBase->EPTRG[0].RXHUBADDR = pED->bfHubAddress;
			portBase->EPTRG[0].TXHUBPORT = pED->bfHubPort;
			portBase->EPTRG[0].RXHUBPORT = pED->bfHubPort;
        } else { 
			portBase->EPTRG[0].TXHUBADDR = 0;
			portBase->EPTRG[0].RXHUBADDR = 0;
			portBase->EPTRG[0].TXHUBPORT = 0;
			portBase->EPTRG[0].RXHUBPORT = 0;
        }
#endif

		switch(pTD->TransferStage){
        case STAGE_SETUP:
            pPkt = (UINT8 *)pTD->sTransfer.lpvControlHeader;
	
			WriteFIFO((DWORD*)&(portBase->FIFO[0]), pPkt, 8);

			/*Setup Stage - SETUP packet*/
			portBase->EPCSR[0].TXCSR = MGC_M_CSR0_TXPKTRDY | MGC_M_CSR0_H_SETUPPKT|
			                           MGC_M_CSR0_H_WR_DATATOGGLE;
            break;
        case STAGE_DATAIN:
            // NOTE: PING is disabled because some MSD do not support it for EP0 IN transfers
            portBase->EPCSR[0].TXCSR = MGC_M_CSR0_H_REQPKT | MGC_M_CSR0_H_DISPING;
            break;
        case STAGE_DATAOUT:
            pPkt = (UINT8 *)pTD->sTransfer.lpvClientBuffer;

			WriteFIFO((DWORD*)&(portBase->FIFO[0]), pPkt, CopyCount);
	        pTD->BytesTransferred += CopyCount;
			pTD->BytesToTransfer -= CopyCount;

			/*Data Stage - OUT packet*/
			portBase->EPCSR[0].TXCSR = MGC_M_CSR0_TXPKTRDY;
            break;
        case STAGE_STATUSIN:
            /*data stage was from host to device - OUT transaction*/
            /*Status Stage - IN packet*/
            portBase->EPCSR[0].TXCSR = MGC_M_CSR0_H_REQPKT|MGC_M_CSR0_H_STATUSPKT |
                                       MGC_M_CSR0_H_WR_DATATOGGLE|MGC_M_CSR0_H_DATATOGGLE;
            break;
        case STAGE_STATUSOUT:
            /*data stage was from device to host - IN transaction*/
            /*Status Stage - OUT packet*/
            // NOTE: PING is disabled because some MSD do not support it for EP0 IN transfers
            portBase->EPCSR[0].TXCSR = MGC_M_CSR0_TXPKTRDY|MGC_M_CSR0_H_STATUSPKT |
                     MGC_M_CSR0_H_WR_DATATOGGLE|MGC_M_CSR0_H_DATATOGGLE|MGC_M_CSR0_H_DISPING;
            break;
        }
        break;

    case TD_OUT_PID:
//		portBase->FADDR = pTD->sTransfer.address;
		portBase->EPTRG[pED->bHostEndPointNum].TXFUNCADDR = pTD->sTransfer.address;
		portBase->EPCSR[pED->bHostEndPointNum].TXMAXP     = pED->bfMaxPacketSize;

        if (pED->epType == TYPE_BULK) {
        	portBase->EPCSR[pED->bHostEndPointNum].TXTYPE = 
                ((pED->bfIsLowSpeed?(0x3<<6):FULL_HIGH_SPEED)|(2<<4)|(pED->bfEndpointNumber&0xf));
			portBase->EPCSR[pED->bHostEndPointNum].TXINTERVAL = pED->bInterval;

#if 1
            if(m_fHighSpeed && !pED->bfIsHighSpeed && pED->bfHubAddress) {
            	portBase->EPTRG[pED->bHostEndPointNum].TXHUBADDR = pED->bfHubAddress;
				portBase->EPTRG[pED->bHostEndPointNum].TXHUBPORT = pED->bfHubPort;
            } else {
            	portBase->EPTRG[pED->bHostEndPointNum].TXHUBADDR = 0;
				portBase->EPTRG[pED->bHostEndPointNum].TXHUBPORT = 0;
            }
#endif		
        }

#ifdef MUSB_USEDMA

        if (pED->pDmaChannel)
        {
            UINT32 nFlags = pTD->sTransfer.dwFlags;

            // Disable EP interrupt
            m_portBase->IRQ_EN_CLR_0 = 1 << pED->bHostEndPointNum;

            if (pED->bfToggleCarry)
                nFlags |= USB_TOGGLE_CARRY;

            BOOL fResult = pED->pDmaChannel->IssueTransfer(
                pED->bfEndpointNumber,
                pTD->sTransfer.address,
                epType,
                pED->bfMaxPacketSize,
                pTD->sTransfer.lpvClientBuffer,
                pTD->sTransfer.paClientBuffer,
                pTD->BytesToTransfer,
                pTD->sTransfer.dwFrames,
                (UINT32 *)pTD->sTransfer.aLengths,
                pTD->sTransfer.adwIsochErrors,
                pTD->sTransfer.adwIsochLengths,
                nFlags,
                pED,
                pTD);

            if (!fResult){
                DEBUGMSG(1, (L"CHW::InitializeTransaction: Failed to start DMA OUT transfer!\r\n"));
                DEBUGCHK(FALSE);
            }
        }
        else

#endif // MUSB_USEDMA

        {
            /*OUT Transaction*/
            pPkt = (UINT8 *)pTD->sTransfer.lpvClientBuffer;
            pData = (UINT8 *)pTD->sTransfer.lpvClientBuffer;
            pData += pTD->BytesTransferred;

            /* Perform a check if the EndPoint is NON-BULK,if so switch back
             * to PIO Mode wherein the CPU in the context of the Calling
             * Thread performs the Transfer into the EndPoint FIFO
             */
            {
                pTD->TXCOUNT = CopyCount;

				WriteFIFO((DWORD*)&(portBase->FIFO[pED->bHostEndPointNum]), pData, CopyCount);
                if(pED->bfToggleCarry)
                    HOST_CSR = (MGC_M_TXCSR_H_WR_DATATOGGLE|MGC_M_TXCSR_H_DATATOGGLE);
                else
                    HOST_CSR = (MGC_M_TXCSR_CLRDATATOG);
				portBase->EPCSR[pED->bHostEndPointNum].TXCSR = HOST_CSR | MGC_M_TXCSR_TXPKTRDY ;
            }
        }
        break;

    case TD_IN_PID:

//		portBase->FADDR = pTD->sTransfer.address;
		portBase->EPTRG[pED->bHostEndPointNum].RXFUNCADDR = pTD->sTransfer.address;
		portBase->EPCSR[pED->bHostEndPointNum].RXMAXP     = pED->bfMaxPacketSize;

        if (pED->epType == TYPE_BULK){
			portBase->EPCSR[pED->bHostEndPointNum].RXTYPE = 
                ((pED->bfIsLowSpeed?(0x3<<6):FULL_HIGH_SPEED)|(2<<4)|(pED->bfEndpointNumber&0xf));
			portBase->EPCSR[pED->bHostEndPointNum].RXINTERVAL = pED->bInterval;

#if 1
            if(m_fHighSpeed && !pED->bfIsHighSpeed && pED->bfHubAddress) {
				portBase->EPTRG[pED->bHostEndPointNum].RXHUBADDR = pED->bfHubAddress;
				portBase->EPTRG[pED->bHostEndPointNum].RXHUBPORT = pED->bfHubPort;
            } else {
				portBase->EPTRG[pED->bHostEndPointNum].RXHUBADDR = 0;
				portBase->EPTRG[pED->bHostEndPointNum].RXHUBPORT = 0;
            }
#endif			
        }

#ifdef MUSB_USEDMA

        // Use DMA transfer if applicable
        if (pED->pDmaChannel){
            UINT32 nFlags = pTD->sTransfer.dwFlags;

            if (pED->bfToggleCarry)
                nFlags |= USB_TOGGLE_CARRY;

            BOOL fResult = pED->pDmaChannel->IssueTransfer(
                pED->bfEndpointNumber,
                pTD->sTransfer.address,
                epType,
                pED->bfMaxPacketSize,
                pTD->sTransfer.lpvClientBuffer,
                pTD->sTransfer.paClientBuffer,
                pTD->BytesToTransfer,
                pTD->sTransfer.dwFrames,
                (UINT32 *)pTD->sTransfer.aLengths,
                pTD->sTransfer.adwIsochErrors,
                pTD->sTransfer.adwIsochLengths,
                nFlags,
                pED,
                pTD);

            if (!fResult){
                DEBUGMSG(1, (L"CHW::InitializeTransaction: Failed to start DMA IN transfer!\r\n"));
                DEBUGCHK(FALSE);
            }
        }
        else

#endif // MUSB_USEDMA

        {
            /*if (pTD->sTransfer.paClientBuffer != NULL)
            {
                RETAILMSG(TRUE,(TEXT("EP %u Buff %08x Rx%x\n"), pED->bHostEndPointNum, pTD->sTransfer.paClientBuffer, pTD->BytesToTransfer));
            }*/

            /*IN Transaction*/
            if(pED->bfToggleCarry)
                HOST_CSR = (MGC_M_RXCSR_H_WR_DATATOGGLE|MGC_M_RXCSR_H_DATATOGGLE);
            else
                HOST_CSR = (MGC_M_RXCSR_CLRDATATOG);
			portBase->EPCSR[pED->bHostEndPointNum].RXCSR = HOST_CSR | MGC_M_RXCSR_H_REQPKT;
        }

        break;
    }
}

#ifdef MUSB_USEDMA

//========================================================================
//!  \fn DmaTransferComplete ()
//!  \brief DMA transfer completion callback.
//!  \param CCppiDmaChannel *pChannel - DMA channel.
//!         UINT32 nStatus - standard Win32 USB transfer completion code.
//!         UINT32 nLength - transfer length.
//!         UINT32 nComplete - actual number of bytes read/written.
//!         PVOID pPrivate - user data pased to CCppiDmaControler::AllocChannel().
//!         PVOID pCookie - user data pased to CCppiDmaChannel::IssueTransfer().
//!  \return None
//========================================================================

void CHW::DmaTransferComplete(CCppiDmaChannel *pChannel, UINT32 nStatus,
    UINT32 nLength, UINT32 nComplete, UINT32 nOptions, PVOID pPrivate, PVOID pCookie)
{
    UNREFERENCED_PARAMETER(pChannel);
    UNREFERENCED_PARAMETER(nLength);
    UNREFERENCED_PARAMETER(nComplete);

    USBED *pED = (USBED *)pPrivate;
    USBTD *pTD = (USBTD *)pCookie;

//    DEBUGCHK(m_pChw);
    DEBUGCHK(pED && pTD);
    DEBUGCHK(pED->pDmaChannel == pChannel);

    (m_pChw[pChannel->m_chwInd])->LockProcessingThread();

    pTD->BytesTransferred += nComplete;
    pTD->BytesToTransfer -= nComplete;

    *pTD->sTransfer.lpdwBytesTransferred = nComplete;
    *pTD->sTransfer.lpfComplete = TRUE;
    *pTD->sTransfer.lpdwError = nStatus;

    if (nStatus != USB_NO_ERROR)
    {
        pED->bfHalted = TRUE;
    }

    pED->bfToggleCarry = (nOptions & USB_TOGGLE_CARRY) ? 1 : 0;
    pED->TransferStatus = STATUS_COMPLETE;

    (m_pChw[pChannel->m_chwInd])->UnlockProcessingThread();

    SetEvent((m_pChw[pChannel->m_chwInd])->m_hUsbProcessingEvent);
}

#endif // MUSB_USEDMA

DWORD CALLBACK CHW::CeResumeThreadStub ( IN PVOID context )
{
    return ((CHW *)context)->CeResumeThread ( );
}
// ******************************************************************
DWORD CHW::CeResumeThread ( )
//
// Purpose: Force the HCD to reset and regenerate itself after power loss.
//
// Parameters: None
//
// Returns: Nothing.
//
// Notes: Because the PDD is probably maintaining pointers to the Hcd and Memory
//   objects, we cannot free/delete them and then reallocate. Instead, we destruct
//   them explicitly and use the placement form of the new operator to reconstruct
//   them in situ. The two flags synchronize access to the objects so that they
//   cannot be accessed before being reconstructed while also guaranteeing that
//   we don't miss power-on events that occur during the reconstruction.
//
//        This function is static
// ******************************************************************
{
    DEBUGMSG(ZONE_ERROR, (TEXT("+CHW::CeResumeThread\n\r")));
    // reconstruct the objects at the same addresses where they were before;
    // this allows us not to have to alert the PDD that the addresses have changed.

    DEBUGCHK( m_fPowerResuming == FALSE );

    // order is important! resuming indicates that the hcd object is temporarily invalid
    // while powerup simply signals that a powerup event has occurred. once the powerup
    // flag is cleared, we will repeat this whole sequence should it get resignalled.
    m_fPowerUpFlag = FALSE;
    m_fPowerResuming = TRUE;

    DeviceDeInitialize();
    for(;;) {  // breaks out upon successful reinit of the object

        if (DeviceInitialize())
            break;
        // getting here means we couldn't reinit the HCD object!
        ASSERT(FALSE);
        DEBUGMSG(ZONE_ERROR, (TEXT("USB cannot reinit the HCD at CE resume; retrying...\n\r")));
        DeviceDeInitialize();
        Sleep(1000);
    }

    // the hcd object is valid again. if a power event occurred between the two flag
    // assignments above then the IST will reinitiate this sequence.
    m_fPowerResuming = FALSE;
    if (m_fPowerUpFlag)
        PowerMgmtCallback(TRUE);

    DEBUGMSG(ZONE_ERROR, (TEXT("-CHW::CeResumeThread\n\r")));
    return 0;
}

//*************************** desclist ******************************************
void CHW::AddElementToListEnd( ListHead *pElement, ListHead **ppQueueHead )
// Adds a given Element into the End of the specified List
{
    pElement->next = NULL;
    if(*ppQueueHead){
        ListHead *pTemp = *ppQueueHead;
        while(pTemp->next)   /*go to end of list*/
            pTemp = pTemp->next;
        pTemp->next = pElement;
    } else
        *ppQueueHead = pElement;
}

void CHW::AddElementToListStart(ListHead *pElement, ListHead **ppQueueHead )
// Adds a given Element to the Start of the List
{
    pElement->next = *ppQueueHead;
    *ppQueueHead = pElement;
}

ListHead * CHW::RemoveElementFromListEnd( ListHead **ppQueueHead )
// brief Removes an Element from the End ofthe specified List
{
    ListHead *pTemp;
    ListHead *pElement;
    pTemp = *ppQueueHead;

	if(*ppQueueHead == NULL) return NULL;

    if(pTemp->next == NULL){ /*only one element in list*/
        *ppQueueHead = NULL;
        return pTemp;
    }
  
    while(pTemp->next->next)
        pTemp = pTemp->next;

    pElement = pTemp->next;
    pTemp->next = pElement->next;
    return pElement;
}

ListHead * CHW::RemoveElementFromListStart( ListHead **ppQueueHead )
// Removes an Element from the Start of the specified List
{
    ListHead *pElement;
	pElement = *ppQueueHead;

	if(*ppQueueHead == NULL) return NULL; 

    *ppQueueHead = pElement->next;
    pElement->next = NULL;

	return pElement;
}

BOOL CHW::RemoveElementFromList( ListHead **ppQueueHead, ListHead *pElement )
// Removes the specified Element from the specified List
{
    ListHead *pTemp = *ppQueueHead;

	if(pTemp == pElement){
        *ppQueueHead = pTemp->next;
        return TRUE;
    }

    while(pTemp->next){
        if(pTemp->next == pElement){
            pTemp->next = pElement->next;
            return TRUE;
        }
        pTemp = pTemp->next;
    }

    return FALSE;
}

void CHW::CreateDescriptors(void)
// Function used to create the EndPoint and Transfer Descriptors.
{
    UINT32 index;

    for(index = 0; index < NO_ED; index++)
        AddElementToListStart((ListHead*)&m_EDs[index],(ListHead**)&m_pFreeED);

    m_EDUsage = 0;

    for(index = 0; index < NO_TD; index++)
        AddElementToListStart((ListHead*)&m_TDs[index],(ListHead**)&m_pFreeTD);

    m_TDUsage = 0;
}

USBED* CHW::AllocateED(void)
// Allocates an EndPoint Descriptor and returns the same to
// calling routine.
{
    USBED *pED = NULL;

    if ((m_EDUsage < NO_ED) && (m_TDUsage < NO_TD))
    {
        pED = (USBED*)RemoveElementFromListStart((ListHead**)&m_pFreeED);
        USBTD *pTD = (USBTD*)RemoveElementFromListStart((ListHead**)&m_pFreeTD);
        memset((PUCHAR) pED, 0, sizeof(USBED));
        memset((PUCHAR) pTD, 0, sizeof(USBTD));

        /*allocate NULL TD*/

        pED->HeadTD = (ListHead *)pTD;
        pED->TailTD = (ListHead *)pTD;

        m_EDUsage ++;
        m_TDUsage ++;
    }

    return pED;
}

USBTD* CHW::AllocateTD(void)
// Allocates a Transfer Descriptor and returns the same to the
// calling routine.
{
    USBTD* pTD = NULL;

    if (m_TDUsage < NO_TD)
    {
        pTD = (USBTD*)RemoveElementFromListStart((ListHead**)&m_pFreeTD);
        memset((PUCHAR) pTD, 0, sizeof(USBTD));

        m_TDUsage++;
    }

    return pTD;
}

void CHW::FreeED(USBED *pElement)
// Frees an EndPoint Descriptor to the Desc Free Pool
{
    if (m_EDUsage > 0) {
        AddElementToListEnd((ListHead*)pElement, (ListHead**)&m_pFreeED);
        m_EDUsage --;
    } else
        DEBUGCHK(FALSE);
}

void CHW::FreeTD(USBTD *pElement)
// Frees a Transfer Descriptor to the Desc Free Pool
{
    if (m_TDUsage > 0) {
        AddElementToListEnd((ListHead*)pElement, (ListHead**)&m_pFreeTD);
        m_TDUsage --;
    } else
        DEBUGCHK(FALSE);
}

#ifdef USB_STAT

void CHW::GetStatInfo(UINT32 * retBuf)
{
	int j;
	for (j=0; j<MGC_MAX_USB_ENDS-1; j++)
		*retBuf++ = (UINT32)m_pProcessEDIn[j];
	
	for (j=0; j<MGC_MAX_USB_ENDS-1; j++)
		*retBuf++ = (UINT32)m_pProcessEDOut[j];

	*retBuf++ = m_EDUsage;
	*retBuf++ = m_TDUsage;
	
}
#endif


