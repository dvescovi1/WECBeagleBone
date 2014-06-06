// All rights reserved ADENEO EMBEDDED 2010
//========================================================================
//   Copyright (c) Texas Instruments Incorporated 2010. All Rights Reserved.
//   Copyright (c) MPC Data Limited 2007.  All rights reserved.
//
//   Use of this software is controlled by the terms and conditions found
//   in the license agreement under which this software has been supplied.
//========================================================================

//! \file UsbFnPdd.cpp
//! \brief AM3517 USB Controller Peripheral Source File
//!
//!  In Windows CE the universal serial bus (USB) function controller
//!  drivers are layered drivers. They contain an MDD and a PDD.
//!  This eases portability across hardware platforms. The client drivers
//!  are abstracted as separate clients that are loaded by the
//!  USB function driver.
//!
//! \version  1.00 Sept 19 2006 File Created

/* WINDOWS CE Public Includes ---------------------------------------------- */
#pragma warning(push)
#pragma warning(disable: 4115 4214 4127)
#include <windows.h>
#include <usbfnoverlapped.h>
#include <usbfn.h>

/* PLATFORM Specific Includes ---------------------------------------------- */
#include "am33x.h"
#include "am33x_usbcdma.h"
#include "am33x_usb.h"
#include "soc_cfg.h"
#include "UsbfnPdd.h"
#include "CppiDma.h"
#pragma warning(push)

/* Global Variables ---------------------------------------------- */

#pragma warning(push)
#pragma warning(disable: 4053 6320)

/* DEBUG dpCurSettings */
#ifdef DEBUG
extern DBGPARAM dpCurSettings = {
    L"UsbFn", {
        L"Error",       L"Warning",     L"Init",        L"Transfer",
        L"Pipe",        L"Send",        L"Receive",     L"USB Events",
        L"PddInit",     L"PddEP0",      L"PddRX",       L"PddTX",
        L"Function",    L"Comments",    L"ISO",         L"PddDMA"
    },

    DBG_ERROR|DBG_WARNING|DBG_INIT|/*DBG_TRANSFER|*/
    /*DBG_PIPE|DBG_SEND|DBG_RECEIVE|DBG_USB_EVENTS|*/
    DBG_PDD_INIT|/*DBG_PDD_EP0|DBG_PDD_RX|DBG_PDD_TX|*/
    /*DBG_FUNCTION|DBG_COMMENT|DBG_PDD_ISO|*/DBG_PDD_DMA|
    /*DBG_CELOG|*/0
};
#endif

#if CPPI_DMA_SUPPORT
/* DMA callback interface */
void cppiCompletionCallback (USBFNPDDCONTEXT *pPdd);
#endif

/* Local routines */
static BOOL InitEndpointDmaBuffer (USBFNPDDCONTEXT *pPdd, DWORD epNum );
static BOOL DeinitEndpointDmaBuffer (USBFNPDDCONTEXT *pPdd, DWORD epNum);

/************************************************************************
* f_DeviceRegParams - Global Var to hold the Device registry parameters
*/
static const DEVICE_REGISTRY_PARAM f_DeviceRegParams[] = {
    {
        L"Index", PARAM_DWORD, FALSE, offset(USBFNPDDCONTEXT, dwIndex),
        fieldsize(USBFNPDDCONTEXT, dwIndex),
        (void *)1
    }, {
        L"Priority256", PARAM_DWORD, FALSE, offset(USBFNPDDCONTEXT, priority256),
        fieldsize(USBFNPDDCONTEXT, priority256),
        (void *)101
    }, {
        L"DmaBufferSize", PARAM_DWORD, FALSE,
        offset(USBFNPDDCONTEXT, dmaBufferSize),
        fieldsize(USBFNPDDCONTEXT, dmaBufferSize),
        (void *)0
    }, {
        L"ActivityEvent", PARAM_STRING, FALSE,
        offset(USBFNPDDCONTEXT, szActivityEvent),
        fieldsize(USBFNPDDCONTEXT, szActivityEvent),
        (void *)""
    }, {
        L"DisablePowerManagement", PARAM_DWORD, FALSE,
        offset(USBFNPDDCONTEXT, disablePowerManagement),
        fieldsize(USBFNPDDCONTEXT, disablePowerManagement),
        (void *)0
    }
#if CPPI_DMA_SUPPORT
//    , {
//        L"DescriptorCount", PARAM_DWORD, FALSE,
//        offset(USBFNPDDCONTEXT, descriptorCount),
//        fieldsize(USBFNPDDCONTEXT, descriptorCount),
//        (void *)CPPI_HD_COUNT
//    }
    ,{
        L"DisableRxGenRNDIS", PARAM_DWORD, FALSE,
        offset(USBFNPDDCONTEXT, disableRxGenRNDIS),
        fieldsize(USBFNPDDCONTEXT, disableRxGenRNDIS),
        (void *)FALSE
    }
#endif
};

#if 0
void memdump(unsigned char * data,   unsigned short num_bytes, unsigned short offset    )
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
            if (l>=10) tmp_str[3-j]=(unsigned char)('A'+l-10);
            else       tmp_str[3-j]=(unsigned char)(l+'0');
            n >>= 4 ;
        }
        tmp_str[4]='\0';
        strcat_s ( (char *)tmp_str, _countof(tmp_str), ": ");
        /*          Output the hex bytes        */
        for (j = i; j < (i+16); j++) {
            int m ;
            if (j < num_bytes)  {
                m=((unsigned int)((unsigned char)*(data+j)))/16 ;
                if (m>=10) tmp_str1[0]='A'+(unsigned char)m-10;
                else       tmp_str1[0]=(unsigned char)m+'0';
                m=((unsigned int)((unsigned char)*(data+j)))%16 ;
                if (m>=10) tmp_str1[1]='A'+(unsigned char)m-10;
                else       tmp_str1[1]=(unsigned char)m+'0';
                tmp_str1[2]='\0';
                strcat_s ((char *)tmp_str, _countof(tmp_str), (char *)tmp_str1);
                strcat_s ((char *)tmp_str, _countof(tmp_str), " ");
            } else {
				strcat_s((char *)tmp_str,_countof(tmp_str), ": ");
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
#endif

BOOL ReadFIFO(USBFNPDDCONTEXT *pPdd, UCHAR endpoint, void *pData, DWORD size)
{
    DWORD total   = size / 4;
    DWORD remain  = size % 4;
    DWORD i		  = 0;
    DWORD* pDword = (DWORD*)pData;

    CSL_UsbRegs* pUsbdRegs = pPdd->pUsbdRegs;
    volatile ULONG *pReg = (volatile ULONG*)(&pUsbdRegs->FIFO[endpoint]);

    // Critical section would be handled outside
    DEBUGMSG(ZONE_PDD_RX, (TEXT("ReadFIFO EP(%d): pData(0x%x) total (0x%x), remain (0x%x), size(0x%x)\r\n"), endpoint, pData, total, remain, size));

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

#if 0
    RETAILMSG(1,(TEXT("Read fifo\r\n")));
    memdump((UCHAR*)pData,(USHORT)size,0);
    RETAILMSG(1,(TEXT("\r\n")));
#endif

    return TRUE;
}

BOOL WriteFIFO(USBFNPDDCONTEXT *pPdd, UCHAR endpoint, void *pData, DWORD size)
{
    DWORD total					= size / 4;
    DWORD remain				= size % 4;
    DWORD i						= 0;
    DWORD* pDword				= (DWORD*)pData;
    CSL_UsbRegs* pUsbdRegs = pPdd->pUsbdRegs;
    volatile ULONG *pReg = (volatile ULONG*)(&pUsbdRegs->FIFO[endpoint]);

    // Critical section would be handled outside
    DEBUGMSG(ZONE_PDD_TX, (TEXT("WriteFIFO: total (0x%x), remain (0x%x), size(0x%x)\r\n"), total, remain, size));    

#if 0
    RETAILMSG(1,(TEXT("Write fifo\r\n")));
    memdump((UCHAR*)pData,(USHORT)size,0);
    RETAILMSG(1,(TEXT("\r\n")));
#endif

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

//========================================================================
//!  \fn VOID
//!     SetupUsbRequest(
//!         USBFNPDDCONTEXT     *pPdd,
//!         PUSB_DEVICE_REQUEST pUsbRequest,
//!         UINT16              epNumber,
//!         UINT16              requestSize
//!         )
//!  \brief This routine is used to read the USB SETUP Packet from the
//!         Control EndPoint FIFO.
//!  \param VOID *pPddContext - Pointer to the PDD Context Structure
//!         PUSB_DEVICE_REQUEST pUsbRequest - USB Request Struct
//!         UINT16 epNumber - EndPoint Number
//!         UINT16 requestSize - Size of the Request.
//!  \return None.
//========================================================================
VOID
SetupUsbRequest(
    USBFNPDDCONTEXT     *pPdd,
    PUSB_DEVICE_REQUEST pUsbRequest,
    UINT16              epNumber,
    UINT16              requestSize
    )
{
    UINT16 pktCounter = requestSize;
    PBYTE pUsbDevReq = (PBYTE)pUsbRequest;

    DEBUGCHK(epNumber == 0);
    DEBUGCHK(requestSize == 8);

    // Read pktCounter bytes from the EndPoint FIFO
	ReadFIFO(pPdd, (UCHAR)epNumber, pUsbDevReq, pktCounter);

    /* Save setup packet direction & size for later use */
    pPdd->setupDirRx = (pUsbRequest->bmRequestType & 0x80) == 0;
    pPdd->setupCount = pUsbRequest->wLength;

    PRINTMSG(ZONE_PDD_EP0,
        (L"SetupUsbRequest: Req 0x%02x T 0x%02x V 0x%02x I 0x%x L 0x%x\r\n",
         pUsbRequest->bmRequestType,
         pUsbRequest->bRequest,
         pUsbRequest->wValue,
         pUsbRequest->wIndex,
         pUsbRequest->wLength));
}

//========================================================================
//!  \fn InterruptThread ()
//!  \brief This is interrupt thread. It controls responses to hardware
//!         interrupt. Note that it has two main blocks. Responses
//!         to Control and non-control End Points.
//!         The routines invoked to handle Non-Control EndPoints
//!         responses will depend on whether DMA Support is enabled
//!         or not.
//
//!  \param VOID *pPddContext - Pointer to the PDD Context Structure
//!  \return None.
//========================================================================
static DWORD WINAPI InterruptThread( VOID *pPddContext )
{
    USBFNPDDCONTEXT *pPdd = pPddContext;
    CSL_UsbRegs *pUsbdRegs = pPdd->pUsbdRegs;
//	OMAP_SYSC_GENERAL_REGS  *pSysConfRegs = pPdd->pSysConfRegs;
    UINT32 usbCoreIntrRegVal;
    UINT32 usbEpIntrRegVal;
    UINT16 intrReg;
    UINT16 intrRxReg;
    UINT16 intrTxReg;
    DWORD code;
    DWORD speed;

    /* Wait for PDD start */
    WaitForSingleObject(pPdd->hStartEvent, INFINITE);

    // Wait for API sets to become ready (function specific)
    USBFNPDD_WaitForAPIsReady();

    /* Release the controller.  This is done after PDD start so that USB packet
       processing begins immediately.  If done earlier the host can timeout the
       function device due to delays in responding (particularly in debug builds). */
    USBPeripheralStart(pPdd);

    while (pPdd->exitIntrThread == FALSE)
    {
        PRINTMSG(ZONE_FUNCTION, (L"+InterruptThread\r\n"));

        /* Wait for interrupt */
        code = WaitForSingleObject(pPdd->hIntrEvent, INFINITE);

        /* Exit thread when we are asked so... */
        if (pPdd->exitIntrThread == TRUE) break;

//#if CPPI_DMA_SUPPORT
        /* Handle DMA interrupts */
//        cppiCompletionCallback(pPdd);
//#endif

        /* Check for the Initial USB RESET Interrupt Condition */

        /* Get interrupt source */
		usbCoreIntrRegVal = pUsbdRegs->IRQ_STATUS_1;
        PRINTMSG(/*ZONE_FUNCTION*/ 0, (L"CORE_INTMASKEDR = 0x%x\r\n", usbCoreIntrRegVal));
		pUsbdRegs->IRQ_STATUS_1 = usbCoreIntrRegVal;

		usbEpIntrRegVal = pUsbdRegs->IRQ_STATUS_0;
        PRINTMSG(/*ZONE_FUNCTION*/ 0, (L"EP_INTMASKEDR = 0x%x\r\n", usbEpIntrRegVal));
		pUsbdRegs->IRQ_STATUS_0 = usbEpIntrRegVal;

//		pSysConfRegs->CONTROL_LVL_INTR_CLEAR |= (1 << 4);

        intrRxReg = (UINT16)((usbEpIntrRegVal   & USB_OTG_RXINT_MASK)  >> USB_OTG_RXINT_SHIFT);
        intrTxReg = (UINT16)((usbEpIntrRegVal   & USB_OTG_TXINT_MASK)  >> USB_OTG_TXINT_SHIFT);
        intrReg   = (UINT16)((usbCoreIntrRegVal & USB_OTG_USBINT_MASK) >> USB_OTG_USBINT_SHIFT);

        /* USB Core Interrupt Handling */
        if (intrReg != 0)
        {
            HandleUsbCoreInterrupt(pPdd, intrReg);

            /* Check for the RESET State and if so, inform the MDD
             * if not done already
             */
            if ( ( ((intrReg & MGC_M_INTR_RESET)  != 0) ||
                   ((intrReg & MGC_M_INTR_RESUME) != 0)) &&
                 (pPdd->resetComplete == FALSE))
            {
                pPdd->resetComplete = TRUE;
                pPdd->ep[0].epStage = MGC_END0_START;
                pPdd->fWaitingForHandshake = FALSE;
                /*
                 * OTG may not detect attach/detach events correctly
                 * on some platforms. Simulate a attach/detach event
                 * to clear any previous state on reset.
                 */
                if (pPdd->attachState == UFN_DETACH)
                {
                    PRINTMSG(ZONE_PDD_INIT, (L">>> ATTACH >>>\r\n"));
                    pPdd->pfnNotify (pPdd->pMddContext, UFN_MSG_BUS_EVENTS,
                                     UFN_ATTACH);
                    speed = (pUsbdRegs->POWER & CSL_USB_POWER_HSMODE_MASK) ? BS_HIGH_SPEED : BS_FULL_SPEED;
                    PRINTMSG(ZONE_PDD_INIT, (L"Bus Speed: %s\r\n",
                                             speed == BS_HIGH_SPEED ? L"HIGH" : L"FULL"));
                    pPdd->pfnNotify (pPdd->pMddContext, UFN_MSG_BUS_SPEED, speed);
                    pPdd->attachState = UFN_ATTACH;
                }
                /* Tell MDD about reset... */
                pPdd->pfnNotify (pPdd->pMddContext, UFN_MSG_BUS_EVENTS,
                                 UFN_RESET);
                PRINTMSG(ZONE_PDD_INIT, (L">>> RESET 0 >>>\r\n"));
            }
            else if ( ((intrReg & MGC_M_INTR_RESET) != 0) &&
                      (pPdd->resetComplete == TRUE))
            {
                pPdd->ep[0].epStage = MGC_END0_START;
                pPdd->fWaitingForHandshake = FALSE;
                /* Tell MDD about reset... */
                pPdd->pfnNotify (pPdd->pMddContext, UFN_MSG_BUS_EVENTS,
                                 UFN_RESET);
                PRINTMSG(ZONE_PDD_INIT, (L">>> RESET 1 >>>\r\n"));
            }
        }

        /* EP interrupt handling.  This needs to be done before EP0 interrupt handling
           to prevent what looks like an RNDIS race condition.  Some EP 0 RNDIS encapsulated
           commands require a TX on EP 1.  While the EP 1 TX is being processed another EP 0
           setup can be received and processed but the EP 1 TX must complete before the new
           setup command.  Processing EP 1 interrupts before EP 0 interrupts seems to be
           enough to ensure this without adding special case code. */
        if ((intrTxReg & ~BIT0) || intrRxReg)
        {
            PRINTMSG(ZONE_PDD_RX|ZONE_PDD_TX, (L"InterruptThread: TX 0x%04x, RX 0x%04x\r\n", intrTxReg & ~BIT0, intrRxReg));
            UsbPddTxRxIntrHandler(pPdd, intrTxReg & ~BIT0, intrRxReg);
        }

        /* Check for EP0 Interrupts */
        if ((intrTxReg & BIT0) != 0)
        {
            /* Do not process EP0 interrupts when a setup request is queued - do not
               want to receive another setup just yet. */
            if (pPdd->fHasQueuedSetupRequest)
            {
                PRINTMSG(ZONE_PDD_EP0, (L"InterruptThread: EP 0 interrupt - ignored, waiting for handshake\r\n"));
            }
            else
            {
                PRINTMSG(ZONE_PDD_EP0, (L"InterruptThread: EP 0 interrupt\r\n"));
                UsbPddEp0IntrHandler(pPdd);
            }
        }

        /* As per the Mentor Graphics Documentation, USB Core
         * Interrupts like SOF, Disconnect and SUSPEND needs to
         * be addressed only after EndPoint Interrupt Processing
         */
        if ((intrReg & MGC_M_INTR_SOF) != 0)
        {
            RETAILMSG(0, (L">>> SOF\r\n"));
            /* Enable the Transmission if any are pending from our side */
        }
        if ((intrReg & MGC_M_INTR_DISCONNECT) != 0)
        {
            PRINTMSG(ZONE_PDD_INIT, (L">>> DISCONNECT\r\n"));

            if (pPdd->attachState == UFN_ATTACH)
            {
                pPdd->pfnNotify (pPdd->pMddContext, UFN_MSG_BUS_EVENTS,
                                 UFN_DETACH);
                pPdd->attachState = UFN_DETACH;
                pPdd->resetComplete = FALSE;
                PRINTMSG(ZONE_PDD_INIT, (L">>> DETACH >>>\r\n"));
            }
        }
        if (intrReg & MGC_M_INTR_SUSPEND)
        {
            PRINTMSG(ZONE_PDD_INIT, (L">>> SUSPEND\r\n"));
            
            // Seem to need a delay when resuming in order for class driver 
            // to successfully re-establish connection.
            Sleep(500);
        }

        /* NOTE:  The irq stays asserted until EOI is written */
        pUsbdRegs->EOIR =  0x00;
        InterruptDone (pPdd->sysIntr);
    }

    /* Disable interrupts */
	pUsbdRegs->IRQ_EN_CLR_1 = USB_CORE_INTR_MASK_ALL;
	pUsbdRegs->IRQ_EN_CLR_0 = USB_EP_INTR_MASK_ALL;
    pUsbdRegs->EOIR = 0x00;

    /* Send detach */
    pPdd->pfnNotify(pPdd->pMddContext, UFN_MSG_BUS_EVENTS, UFN_DETACH);
    pPdd->attachState = UFN_DETACH;
    pPdd->resetComplete = FALSE;

    PRINTMSG(ZONE_FUNCTION, (L"-InterruptThread\r\n"));

    return ERROR_SUCCESS;
}

//========================================================================
//!  \fn UfnPdd_ContextSetup ()
//!  \brief Performs the Initialization of the USB FN PDD Context
//!         Structure. This is invoked by the UfnPdd_Init() Routine.
//!
//!  \param LPCTSTR szActiveKey  - Pointer to the Registry Path
//!         USBFNPDDCONTEXT **ppPddContext - Pointer to USB Context Struct
//!  \return BOOL - Returns TRUE or FALSE
//========================================================================
BOOL
UfnPdd_ContextSetup(
    LPCTSTR szActiveKey,
    USBFNPDDCONTEXT **ppPddContext
    )
{
    BOOL retValue = FALSE;
    CSL_UsbRegs *pUsbdRegs = NULL;
    CSL_CppiRegs *pCppiRegs = NULL;
	PHYSICAL_ADDRESS pa = {0};
    DWORD usbIrqVal;

    PRINTMSG(ZONE_FUNCTION, (L"+UfnPdd_ContextSetup\r\n"));

    // Initialise platform PDD
    if (!USBFNPDD_Init())
    {
        DEBUGMSG(ZONE_ERROR, (L"UfnPdd_ContextSetup: Failed to initialise platform USBFN PDD\r\n"));
        goto cleanUp;
    }

    /* Allocate PDD object */
    if (NULL == ((*ppPddContext) = (USBFNPDDCONTEXT *)
                 LocalAlloc(LPTR, sizeof(USBFNPDDCONTEXT))))
    {
        ERRORMSG (TRUE, (TEXT("ERROR: Unable to Alloc PDD Object!!\r\n")));
        goto cleanUp;
    }

    /* Initialize critical section */
    InitializeCriticalSection(&(*ppPddContext)->epCS);
    (*ppPddContext)->devState = 0;

    PRINTMSG(ZONE_PDD_INIT, (
                 L"UfnPdd_ContextSetup: Reading Registry Information\r\n"));

    /* Read device parameters */
    if (GetDeviceRegistryParams (szActiveKey, *ppPddContext,
                                 dimof(f_DeviceRegParams), f_DeviceRegParams) != ERROR_SUCCESS)
    {
        ERRORMSG(TRUE, (L"ERROR: Failed read registry parameters\r\n"
                     ));
        goto cleanUp;
    }

	// hardcoded as must be the same for USBH and USBFN on the both ports
	(*ppPddContext)->descriptorCount = CPPI_HD_COUNT; 
		
	(*ppPddContext)->irq = ((*ppPddContext)->dwIndex == 1) ? IRQ_USBINT0 : IRQ_USBINT1;
	
    /* Open activity event */
    if (wcslen((*ppPddContext)->szActivityEvent) > 0) {
        (*ppPddContext)->hActivityEvent =
            OpenEvent(EVENT_ALL_ACCESS, FALSE, (*ppPddContext)->szActivityEvent);
    }

    /* Start powered up */
    (*ppPddContext)->currentPowerState = D0;
    (*ppPddContext)->parentBus = CreateBusAccessHandle(szActiveKey);

    if ( (*ppPddContext)->parentBus == NULL )
    {
        ERRORMSG (TRUE, (L"ERROR: Failed open bus driver\r\n"
                      ));
        goto cleanUp;
    }

    /* Map the USBD registers */
    pa.LowPart = ((*ppPddContext)->dwIndex == 1)? AM33X_USB0_REGS_PA : AM33X_USB1_REGS_PA ;

    PRINTMSG(ZONE_PDD_INIT, (L"UfnPdd_ContextSetup: Mapping Region 0x%08x Size 0x%x\r\n",
                 pa.LowPart, sizeof(CSL_UsbRegs)));

    pUsbdRegs = (CSL_UsbRegs *) MmMapIoSpace(pa, sizeof(CSL_UsbRegs), FALSE);
    if ( pUsbdRegs == NULL )
    {
        ERRORMSG (TRUE, (L"ERROR: Controller registers mapping failed\r\n"));
        goto cleanUp;
    }
    (*ppPddContext)->pUsbdRegs = pUsbdRegs;

    /* Map the CPPI registers */
    pa.LowPart = AM33X_CPPI_REGS_PA;
    pCppiRegs = (CSL_CppiRegs *) MmMapIoSpace(pa, sizeof(CSL_CppiRegs), FALSE);
    if ( pCppiRegs == NULL )
    {
        ERRORMSG (TRUE, (L"ERROR: CPPI registers mapping failed\r\n"));
        goto cleanUp;
    }
    (*ppPddContext)->pCppiRegs = pCppiRegs;

#if 0
    PRINTMSG(ZONE_PDD_INIT, (L"UfnPdd_ContextSetup: Mapping Region 0x%08x Size 0x%x\r\n",
                 OMAP_SYSC_GENERAL_REGS_PA, sizeof (OMAP_SYSC_GENERAL_REGS)));

    /* Map the sys config registers */
    pa.LowPart = (LONGLONG)OMAP_SYSC_GENERAL_REGS_PA;
    (*ppPddContext)->pSysConfRegs = (OMAP_SYSC_GENERAL_REGS*) MmMapIoSpace(pa,
                                                                sizeof (OMAP_SYSC_GENERAL_REGS),
                                                               FALSE);
    if ( (*ppPddContext)->pSysConfRegs == NULL )
    {
        ERRORMSG (TRUE, (L"ERROR: SYSCONF registers mapping failed\r\n"
                      ));
        goto cleanUp;
    }

#endif
    PRINTMSG(ZONE_PDD_INIT, (L"UfnPdd_ContextSetup: Allocated USB 0x%08x\r\n",
                 (*ppPddContext)->pUsbdRegs));

    /* Perform the USB Core Initialization */
    USBPeripheralInit(*ppPddContext) ;

    (*ppPddContext)->resetComplete  = FALSE;

    /* Create start event */
    (*ppPddContext)->hStartEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	usbIrqVal = (*ppPddContext)->irq;

    if ( !KernelIoControl (IOCTL_HAL_REQUEST_SYSINTR,  &usbIrqVal,
                           sizeof(DWORD), &(*ppPddContext)->sysIntr,
                           sizeof((*ppPddContext)->sysIntr), NULL))
    {
        ERRORMSG (TRUE, (L"ERROR: IOCTL_HAL_REQUEST_SYSTINR call failed\r\n"
                      ));
        goto cleanUp;
    }

    PRINTMSG(ZONE_PDD_INIT, (
                 L"UfnPdd_ContextSetup: USB Interrupt 0x%x SYSINTR 0x%x\r\n",
                 usbIrqVal , (*ppPddContext)->sysIntr));

    retValue = TRUE;

cleanUp:
    PRINTMSG(ZONE_FUNCTION, (L"-UfnPdd_ContextSetup\r\n"));

    return retValue;
}

//========================================================================
//!  \fn UfnPdd_ContextTeardown ()
//!  \brief Performs the teardown of the USB FN PDD Context
//!         Structure. This is invoked by the UfnPdd_Init() Routine and
//!         UfnPdd_Deinit() routines.
//!
//!  \param USBFNPDDCONTEXT *pPddContext - PDD Context Struct
//!  \return None.
//========================================================================
void
UfnPdd_ContextTeardown(
    USBFNPDDCONTEXT *pPddContext
    )
{
    PRINTMSG(ZONE_FUNCTION, (L"+UfnPdd_ContextTeardown\r\n"));

    /* Teardown Sequence will be from bottom to top order */

    if (pPddContext != NULL)
    {
        if (pPddContext->sysIntr)
        {
            /* Mask the Interrupt */
            InterruptMask (pPddContext->sysIntr, TRUE);

            /* Signal the IST Thread to Stop Functioning */

            pPddContext->exitIntrThread = TRUE;
            SetEvent (pPddContext->hIntrEvent) ;

            /* Release the SYSINTR */
            KernelIoControl (IOCTL_HAL_RELEASE_SYSINTR, &(pPddContext->sysIntr),
                             sizeof(DWORD), NULL, 0, NULL);
        }

        if (pPddContext->hIntrEvent != NULL)
        {
            CloseHandle (pPddContext->hIntrEvent);
        }

        {
        DWORD epNum;
        for (epNum = 0; epNum < USBD_EP_COUNT; epNum++)
            if (pPddContext->ep[epNum].pDmaBuffer)
                DeinitEndpointDmaBuffer(pPddContext, epNum);
        }

#if 0
        if (pPddContext->pSysConfRegs != NULL)
        {
            MmUnmapIoSpace ((PVOID)pPddContext->pSysConfRegs, sizeof(*(pPddContext->pSysConfRegs)));
        }
#endif
        if (pPddContext->pUsbPhyReset != NULL)
        {
            MmUnmapIoSpace ((PVOID)pPddContext->pUsbPhyReset, sizeof(pPddContext->pUsbPhyReset));
        }

        if (pPddContext->pCppiRegs != NULL)
        {
            MmUnmapIoSpace (pPddContext->pCppiRegs, sizeof(*(pPddContext->pCppiRegs)));
        }

        if (pPddContext->pUsbdRegs != NULL)
        {
            MmUnmapIoSpace (pPddContext->pUsbdRegs, sizeof(*(pPddContext->pUsbdRegs)));
        }

        if (pPddContext->parentBus != NULL)
        {
            CloseBusAccessHandle (pPddContext->parentBus);
        }

        if (pPddContext->hActivityEvent)
        {
            CloseHandle(pPddContext->hActivityEvent);
            pPddContext->hActivityEvent = NULL;
        }

        /* Last one in the Sequence */
        DeleteCriticalSection (&pPddContext->epCS);
        LocalFree (pPddContext);
    }
    PRINTMSG(ZONE_FUNCTION, (L"-UfnPdd_ContextTeardown\r\n"));
}

//========================================================================
//!  \fn Log2 ()
//!  \brief Trivial log with base 2 function used in EP configuration.
//!  \param DWORD value - Value for which the Log is to be found
//!  \return DWORD.
//========================================================================
static
DWORD
Log2(
    DWORD value
    )
{
    DWORD rc = 0;
    while ( value != 0 )
    {
        value >>= 1;
        rc++;
    }
    if ( rc > 0 ) rc--;
    return rc;
}

//========================================================================
//!  \fn UfnPdd_PowerDown ()
//!  \brief TO - DO
//!  \param TO - DO
//!  \return None.
//========================================================================
VOID
WINAPI
UfnPdd_PowerDown(
    VOID *pPddContext
    )
{
	UNREFERENCED_PARAMETER(pPddContext);

    PRINTMSG(ZONE_FUNCTION, (L"UfnPdd_PowerDown\r\n"));
}

//========================================================================
//!  \fn UfnPdd_PowerUp ()
//!  \brief TO - DO
//!  \param TO - DO
//!  \return None.
//========================================================================
VOID
WINAPI
UfnPdd_PowerUp(
    VOID *pPddContext
    )
{
	UNREFERENCED_PARAMETER(pPddContext);

    PRINTMSG(ZONE_FUNCTION, (L"UfnPdd_PowerUp\r\n"));

    // Handle power changes through bus power ioctls
    //USBPeripheralPowerUp(pPddContext);
    //USBPeripheralStart(pPddContext);
}

//========================================================================
//!  \fn SetPowerState ()
//!  \brief TO - DO.
//!  \param TO - DO
//!  \return None.
//========================================================================
static
VOID
SetPowerState(
    USBFNPDDCONTEXT *pPdd,
    CEDEVICE_POWER_STATE newPowerState
    )
{
    PREFAST_ASSERT(pPdd != NULL);

    PRINTMSG(ZONE_FUNCTION, (L"SetPowerState: USBFN entering D%u\r\n", newPowerState));

	if (newPowerState != pPdd->currentPowerState)
    {
        if (newPowerState == D1 || newPowerState == D2)
        {
            newPowerState = D0;
        }
    }

    if (newPowerState != pPdd->currentPowerState)
    {
        if ((newPowerState < pPdd->currentPowerState) && (pPdd->parentBus != NULL))
        {
            if (SetDevicePowerState(pPdd->parentBus, newPowerState, NULL) &&
                (newPowerState < D3) && (pPdd->currentPowerState >= D3))
            {
                if(pPdd->disablePowerManagement == 0)
                {
                    USBPeripheralPowerUp(pPdd);
                    USBPeripheralStart(pPdd);
                }
            }
        }

        if ((newPowerState > pPdd->currentPowerState) && (pPdd->parentBus != NULL))
        {
            if (SetDevicePowerState(pPdd->parentBus, newPowerState, NULL) &&
                (newPowerState >= D3) && (pPdd->currentPowerState < D3))
            {
				USBPeripheralEndSession(pPdd);

                if(pPdd->disablePowerManagement == 0)
                {
                    USBPeripheralPowerDown(pPdd);
                }
            }
        }

        pPdd->currentPowerState = newPowerState;
    }
}

//========================================================================
//!  \fn ContinueEp0TxTransfer ()
//!  \brief This function sends next packet from transaction buffer. It is
//!         called from interrupt thread and UfnPdd_IssueTransfer.
//!
//!  \param USBFNPDDCONTEXT *pPdd -  PDD Context Struct Pointer
//!         DWORD  endPoint - EndPoint on Which the Tx Transfer is requested
//!  \return None.
//========================================================================
VOID
ContinueEp0TxTransfer(
    USBFNPDDCONTEXT *pPdd,
    DWORD            endPoint
    )
{
    CSL_UsbRegs *pUsbdRegs = pPdd->pUsbdRegs;
    UsbFnEp     *pEP = &pPdd->ep[0];
    STransfer   *pTransfer = pEP->pTransfer;
    BOOL complete = FALSE;
    volatile UINT16 *pepCtrlReg;
    UINT16  ep0CsrReg;
    DWORD space = 0;
    DWORD remain = 0;
    UCHAR *pBuffer = NULL;

    DEBUGCHK(endPoint == 0);

    PRINTMSG(ZONE_PDD_EP0, (L"ContinueEp0TxTransfer: "
                           L"pTransfer 0x%08x (%d, %d, 0x%0x)\r\n",
                           pTransfer, pTransfer != NULL ? pTransfer->cbBuffer : 0,
                           pTransfer != NULL ? pTransfer->cbTransferred : 0,
                           pTransfer != NULL ? pTransfer->dwUsbError : -1));

    if (pTransfer == NULL)
    {
        PRINTMSG(ZONE_ERROR, (L"ContinueEp0TxTransfer: Transfer NULL!\r\n"));
        pEP->epStage = MGC_END0_START;
        goto done;
    }

    /* Enter CriticalSection */
    LOCK_ENDPOINT(pPdd);

    pepCtrlReg = &pUsbdRegs->EPCSR[ endPoint ].PERI_TXCSR;
    ep0CsrReg = (*pepCtrlReg);

    /* If the hardware has set the STALL Bits, clearing them before
     * will ensure smooth USB Enumeration Process. In case we do
     * not perform the same, the current reply being sent to
     * the host can be interpreted as Stall or incomplete reply
     * write into the EP FIFO and update the TxPktRdy Bits
     */
    if ((ep0CsrReg & MGC_M_CSR0_P_SENTSTALL) != 0)
    {
        ep0CsrReg &= MGC_EP0_STALL_BITS;
        ep0CsrReg |= MGC_M_CSR0_P_SVDRXPKTRDY;
        (*pepCtrlReg) = ep0CsrReg;
    }

    /* First time through, check if ZLP is required for this transfer */
    if (pTransfer->cbTransferred == 0)
    {
        if (pTransfer->cbBuffer > 0 &&
            pTransfer->cbBuffer < pPdd->setupRequest.wLength &&
            (pTransfer->cbBuffer % pEP->maxPacketSize) == 0)
        {
            pEP->zlpReq = TRUE;
        }
        else
        {
            pEP->zlpReq = FALSE;
        }
    }

    /* Check if we have Transmitted Entire Packet */
    if ((pTransfer->cbTransferred == pTransfer->cbBuffer) && !pEP->zlpReq)
    {
        PRINTMSG(ZONE_PDD_EP0, (L"ContinueEp0TxTransfer: TX complete\r\n"));
        pTransfer->dwUsbError = UFN_NO_ERROR;
        complete       = TRUE;
        pEP->epStage   = MGC_END0_START;
        goto cleanUp;
    }

    __try
    {
        pBuffer = (UCHAR*)pTransfer->pvBuffer + pTransfer->cbTransferred;
        space = pTransfer->cbBuffer - pTransfer->cbTransferred;

        /* How many bytes we can send just now? */
        remain = pEP->maxPacketSize;
        if (remain > space) remain = space;

        /* Final ZLP? */
        if (remain == 0 && pEP->zlpReq)
            pEP->zlpReq = FALSE;

		/* Write data to FIFO */
        PRINTMSG(ZONE_PDD_EP0, (L"ContinueEp0TxTransfer: Writing %d bytes to FIFO\r\n", remain));
		WriteFIFO(pPdd, (UCHAR)endPoint, pBuffer, remain);

		/* We transfered some data */
        //pTransfer->cbTransferred = pTransfer->cbBuffer - space;
        pTransfer->cbTransferred += remain;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        pTransfer->dwUsbError = UFN_CLIENT_BUFFER_ERROR;
        complete = TRUE;
        PRINTMSG(ZONE_ERROR, (L"ContinueEp0TxTransfer: Client buffer exception!\r\n"));
    }

    /* Inform the USB Core that There is data in the FIFO */
    ep0CsrReg &= MGC_EP0_STALL_BITS;
    ep0CsrReg |= MGC_M_CSR0_TXPKTRDY;

    /* Check if we have Transmitted Entire Packet */
    if (pTransfer->cbTransferred == pTransfer->cbBuffer && !pEP->zlpReq)
    {
        ep0CsrReg |= MGC_M_CSR0_P_DATAEND;

        /* Now wait for final interrupt before completing TX */
        pEP->epStage = MGC_END0_STAGE_STATUSOUT;
    }

cleanUp:
    (*pepCtrlReg) = ep0CsrReg;

    /* Leave CriticalSection */
    UNLOCK_ENDPOINT(pPdd);

    PRINTMSG(FALSE, /*TRUE, ZONE_SEND*/
        (L"Wrote 0x%02x into EP%u_CSR_REG\r\n", ep0CsrReg, endPoint));

    /* If transaction is complete we should tell MDD */
    if (complete == TRUE)
    {
        PRINTMSG(ZONE_PDD_EP0,
                 (L"ContinueEp0TxTransfer: "
                  L"pTransfer 0x%08x (%d, %d, 0x%0x) - notify complete\r\n",
                  pTransfer, pTransfer != NULL ? pTransfer->cbBuffer : 0,
                  pTransfer != NULL ? pTransfer->cbTransferred : 0,
                  pTransfer != NULL ? pTransfer->dwUsbError : -1));

        pEP->pTransfer = NULL;
        pPdd->pfnNotify (pPdd->pMddContext, UFN_MSG_TRANSFER_COMPLETE,
                         (DWORD)pTransfer);
    }

done:
    return ;
}

//========================================================================
//!  \fn ContinueTxTransfer ()
//!  \brief This function sends next packet from transaction buffer. It is
//!         called from interrupt thread and UfnPdd_IssueTransfer.
//!
//!  \param USBFNPDDCONTEXT *pPdd -  PDD Context Struct Pointer
//!         DWORD  endPoint - EndPoint on Which the Tx Transfer is requested
//!  \return None.
//========================================================================
VOID
ContinueTxTransfer(
    USBFNPDDCONTEXT *pPdd,
    DWORD            endPoint
    )
{
    CSL_UsbRegs *pUsbdRegs = pPdd->pUsbdRegs;
    UsbFnEp     *pEP = &pPdd->ep[endPoint];
    STransfer   *pTransfer = pEP->pTransfer;
    BOOL complete = FALSE;
    volatile UINT16 *pepCtrlReg;
    UINT16  epCsrReg;
	DWORD space = 0;
    DWORD count = 0;
    UCHAR *pBuffer = NULL;

    /* EP0 has its own TX handler */
    DEBUGCHK(endPoint != 0);

    PRINTMSG(ZONE_PDD_TX,
             (L"ContinueTxTransfer: EP %d, pTransfer 0x%08x (%d, %d, 0x%0x)\r\n",
              endPoint, pTransfer, pTransfer != NULL ? pTransfer->cbBuffer : 0,
              pTransfer != NULL ? pTransfer->cbTransferred : 0,
              pTransfer != NULL ? pTransfer->dwUsbError : -1));

    if (pTransfer == NULL)
    {
        PRINTMSG(ZONE_ERROR, (L"ContinueTxTransfer: EP %d, Transfer NULL!\r\n", endPoint));
        pEP->epStage = MGC_END0_START;
        goto done;
    }

    /* Enter CriticalSection */
    LOCK_ENDPOINT(pPdd);

    pEP->usingDma = FALSE;

    pepCtrlReg = &pUsbdRegs->EPCSR[ endPoint ].PERI_TXCSR;
    epCsrReg = (*pepCtrlReg);

    /* Is this final interrupt of transfer? */
    if ((pTransfer->cbTransferred == pTransfer->cbBuffer) &&
        (pEP->zeroLength == FALSE))
    {
        PRINTMSG(ZONE_PDD_TX, (L"ContinueTxTransfer: EP %d, TX complete\r\n", endPoint));
        pTransfer->dwUsbError = UFN_NO_ERROR;
        complete = TRUE;
        pEP->epStage = MGC_END0_START;
        epCsrReg &= ~(MGC_M_TXCSR_FIFONOTEMPTY | MGC_M_TXCSR_P_UNDERRUN);

#ifdef CPPI_DMA_SUPPORT
        /* For DMA case, TX interrupt only enabled when required */
		pUsbdRegs->IRQ_EN_CLR_0 = (1 << endPoint) << USB_OTG_TXINT_SHIFT;
#endif

        /* NOTE: Class driver is responsible for sending zero len packet if
         * the transfer was a multiple of the FIFO Size.
         */
        goto cleanUp;
    }

    /* Packet TX should always have completed */
    if ((epCsrReg & MGC_M_TXCSR_TXPKTRDY) != 0)
    {
        PRINTMSG(ZONE_ERROR, (L"ContinueTxTransfer: EP %d, Error, packet TX not complete!\r\n", endPoint));
        DEBUGCHK(0);
        UNLOCK_ENDPOINT (pPdd);
        goto done;
    }

    /* This EP has entered the TX State wherein it Transmits
     * data to the Host. Depending on whether the Size fits
     * the FIFO in one single write or multiple writes, the EP
     * will return back to IDLE in this iteration itself or
     * further iterations
     */
    pEP->epStage = MGC_END0_STAGE_TX;

    __try
    {
        pBuffer = (UCHAR*)pTransfer->pvBuffer + pTransfer->cbTransferred;
        space = pTransfer->cbBuffer - pTransfer->cbTransferred;

        /* Non Zero Endpoint: No zero length padding needed. */
        pEP->zeroLength = FALSE;

        /* How many bytes we can send just now? */
        count = pEP->maxPacketSize;
        if (count > space) count = space;

        PRINTMSG(ZONE_PDD_TX,
                 (L"ContinueTxTransfer: EP %d, Writing %d bytes, maxSize %d, left %d\r\n",
                  endPoint, count, pEP->maxPacketSize, space));

        /* Write data to FIFO */
		WriteFIFO(pPdd, (UCHAR)endPoint, pBuffer, count);

		/*
          * We will update the pTransfer->cbTransferred here
          * because we would like to receive an ACK from the Host
          * before declaring that the data was sent successfully
          * to the Host. It results in a one interrupt cycle
          * delay
        */
        //pTransfer->cbTransferred = pTransfer->cbBuffer - space;
        pTransfer->cbTransferred += count;

#ifdef CPPI_DMA_SUPPORT
        /* For DMA case, TX interrupt only enabled when required */
		pUsbdRegs->IRQ_EN_SET_0 = (1 << endPoint) << USB_OTG_TXINT_SHIFT;
#endif
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        pTransfer->dwUsbError = UFN_CLIENT_BUFFER_ERROR;
        complete = TRUE;
        PRINTMSG(ZONE_ERROR, (L"Continue0TxTransfer: Client buffer exception!\r\n"));
    }

    epCsrReg &= ~(MGC_M_TXCSR_AUTOSET | MGC_M_TXCSR_DMAENAB);
    epCsrReg &= ~(MGC_M_TXCSR_FIFONOTEMPTY | MGC_M_TXCSR_P_UNDERRUN);
    epCsrReg |= (MGC_M_TXCSR_MODE | MGC_M_TXCSR_TXPKTRDY);

    /* Check for the current transfer Size. For all small sized pkts < 64
     * this condition will become TRUE for the first iteration itself.
     * However, when the MDD Tries to send 512 bytes of data,
     * this logic will become TRUE only for the last chunk.
     */

    /* Check if we have Transmitted Entire Packet */
    if (pTransfer->cbTransferred == pTransfer->cbBuffer)
    {
        /* Now wait for final interrupt before completing TX */
        PRINTMSG(ZONE_PDD_TX, (L"ContinueTxTransfer: EP %d, all bytes written\r\n", endPoint));
    }

cleanUp:

    (*pepCtrlReg) = epCsrReg;

    /* Leave CriticalSection */
    UNLOCK_ENDPOINT(pPdd);

    PRINTMSG(FALSE, /*TRUE ZONE_SEND, (endPoint !=0)*/
        (L"Wrote 0x%02x into EP%u_CSR\r\n", epCsrReg, endPoint));

    /* If transaction is complete we should tell MDD */
    if (complete == TRUE)
    {
        PRINTMSG(ZONE_PDD_TX,
                 (L"ContinueTxTransfer: "
                  L"EP %d, pTransfer 0x%08x (%d, %d, 0x%0x) - notify complete\r\n",
                  endPoint, pTransfer, pTransfer != NULL ? pTransfer->cbBuffer : 0,
                  pTransfer != NULL ? pTransfer->cbTransferred : 0,
                  pTransfer != NULL ? pTransfer->dwUsbError : -1));
        pEP->pTransfer = NULL;
        pPdd->pfnNotify (pPdd->pMddContext, UFN_MSG_TRANSFER_COMPLETE,
                         (DWORD)pTransfer);
    }

done:
    return;
}
//========================================================================
//!  \fn StartRxTransfer(
//!         USBFNPDDCONTEXT *pPdd,
//!         DWORD            endPoint
//!         )
//!  \brief This routine is used to startup the Receive Operation for
//!         given EndPoint.
//!
//!  \param USBFNPDDCONTEXT *pPdd - PDD Context Structure Pointer
//!         DWORD endPoint - EndPoint
//!  \return None.
//========================================================================
static
VOID
StartRxTransfer(
    USBFNPDDCONTEXT *pPdd,
    DWORD            endPoint
    )
{
    CSL_UsbRegs     *pUsbdRegs = pPdd->pUsbdRegs;
    UsbFnEp         *pEP       = &pPdd->ep[endPoint];

#ifdef DEBUG
	STransfer		*pTransfer = pEP->pTransfer;

    PRINTMSG(ZONE_PDD_RX,
             (L"StartRxTransfer: EP %d, pTransfer 0x%08x (%d, %d, 0x%0x)\r\n",
              endPoint, pTransfer, pTransfer != NULL ? pTransfer->cbBuffer : 0,
              pTransfer != NULL ? pTransfer->cbTransferred : 0,
              pTransfer != NULL ? pTransfer->dwUsbError : -1));
#endif

    if (endPoint != 0)
    {
        pEP->usingDma = FALSE;

        /* Enable RX interrupt now we have a transfer for this EP */
        pUsbdRegs->IRQ_EN_SET_0 = (1 << endPoint) << USB_OTG_RXINT_SHIFT;
    }
}

//========================================================================
//!  \fnVOID
//!     ContinueEp0RxTransfer(
//!         USBFNPDDCONTEXT *pPdd,
//!         DWORD            endPoint
//!         )
//!  \brief This function receives data sent from the Host to the Controller
//!         on endpoint 0.
//!
//!  \param USBFNPDDCONTEXT *pPdd - PDD Context Structure Pointer
//!         DWORD endPoint -  EndPoint from Which Data needs to be unloaded.
//!  \return None.
//========================================================================
VOID
ContinueEp0RxTransfer(
    USBFNPDDCONTEXT *pPdd,
    DWORD            endPoint
    )
{
    CSL_UsbRegs    *pUsbRegs     = pPdd->pUsbdRegs;
    UsbFnEp        *pEP           = &pPdd->ep[endPoint];
    STransfer *pTransfer = pEP->pTransfer;
    BOOL complete = FALSE;
    volatile UINT16 *pepCtrlReg;
    UINT16  epCsrReg;
    UINT16  rcvSize = 0;
    DWORD space;
    DWORD count;
    UCHAR *pBuffer = NULL;
	WORD maxSize;

    PRINTMSG(ZONE_PDD_EP0,
             (L"ContinueEp0RxTransfer: pTransfer 0x%08x (%d, %d, 0x%0x)\r\n",
              pTransfer, pTransfer != NULL ? pTransfer->cbBuffer : 0,
              pTransfer != NULL ? pTransfer->cbTransferred : 0,
              pTransfer != NULL ? pTransfer->dwUsbError : -1));

    /* Enter CriticalSection */
    LOCK_ENDPOINT(pPdd);

    pepCtrlReg = &pUsbRegs->EPCSR[ endPoint ].PERI_TXCSR;

    epCsrReg = (*pepCtrlReg);

    /* Got EP 0 interrupt before MDD issued transfer? */
    if (pTransfer == NULL)
    {
        PRINTMSG(ZONE_PDD_EP0, (L"ContinueEp0RxTransfer: No transfer yet"));
        goto cleanUp;
    }

    /* Is this a status phase (IN) interrupt for EP 0? */
    if (pEP->epStage == MGC_END0_STAGE_STATUSIN)
    {
        PRINTMSG(ZONE_PDD_EP0, (L"ContinueEp0RxTransfer: RX complete\r\n"));
        pEP->epStage = MGC_END0_START;
        goto cleanUp;
    }

    rcvSize = pUsbRegs->EPCSR[ endPoint ].RXCOUNT;
    maxSize = pEP->maxPacketSize;

    /* If controller NAKs the host we can get an interrupt before the whole
       packet is received - just ignore and wait for whole packet to arrive. */
    if (rcvSize != maxSize && rcvSize != (pTransfer->cbBuffer - pTransfer->cbTransferred))
        goto cleanUp;

    /* Update the State of this EP */
    pEP->epStage = MGC_END0_STAGE_RX;
    __try
    {
        pBuffer = (UCHAR*)pTransfer->pvBuffer + pTransfer->cbTransferred;
        space = pTransfer->cbBuffer - pTransfer->cbTransferred;

        /* Get number of bytes to be unloaded from FIFO
         * Check the FIFO's Max Pkt Size against the Size
         * available in the pTransfer Buffer
         */

        /* First Check what amount of space is available in the pTransfer
         * Buffer and also the amount of data sent by the Host. The
         * minimum of these two needs to be considered
         */
        count = (rcvSize > space) ? space : rcvSize;

        PRINTMSG(ZONE_PDD_EP0,
                 (L"ContinueEp0RxTransfer: Reading %d bytes, maxSize %d, left %d\r\n",
                  count, maxSize, space));
        
		/* Read data */
		ReadFIFO(pPdd, (UCHAR)endPoint, pBuffer, count);

		/* We transfered some data */
        pTransfer->cbTransferred += count ;

        /* Is this end of transfer? We will determine this
         * situation based on two conditions.
         * 1. The Buffer Size inside the pTransfer is filled up
         * 2. The Received pkt Size is lesser than the maximum
         * size of the RxFIFO. The Spec says that if huge number
         * of data needs to be sent to the Controller, it will
         * send it in multiples of the wMaxPacketSize field
         * of the EndPoint Descriptor. The last residue data is
         * sent in a packet which has length lesser than wMaxPacketSize
         */
        if ((pTransfer->cbTransferred == pTransfer->cbBuffer) ||
            (count < maxSize))
        {
            PRINTMSG(ZONE_PDD_EP0, (L"ContinueEp0RxTransfer: All bytes received\r\n"));

            /* Hand the packet to MDD at this point */
            pTransfer->dwUsbError = UFN_NO_ERROR;
            complete = TRUE;

            /* Wait for the status in phase to complete */
            pEP->epStage = MGC_END0_STAGE_STATUSIN;

            /* Note: We do not ACK the final packet until UfnPdd_SendControlStatusHandshake(). */
            // epCsrReg |= MGC_M_CSR0_P_DATAEND;
        }

        /* Note: We do not ACK the final packet until UfnPdd_SendControlStatusHandshake(). */
        if (!complete)
        {
            epCsrReg &= ~MGC_M_RXCSR_RXPKTRDY;
            epCsrReg |= MGC_M_CSR0_P_SVDRXPKTRDY;
            (*pepCtrlReg) = epCsrReg;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        pTransfer->dwUsbError = UFN_CLIENT_BUFFER_ERROR;
        complete = TRUE;
        PRINTMSG(ZONE_ERROR, (L"ContinueEp0RxTransfer: Client buffer exception!\r\n"));
    }

cleanUp:

    /* Leave CriticalSection */
    UNLOCK_ENDPOINT(pPdd);

    /* If transaction is complete we should tell MDD */
    if (complete == TRUE)
    {
        PRINTMSG(ZONE_PDD_EP0, (L"ContinueEp0RxTransfer: "
                 L"pTransfer 0x%08x (%d, %d, 0x%0x) - notify complete\r\n",
                 pTransfer, pTransfer != NULL ? pTransfer->cbBuffer : 0,
                 pTransfer != NULL ? pTransfer->cbTransferred : 0,
                 pTransfer != NULL ? pTransfer->dwUsbError : -1));
        pEP->pTransfer = NULL;
        pPdd->pfnNotify(pPdd->pMddContext, UFN_MSG_TRANSFER_COMPLETE, (DWORD)pTransfer);
    }

    return;
}

//========================================================================
//!  \fnVOID
//!     ContinueRxTransfer(
//!         USBFNPDDCONTEXT *pPdd,
//!         DWORD            endPoint
//!         )
//!  \brief This function receives data sent from the Host to the Controller.
//!         It is used for End-Points which are configured as BULK-OUT,
//!         ISO-OUT. This routine will read the data from the EP FIFO and
//!         update the USB EndPoint Status Structur UsbFnEp for the specified
//!         EndPoint. If the Entire data was read from the EP FIFO, the
//!         same is informed to the MDD through the MDD Notification
//!         UFN_MSG_TRANSFER_COMPLETE.
//!
//!  \param USBFNPDDCONTEXT *pPdd - PDD Context Structure Pointer
//!         DWORD endPoint -  EndPoint from Which Data needs to be unloaded.
//!  \return None.
//========================================================================
VOID
ContinueRxTransfer(
    USBFNPDDCONTEXT *pPdd,
    DWORD            endPoint
    )
{
    CSL_UsbRegs    *pUsbRegs     = pPdd->pUsbdRegs;
    UsbFnEp        *pEP           = &pPdd->ep[endPoint];
    STransfer *pTransfer = pEP->pTransfer;
    BOOL complete = FALSE;
    volatile UINT16 *pepCtrlReg;
    UINT16  epCsrReg;
    UINT16  rcvSize = 0;
    DWORD space;
    DWORD count;
    UCHAR *pBuffer = NULL;
	WORD maxSize;

    PRINTMSG(ZONE_PDD_RX,
             (L"ContinueRxTransfer: EP %d, pTransfer 0x%08x (%d, %d, 0x%0x)\r\n",
              endPoint, pTransfer, pTransfer != NULL ? pTransfer->cbBuffer : 0,
              pTransfer != NULL ? pTransfer->cbTransferred : 0,
              pTransfer != NULL ? pTransfer->dwUsbError : -1));

    /* Enter CriticalSection */
    LOCK_ENDPOINT(pPdd);

    pepCtrlReg = &pUsbRegs->EPCSR[ endPoint ].PERI_RXCSR;
    epCsrReg = (*pepCtrlReg);

    /* Should always have a transfer */
    if (pTransfer == NULL)
    {
        PRINTMSG(ZONE_ERROR, (L"ContinueRxTransfer: EP %d, Error, NULL transfer, CSR 0x%x",
                              endPoint, epCsrReg));
        goto cleanUp;
    }

    /* Arrived here before pkt received (probably after a NAK)? */
    if ((epCsrReg & MGC_M_RXCSR_RXPKTRDY) == 0)
    {
        PRINTMSG(ZONE_PDD_RX, (L"ContinueRxTransfer: EP %d, No packet received, CSR 0x%x",
                               endPoint, epCsrReg));
        goto cleanUp;
    }

    rcvSize = pUsbRegs->EPCSR[ endPoint ].RXCOUNT;
    maxSize = pEP->maxPacketSize;

    /* Update the State of this EP */
    pEP->epStage = MGC_END0_STAGE_RX;
    __try
    {
        pBuffer = (UCHAR*)pTransfer->pvBuffer + pTransfer->cbTransferred;
        space = pTransfer->cbBuffer - pTransfer->cbTransferred;

        /* Get number of bytes to be unloaded from FIFO
         * Check the FIFO's Max Pkt Size against the Size
         * available in the pTransfer Buffer
         */

        /* First Check what amount of space is available in the pTransfer
         * Buffer and also the amount of data sent by the Host. The
         * minimum of these two needs to be considered
         */
        count = (rcvSize > space) ? space : rcvSize;

        PRINTMSG(ZONE_PDD_RX,
                 (L"ContinueRxTransfer: EP %d, Reading %d bytes, maxSize %d, left %d\r\n",
                  endPoint, count, maxSize, space));

		/* Read data */
		ReadFIFO(pPdd, (UCHAR)endPoint, pBuffer, count);

        /* We transfered some data */
        pTransfer->cbTransferred += count ;

        /* Is this end of transfer? We will determine this
         * situation based on two conditions.
         * 1. The Buffer Size inside the pTransfer is filled up
         * 2. The Received pkt Size is lesser than the maximum
         * size of the RxFIFO. The Spec says that if huge number
         * of data needs to be sent to the Controller, it will
         * send it in multiples of the wMaxPacketSize field
         * of the EndPoint Descriptor. The last residue data is
         * sent in a packet which has length lesser than wMaxPacketSize
         */
        if ((pTransfer->cbTransferred == pTransfer->cbBuffer) ||
            (count < maxSize))
        {
            /* Done */
            PRINTMSG(ZONE_PDD_RX, (L"ContinueRxTransfer: EP %d, RX complete\r\n", endPoint));
            pTransfer->dwUsbError = UFN_NO_ERROR;
            pEP->epStage   = MGC_END0_START;
            complete       = TRUE;

            /* Disable RX interrupt */
			pUsbRegs->IRQ_EN_CLR_0 = (1 << endPoint) << USB_OTG_RXINT_SHIFT;
        }

        /* Packet Unloaded from RX FIFO, clear RxPktRDY */
        epCsrReg &= ~MGC_M_RXCSR_RXPKTRDY;
        (*pepCtrlReg) = epCsrReg;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        pTransfer->dwUsbError = UFN_CLIENT_BUFFER_ERROR;
        complete = TRUE;
        PRINTMSG(ZONE_ERROR, (L"ContinueRxTransfer: Client buffer exception!\r\n"));
    }

cleanUp:

    /* Leave CriticalSection */
    UNLOCK_ENDPOINT(pPdd);

    /* If transaction is complete we should tell MDD */
    if (complete == TRUE)
    {
        PRINTMSG(ZONE_PDD_RX, (L"ContinueRxTransfer: "
                 L"EP %d, pTransfer 0x%08x (%d, %d, 0x%0x) - notify complete\r\n",
                 endPoint, pTransfer, pTransfer != NULL ? pTransfer->cbBuffer : 0,
                 pTransfer != NULL ? pTransfer->cbTransferred : 0,
                 pTransfer != NULL ? pTransfer->dwUsbError : -1));
        pEP->pTransfer = NULL;
        pPdd->pfnNotify(pPdd->pMddContext, UFN_MSG_TRANSFER_COMPLETE, (DWORD)pTransfer);
    }

    return;
}

//========================================================================
//!  \fn ContinueTxDmaTransfer(
//!    USBFNPDDCONTEXT *pPdd,
//!    DWORD            endPoint
//!    )
//!  \brief This routine is used to setup the DMA Operation for the
//!         Tx Data EndPoints. Right now, it just copies the data to be
//!         transmitted from the MDD Buffer to the PDD's DMA Buffer Area.
//!         It then invokes the DMA Controller's Channel Program
//!         Handler which in turn initiates the Tx DMA Transfer.
//!
//!  \param USBFNPDDCONTEXT *pPdd - Pointer to the USB Context Structure
//!         DWORD endPoint - EndPoint on which Transfer is to be taken up
//!  \return None.
//========================================================================
static
VOID
ContinueTxDmaTransfer(
    USBFNPDDCONTEXT *pPdd,
    DWORD            endPoint
    )
{
    CSL_UsbRegs    *pUsbdRegs     = pPdd->pUsbdRegs;
    UsbFnEp        *pEP           = &pPdd->ep[endPoint];
    STransfer      *pTransfer     = pEP->pTransfer;
    BOOL complete = FALSE;
    UCHAR *pBuffer = NULL;
    UINT16      epCsrReg;

#ifdef CPPI_DMA_SUPPORT
    struct dma_controller * pDmaCntrl =
                (struct dma_controller *)pPdd->pDmaCntrl;
#endif /* #ifdef CPPI_DMA_SUPPORT */

    DWORD size = 0;
    DWORD fullPackets;
    DWORD lastPacketSize;
    UINT32 paBuffer;


    PRINTMSG(ZONE_PDD_TX, (L"ContinueTxDmaTransfer: "
                           L"EP %d, pTransfer 0x%08x (%d, %d, 0x%0x)\r\n",
                           endPoint, pTransfer, pTransfer != NULL ? pTransfer->cbBuffer : 0,
                           pTransfer != NULL ? pTransfer->cbTransferred : 0,
                           pTransfer != NULL ? pTransfer->dwUsbError : -1));


    /* Enter CriticalSection */
    LOCK_ENDPOINT(pPdd);

    if (pTransfer == NULL) goto cleanUp;

    pEP->usingDma = TRUE;

    /* Currently only support phyisical buffer addresses with overlapped ISO */
    if (pEP->endpointType == USB_ENDPOINT_TYPE_ISOCHRONOUS &&
        (pTransfer->dwFlags & USB_OVERLAPPED) &&
        pTransfer->dwBufferPhysicalAddress == 0)
    {
        PRINTMSG(ZONE_ERROR,
                 (L"ContinueTxDmaTransfer: EP %d, ISO transfers only supported with physical buffer addresses\r\n",
                  endPoint));
        goto cleanUp;
    }

    /* Is this final interrupt of transfer? */
    if ((pTransfer->cbTransferred == pTransfer->cbBuffer) && !pEP->zeroLength)
    {
        pTransfer->dwUsbError = UFN_NO_ERROR;
        complete = TRUE;
        goto cleanUp;
    }

    /* Enter TX stage */
    pEP->epStage = MGC_END0_STAGE_TX;

    /* Get size and buffer position */
    size = pTransfer->cbBuffer - pTransfer->cbTransferred;

    /* If we are using PDD buffer we must check for maximal size
     * Note that we cannot transfer sizes greater than our DMA
     * Buffer Sizes. In case the total transfer requested by the
     * MDD layer is greater than our DMA Buffer Sizes, then the
     * whole Transfer needs to be stage managed by breaking into
     * chunks.
     */
    if ((pTransfer->dwBufferPhysicalAddress == 0) &&
        (size > pPdd->dmaBufferSize))
    {
        /* check for the Size below */
        PRINTMSG(ZONE_PDD_TX, (L"EP %u Transfer Size 0x%08x DMABufSize  0x%x\r\n",
                               endPoint, size, pPdd->dmaBufferSize));
        size = pPdd->dmaBufferSize;
    }

    /*
     * Depending on size we should use different DMA mode, we should
     * process packet aligned transfer in separate way to avoid
     * zero length packet at end of transfer.
     */
    fullPackets = size / pEP->maxPacketSize;
    lastPacketSize = size - fullPackets * pEP->maxPacketSize;

    if (pTransfer->dwBufferPhysicalAddress == 0)
    {
        /* Point to the appropriate offset in the PDD Buffer that
         * needs to be copied into the Driver's DMA Buffer.
         */
        pBuffer  = pTransfer->pvBuffer;
        pBuffer += pTransfer->cbTransferred;
        paBuffer = pEP->paDmaBuffer;

        __try
        {
            if (size > 0) memcpy(pEP->pDmaBuffer, pBuffer, size);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            pTransfer->dwUsbError = UFN_CLIENT_BUFFER_ERROR;
            complete = TRUE;
            PRINTMSG(ZONE_ERROR, (L"ContinueTxDmaTransfer: Client buffer exception!\r\n"));
        }
    }
    else
    {
        paBuffer  = pTransfer->dwBufferPhysicalAddress;
        paBuffer += pTransfer->cbTransferred;
        DEBUGMSG(ZONE_PDD_TX,
                 (L"ContinueTxDmaTransfer: EP %d, DMA from client buffer 0x%08x\r\n",
                  endPoint, paBuffer));
    }

    epCsrReg = pUsbdRegs->EPCSR[ endPoint ].PERI_TXCSR;
    epCsrReg &= ~(MGC_M_TXCSR_AUTOSET);
    epCsrReg |=  (MGC_M_TXCSR_DMAENAB | MGC_M_TXCSR_MODE);
    pUsbdRegs->EPCSR[ endPoint ].PERI_TXCSR = epCsrReg;

    if (complete) goto cleanUp;

#ifdef CPPI_DMA_SUPPORT
    // Overlapped transfer?
    pEP->pOverlappedInfo = NULL;
    if (pTransfer->dwFlags & USB_OVERLAPPED)
    {
        pEP->pOverlappedInfo = (PUSB_OVERLAPPED_INFO)pTransfer->pvPddTransferInfo;
    }

//#define OVERLAPPED_TEST
#ifdef OVERLAPPED_TEST
    {
        static USB_OVERLAPPED_INFO ov;
        ov.lpUserData = NULL;
        ov.dwBytesToIssueCallback = pTransfer->cbBuffer * 3 / 4;
        pEP->pOverlappedInfo = &ov;
    }
#endif

    /* Configure the DMA Controller now */

    /* Note:
     * 1. The maxPacketSize specifies the EP Max Fifo Size. paDmaTx0Buffer
     *    points to the physical Address of the Driver's Tx Buffer.
     *    Size specifies the transfer count.
     *
     * 2. The Transfer count is set to the minimum of the Driver's
     *    DMA Tx Buffer Size and pTransfer->cbBuffer.
     * In case the transfer size is greater than the Tx Buffer size,
     * then this routine will be invoked again from the Interrupt
     * Context also.
     */
    pDmaCntrl->channelProgram (pPdd->ep[endPoint].pDmaChan,
                                pPdd->ep[endPoint].maxPacketSize,
                                paBuffer,
                                size);
#endif /* #ifdef CPPI_DMA_SUPPORT */

cleanUp:
    UNLOCK_ENDPOINT (pPdd);

    return;
}

//========================================================================
//!  \fn TxDmaTransferComplete ()
//!  \brief This function performs the DMA Completion processing for
//!         Tx DMA Operations. This routine first checks if the Transfer
//!         count is equal to the Size requested by the MDD Layer. If
//!         not, it invokes the ContinueTxDMATransfer() to start
//!         another chunk of Transfer. If the transfer is totally
//!         complete, it informs the MDD Layer that this transfer is
//!         complete.
//!         Note that handling of Zero Byte packet transfer for
//!         Transfers which are multiples of the EP Fifo Sizes are
//!         Handled by the CPPI Module itself. We do not need to
//!         perform the same here.
//!
//!  \param USBFNPDDCONTEXT *pPdd -  PDD Context Struct Pointer
//!         DWORD  endPoint - EndPoint on Which the Tx Transfer is requested
//!  \return None.
//========================================================================
int
TxDmaTransferComplete(
    void    *pDriverContext,
    int     chanNumber,
    int     endPoint
    )
{
    USBFNPDDCONTEXT *pPdd = (USBFNPDDCONTEXT *)pDriverContext;
    CSL_UsbRegs     *pUsbdRegs = pPdd->pUsbdRegs;
    UsbFnEp         *pEP = &pPdd->ep[endPoint];
    STransfer       *pTransfer = pEP->pTransfer;
    BOOL            complete = FALSE;
    volatile UINT16 *pepCtrlReg;
    UINT16          epCsrReg;

#ifdef CPPI_DMA_SUPPORT
    struct cppi_channel *pDmaChan = (struct cppi_channel *)
                                    pPdd->ep[endPoint].pDmaChan;
#endif /* #ifdef CPPI_DMA_SUPPORT */

	UNREFERENCED_PARAMETER(chanNumber);

    PRINTMSG(ZONE_PDD_TX,
             (L"TxDmaTransferComplete: "
              L"EP %d, pTransfer 0x%08x (%d, %d, 0x%0x)\r\n",
              endPoint, pTransfer, pTransfer != NULL ? pTransfer->cbBuffer : 0,
              pTransfer != NULL ? pTransfer->cbTransferred : 0,
              pTransfer != NULL ? pTransfer->dwUsbError : -1));

    /* Sanity Check */
    if (pTransfer == NULL)
    {
        /* Not an error if transfer was aborted */
        if (pEP->epStage != MGC_END0_START)
            ERRORMSG(TRUE,
                (L"EP %u Invalid DMA Completion\r\n", endPoint));
        return 1;
    }
    /* Enter CriticalSection */
    LOCK_ENDPOINT(pPdd);

    /* Update the cbTransferred member based on the DMA Transfer Count */
#ifdef CPPI_DMA_SUPPORT
    pTransfer->cbTransferred += pDmaChan->actualLen;
#endif

    /* For overlapped transfers we only get callback when transfer is complete and
       bytes transferred may be less than the buffer size. */
    if ((pTransfer->cbTransferred >= pTransfer->cbBuffer) ||
        (pEP->pOverlappedInfo != NULL))
    {
        /* DMA of packet into FIFO is complete.  Check if packet has gone out on bus. */
        pepCtrlReg = &pUsbdRegs->EPCSR[ endPoint ].PERI_TXCSR;
        epCsrReg = *pepCtrlReg;
        if ((epCsrReg & MGC_M_TXCSR_TXPKTRDY) != 0)
        {
            /* Need to wait for packet to go out onto bus. */
            PRINTMSG(ZONE_PDD_TX,
                     (L"TxDmaTransferComplete: EP %d, Wait for bus write\r\n", endPoint));

            /* Update stage to indicate fifo wait */
            pEP->epStage = MGC_END0_STAGE_ACKWAIT;

            /* Clear any pending EP interrupt */
            pUsbdRegs->IRQ_STATUS_0 = (1 << endPoint) << USB_OTG_TXINT_SHIFT;

            /* Enable TX interrupt */
            pUsbdRegs->IRQ_EN_SET_0 = (1 << endPoint) << USB_OTG_TXINT_SHIFT;

            /* Race condition - check if packet TX just completed */
            epCsrReg = *pepCtrlReg;
            if ((epCsrReg & MGC_M_TXCSR_TXPKTRDY) == 0)
            {
                pEP->epStage = MGC_END0_START;
                pTransfer->dwUsbError = UFN_NO_ERROR;
                complete = TRUE;

                /* Disable TX interrupt */
                pUsbdRegs->IRQ_EN_CLR_0 = (1 << endPoint) << USB_OTG_TXINT_SHIFT;
                pUsbdRegs->IRQ_STATUS_0 = (1 << endPoint) << USB_OTG_TXINT_SHIFT;
            }
        }
        else
        {
            /* Now that the Transfer is complete, return back to IDLE State */
            pEP->epStage = MGC_END0_START;
            pTransfer->dwUsbError = UFN_NO_ERROR;
            complete = TRUE;
        }
    }
    else
    {
        /* Transfer is not complete, only one chunk was completed.
         * Need to start another Chunk of DMA Transfer.
         */
        ContinueTxDmaTransfer(pPdd, endPoint);
    }

    /* Leave CriticalSection */
    UNLOCK_ENDPOINT(pPdd);

    if (complete == TRUE)
    {
        /* If transaction is complete we should tell MDD */
        PRINTMSG(ZONE_PDD_TX,
            (L"TxDmaTransferComplete: "
             L"EP %d, pTransfer 0x%08x (%d, %d, 0x%0x) - notify complete\r\n",
             endPoint, pTransfer, pTransfer != NULL ? pTransfer->cbBuffer : 0,
             pTransfer != NULL ? pTransfer->cbTransferred : 0,
             pTransfer != NULL ? pTransfer->dwUsbError : -1));
        pEP->pTransfer = NULL;
        pPdd->pfnNotify (pPdd->pMddContext, UFN_MSG_TRANSFER_COMPLETE,
                            (DWORD)pTransfer);
    }
    return 0;
}

//========================================================================
//!  \fn TxDmaFifoComplete ()
//!  \brief This function handles the case where the packet has actually 
//!         gone out on the bus.
//!
//!  \param USBFNPDDCONTEXT *pPdd -  PDD Context Struct Pointer
//!         DWORD  endPoint - EndPoint on Which the Tx Transfer is requested
//!  \return None.
//========================================================================
int
TxDmaFifoComplete(
    void    *pDriverContext,
    int     endPoint
    )
{
    USBFNPDDCONTEXT *pPdd = (USBFNPDDCONTEXT *)pDriverContext;
    CSL_UsbRegs     *pUsbdRegs = pPdd->pUsbdRegs;
    UsbFnEp         *pEP = &pPdd->ep[endPoint];
    STransfer       *pTransfer = pEP->pTransfer;
    BOOL            complete = FALSE;
    volatile UINT16 *pepCtrlReg;
    UINT16          epCsrReg;

    PRINTMSG(ZONE_PDD_TX,
             (L"TxDmaFifoComplete: "
              L"EP %d, pTransfer 0x%08x (%d, %d, 0x%0x)\r\n",
              endPoint, pTransfer, pTransfer != NULL ? pTransfer->cbBuffer : 0,
              pTransfer != NULL ? pTransfer->cbTransferred : 0,
              pTransfer != NULL ? pTransfer->dwUsbError : -1));

    /* Sanity Check */
    if (pTransfer == NULL)
    {
        if (pEP->epStage != MGC_END0_START)
            ERRORMSG(TRUE, (L"EP %u Invalid DMA Completion\r\n", endPoint));
        return 1;
    }

    /* Check we were waiting for the final ACK interrupt */
    if (pEP->epStage != MGC_END0_STAGE_ACKWAIT)
    {
        PRINTMSG(ZONE_PDD_TX,
                 (L"TxDmaFifoComplete: EP %d, Not in fifo wait, transferred %d, len %d\r\n",
                  endPoint, pTransfer->cbTransferred, pTransfer->cbBuffer));
        return 0;
    }

    /* Enter CriticalSection */
    LOCK_ENDPOINT(pPdd);

    /* Check that packet has gone out on bus. */
    pepCtrlReg = &pUsbdRegs->EPCSR[ endPoint ].PERI_TXCSR;
    epCsrReg = *pepCtrlReg;
    if ((epCsrReg & MGC_M_TXCSR_TXPKTRDY) != 0)
    {
        /* Need to keep waiting */
        PRINTMSG(ZONE_PDD_TX,
                 (L"TxDmaFifoComplete: EP %d, Wait for bus write\r\n", endPoint));
    }
    else
    {
        /* Now that the Transfer is complete, return back to IDLE State */
        pEP->epStage = MGC_END0_START;
        pTransfer->dwUsbError = UFN_NO_ERROR;
        complete = TRUE;
        
        /* Disable TX interrupt */
        pUsbdRegs->IRQ_EN_CLR_0 = (1 << endPoint) << USB_OTG_TXINT_SHIFT;
        pUsbdRegs->IRQ_STATUS_0 = (1 << endPoint) << USB_OTG_TXINT_SHIFT;
    }

    /* Leave CriticalSection */
    UNLOCK_ENDPOINT(pPdd);

    if (complete == TRUE)
    {
        /* If transaction is complete we should tell MDD */
        PRINTMSG(ZONE_PDD_TX,
                 (L"TxDmaFifoComplete: "
             L"EP %d, pTransfer 0x%08x (%d, %d, 0x%0x) - notify complete\r\n",
             endPoint, pTransfer, pTransfer != NULL ? pTransfer->cbBuffer : 0,
             pTransfer != NULL ? pTransfer->cbTransferred : 0,
             pTransfer != NULL ? pTransfer->dwUsbError : -1));
        pEP->pTransfer = NULL;
        pPdd->pfnNotify (pPdd->pMddContext, UFN_MSG_TRANSFER_COMPLETE,
                         (DWORD)pTransfer);
    }

    return 0;
}

//========================================================================
//!  \fn static VOID
//!     StartRxDmaTransfer(
//!         USBFNPDDCONTEXT *pPdd,
//!         DWORD            endPoint
//!         )
//!  \brief Function to setup the Receive EndPoint for DMA Transfer.
//!         It configures the Receive CSR Registers into DMA Mode and
//!         also performs a check as to what maximum chunk of data
//!         can be received in one single DMA Transfer.
//!
//!  \param USBFNPDDCONTEXT *pPdd - USB PDD Context Struct
//!         DWORD endPoint - EndPoint on which the Transfer is requested
//!  \return None.
//========================================================================
static VOID
StartRxDmaTransfer(
    USBFNPDDCONTEXT *pPdd,
    DWORD            endPoint
    )
{
    CSL_UsbRegs    *pUsbdRegs = pPdd->pUsbdRegs;
    UsbFnEp        *pEP = &pPdd->ep[endPoint];
    STransfer      *pTransfer = pEP->pTransfer;

    DWORD size;
    USHORT cfn;
    UINT16 rxCsrVal;

#ifdef CPPI_DMA_SUPPORT
    UINT32  dmaAddress;

    struct dma_controller * pDmaCntrl =
                (struct dma_controller *)pPdd->pDmaCntrl;

#endif /* #ifdef CPPI_DMA_SUPPORT */

    PRINTMSG(ZONE_PDD_RX,
             (L"StartRxDmaTransfer: EP %d, pTransfer 0x%08x (%d, %d, 0x%0x)\r\n",
              endPoint, pTransfer,
              pTransfer != NULL ? pTransfer->cbBuffer : 0,
              pTransfer != NULL ? pTransfer->cbTransferred : 0,
              pTransfer != NULL ? pTransfer->dwUsbError : -1));

    /* Lock the EndPoint */
    LOCK_ENDPOINT (pPdd);

    if (pTransfer == NULL) goto cleanUp;

    pEP->usingDma = TRUE;

    /* Currently only support phyisical buffer addresses with overlapped ISO */
    if (pEP->endpointType == USB_ENDPOINT_TYPE_ISOCHRONOUS &&
        (pTransfer->dwFlags & USB_OVERLAPPED) &&
        pTransfer->dwBufferPhysicalAddress == 0)
    {
        PRINTMSG(ZONE_ERROR,
                 (L"StartRxDmaTransfer: EP %d, ISO transfers only supported with physical buffer addresses\r\n",
                  endPoint));
        goto cleanUp;
    }

    /* Get size of Data to be received */
    size = pTransfer->cbBuffer - pTransfer->cbTransferred;

    /* If we are using PDD buffer we must check for maximal size
     * Our DMA Buffer Sizes are limited. Hence, we will have to
     * chunk up the entire DMA Transfer in multiple blocks
     * wherein each block is equal to the Total DMA Buffer
     * Size
     */
    if ((pTransfer->dwBufferPhysicalAddress == 0) &&
        (size > pPdd->dmaBufferSize))
    {
        /* check for the Size below */
        PRINTMSG(ZONE_PDD_RX,
                 (L"StartRxDmaTransfer: EP %d, transfer size 0x%x larger than DMABufSize 0x%x\r\n",
                  endPoint, size, pPdd->dmaBufferSize));
        size = pPdd->dmaBufferSize;
    }

    /* Calculate number of USB packets (aka frames) */
    cfn = (USHORT)((size + pEP->maxPacketSize - 1)/pEP->maxPacketSize);

    /* Configure the RxCSR in DMA Mode */

    rxCsrVal = pUsbdRegs->EPCSR[ endPoint ].PERI_RXCSR;
    rxCsrVal &= ~(MGC_M_RXCSR_AUTOCLEAR | MGC_M_RXCSR_DMAMODE);
    /* Note: Must ensure that no bits are cleared here, particularly RXPKTRDY,
       which could be set between reading and writing the register. */
    rxCsrVal |= MGC_M_RXCSR_DMAENAB | MGC_M_RXCSR_P_WZC_BITS;
    pUsbdRegs->EPCSR[ endPoint ].PERI_RXCSR = rxCsrVal;

#ifdef CPPI_DMA_SUPPORT
    /* Disable the Rx interrupts for this EndPoint */
    pUsbdRegs->IRQ_EN_CLR_0 = (1 << endPoint) << USB_OTG_RXINT_SHIFT;

    if (pTransfer->dwBufferPhysicalAddress == 0)
    {
        dmaAddress = pEP->paDmaBuffer;
    }
    else
    {
        dmaAddress = (UINT32)pTransfer->dwBufferPhysicalAddress;
        PRINTMSG(ZONE_PDD_RX,
                 (L"StartRxDmaTransfer: EP %d, DMA to client buffer 0x%08x\r\n",
                  endPoint, dmaAddress));
    }

    // Overlapped transfer?
    pEP->pOverlappedInfo = NULL;
    if (pTransfer->dwFlags & USB_OVERLAPPED)
    {
        pEP->pOverlappedInfo = (PUSB_OVERLAPPED_INFO)pTransfer->pvPddTransferInfo;
    }

#ifdef OVERLAPPED_TEST
    {
        static USB_OVERLAPPED_INFO ov;
        ov.lpUserData = NULL;
        ov.dwBytesToIssueCallback = pTransfer->cbBuffer * 3 / 4;
        pEP->pOverlappedInfo = &ov;
    }
#endif

    /* Verify the Rx DMA Channel Condition Once
     * The Third argument is a bool which checks whether we
     * require multiple BDs or the entire request can be
     * serviced in one single BD. If the requested Packet
     * size is greater than the EpFifoMaxSize, then we
     * require multiple BDs to service this request
     */
    pDmaCntrl->channelProgram(
        pPdd->ep[endPoint].pDmaChan,
        pPdd->ep[endPoint].maxPacketSize,
        dmaAddress,
        size);

#endif  /* CPPI_DMA_SUPPORT */

cleanUp:
    /* Unlock the EndPoint */
    UNLOCK_ENDPOINT (pPdd);

    return;
}

//========================================================================
//!  \fn RxDmaTransferComplete ()
//!  \brief This function performs the DMA Completion processing for Rx
//!         DMA Channels. Note that when the MDD Layer expects transfers
//!         greater than the PDD's DMA Buffer sizes, we need to invoke
//!         StartRxDmaTransfer() once again to initiate the next chunk of
//!         Transfer.
//!         Firstly, the data captured in the DMA Rx Buffer is copied into
//!         MDD's Buffer. The Buffer parameters are updated. If the total
//!         count is reached, the same is informed to MDD, or else another
//!         Rx DMA Transfer is initiated.
//!
//!  \param USBFNPDDCONTEXT *pPdd -  PDD Context Struct Pointer
//!         DWORD  endPoint - EndPoint on Which the Tx Transfer is requested
//!  \return None.
//========================================================================
int
RxDmaTransferComplete(
    void    *pDriverContext,
    int     chanNumber,
    int     endPoint
    )
{
    USBFNPDDCONTEXT *pPdd = (USBFNPDDCONTEXT *)pDriverContext;
    CSL_UsbRegs     *pUsbdRegs = pPdd->pUsbdRegs;
    UsbFnEp         *pEP = &pPdd->ep[endPoint];
    STransfer       *pTransfer = pEP->pTransfer;
    UCHAR *pBuffer = NULL;
    DWORD size;
    BOOL complete = FALSE;
    DWORD   bytesCopied = 0;
    UINT16 rxCsrVal = 0;

#ifdef CPPI_DMA_SUPPORT
    struct cppi_channel *pDmaChan = (struct cppi_channel *)
                                    pPdd->ep[endPoint].pDmaChan;
#endif /* #ifdef CPPI_DMA_SUPPORT */

	UNREFERENCED_PARAMETER(chanNumber);

    PRINTMSG(ZONE_PDD_RX,
             (L"RxDmaTransferComplete: EP %u, pTransfer 0x%08x (%d, %d, 0x%0x)\r\n",
              endPoint, pTransfer, pTransfer != NULL ? pTransfer->cbBuffer : 0,
              pTransfer != NULL ? pTransfer->cbTransferred : 0,
              pTransfer != NULL ? pTransfer->dwUsbError : -1));

    /* Sanity Check */
    if (pTransfer == NULL)
    {
        /* Not an error if transfer was aborted */
        if (pEP->epStage != MGC_END0_START)
            ERRORMSG(TRUE,
                     (L"EP %u Invalid DMA Completion\r\n", endPoint));
        return (1);
    }
    /* Enter CriticalSection */
    LOCK_ENDPOINT(pPdd);

    if (pTransfer->dwBufferPhysicalAddress == 0)
    {
        /* Copy Data back from DMA Buffer to Driver Buffer */
        pBuffer = (UCHAR*)pTransfer->pvBuffer + pTransfer->cbTransferred;

        /* Get size and buffer position */
        size = pTransfer->cbBuffer - pTransfer->cbTransferred;

#ifdef CPPI_DMA_SUPPORT
        bytesCopied = pDmaChan->actualLen;

        if (bytesCopied > size)
        {
            PRINTMSG(ZONE_ERROR,
                     (L"RxDmaTransferComplete: EP %u, Error, actual DMA len 0x%x, buffer left 0x%X\r\n",
                      endPoint, pDmaChan->actualLen, pBuffer));
            bytesCopied = size;
        }
#endif /* #ifdef CPPI_DMA_SUPPORT */

        __try
        {
            /* Copy into MDD Buffer from DMA Buffer */
            if (bytesCopied > 0)
                memcpy(pBuffer, pEP->pDmaBuffer, bytesCopied);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            pTransfer->dwUsbError = UFN_CLIENT_BUFFER_ERROR;
            complete = TRUE;
            PRINTMSG(ZONE_ERROR, (L"RxDmaTransferComplete: Client buffer exception!\r\n"));
        }

        pTransfer->cbTransferred += bytesCopied;

        /* Now Perform a check if the complete chunk is over or not */

        /* When we get less than buffer size or transaction buffer
         * is full transfer finished.
         */
        if ((bytesCopied < pPdd->dmaBufferSize) ||
            (pTransfer->cbTransferred >= pTransfer->cbBuffer))
        {
            /* Now that the Transfer is complete, return back to IDLE State */
            pTransfer->dwUsbError = UFN_NO_ERROR;
            pEP->epStage   = MGC_END0_START;
            complete = TRUE;
        }
        else
        {
            /* Start another transfer */
            StartRxDmaTransfer(pPdd, endPoint);
        }
    }
    else
    {
#ifdef CPPI_DMA_SUPPORT
        /* Data copied direct to client buffer */
        pTransfer->cbTransferred += pDmaChan->actualLen;
        pTransfer->dwUsbError = UFN_NO_ERROR;
        pEP->epStage = MGC_END0_START;
        complete = TRUE;
#endif
    }

    if (complete == TRUE)
    {
        /* Disable DMA */
        rxCsrVal = pUsbdRegs->EPCSR[ endPoint ].PERI_RXCSR;
        rxCsrVal &= ~(MGC_M_RXCSR_DMAENAB);
        rxCsrVal |= MGC_M_RXCSR_P_WZC_BITS;
        pUsbdRegs->EPCSR[ endPoint ].PERI_RXCSR = rxCsrVal;
    }

    /* Leave CriticalSection */
    UNLOCK_ENDPOINT(pPdd);

    if (complete == TRUE)
    {
        /* If transaction is complete we should tell MDD */
        PRINTMSG(ZONE_PDD_RX,
            (L"RxDmaTransferComplete: "
             L"EP %d, pTransfer 0x%08x (%d, %d, 0x%0x) - notify complete\r\n",
             endPoint, pTransfer, pTransfer != NULL ? pTransfer->cbBuffer : 0,
             pTransfer != NULL ? pTransfer->cbTransferred : 0,
             pTransfer != NULL ? pTransfer->dwUsbError : -1));
        pEP->pTransfer = NULL;
        pPdd->pfnNotify (pPdd->pMddContext, UFN_MSG_TRANSFER_COMPLETE,
                        (DWORD)pTransfer);
    }

    return (0);
}

//========================================================================
//!  \fn WINAPI
//!     UfnPdd_IssueTransfer(
//!             PVOID       pvPddContext,
//!             DWORD       endpoint,
//!             PSTransfer  pTransfer
//!             )
//!  \brief This function sets up a transfer to be performed the next time
//!         the host sends an IN token packet or an OUT token packet,
//!         as appropriate.
//!  \param PVOID pvPddContext - Pointer to USB PDD Context
//!         DWORD endpoint - EP on which Transfer is Reqd
//!         PSTransfer  pTransfer - Transfer Data Structure
//!  \return DWORD - ERROR_SUCCESS if successful .
//========================================================================
DWORD
WINAPI
UfnPdd_IssueTransfer(
    PVOID       pvPddContext,
    DWORD       endpoint,
    PSTransfer  pTransfer
    )
{
    DWORD rc = ERROR_SUCCESS;

    USBFNPDDCONTEXT *pPdd       = (USBFNPDDCONTEXT *)pvPddContext;

    if (endpoint == 0)
    {
        PRINTMSG(ZONE_PDD_EP0, (L"UfnPdd_IssueTransfer: EP 0, %s, len %d\r\n",
                                TRANSFER_IS_IN(pTransfer) ? L"TX" : L"RX", pTransfer->cbBuffer));
    }
    else if (TRANSFER_IS_IN(pTransfer))
    {
        PRINTMSG(ZONE_PDD_TX, (L"UfnPdd_IssueTransfer: EP %d, TX, len %d\r\n",
                               endpoint, pTransfer->cbBuffer));
    }
    else
    {
        PRINTMSG(ZONE_PDD_RX, (L"UfnPdd_IssueTransfer: EP %d, RX, len %d\r\n",
                               endpoint, pTransfer->cbBuffer));
    }

    /* Check that EP is initialised */
    if (!pPdd->ep[endpoint].epInitialised)
    {
        ERRORMSG(TRUE, (L"UfnPdd_IssueTransfer: EP%u Error, Endpoint not initialised\r\n",
                        endpoint));
        rc = ERROR_INVALID_PARAMETER;
        goto Exit;
    }

    LOCK_ENDPOINT (pPdd);

    /* Save transfer for interrupt thread */
    DEBUGCHK(pPdd->ep[endpoint].pTransfer == NULL);
    pPdd->ep[endpoint].pTransfer = pTransfer;

    /* If transfer buffer is NULL length must be zero */
    if (pTransfer->pvBuffer == NULL)
    {
        pTransfer->cbBuffer = 0 ;
    }

    DEBUGCHK(pTransfer->dwUsbError == UFN_NOT_COMPLETE_ERROR);

    UNLOCK_ENDPOINT (pPdd);

    /* Depending on direction */
    if (TRANSFER_IS_IN(pTransfer))
    {
        pPdd->ep[endpoint].zeroLength = (pTransfer->cbBuffer == 0);

        if (endpoint == 0)
        {
            ContinueEp0TxTransfer(pPdd, endpoint);
        }
        else
        {
#ifdef CPPI_DMA_SUPPORT
            ContinueTxDmaTransfer(pPdd, endpoint);
#else
            ContinueTxTransfer(pPdd, endpoint);
#endif
        }
    }
    else
    {
        pPdd->ep[endpoint].zeroLength = FALSE;
        pPdd->ep[endpoint].epStage = MGC_END0_STAGE_RX;

        if (endpoint == 0)
        {
            // MDD does not support synchronous completion callbacks so always 
            // wait for RX interrupt before processing an EP0 RX transfer.
            //ContinueEp0RxTransfer (pPdd, endpoint);
        }
        else
        {
#ifdef CPPI_DMA_SUPPORT
            /* Do not use DMA for zero length packets */
            if (pTransfer->cbBuffer != 0)
            { 
                StartRxDmaTransfer(pPdd, endpoint);
            }
            else
#endif
            {
                StartRxTransfer (pPdd, endpoint);
            }
        }
    }

    // Signal activity
    if (pPdd->hActivityEvent)
        SetEvent(pPdd->hActivityEvent);

Exit:
    return (rc);
}

//========================================================================
//!  \fn WINAPI
//!     UfnPdd_AbortTransfer(
//!         PVOID           pvPddContext,
//!         DWORD           endpoint,
//!         PSTransfer      pTransfer
//!         )
//!  \brief This function aborts a transfer that has already started.
//!         This should cause the endpoint's first in, first out (FIFO)
//!         buffer to empty.
//!  \param PVOID pvPddContext - Pointer to USB PDD Context Struct
//!         DWORD endpoint - EndPoint Number
//!         PSTransfer pTransfer - Pointer to the Transfer
//!  \return DWORD - ERROR_SUCCESS if successful.
//========================================================================
DWORD
WINAPI
UfnPdd_AbortTransfer(
    PVOID           pvPddContext,
    DWORD           endpoint,
    PSTransfer      pTransfer
    )
{
    DWORD rc = ERROR_SUCCESS;
    USBFNPDDCONTEXT *pPdd      = (USBFNPDDCONTEXT *)pvPddContext;
    CSL_UsbRegs     *pUsbdRegs = pPdd->pUsbdRegs;
    UINT16 epCtrlReg;

#ifdef CPPI_DMA_SUPPORT
    struct dma_controller * pDmaCntrl =
                (struct dma_controller *)pPdd->pDmaCntrl;
#endif /* #ifdef CPPI_DMA_SUPPORT */

    DEBUGCHK (endpoint < USBD_EP_COUNT);
    PRINTMSG(ZONE_PDD_TX|ZONE_PDD_RX|ZONE_PDD_EP0, (L"UfnPdd_AbortTransfer: EP %u\r\n", endpoint));

    /* Abort the Transfer by Setting the DataEnd Bit */
    LOCK_ENDPOINT (pPdd);
    epCtrlReg = pUsbdRegs->EPCSR[ endpoint ].PERI_TXCSR;
    epCtrlReg |= MGC_M_CSR0_P_DATAEND;
    pUsbdRegs->EPCSR[ endpoint ].PERI_TXCSR = epCtrlReg;
    UNLOCK_ENDPOINT (pPdd);

#ifdef CPPI_DMA_SUPPORT
    if (endpoint != 0)
        pDmaCntrl->pfnChannelAbort (pPdd->ep[endpoint].pDmaChan);
#endif /* #ifdef CPPI_DMA_SUPPORT */

    /* Finish transfer */
    LOCK_ENDPOINT (pPdd);
    pPdd->ep[endpoint].pTransfer = NULL;
    pTransfer->dwUsbError = UFN_CANCELED_ERROR;
    pPdd->ep[endpoint].epStage = MGC_END0_START;
    UNLOCK_ENDPOINT (pPdd);

    /* Inform MDD that the Current Transfer is Completed */
    PRINTMSG(ZONE_PDD_TX|ZONE_PDD_RX|ZONE_PDD_EP0,
             (L"AbortTransfer: EP %x - indicating complete to MDD\r\n", endpoint));
    pPdd->pfnNotify (pPdd->pMddContext, UFN_MSG_TRANSFER_COMPLETE,
                        (DWORD)pTransfer);

    return (rc);
}

//========================================================================
//!  \fn    WINAPI
//!         UfnPdd_StallEndpoint(
//!             PVOID           pvPddContext,
//!             DWORD           endpoint
//!             )
//!  \brief This function is called by MDD to set
//!         endpoint to stall mode (halted).
//!  \param PVOID pvPddContext - Pointer to USB PDD Context Struct
//!         DWORD endpoint - EndPoint Number
//!  \return DWORD - ERROR_SUCCESS if succesful.
//========================================================================
DWORD
WINAPI
UfnPdd_StallEndpoint(
    PVOID           pvPddContext,
    DWORD           endpoint
    )
{
    DWORD rc = ERROR_SUCCESS;

    USBFNPDDCONTEXT *pPdd      = (USBFNPDDCONTEXT *)pvPddContext;
    CSL_UsbRegs     *pUsbdRegs = pPdd->pUsbdRegs;
    volatile UINT16 *pepCtrlReg;
    UINT16  epCtrlVal;
    UINT16  epStallBit;

    DEBUGCHK (endpoint < USBD_EP_COUNT);

    PRINTMSG(ZONE_PDD_TX|ZONE_PDD_RX|ZONE_PDD_EP0,
        (L"UfnPdd_StallEndpoint: EP %d\r\n", endpoint));

    /* Critical Section Support */
    LOCK_ENDPOINT (pPdd);

    /* First check which Register is to be Read */
    if (endpoint == 0)
    {
        pepCtrlReg = &pUsbdRegs->EPCSR[ endpoint ].PERI_TXCSR;
        epStallBit = MGC_M_CSR0_P_SENDSTALL;
        pPdd->ep[0].epStage = MGC_END0_START;
    }
    else
    {
        if (pPdd->ep[endpoint].dirRx == TRUE)
        {
            pepCtrlReg = &pUsbdRegs->EPCSR[ endpoint ].PERI_RXCSR;
            epStallBit = MGC_M_RXCSR_P_SENDSTALL;
        }
        else
        {
            pepCtrlReg = &pUsbdRegs->EPCSR[ endpoint ].PERI_TXCSR;
            epStallBit = MGC_M_TXCSR_P_SENDSTALL;
        }
    }

    epCtrlVal = (*pepCtrlReg);

    /* Stall the EndPoint by Setting the SENDSTALL Bit */
    epCtrlVal |= epStallBit;

    /* Write back */
    (*pepCtrlReg) = epCtrlVal;

    pPdd->ep[endpoint].stalled = TRUE;

    UNLOCK_ENDPOINT (pPdd);

    return (rc);
}

//========================================================================
//!  \fn DWORD
//!         UfnPdd_ClearEndpointStall(
//!             PVOID           pvPddContext,
//!             DWORD           endpoint
//!             )
//!  \brief This function is called by MDD to clear
//!         endpoint stall mode (halted).
//!  \param PVOID pvPddContext - Pointer to USB PDD Context Struct
//!         DWORD endpoint - EndPoint Number
//!  \return DWORD - ERROR_SUCCESS if succesful.
//========================================================================
DWORD
WINAPI
UfnPdd_ClearEndpointStall(
    PVOID           pvPddContext,
    DWORD           endpoint
    )
{
    DWORD rc = ERROR_SUCCESS;
    USBFNPDDCONTEXT *pPdd      = (USBFNPDDCONTEXT *)pvPddContext;
    CSL_UsbRegs     *pUsbdRegs = pPdd->pUsbdRegs;

    volatile UINT16 *pepReg;
    UINT16  epCtrlVal;

    DEBUGCHK (endpoint < USBD_EP_COUNT);

    PRINTMSG(ZONE_PDD_TX|ZONE_PDD_RX|ZONE_PDD_EP0,
        (L"UfnPdd_ClearEndpointStall: EP %u\r\n", endpoint));

    /* Critical Section Support */
    LOCK_ENDPOINT (pPdd);

    /* First check which Register is to be Read */
    if (endpoint == 0)
    {
        pepReg = &pUsbdRegs->EPCSR[ endpoint ].PERI_TXCSR;
        epCtrlVal = (*pepReg);
        epCtrlVal &= ~(MGC_M_CSR0_P_SENDSTALL | MGC_M_CSR0_P_SENTSTALL);
    }
    else
    {
        /* USB Class Drivers likes to use this EndPoint
         * as Receive EndPoint. For Receive EndPoints, configure
         * the RXCSR Register of the USB Controller, bits
         * */
        if (pPdd->ep[endpoint].dirRx == TRUE)
        {
            pepReg = &pUsbdRegs->EPCSR[ endpoint ].PERI_RXCSR;
            epCtrlVal = (*pepReg);
            epCtrlVal &= ~(MGC_M_RXCSR_P_SENDSTALL | MGC_M_RXCSR_P_SENTSTALL);
            epCtrlVal &= ~MGC_M_RXCSR_P_WZC_BITS;
            epCtrlVal |= MGC_M_RXCSR_CLRDATATOG;
        }
        else
        {
            pepReg = &pUsbdRegs->EPCSR[ endpoint ].PERI_TXCSR;
            epCtrlVal = (*pepReg);
            epCtrlVal &= ~(MGC_M_TXCSR_P_SENDSTALL | MGC_M_TXCSR_P_SENTSTALL);
            epCtrlVal &= ~MGC_M_TXCSR_P_WZC_BITS;
            epCtrlVal |= MGC_M_TXCSR_CLRDATATOG;
        }
    }

    /* Clear SENDSTALL and SENTSTALL to bring the EP out of Stall Condition */
    (*pepReg) = epCtrlVal;

    pPdd->ep[endpoint].stalled = FALSE;

    /* leave CriticalSection */
    UNLOCK_ENDPOINT (pPdd);

    return (rc);
}

//========================================================================
//!  \fn DWORD
//!         UfnPdd_IsEndpointHalted(
//!             PVOID   pvPddContext,
//!             DWORD   endpoint,
//!             PBOOL   pHalted
//!             )
//!  \brief This function checks to see if the endpoint is stalled.
//!  \param PVOID pvPddContext - Pointer to USB PDD Context Struct
//!         DWORD endpoint - EndPoint Number
//!         PBOOL pHalted - Argument to store the Halt Status
//!  \return DWORD - ERROR_SUCCESS if succesful.
//========================================================================
DWORD
WINAPI
UfnPdd_IsEndpointHalted(
    PVOID   pvPddContext,
    DWORD   endpoint,
    PBOOL   pHalted
    )
{
    DWORD rc = ERROR_SUCCESS;
    USBFNPDDCONTEXT *pPdd = (USBFNPDDCONTEXT *)pvPddContext;

    PRINTMSG(ZONE_PDD_TX|ZONE_PDD_RX|ZONE_PDD_EP0,
             (L"UfnPdd_IsEndpointHalted: EP %u\r\n", endpoint));

    DEBUGCHK (endpoint < USBD_EP_COUNT);
    DEBUGCHK (pHalted != NULL);

    LOCK_ENDPOINT (pPdd);

    /* There is no HALT Bit in the USB Controller EP Status Registers
     * in the SOC. However, the driver tries to maintain the
     * Stall status for each EndPoint that it operates upon. We
     * will use this status flag to detect whether the EndPoint
     * is currently Active or Halted.
     */
    *pHalted = pPdd->ep[endpoint].stalled ;
    UNLOCK_ENDPOINT (pPdd);

    return (rc);

}


//========================================================================
//!  \fn DWORD
//!         UfnPdd_SendControlStatusHandshake(
//!                 PVOID           pvPddContext,
//!                 DWORD           endpoint
//!                 )
//!  \brief This function causes endpoint zero to initiate the control
//!         status phase of a control transfer. This causes the control
//!         transfer to be completed with either an ACK handshake packet
//!         or a STALL handshake packet.
//!  \param TO - DO
//!  \return None.
//========================================================================
DWORD
WINAPI
UfnPdd_SendControlStatusHandshake(
    PVOID           pvPddContext,
    DWORD           endpoint
    )
{
    DWORD rc = ERROR_SUCCESS;

    USBFNPDDCONTEXT *pPdd       = (USBFNPDDCONTEXT *)pvPddContext;
    CSL_UsbRegs     *pUsbdRegs  = pPdd->pUsbdRegs;
    UsbFnEp         *pEP        = &pPdd->ep[endpoint];
    UINT16  ep0CsrReg;
    volatile UINT16 *pepCtrlReg;

    /* Handshake control for EP0 RX transfers is implemented.  Class drivers
       can perform EP stall, deinit and init calls between receiving the 
       setup packet and calling this handshake fn so the EP0 setup RX transfer
       is not completed until this handshake is performed.  This prevents the 
       host from sending packets on the other endpoints.
       The EP0 RX handshake is performed by delaying setting of the SVDRXPKTRDY 
       and DATAEND bits in the CSR.  This delays the STATUS IN part of the the 
       transfer and causes the controller to NAK the host until we are ready.
       Note: Delaying setting of just DATAEND causes a SETUPEND error.
     */

    PRINTMSG(ZONE_PDD_EP0, (L"UfnPdd_SendControlStatusHandshake: EP %d\r\n", endpoint));

    LOCK_ENDPOINT (pPdd);

    /* May have just received a new setup so check EP 0 stage */
    if (pEP->epStage == MGC_END0_START)
    {
        /* Deliver a queued setup packet */
        if (pPdd->fHasQueuedSetupRequest)
        {
            pPdd->fHasQueuedSetupRequest = FALSE;

            if (pPdd->queuedSetupRequest.wLength == 0)
            {
                /* Already done DATAEND when setup received */
            }
            else if ((pPdd->queuedSetupRequest.bmRequestType & USB_ENDPOINT_DIRECTION_MASK) != 0)
            {
                pEP->epStage = MGC_END0_STAGE_TX;
            }
            else
            {
                pEP->epStage = MGC_END0_STAGE_RX;
            }

            /* Will need another handshake call */
            pPdd->fWaitingForHandshake = TRUE;

            UNLOCK_ENDPOINT (pPdd);

            PRINTMSG(ZONE_PDD_EP0,
                     (L"UfnPdd_SendControlStatusHandshake: Setup pkt, len %d - sent to MDD\r\n",
                      pPdd->queuedSetupRequest.wLength));

            pPdd->pfnNotify(pPdd->pMddContext, UFN_MSG_SETUP_PACKET, (DWORD)&pPdd->queuedSetupRequest);

            // Call Ep0 interrupt handler to check if datas were received in fifo but was not handled 
            // beceause interrupt was skipped due to an already queuing setup packet.
            UsbPddEp0IntrHandler(pPdd);
        }
        else
        {
            /* No longer waiting */
            pPdd->fWaitingForHandshake = FALSE;

            UNLOCK_ENDPOINT (pPdd);
        }
    }
    else if (pEP->epStage == MGC_END0_STAGE_STATUSIN)
    {
        /* Complete the RX transfer */
        pepCtrlReg = &pUsbdRegs->EPCSR[0].PERI_TXCSR;
        ep0CsrReg = (*pepCtrlReg);
        ep0CsrReg |= MGC_M_CSR0_P_DATAEND;
        ep0CsrReg &= ~MGC_M_RXCSR_RXPKTRDY;
        ep0CsrReg |= MGC_M_CSR0_P_SVDRXPKTRDY;
        (*pepCtrlReg) = ep0CsrReg;

        pEP->epStage = MGC_END0_START;

        /* No longer waiting */
        pPdd->fWaitingForHandshake = FALSE;

        UNLOCK_ENDPOINT (pPdd);
    }
    else
    {
        DEBUGCHK(pPdd->fHasQueuedSetupRequest == FALSE);

        UNLOCK_ENDPOINT (pPdd);
    }

    return (rc);
}

//========================================================================
//!  \fn UfnPdd_InitiateRemoteWakeup ()
//!  \brief Initiates Wakeup for the USB Controller
//!  \param PVOID pvPddContext - PDD Context
//!  \return None.
//========================================================================
DWORD
WINAPI
UfnPdd_InitiateRemoteWakeup(
    PVOID pvPddContext
    )
{
    DWORD rc = ERROR_SUCCESS;
    USBFNPDDCONTEXT *pPdd       = (USBFNPDDCONTEXT *)pvPddContext;
    UINT8      powerRegVal;
    PRINTMSG(ZONE_FUNCTION, /*TRUE */
            (L"UfnPdd_InitiateRemoteWakeup:\r\n"));

    LOCK_ENDPOINT (pPdd);

    /* Write into the Power Registers RESUME Bit to initiate
     * REMOTE WAKEUP from the Suspended State
     */

    powerRegVal = pPdd->pUsbdRegs->POWER;
    powerRegVal |= MGC_M_POWER_RESUME;
    pPdd->pUsbdRegs->POWER = powerRegVal;

    UNLOCK_ENDPOINT (pPdd);

    return (rc);

}

//========================================================================
//!  \fn DWORD
//!     UfnPdd_IOControl(
//!         VOID            *pPddContext,
//!         IOCTL_SOURCE    source,
//!         DWORD           code,
//!         UCHAR           *pInBuffer,
//!         DWORD           inSize,
//!         UCHAR           *pOutBuffer,
//!         DWORD           outSize,
//!         DWORD           *pOutSize
//!         )
//!  \brief This routine handles all the IOCTL Calls from the USB MDD
//!         Layer. It is invoked from the MDD Layer whenever any Class
//!         Drivers invoke a DeviceIoControl() call on the UFN Device.
//!         The MDD Layer will invoke this function for PDD Supported
//!         Ioctls and for those which it does not understand.
//!
//!  \param VOID *pPddContext - PDD Context Structure Pointer
//!         IOCTL_SOURCE source -  Source of the IOCTL
//!         DWORD code - IOCTL Code
//!  \return DWORD - ERROR_SUCCESS if Successful and
//!                  ERROR_INVALID_PARAMETER if fail
//========================================================================
DWORD
WINAPI
UfnPdd_IOControl(
    VOID            *pPddContext,
    IOCTL_SOURCE    source,
    DWORD           code,
    UCHAR           *pInBuffer,
    DWORD           inSize,
    UCHAR           *pOutBuffer,
    DWORD           outSize,
    DWORD           *pOutSize
    )
{
    DWORD rc = ERROR_INVALID_PARAMETER;
    USBFNPDDCONTEXT *pPdd = (USBFNPDDCONTEXT *)pPddContext;

    CE_BUS_POWER_STATE *pBusPowerState;
    CEDEVICE_POWER_STATE devicePowerState;

	UNREFERENCED_PARAMETER(pOutBuffer);
	UNREFERENCED_PARAMETER(pOutSize);
	UNREFERENCED_PARAMETER(outSize);

    PRINTMSG(ZONE_FUNCTION, (L"UfnPdd_IOControl:\r\n"));

    switch (code)
    {

        case IOCTL_UFN_PDD_SET_CONFIGURATION:
            PRINTMSG(ZONE_PDD_INIT,
                     (L"IOCTL_UFN_PDD_SET_CONFIGURATION: value 0x%x\r\n", *((DWORD*)pInBuffer)));
            /* MDD will handle this request */

            /* When testing RNDIS with one particular PC a delay was needed here */
#ifdef UFN_SET_CONFIG_DELAY_REQUIRED
            Sleep(100);
#endif
            break;

        case IOCTL_UFN_PDD_SET_INTERFACE:
            PRINTMSG(ZONE_PDD_INIT,
                     (L"IOCTL_UFN_PDD_SET_INTERFACE: value 0x%x\r\n", *((DWORD*)pInBuffer)));
            /* MDD will handle this request */
            break;

            /* In a debug build we get a DEBUGCHK from VirtualCopy due
             * to the way the RNDIS client driver allocates its DMA
             * buffers.  Prevent the RNDIS client driver from allocating
             * DMA buffers by disabling this IOCTL.
             */
#ifndef DEBUG
        case IOCTL_UFN_GET_PDD_INFO:
        {
            UFN_PDD_INFO info;

            PRINTMSG(ZONE_PDD_INIT, (L"IOCTL_UFN_GET_PDD_INFO:\r\n"));
            if (source != BUS_IOCTL)
                break;
            if ((pOutBuffer == NULL) ||
                (outSize < sizeof(UFN_PDD_INFO)))
                break;
            info.InterfaceType = Internal;
            info.BusNumber = 0;
            info.dwAlignment = sizeof(DWORD);

            memcpy (pOutBuffer, &info, sizeof(UFN_PDD_INFO));
            rc = ERROR_SUCCESS;
            break;
        }
#endif

        case IOCTL_BUS_GET_POWER_STATE:
            PRINTMSG(ZONE_PDD_INIT, (L"IOCTL_BUS_GET_POWER_STATE:\r\n"));
            if (source != MDD_IOCTL)
                break;
            if ((pInBuffer == NULL) ||
                (inSize < sizeof(CE_BUS_POWER_STATE)))
                break;
            pBusPowerState = (CE_BUS_POWER_STATE*)pInBuffer;

            memcpy (pBusPowerState->lpceDevicePowerState, &pPdd->currentPowerState,
                    sizeof(CEDEVICE_POWER_STATE));
            rc = ERROR_SUCCESS;
            break;
        case IOCTL_BUS_SET_POWER_STATE:
            PRINTMSG(ZONE_PDD_INIT, (L"IOCTL_BUS_SET_POWER_STATE:\r\n"));
            if (source != MDD_IOCTL)
                break;
            if ((pInBuffer == NULL) ||
                (inSize < sizeof(CE_BUS_POWER_STATE)))
                break;
            pBusPowerState = (CE_BUS_POWER_STATE*)pInBuffer;

            memcpy (&devicePowerState, pBusPowerState->lpceDevicePowerState,
                    sizeof(CEDEVICE_POWER_STATE));

            PRINTMSG(ZONE_PDD_INIT, (L"IOCTL_BUS_SET_POWER_STATE:\r\n"));
            SetPowerState(pPdd , devicePowerState);
            rc = ERROR_SUCCESS;
            break;
        case IOCTL_POWER_SET:
            PRINTMSG(ZONE_PDD_INIT, (L"IOCTL_POWER_SET\r\n"));
            break;
        case IOCTL_POWER_GET:
            PRINTMSG(ZONE_PDD_INIT, (L"IOCTL_POWER_GET\r\n"));
            break;
        default:
            PRINTMSG(ZONE_PDD_INIT, (L"Unknown IOCTL: %d\r\n", code));
        }
    return rc;
}

//========================================================================
//!  \fn DWORD
//!         UfnPdd_Deinit(
//!             VOID *pPddContext
//!         )
//!  \brief This function deinitializes the PDD.
//!  \param VOID *pPddContext - PDD Context Structure Pointer
//!  \return DWORD - ERROR_SUCESS if successful.
//========================================================================
DWORD
WINAPI
UfnPdd_Deinit(
    VOID *pPddContext
    )
{
    DWORD rc = ERROR_SUCCESS;
    BOOL endpointsNotIdle = TRUE;
    DWORD tc0 = GetTickCount();
    DWORD endpoint;

    USBFNPDDCONTEXT *pPdd = (USBFNPDDCONTEXT *)pPddContext;

    DEBUGCHK(pPddContext != NULL);

    // Wait for endpoints to go into idle state before completing Deinit
    PRINTMSG(ZONE_FUNCTION, (L"UfnPdd_Deinit: waiting for endpoints to go idle\r\n"));
    while((endpointsNotIdle && ((GetTickCount() - tc0) < 5000)))
    {
        // reset flag
        endpointsNotIdle = FALSE;

        // look for endpoint status
        for(endpoint = 0; endpoint < USBD_EP_COUNT; endpoint++)
        {
            // only poll endpoints that have been initialised
            if((pPdd->ep[endpoint].epInitialised)&&
               (pPdd->ep[endpoint].epStage != MGC_END0_START))
            {
                PRINTMSG(ZONE_FUNCTION, (L"UfnPdd_Deinit: endpoint %d in state %d (not idle!)\r\n",
                    endpoint, pPdd->ep[endpoint].epStage));
                endpointsNotIdle = TRUE;
            }
        }

        // wait before polling again
        Sleep(10);
    }

    if(endpointsNotIdle)
    {
        // timed out waiting for endpoints to go idle
        PRINTMSG(ZONE_ERROR, (L"UfnPdd_Deinit: timed out waiting for endpoints to go idle\r\n"));
    }

#ifdef CPPI_DMA_SUPPORT

    /* Deinitialize the DMA Controller Object */
    cppiControllerDeinit(pPdd);
    pPdd->pDmaCntrl = NULL;

#endif /* #ifdef CPPI_DMA_SUPPORT *

    /* Perform the De-initialization */
    UfnPdd_ContextTeardown(pPdd);

    PRINTMSG(ZONE_FUNCTION, (L"UfnPdd_Deinit:\r\n"));

    return rc;

}

//========================================================================
//!  \fn DWORD
//!         UfnPdd_DeregisterDevice(
//!         VOID *pPddContext
//!         )
//!  \brief This function deregisters a device.
//!  \param VOID *pPddContext - USB PDD Context Structure Pointer
//!  \return DWORD - ERROR_SUCCESS if successful.
//========================================================================
DWORD
WINAPI
UfnPdd_DeregisterDevice(
    VOID *pPddContext
    )
{
    DWORD rc = ERROR_SUCCESS;
    UINT16      epCsrReg = 0;
    UINT16      epCount  = 0;
    UINT8       powerRegVal;

    USBFNPDDCONTEXT *pPdd = (USBFNPDDCONTEXT *)pPddContext;
    CSL_UsbRegs     *pUsbdRegs = pPdd->pUsbdRegs;

    PRINTMSG(ZONE_FUNCTION, /*TRUE, */
            (L"UfnPdd_DeregisterDevice:\r\n"));

    LOCK_ENDPOINT (pPdd);

    /* Flush the FIFO of EndPoint 0 */
    epCsrReg = pUsbdRegs->EPCSR[ 0 ].PERI_TXCSR;
    epCsrReg |= (MGC_M_CSR0_FLUSHFIFO);
    pUsbdRegs->EPCSR[ 0 ].PERI_TXCSR = epCsrReg;

    /* Flush the FIFOs of all the Data EndPoints */
    for(epCount = 1; epCount < USBD_EP_COUNT; epCount++)
    {
        if (pPdd->ep[epCount].dirRx == TRUE)
        {
            epCsrReg = pUsbdRegs->EPCSR[ epCount ].PERI_RXCSR;
            epCsrReg |= MGC_M_RXCSR_FLUSHFIFO;
            pUsbdRegs->EPCSR[ epCount ].PERI_RXCSR = epCsrReg;
        }
        else
        {
            epCsrReg = pUsbdRegs->EPCSR[ epCount ].PERI_TXCSR;
            epCsrReg |= MGC_M_TXCSR_FLUSHFIFO;
            pUsbdRegs->EPCSR[ epCount ].PERI_TXCSR = epCsrReg;
        }

        /* Clear config */
        pPdd->ep[epCount].maxPacketSize = 0;
        pPdd->ep[epCount].fifoOffset = 0;
        pPdd->ep[epCount].epInitialised = FALSE;
    }

    /* Put the Controller in SUSPEND State */

    /* Write into the Power Register's Enable SUSPEND Bit to initiate
     * the SUSPENDM Signal.
     */
    powerRegVal = pUsbdRegs->POWER;
    powerRegVal |= MGC_M_POWER_ENSUSPEND;
    pUsbdRegs->POWER = powerRegVal;

    UNLOCK_ENDPOINT (pPdd);

    return (rc);

}

//========================================================================
//!  \fn DWORD
//!         UfnPdd_Stop(
//!         VOID *pPddContext
//!         )
//!  \brief This function stops the USB function device.  This function
//!         is called before UfnPdd_DeregisterDevice. It should de-attach
//          device to USB bus
//!  \param VOID *pPddContext - PDD Context Structure Pointer
//!  \return DWORD - ERROR_SUCCESS if successful.
//========================================================================
DWORD
WINAPI
UfnPdd_Stop(
    VOID *pPddContext
    )
{
    DWORD rc = ERROR_SUCCESS;
    USBFNPDDCONTEXT *pPdd = (USBFNPDDCONTEXT *)pPddContext;
#ifdef CPPI_DMA_SUPPORT
    struct dma_controller * pDmaCntrl =
                (struct dma_controller *)pPdd->pDmaCntrl;
#endif /* #ifdef CPPI_DMA_SUPPORT */

    PRINTMSG(ZONE_FUNCTION, (L"UfnPdd_Stop:\r\n"));

#ifdef CPPI_DMA_SUPPORT
    /* Stop the DMA Controller Object */
    if (pDmaCntrl != NULL)
    {
        pDmaCntrl->pfnStop(pPdd->pDmaCntrl);
    }
#endif /* #ifdef CPPI_DMA_SUPPORT */

    /* Stop the IST */
    pPdd->exitIntrThread = TRUE;

    if (pPdd->hIntrThread && pPdd->hIntrEvent)
    {
        InterruptDisable(pPdd->sysIntr);
        SetEvent(pPdd->hIntrEvent);
        WaitForSingleObject(pPdd->hIntrThread, INFINITE);
    }

    CloseHandle(pPdd->hIntrThread);
    CloseHandle(pPdd->hIntrEvent);

    pPdd->hIntrThread = NULL;
    pPdd->hIntrEvent  = NULL;

    return (rc);

}

//========================================================================
//!  \fn DWORD
//!         UfnPdd_DeinitEndpoint(
//!         PVOID           pvPddContext,
//!          DWORD           endpoint
//!         )
//!  \brief This function is called when pipe to endpoit is closed. We stall
//!         endpoints there...
//!  \param PVOID pvPddContext - PDD Context Struct Pointer
//!         DWORD endpoint - EndPoint Number
//!  \return DWORD - ERROR_SUCCESS if Successful.
//========================================================================
DWORD
WINAPI
UfnPdd_DeinitEndpoint(
    PVOID           pvPddContext,
    DWORD           endpoint
    )
{
    DWORD rc = ERROR_SUCCESS;
    DWORD intrRegVal = 0;

    UINT16      epCsrReg = 0;
    USBFNPDDCONTEXT *pPdd = (USBFNPDDCONTEXT *)pvPddContext;
    CSL_UsbRegs     *pUsbdRegs = pPdd->pUsbdRegs;

#ifdef CPPI_DMA_SUPPORT
    struct dma_controller *pDma   = (struct dma_controller *)pPdd->pDmaCntrl;
    struct cppi_channel *pDmaChan = (struct cppi_channel *)
                                    pPdd->ep[endpoint].pDmaChan;
#endif /* #ifdef CPPI_DMA_SUPPORT */

    PRINTMSG(ZONE_FUNCTION, /*TRUE, */
            (L"UfnPdd_DeinitEndpoint: %d\r\n", endpoint));

    DEBUGCHK(endpoint < USBD_EP_COUNT);

    LOCK_ENDPOINT (pPdd);

    /* Check which EndPoint is it */
    if (endpoint == 0)
    {
        /* For EndPoint Zero, Abort the Current Transfer by setting
         * up the ServRxPktRdy and the SetupEnd Bits
         */
        epCsrReg = pUsbdRegs->EPCSR[ endpoint ].PERI_TXCSR;
        epCsrReg |= (MGC_M_CSR0_P_SVDSETUPEND | MGC_M_CSR0_P_SVDRXPKTRDY);
        pUsbdRegs->EPCSR[ endpoint ].PERI_TXCSR = epCsrReg;
        intrRegVal |= (1 << endpoint) << USB_OTG_TXINT_SHIFT;
        pPdd->fWaitingForHandshake = FALSE;
    }
    else
    {
#ifdef CPPI_DMA_SUPPORT
        DeinitEndpointDmaBuffer(pPdd, endpoint);
#endif

        if (pPdd->ep[endpoint].dirRx == FALSE)
        {
            epCsrReg = pUsbdRegs->EPCSR[ endpoint ].PERI_TXCSR;
            epCsrReg |= MGC_M_TXCSR_FLUSHFIFO;
            pUsbdRegs->EPCSR[ endpoint ].PERI_TXCSR = epCsrReg;
            intrRegVal |= (1 << endpoint) << USB_OTG_TXINT_SHIFT;
        }
        else
        {
            epCsrReg = pUsbdRegs->EPCSR[ endpoint ].PERI_RXCSR;
            epCsrReg |= MGC_M_RXCSR_FLUSHFIFO;
            pUsbdRegs->EPCSR[ endpoint ].PERI_RXCSR = epCsrReg;
            intrRegVal |= (1 << endpoint) << USB_OTG_RXINT_SHIFT;
        }

#ifdef CPPI_DMA_SUPPORT
        /* Try Releasing the Allocated CPPI DMA Channel */
        if ((endpoint != 0) && (pDmaChan != NULL))
        {
            pDma->pfnChannelRelease ((struct dma_channel *)pDmaChan);
            pPdd->ep[endpoint].pDmaChan = NULL;
        }
#endif /* #ifdef CPPI_DMA_SUPPORT */
    }

    /* Disable the interrupts for this EndPoint */
    pUsbdRegs->IRQ_EN_CLR_0 = intrRegVal;

    /* Now Update the Status Vars of this EP */
    pPdd->ep[endpoint].epStage   = MGC_END0_START;
    pPdd->ep[endpoint].pTransfer = NULL;
    pPdd->ep[endpoint].epInitialised = FALSE;

    UNLOCK_ENDPOINT (pPdd);

    /* Allow things to settle before EP is re-initialised */
    Sleep(20);

    return (rc);
}


//========================================================================
//!  \fn DWORD
//!         UfnPdd_InitEndpoint(
//!             PVOID                       pvPddContext,
//!             DWORD                       endpoint,
//!             UFN_BUS_SPEED               speed,
//!             PUSB_ENDPOINT_DESCRIPTOR    pEndpointDesc,
//!             PVOID                       pvReserved,
//!             BYTE                        configurationValue,
//!             BYTE                        interfaceNumber,
//!             BYTE                        alternateSetting)
//!  \brief This function configures and allocates an endpoint.
//!         This function verifies that the target endpoint supports the
//!         specified settings and that the target endpoint is not
//!         already allocated. If this is the case, the endpoint is
//!         configured to support the specified settings and then allocated.
//!         This process might include enabling the interrupt of the
//!         associated endpoint.
//!
//!  \param PVOID pvPddContext - Pointer to the USB PDD Context Struct
//!         DWORD endpoint - EndPoint to be Initialized
//!         UFN_BUS_SPEED speed - Speed of the USB EndPoint
//!         PUSB_ENDPOINT_DESCRIPTOR    pEndpointDesc - Descriptor for EP
//!         PVOID pvReserved
//!         BYTE  configurationValue - EP Config Value
//!         BYTE  interfaceNumber - Interface Number
//!         BYTE  alternateSetting - Alternate Settings
//!  \return DWORD - ERROR_SUCCESS if successful.
//========================================================================
DWORD
WINAPI
UfnPdd_InitEndpoint(
    PVOID                       pvPddContext,
    DWORD                       endpoint,
    UFN_BUS_SPEED               speed,
    PUSB_ENDPOINT_DESCRIPTOR    pEndpointDesc,
    PVOID                       pvReserved,
    BYTE                        configurationValue,
    BYTE                        interfaceNumber,
    BYTE                        alternateSetting
    )
{
    DWORD rc                      = ERROR_SUCCESS;
    USBFNPDDCONTEXT *pPdd         = (USBFNPDDCONTEXT *)pvPddContext;
    CSL_UsbRegs     *pUsbdRegs    = pPdd->pUsbdRegs;
    UINT8           fifoSz        = 0;
    UINT8           epAddress     = 0;
    USHORT          epMaxPktSize  = 0;
    UINT16          epRegVal      = 0;
    volatile UINT16 *pepCsrReg    = NULL;
    BYTE            transferType  = 0;
    BOOL            modeOut		  = FALSE;
    DWORD           intrRegVal    = 0;

#ifdef DEBUG
	SETFNAME();
    FUNCTION_ENTER_MSG();
#endif

	UNREFERENCED_PARAMETER(alternateSetting);
	UNREFERENCED_PARAMETER(interfaceNumber);
	UNREFERENCED_PARAMETER(configurationValue);
	UNREFERENCED_PARAMETER(pvReserved);
	UNREFERENCED_PARAMETER(speed);

    /* Get Information from the given EndPoint Descriptor Struct */
    epAddress    = pEndpointDesc->bEndpointAddress;
    epMaxPktSize = (pEndpointDesc->wMaxPacketSize &
                    USB_ENDPOINT_MAX_PACKET_SIZE_MASK);
    transferType = pEndpointDesc->bmAttributes & USB_ENDPOINT_TYPE_MASK;
    intrRegVal = 0;

    PRINTMSG(ZONE_PDD_INIT,
             (L"+UfnPdd_InitEndpoint EP%u PktSize 0x%x Type 0x%x ADDR 0x%x, Speed 0x%x\r\n",
              endpoint, epMaxPktSize, transferType, epAddress, speed));

    /* Must power up when an EP is initialised */
    if ((pPdd->currentPowerState == D3) || (pPdd->currentPowerState == D4))
    {
        SetPowerState(pPdd, D0);
    }

    /* Convert the MaxPktSize to the USB Controller Convention
     * Maximum packet size to be allowed (before any splitting within the
     * FIFO of Bulk packets prior to transmission). If m = SZ[3:0], the
     * FIFO size is calculated as 2^(m+3) for single packet buffering and
     * 2^(m+4) for dual packet buffering.
     */
    fifoSz = (UINT8) Log2(epMaxPktSize) - 3;


    // Get access to the indexed regs
    pUsbdRegs->INDEX = (UINT8)endpoint;

    /* Select EP */
    if (endpoint != 0)
    {
        DEBUGCHK(endpoint < USBD_EP_COUNT);

        /* Check that FIFO offset has been determined in UfnPdd_RegisterDevice() */
        if (pPdd->ep[endpoint].fifoOffset == 0)
        {
            ERRORMSG(TRUE, (L"UfnPdd_InitEndpoint: EP%u Error, FIFO offset not determined\r\n",
                            endpoint));
            rc = ERROR_INVALID_PARAMETER;
            goto Exit;
        }

        /* Setup Direction (mode_in bit)  */

        /* If Bit7 of the EP Address is not set, then it is Host to Device
         * The macro USB_ENDPOINT_DIRECTION_OUT from usb100.h checks for
         * bit 7 not set. Hence this EndPoint is Receive for us.
         */
        modeOut = USB_ENDPOINT_DIRECTION_OUT(epAddress);

        pPdd->ep[endpoint].maxPacketSize = epMaxPktSize;
        pPdd->ep[endpoint].endpointType  = transferType;

#ifdef CPPI_DMA_SUPPORT
        InitEndpointDmaBuffer(pPdd, endpoint);
#endif

        // Clear any previous setup
        pUsbdRegs->TXMAXP = 0;
        pUsbdRegs->TXFIFOSZ = 0;
        pUsbdRegs->RXMAXP = 0;
        pUsbdRegs->RXFIFOSZ = 0;

        if (modeOut == TRUE) // RX endpoint
        {
            // Max packet and FIFO size

            pUsbdRegs->RXMAXP = epMaxPktSize;
            pUsbdRegs->RXFIFOSZ = fifoSz;

            pepCsrReg = &pUsbdRegs->PERI_RXCSR;

            epRegVal = (*pepCsrReg);
            epRegVal &= ~(MGC_M_RXCSR_AUTOCLEAR | MGC_M_RXCSR_DMAMODE);
            epRegVal |= (MGC_M_RXCSR_CLRDATATOG | MGC_M_RXCSR_FLUSHFIFO);
            if (transferType == USB_ENDPOINT_TYPE_INTERRUPT)
                epRegVal |= MGC_M_RXCSR_DISNYET;
            else
                epRegVal &= ~MGC_M_RXCSR_DISNYET;
            (*pepCsrReg) = epRegVal;

            pUsbdRegs->RXFIFOADDR = (UINT16)(pPdd->ep[endpoint].fifoOffset >> 3);
            pPdd->ep[endpoint].dirRx = TRUE;

            // RX interrupt is enabled only when a transfer is issued by MDD.
            // We do not want to handle the RX interrupt before we have a
            // transfer structure to receive data into.
            intrRegVal = 0;
        }
        else // TX endpoint
        {
            // Max packet and FIFO size

            pUsbdRegs->TXMAXP = epMaxPktSize;
            pUsbdRegs->TXFIFOSZ = fifoSz;

            pepCsrReg = &pUsbdRegs->PERI_TXCSR;
            epRegVal = (*pepCsrReg);
            epRegVal |= (MGC_M_TXCSR_MODE | MGC_M_TXCSR_CLRDATATOG |
                         MGC_M_TXCSR_FLUSHFIFO);

            /* Write into the PERI_TXCSR Register to set it as TX EP */
            (*pepCsrReg) = epRegVal;

            pUsbdRegs->TXFIFOADDR = (UINT16)(pPdd->ep[endpoint].fifoOffset >> 3);
            pPdd->ep[endpoint].dirRx = FALSE;

            // TX interrupt is enabled for non-DMA
#ifndef CPPI_DMA_SUPPORT
            intrRegVal = (1 << endpoint) << USB_OTG_TXINT_SHIFT;
#else
            intrRegVal = 0;
#endif
        }

        /* Set Transfer Type */
        DEBUGCHK(transferType != USB_ENDPOINT_TYPE_CONTROL);
        switch(transferType)
        {
        case USB_ENDPOINT_TYPE_ISOCHRONOUS:
            /* Set the ISO bit */
            epRegVal = (*pepCsrReg);
            epRegVal |= BIT14;
            (*pepCsrReg) = epRegVal;
            break;
        case USB_ENDPOINT_TYPE_BULK:
        case USB_ENDPOINT_TYPE_INTERRUPT:
        default:
            /* Clear ISO bit - Set type to Bulk */
            epRegVal = (*pepCsrReg);
            epRegVal &= ~BIT14;
            (*pepCsrReg) = epRegVal;
        }
        PRINTMSG(ZONE_PDD_INIT, /*TRUE*/
                 (L"EP %u FIFOSZ 0x%x FIFO 0x%x CSR 0x%x\r\n",
                  endpoint, fifoSz, pPdd->ep[endpoint].fifoOffset, epRegVal));

        /* Clear the EndPoint Stall */
        UfnPdd_ClearEndpointStall(pvPddContext, endpoint);
    }
    else
    {
        /* Configure the EP0 Registers first for
         * Core Events Interrupts on the EP0 Interrupt Enable register
         */

        /* use pre-computed values for EP0  */
        pUsbdRegs->TXFIFOSZ = 3;
        pUsbdRegs->RXFIFOSZ = 3;
        pUsbdRegs->TXFIFOADDR = 0;
        pUsbdRegs->RXFIFOADDR = 0;

        /* Update the Global FIFO Usage Count */
        pPdd->ep[endpoint].maxPacketSize = epMaxPktSize;
        /* Let us use the Starting of the FIFO RAM for EP0 */
        pPdd->ep[endpoint].fifoOffset    = 0;

        // Interrupt mask
        intrRegVal  = (1 << endpoint) << USB_OTG_RXINT_SHIFT;
        intrRegVal |= (1 << endpoint) << USB_OTG_TXINT_SHIFT;

        pPdd->fWaitingForHandshake = FALSE;
    }

    pPdd->ep[endpoint].epNumber = (UINT16)endpoint;

#ifdef CPPI_DMA_SUPPORT
    /* Select EP */
    if ((endpoint != 0) &&
        (pPdd->ep[endpoint].pDmaChan == NULL))
    {
        struct dma_controller *pDma =  (struct dma_controller *)pPdd->pDmaCntrl;

        if (pDma != NULL)
        {
            pPdd->ep[endpoint].pDmaChan =  (PVOID)
                pDma->pfnChannelAlloc(pPdd->pDmaCntrl, &pPdd->ep[endpoint],
                                      (UINT8)(modeOut == FALSE),
                                      (modeOut == FALSE) ?
                                      TxDmaTransferComplete : RxDmaTransferComplete);
        }
    }
#endif /* #ifdef  CPPI_DMA_SUPPORT */

    pPdd->ep[endpoint].stalled   = FALSE;

    /* Enable the Tx and Rx interrupts */
    pUsbdRegs->IRQ_EN_SET_0 = intrRegVal;

    pUsbdRegs->INDEX = 0;

    PRINTMSG(ZONE_PDD_INIT, /* TRUE */ (L"INTMSKR 0x%08x %08x\r\n", pUsbdRegs->IRQ_STATUS_0, intrRegVal));

    /* EP is ready */
    pPdd->ep[endpoint].epInitialised = TRUE;

Exit:
    FUNCTION_LEAVE_MSG();

    return rc;
}

//========================================================================
//!  \fn DWORD
//!     UfnPdd_SetAddress(
//!         PVOID pvPddContext,
//!         BYTE  address
//!         )
//!  \brief This function sets the address of the device on the USB bus.
//!         Not all device controllers expose the USB address, because
//!         some device controllers handle the SET_ADDRESS standard request
//!         internally.
//!  \param PVOID pvPddContext - Pointer to the USB Context Struct
//!         BYTE  address - Address to be Assigned
//!  \return DWORD - ERROR_SUCCESS if successful.
//========================================================================
DWORD
WINAPI
UfnPdd_SetAddress(
    PVOID pvPddContext,
    BYTE  address
    )
{
    DWORD rc = ERROR_SUCCESS;
    USBFNPDDCONTEXT *pPdd = (USBFNPDDCONTEXT *)pvPddContext;

    PRINTMSG(ZONE_PDD_INIT, (L"UfnPdd_SetAddress: %d\r\n", address));

    /* Write into the FADDR Register */
    pPdd->devAddress = (address & 0x7F);
    pPdd->pUsbdRegs->FADDR = pPdd->devAddress;

    return (rc);
}

//========================================================================
//!  \fn DWORD
//!         UfnPdd_Start(
//!             VOID *pPddContext
//!             )
//!  \brief This function is called after UfnPdd_RegisterDevice.
//!         It should attach device to USB bus.
//!  \param VOID *pPddContext - Pointer to the USB Context Struct
//!  \return DWORD  - ERROR_SUCCESS if Successful.
//========================================================================
DWORD
WINAPI
UfnPdd_Start(
    VOID *pPddContext
    )
{
    DWORD rc = ERROR_SUCCESS;
    UINT32 usbIrqRegVal = 0;

    USBFNPDDCONTEXT  *pPdd = (USBFNPDDCONTEXT *)pPddContext;

#ifdef CPPI_DMA_SUPPORT
    /* Initialize the DMA Controller Object */
    struct dma_controller *pDmaCntrl = (struct dma_controller *)pPdd->pDmaCntrl;
#endif

    PRINTMSG(ZONE_FUNCTION, (L"UfnPdd_Start:\r\n"));

    /* Create interrupt event */
    pPdd->hIntrEvent = CreateEvent(0, FALSE, FALSE, NULL);
    if ( pPdd->hIntrEvent == NULL )
    {
        ERRORMSG (TRUE, (L"ERROR: Error creating interrupt event\r\n"));
        rc = ERROR_GEN_FAILURE;
        goto Exit;
    }
    
    /* Initialize interrupt */
    if (!InterruptInitialize( pPdd->sysIntr, pPdd->hIntrEvent, NULL, 0))
    {
        ERRORMSG (TRUE, (L"ERROR: Interrupt initialization failed\r\n"));
        rc = ERROR_GEN_FAILURE;
        goto Exit;
    }

    /* Run interrupt thread */
    pPdd->exitIntrThread = FALSE;
    pPdd->hIntrThread = CreateThread (NULL, 0, InterruptThread, pPdd, 0, NULL);
    if ( pPdd->hIntrThread == NULL )
    {
        ERRORMSG (TRUE, (L"ERROR: Interrupt thread creation failed\r\n"));
        rc = ERROR_GEN_FAILURE;
        goto Exit;
    }
    CeSetThreadPriority(pPdd->hIntrThread, pPdd->priority256);

    PRINTMSG(ZONE_PDD_INIT, (L"UfnPdd_Start: USBFnPDD IntrThread 0x%08x\r\n",
                             pPdd->hIntrThread));

    /* Power up */
    if ((pPdd->currentPowerState == D3) || (pPdd->currentPowerState == D4))
    {
        SetPowerState(pPdd, D0);
    }

    /* Now Enable the CORE Interrupts */
    usbIrqRegVal = USB_OTG_USBINT_MASK & ~(CSL_USB_INTRUSB_SOF_MASK << CSL_USB_INTMSKR_USB_SHIFT);
    pPdd->pUsbdRegs->IRQ_EN_SET_1 = usbIrqRegVal;

    PRINTMSG(ZONE_PDD_INIT, (L"UfnPdd_Start: Reenabled all Interrupts \r\n"));
    DumpUsbRegisters(pPdd);

#ifdef CPPI_DMA_SUPPORT
    /* Start the DMA Controller Object */
    if (pDmaCntrl != NULL)
    {
        pDmaCntrl->pfnStart(pPdd->pDmaCntrl);
    }
#endif /* #ifdef CPPI_DMA_SUPPORT */

    /* Release the IST */
    SetEvent(pPdd->hStartEvent);

Exit:
    return (rc);

}

//========================================================================
//!  \fn DWORD
//!   UfnPdd_RegisterDevice(
//!    PVOID                           pvPddContext,
//!    PCUSB_DEVICE_DESCRIPTOR         pHighSpeedDeviceDesc,
//!    PCUFN_CONFIGURATION             pHighSpeedConfig,
//!    PCUSB_CONFIGURATION_DESCRIPTOR  pHighSpeedConfigDesc,
//!    PCUSB_DEVICE_DESCRIPTOR         pFullSpeedDeviceDesc,
//!    PCUFN_CONFIGURATION             pFullSpeedConfig,
//!    PCUSB_CONFIGURATION_DESCRIPTOR  pFullSpeedConfigDesc,
//!    PCUFN_STRING_SET                pStringSets,
//!    DWORD                           cStringSets
//!    )
//!  \brief This function is called by MDD after device configuration
//!         was sucessfully verified by UfnPdd_IsEndpointSupportable and
//!         UfnPdd_IsConfigurationSupportable. It should initialize
//!         hardware for given configuration.
//!  \param PVOID pvPddContext - PDD Context Struct
//!         PCUSB_DEVICE_DESCRIPTOR pHighSpeedDeviceDesc - High Speed
//!                 Device Descriptor
//!         PCUFN_CONFIGURATION pHighSpeedConfig  - Configuration for
//!                 High Speed Device
//!         PCUSB_CONFIGURATION_DESCRIPTOR  pHighSpeedConfigDesc -
//!             Configuration Descriptor for High Speed Device
//!         PCUSB_DEVICE_DESCRIPTOR pFullSpeedDeviceDesc - Device
//!             Descriptor for High Speed Device
//!  \return DWORD - ERROR_SUCCESS if succesful.
//========================================================================
DWORD
WINAPI
UfnPdd_RegisterDevice(
    PVOID                           pvPddContext,
    PCUSB_DEVICE_DESCRIPTOR         pHighSpeedDeviceDesc,
    PCUFN_CONFIGURATION             pHighSpeedConfig,
    PCUSB_CONFIGURATION_DESCRIPTOR  pHighSpeedConfigDesc,
    PCUSB_DEVICE_DESCRIPTOR         pFullSpeedDeviceDesc,
    PCUFN_CONFIGURATION             pFullSpeedConfig,
    PCUSB_CONFIGURATION_DESCRIPTOR  pFullSpeedConfigDesc,
    PCUFN_STRING_SET                pStringSets,
    DWORD                           cStringSets
    )
{
    DWORD dwFullNAltSettings;
    DWORD dwHighNAltSettings;
    DWORD rc = ERROR_INVALID_PARAMETER;

    USBFNPDDCONTEXT *pPdd = (USBFNPDDCONTEXT *)pvPddContext;
    UFN_INTERFACE   *pIFC = NULL;
    UFN_ENDPOINT    *pEP  = NULL;
    DWORD   ifc			  = 0;
    DWORD   epx           = 0;
    UINT16  ep            = 0;
    WORD    offset        = 0;

	UNREFERENCED_PARAMETER(cStringSets);
	UNREFERENCED_PARAMETER(pStringSets);
	UNREFERENCED_PARAMETER(pFullSpeedConfigDesc);
	UNREFERENCED_PARAMETER(pFullSpeedDeviceDesc);
	UNREFERENCED_PARAMETER(pHighSpeedConfigDesc);
	UNREFERENCED_PARAMETER(pHighSpeedDeviceDesc);

    PRINTMSG(ZONE_FUNCTION, (L"+UsbFnPdd_RegisterDevice\r\n"));

    /* Remember self powered flag */
    pPdd->selfPowered = (pFullSpeedConfig->Descriptor.bmAttributes & 0x20) != 0;

    dwFullNAltSettings = GetTotalAltSettingCount(pFullSpeedConfig);
    if( dwFullNAltSettings == INVALID_ALTSETTING_COUNT )
    {
        ERRORMSG(TRUE, (TEXT("UfnPdd_RegisterDevice: Error retrieving full-speed altsetting count")) );
        return ERROR_INVALID_PARAMETER;
    }

    dwHighNAltSettings = GetTotalAltSettingCount(pHighSpeedConfig);
    if( dwHighNAltSettings == INVALID_ALTSETTING_COUNT )
    {
        ERRORMSG(TRUE, (TEXT("UfnPdd_RegisterDevice: Error retrieving full-speed altsetting count")) );
        return ERROR_INVALID_PARAMETER;
    }

    PRINTMSG(ZONE_PDD_INIT,
             (L"FullSpeed NumInterfaces 0x%x, altsettings=(full=%d, high=%d) maxPktSize 0x%x:\r\n",
              pFullSpeedConfig->Descriptor.bNumInterfaces,
              dwFullNAltSettings, dwHighNAltSettings,
              pFullSpeedDeviceDesc->bMaxPacketSize0));


    /* Determine FIFO offsets for each EP */

    /* Clear config */
    for (ep = 0; ep < USBD_EP_COUNT; ++ep)
    {
        pPdd->ep[ep].maxPacketSize = 0;
        pPdd->ep[ep].fifoOffset = 0;
    }

    /* Full speed config first */
    for (ifc = 0; ifc < dwFullNAltSettings; ifc++)
    {
        pIFC = &pFullSpeedConfig->pInterfaces[ifc];

        PRINTMSG(ZONE_PDD_INIT, (L"FS Interface 0x%x EP_COUNT 0x%x:\r\n",
                                 ifc, pIFC->Descriptor.bNumEndpoints));

        for (epx = 0; epx < pIFC->Descriptor.bNumEndpoints; epx++)
        {
            pEP = &pIFC->pEndpoints[epx];

            PRINTMSG(ZONE_PDD_INIT, (L"FS EP_NUM 0x%x EP MaxPktSize 0x%x\r\n",
                                     pEP->Descriptor.bEndpointAddress & 0x0F,
                                     pEP->Descriptor.wMaxPacketSize));

            /* Get EP address */
            ep = pEP->Descriptor.bEndpointAddress & 0x0F;

            /* Save max packet size */
            pPdd->ep[ep].maxPacketSize = pEP->Descriptor.wMaxPacketSize;
        }
    }

    /* High speed config */
    for (ifc = 0; ifc < dwHighNAltSettings; ifc++)
    {
        pIFC = &pHighSpeedConfig->pInterfaces[ifc];

        PRINTMSG(ZONE_PDD_INIT, (L"HS Interface 0x%x EP_COUNT 0x%x:\r\n",
                                 ifc, pIFC->Descriptor.bNumEndpoints));

        for (epx = 0; epx < pIFC->Descriptor.bNumEndpoints; epx++)
        {
            pEP = &pIFC->pEndpoints[epx];

            PRINTMSG(ZONE_PDD_INIT, (L"HS EP_NUM 0x%x EP MaxPktSize 0x%x\r\n",
                                     pEP->Descriptor.bEndpointAddress & 0x0F,
                                     pEP->Descriptor.wMaxPacketSize));

            /* Get EP address */
            ep = pEP->Descriptor.bEndpointAddress & 0x0F;

            /* Save max packet size */
            if (pEP->Descriptor.wMaxPacketSize > pPdd->ep[ep].maxPacketSize)
                pPdd->ep[ep].maxPacketSize = pEP->Descriptor.wMaxPacketSize;
        }
    }

    /* Now determine fifo offsets */

    /* Allow for EP0 fifo */
    offset = 64;

    for (ep = 0; ep < USBD_EP_COUNT; ++ep)
    {
        if (pPdd->ep[ep].maxPacketSize > 0)
        {
            pPdd->ep[ep].fifoOffset = offset;
            offset = offset + pPdd->ep[ep].maxPacketSize;
        }
    }

    /* Clear EP's maxPacketSize */
    for (ep = 0; ep < USBD_EP_COUNT; ++ep)
        pPdd->ep[ep].maxPacketSize = 0;

    rc = ERROR_SUCCESS;

    PRINTMSG(ZONE_FUNCTION, (L"-UfnPdd_RegisterDevice:\r\n"));
    return (rc);
}

//========================================================================
//!  \fn DWORD
//!     UfnPdd_IsEndpointSupportable(
//!         PVOID                       pvPddContext,
//!         DWORD                       endpoint,
//!         UFN_BUS_SPEED               speed,
//!         PUSB_ENDPOINT_DESCRIPTOR    pEndpointDesc,
//!         BYTE                        configurationValue,
//!         BYTE                        interfaceNumber,
//!         BYTE                        alternateSetting
//!         )
//!  \brief This function is called by MDD to verify if EP can be supported
//!         on hardware. It is called after UfnPdd_IsConfigurationSupportable.
//!         We must verify configuration in this function, so we already
//!         know that EPs are valid. Only information we can update there
//!         is maximal packet size for EP0.
//!  \param PVOID  pvPddContext - PDD Context Structure Pointer
//!         DWORD  endpoint - End Point Number
//!         UFN_BUS_SPEED speed - Speed Enumeration Value for this EP
//!         PUSB_ENDPOINT_DESCRIPTOR pEndpointDesc - EP Descriptor
//!         BYTE   configurationValue - Config Value
//!         BYTE   interfaceNumber - Alternate Interface Number
//!         BYTE   alternateSetting - Alternate Setting
//!  \return DWORD  - ERROR_SUCCESS if successful.
//========================================================================
DWORD
WINAPI
UfnPdd_IsEndpointSupportable(
    PVOID                       pvPddContext,
    DWORD                       endpoint,
    UFN_BUS_SPEED               speed,
    PUSB_ENDPOINT_DESCRIPTOR    pEndpointDesc,
    BYTE                        configurationValue,
    BYTE                        interfaceNumber,
    BYTE                        alternateSetting
    )
{
    DWORD rc = ERROR_INVALID_PARAMETER;
    WORD  packetSize;
    BYTE  transferType;

	UNREFERENCED_PARAMETER(alternateSetting);
	UNREFERENCED_PARAMETER(interfaceNumber);
	UNREFERENCED_PARAMETER(configurationValue);
	UNREFERENCED_PARAMETER(speed);
	UNREFERENCED_PARAMETER(pvPddContext);

    PRINTMSG(ZONE_FUNCTION, (L"+UfnPdd_IsEndpointSupportable EP %u\r\n",
                             endpoint));

    /* Update maximal packet size for EP0 */
    if (endpoint == 0)
    {
        PRINTMSG(ZONE_PDD_INIT, (L"EP0 MaxPktSize 0x%x\r\n",
                                 pEndpointDesc->wMaxPacketSize));

        DEBUGCHK(pEndpointDesc->wMaxPacketSize <= 64);
        DEBUGCHK(pEndpointDesc->bmAttributes == USB_ENDPOINT_TYPE_CONTROL);
        pEndpointDesc->wMaxPacketSize = 64;
        rc = ERROR_SUCCESS ;
    }
    else if (endpoint < USBD_EP_COUNT)
    {
        transferType = pEndpointDesc->bmAttributes & USB_ENDPOINT_TYPE_MASK;
        DEBUGCHK(transferType != USB_ENDPOINT_TYPE_CONTROL);

        /* Validate and adjust packet size */
        packetSize =
            (pEndpointDesc->wMaxPacketSize & USB_ENDPOINT_MAX_PACKET_SIZE_MASK);

        switch(transferType)
        {
            /* Isochronous EndPoint  */
        case USB_ENDPOINT_TYPE_ISOCHRONOUS:
            PRINTMSG(ZONE_PDD_INIT, (TEXT("Isochronous endpoint\r\n")));
            rc = ERROR_SUCCESS;
            break;
        case USB_ENDPOINT_TYPE_BULK:
        case USB_ENDPOINT_TYPE_INTERRUPT:
            PRINTMSG(ZONE_PDD_INIT,
                     (TEXT("Bulk or Intr EndPoint PktSize 0x%x\r\n"),
                      packetSize));
            rc = ERROR_SUCCESS ;
            break;
        default:
            rc = ERROR_INVALID_PARAMETER;
            break;
        }

        /*
         * If Requested Size is larger than what is supported ... change it.
         * Note only try and change it if no errors so far... meaning Ep is
         * Supportable.
         */

    }

    return (rc);

}

//========================================================================
//!  \fn DWORD
//!         UfnPdd_IsConfigurationSupportable(
//!         PVOID                       pvPddContext,
//!         UFN_BUS_SPEED               speed,
//!         PUFN_CONFIGURATION          pConfiguration
//!         )
//!  \brief This function is called before UfnPdd_RegisterDevice. It should
//!         verify that USB device configuration can be supported on hardware.
//!         Function can modify EP size and/or EP address.
//
//!  \param PVOID  pvPddContext - PDD Context Structure Pointer
//!         UFN_BUS_SPEED speed - Speed of the USB Bus
//!         PUFN_CONFIGURATION pConfiguration -  USB Configuration
//!  \return DWORD - ERROR_SUCCESS if successful.
//========================================================================
DWORD
WINAPI
UfnPdd_IsConfigurationSupportable(
    PVOID                       pvPddContext,
    UFN_BUS_SPEED               speed,
    PUFN_CONFIGURATION          pConfiguration
    )
{
    DWORD dwNAltSettings = 0;
    DWORD rc             = ERROR_INVALID_PARAMETER;
    UFN_INTERFACE *pIFC  = NULL;
    UFN_ENDPOINT *pEP    = NULL;
    WORD ifc             = 0;
	WORD epx             = 0;
	WORD count           = 0;
    WORD offset          = 0;
	WORD size            = 0;
    WORD maxPktSize      = 0;

	UNREFERENCED_PARAMETER(pvPddContext);

    PRINTMSG(ZONE_FUNCTION, (L"+UfnPdd_IsConfigurationSupportable:\r\n"));

    /* We must start with offset 64 (EP0 size) */
    offset = 64;

    /* Clear number of end points */
    count = 0;

    PRINTMSG(ZONE_PDD_INIT, (L"NumInterfaces 0x%x\r\n",
                             pConfiguration->Descriptor.bNumInterfaces));

    dwNAltSettings = GetTotalAltSettingCount(pConfiguration);
    if( dwNAltSettings == INVALID_ALTSETTING_COUNT )
    {
        ERRORMSG(TRUE, (TEXT("UfnPdd_IsConfigurationSupportable: Error retrieving full-speed altsetting count")) );
        return ERROR_INVALID_PARAMETER;
    }


    /* For each interface in configuration */
    for (ifc = 0; ifc < dwNAltSettings; ifc++)
    {
        /* For each endpoint in interface */
        pIFC = &pConfiguration->pInterfaces[ifc];
        for (epx = 0; epx < pIFC->Descriptor.bNumEndpoints; epx++)
        {
            pEP = &pIFC->pEndpoints[epx];

            /* We support maximal sizes 8, 16, 32 and 64 bytes for non-ISO */
            size = pEP->Descriptor.wMaxPacketSize;

            PRINTMSG(ZONE_PDD_INIT, (L"EP %u Addr 0x%x Attributes 0x%x MaxPktSize 0x%x\r\n",
                                     epx, pEP->Descriptor.bEndpointAddress,
                                     pEP->Descriptor.bmAttributes,
                                     pEP->Descriptor.wMaxPacketSize));
            /* First round size to supported sizes */
            size = (WORD)(1 << Log2(size));

            if (speed == BS_HIGH_SPEED)
            {
                switch (pEP->Descriptor.bmAttributes & 0x03)
                {
                case USB_ENDPOINT_TYPE_CONTROL:     maxPktSize = USB_HIGH_SPEED_CONTROL_MAX_PACKET_SIZE; break;
                case USB_ENDPOINT_TYPE_ISOCHRONOUS: maxPktSize = USB_HIGH_SPEED_ISOCHRONOUS_MAX_PACKET_SIZE; break;
                case USB_ENDPOINT_TYPE_BULK:        maxPktSize = USB_HIGH_SPEED_BULK_MAX_PACKET_SIZE; break;
                case USB_ENDPOINT_TYPE_INTERRUPT:   maxPktSize = USB_HIGH_SPEED_INTERRUPT_MAX_PACKET_SIZE; break;
                }
            }
            else
            {
                switch (pEP->Descriptor.bmAttributes & 0x03)
                {
                case USB_ENDPOINT_TYPE_CONTROL:     maxPktSize = USB_FULL_SPEED_CONTROL_MAX_PACKET_SIZE; break;
                case USB_ENDPOINT_TYPE_ISOCHRONOUS: maxPktSize = USB_FULL_SPEED_ISOCHRONOUS_MAX_PACKET_SIZE; break;
                case USB_ENDPOINT_TYPE_BULK:        maxPktSize = USB_FULL_SPEED_BULK_MAX_PACKET_SIZE; break;
                case USB_ENDPOINT_TYPE_INTERRUPT:   maxPktSize = USB_FULL_SPEED_INTERRUPT_MAX_PACKET_SIZE; break;
                }
            }
            if (size > maxPktSize)
                size = maxPktSize;
            PRINTMSG(ZONE_PDD_INIT, (L"EPAddr 0x%x Size %d\r\n",
                                     pEP->Descriptor.bEndpointAddress, size));

            /* Update EP size */
            pEP->Descriptor.wMaxPacketSize = size;

            /* If registry for DMA end points isn't set try to choose */
            if ((pEP->Descriptor.bEndpointAddress & 0x80) != 0)
            {
                PRINTMSG(ZONE_PDD_INIT, (L"TX EP 0x%02x\r\n",
                                         pEP->Descriptor.bEndpointAddress));
            }
            else
            {
                PRINTMSG(ZONE_PDD_INIT, (L"RX EP 0x%02x\r\n",
                                         pEP->Descriptor.bEndpointAddress));
            }
            /* Calculate total buffer size */
            offset = offset + size;
        }
        /* Add number of end points to total count */
        count = count + pIFC->Descriptor.bNumEndpoints;
    }

    PRINTMSG(ZONE_PDD_INIT, (L"Total EP Count 0x%x Offset 0x%x\r\n",
                             count, offset));

    /* Can we support this configuration? */
    if (count < USBD_EP_COUNT && offset <= MGC_FIFO_RAM_SIZE)
    {
        rc = ERROR_SUCCESS;
    }
/*cleanUp:*/
    return rc;

}

//========================================================================
//!  \fn UfnPdd_Init(
//!         LPCTSTR szActiveKey,
//!         VOID   *pMddContext,
//!         UFN_MDD_INTERFACE_INFO *pMddIfc,
//!         UFN_PDD_INTERFACE_INFO *pPddIfc
//!         )
//!  \brief This function is called by MDD on driver load. It should reset
//!         driver, fill PDD interface structure. It can also get SYSINTR,
//!         initialize interrupt thread and start interrupt thread.
//!         It must not attach device to USB bus.
//
//!  \param  LPCTSTR szActiveKey - Pointer to the Registry Key
//!          VOID   *pMddContext - Pointer to the MDD Context Struct
//!          UFN_MDD_INTERFACE_INFO *pMddIfc - MDD Interface Information
//!          UFN_PDD_INTERFACE_INFO *pPddIfc - PDD Interface Information
//!
//!  \return DWORD - ERROR_SUCCESS on Success and ERROR_INVALID_PARAMETER
//!                  otherwise
//========================================================================
DWORD
WINAPI
UfnPdd_Init(
    LPCTSTR szActiveKey,
    VOID   *pMddContext,
    UFN_MDD_INTERFACE_INFO *pMddIfc,
    UFN_PDD_INTERFACE_INFO *pPddIfc
    )
{
    DWORD rc = ERROR_INVALID_PARAMETER;

    USBFNPDDCONTEXT * pPdd      = NULL;

    PRINTMSG(ZONE_FUNCTION, (L"+UfnPdd_Init\r\n"));

    /* Invoke the UfnPdd_ContextSetup Routine to setup the
     * USBFN PDD Context Struct. Note that if there are any
     * platform specific steps required, they can be
     * moved away into this routine thus keeping the UfnPdd_Init
     * generic across
     */
    if (FALSE == UfnPdd_ContextSetup(szActiveKey, &pPdd))
    {
        /* Failure, Invoke UfnPdd_ContextTeardown */
        UfnPdd_ContextTeardown(pPdd) ;
        goto cleanUp;
    }

#ifdef CPPI_DMA_SUPPORT

    /* Initialize the DMA Controller Object */
    pPdd->pDmaCntrl = cppiControllerInit(pPdd);

#endif /* #ifdef CPPI_DMA_SUPPORT *

    /* Set PDD interface */
    pPddIfc->dwVersion = UFN_PDD_INTERFACE_VERSION;
    pPddIfc->dwCapabilities = UFN_PDD_CAPS_SUPPORTS_FULL_SPEED | UFN_PDD_CAPS_SUPPORTS_ALTERNATE_INTERFACES;
#ifndef UFN_DISABLE_HIGH_SPEED
    pPddIfc->dwCapabilities |= UFN_PDD_CAPS_SUPPORTS_HIGH_SPEED;
#endif
    pPddIfc->dwEndpointCount = USBD_EP_COUNT;
    pPddIfc->pvPddContext = pPdd;
    pPddIfc->pfnDeinit = UfnPdd_Deinit;
    pPddIfc->pfnIsConfigurationSupportable = UfnPdd_IsConfigurationSupportable;
    pPddIfc->pfnIsEndpointSupportable = UfnPdd_IsEndpointSupportable;
    pPddIfc->pfnInitEndpoint = UfnPdd_InitEndpoint;
    pPddIfc->pfnRegisterDevice = UfnPdd_RegisterDevice;
    pPddIfc->pfnDeregisterDevice = UfnPdd_DeregisterDevice;
    pPddIfc->pfnStart = UfnPdd_Start;
    pPddIfc->pfnStop = UfnPdd_Stop;
    pPddIfc->pfnIssueTransfer = UfnPdd_IssueTransfer;
    pPddIfc->pfnAbortTransfer = UfnPdd_AbortTransfer;
    pPddIfc->pfnDeinitEndpoint = UfnPdd_DeinitEndpoint;
    pPddIfc->pfnStallEndpoint = UfnPdd_StallEndpoint;
    pPddIfc->pfnClearEndpointStall = UfnPdd_ClearEndpointStall;
    pPddIfc->pfnSendControlStatusHandshake = UfnPdd_SendControlStatusHandshake;
    pPddIfc->pfnSetAddress = UfnPdd_SetAddress;
    pPddIfc->pfnIsEndpointHalted = UfnPdd_IsEndpointHalted;
    pPddIfc->pfnInitiateRemoteWakeup = UfnPdd_InitiateRemoteWakeup;
    pPddIfc->pfnPowerDown = UfnPdd_PowerDown;
    pPddIfc->pfnPowerUp = UfnPdd_PowerUp;
    pPddIfc->pfnIOControl = UfnPdd_IOControl;

    /* Save MDD context & notify function */
    pPdd->pMddContext = pMddContext;
    pPdd->pfnNotify = pMddIfc->pfnNotify;
    pPdd->attachState = UFN_DETACH;
    pPdd->resetComplete = FALSE;

    /* Default EP0 State */
    pPdd->ep[0].epStage = MGC_END0_STAGE_SETUP;
    pPdd->fWaitingForHandshake = FALSE;

    PRINTMSG(ZONE_FUNCTION, (L"-UfnPdd_Init\r\n"));

    /* Done */
    rc = ERROR_SUCCESS;
cleanUp:
    return rc;
}

BOOL
InitEndpointDmaBuffer (
    USBFNPDDCONTEXT *pPdd,
    DWORD            epNum
    )
{
    BOOL rc = FALSE;
    UsbFnEp *ep = &pPdd->ep[epNum];
    DWORD size = pPdd->dmaBufferSize;

    DMA_ADAPTER_OBJECT Adapter;
    PHYSICAL_ADDRESS pa;

    DEBUGMSG(ZONE_PDD_INIT,
        (L"+InitEndpointDmaBuffer: EP%u - %u bytes\r\n",
        epNum,
        size));

    if (ep->pDmaBuffer != NULL) {
        ERRORMSG(TRUE,
            (L"InitEndpointDmaBuffer: ERROR - DMA buffer already allocated\r\n"));
        goto exit;
    }

    Adapter.ObjectSize = sizeof(DMA_ADAPTER_OBJECT);
    Adapter.InterfaceType = Internal;
    Adapter.BusNumber = 0;

    LOCK_ENDPOINT(pPdd);
    ep->pDmaBuffer  = (UCHAR *)HalAllocateCommonBuffer(&Adapter, size, &pa, FALSE);
    ep->paDmaBuffer = pa.LowPart;
    UNLOCK_ENDPOINT(pPdd);

    if (ep->pDmaBuffer == NULL)
    {
        ERRORMSG(TRUE,
            (L"InitEndpointDmaBuffer: ERROR - Failed to allocate DMA buffer\r\n"));
        goto exit;
    }
    memset(ep->pDmaBuffer, 0, size);
    rc = TRUE;

    DEBUGMSG(ZONE_PDD_INIT,
        (L"InitEndpointDmaBuffer: Allocated DMA buffer - PA 0x%08x VA 0x%08x Size 0x%x\r\n",
        ep->pDmaBuffer,
        ep->paDmaBuffer,
        size));

exit:
    DEBUGMSG(ZONE_PDD_INIT,
        (L"-InitEndpointDmaBuffer: %s\r\n",
        rc ?
            L"SUCCEEDED" :
            L"FAILED"));

    return rc;
}

BOOL
DeinitEndpointDmaBuffer (
    USBFNPDDCONTEXT *pPdd,
    DWORD            epNum
    )
{
    BOOL rc = FALSE;
    UsbFnEp *ep = &pPdd->ep[epNum];
    DWORD size = pPdd->dmaBufferSize;

    DMA_ADAPTER_OBJECT Adapter;
    PHYSICAL_ADDRESS pa;

    DEBUGMSG(ZONE_PDD_INIT,
        (L"+DeinitEndpointDmaBuffer: EP%u - %u bytes\r\n",
        epNum,
        size));

    if (ep->pDmaBuffer == NULL) {
        ERRORMSG(TRUE,
            (L"DeinitEndpointDmaBuffer: ERROR - No DMA buffer\r\n"));
        goto exit;
    }
    LOCK_ENDPOINT(pPdd);
    pa.QuadPart = ep->paDmaBuffer;
    HalFreeCommonBuffer(&Adapter, size, pa, ep->pDmaBuffer, FALSE);
    ep->pDmaBuffer = NULL;
    UNLOCK_ENDPOINT(pPdd);
    rc = TRUE;

exit:
    DEBUGMSG(ZONE_PDD_INIT,
        (L"-DeinitEndpointDmaBuffer: %s\r\n",
        rc ?
            L"SUCCEEDED" :
            L"FAILED"));

    return rc;
}

//========================================================================
//!  \fn GetTotalAltSettingCount(
//!         PCUFN_CONFIGURATION pConfiguration,
//!         )
//!  \brief Retrieves the total number of alternate settings
//!         distributed across all interfaces of the configuration passed in. 
//!
//!  \param  PCUFN_CONFIGURATION pConfiguration - configuration pointer
//!  \return DWORD - on success: count of alternate settings 
//!                  (which is greater than or equal the number of interfaces).
//!                  on failure: on failure:  INVALID_ALTSETTING_COUNT
//========================================================================
DWORD
GetTotalAltSettingCount( 
      PCUFN_CONFIGURATION pConfiguration
      )
{
    DWORD dwConfigSize;
    DWORD nAltSettings;
    DWORD nInterfaces;
    DWORD bPrevInterface;
    PUFN_INTERFACE pAltSetting;
    PUFN_ENDPOINT pEndpoint;
    DWORD epcount;
    DWORD dwConfigSerializedSize = pConfiguration->Descriptor.wTotalLength;

    // NOTE: we're only using the LOWORD of cbExtended to work with
    //       one audio driver that uses the HIWORD to pass parsing hints to the MDD.
    //       This limits the cfgextended to 64k, which is plenty.
    dwConfigSize = pConfiguration->Descriptor.bLength + 
                   LOWORD(pConfiguration->cbExtended);

    if( dwConfigSize >= dwConfigSerializedSize )
    {
        // no room in serialized config for any interfaces or alt settings
        return 0;
    }

    // point to first interface
    nAltSettings = 0;
    nInterfaces = 1; // we know there's at least one, as we handled the empty case above.
    pAltSetting = pConfiguration->pInterfaces;
    bPrevInterface = pAltSetting->Descriptor.bInterfaceNumber;
    while( dwConfigSize < dwConfigSerializedSize )
    {
        // if we the interface changed from the last we saw
        if( pAltSetting->Descriptor.bInterfaceNumber != bPrevInterface )
        {
            ++nInterfaces; // keep interface count

            // check for size inconsistency
            if( nInterfaces > pConfiguration->Descriptor.bNumInterfaces )
            {
                ERRORMSG(TRUE, (TEXT("UFN: GetTotalAltSettingCount:")
                         TEXT("inconsistent size(%d), nInterfaces(%d) bNumInterfaces(%d)"),
                         dwConfigSerializedSize, nInterfaces, 
                         (int)(pConfiguration->Descriptor.bNumInterfaces) ) );
                return INVALID_ALTSETTING_COUNT;
            }
            bPrevInterface = pAltSetting->Descriptor.bInterfaceNumber;
        }

        dwConfigSize += (pAltSetting->Descriptor.bLength + pAltSetting->cbExtended);

        // point to first endpoint
        pEndpoint = pAltSetting->pEndpoints;
        for( epcount = 0; 
            epcount < pAltSetting->Descriptor.bNumEndpoints; 
            ++epcount )
        {
            dwConfigSize += 
               (pEndpoint->Descriptor.bLength + pEndpoint->cbExtended);
            ++pEndpoint;
        }

        ++nAltSettings;
        ++pAltSetting;
    } // while size not matched

    return nAltSettings;
}

//========================================================================
//!  \fn UfnPdd_DllEntry ()
//!  \brief This is the PDD Entry-Point Routine invoked by the USB Function
//!         MDD Module ufnmddbase.lib. The DllEntry routine of the USB
//!         MDD Layer invokes this routine.
//!
//!  \param HANDLE hDllHandle - Handle of the DLL
//!         DWORD reason - Event for which the DLL Entry Routine called
//!         VOID *pReserved - Reserved Pointer
//!
//!  \return BOOL - Returns TRUE or FALSE
//========================================================================
BOOL
WINAPI
UfnPdd_DllEntry(
    HANDLE hDllHandle,
    DWORD reason,
    VOID *pReserved
    )
{
	UNREFERENCED_PARAMETER(pReserved);

    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        DEBUGREGISTER(hDllHandle);
        DisableThreadLibraryCalls((HMODULE)hDllHandle);
        break;

    case DLL_PROCESS_DETACH:
        break;
    }

    return TRUE;
}

#pragma warning(pop)

