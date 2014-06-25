// All rights reserved ADENEO EMBEDDED 2010
//========================================================================
//   Copyright (c) Texas Instruments Incorporated 2010. All Rights Reserved.
//   Copyright (c) MPC Data Limited 2007.  All rights reserved.
//
//   Use of this software is controlled by the terms and conditions found
//   in the license agreement under which this software has been supplied.
//========================================================================

//! \file UsbPeripheral.c
//! \brief AM3517 USB Controller Peripheral Source File. This Source File
//!        contains the routines which directly interact with the
//!        USB Controller.
//!
//! \version  1.00 Sept 19 2006 File Created

#pragma warning(push)
#pragma warning(disable: 4115 4214)
/* WINDOWS CE Public Includes ---------------------------------------------- */
#include <windows.h>
#include <nkintr.h>
#include <ddkreg.h>
#include <usbfn.h>

/* PLATFORM Specific Includes ---------------------------------------------- */
#include "am33x.h"
#include "am33x_usb.h"
#include "am33x_usbcdma.h"
#include "oal_clock.h"
#include "sdk_padcfg.h"
#include "UsbFnPdd.h"
#pragma warning(pop)

#pragma warning(push)
#pragma warning(disable: 4053)

//========================================================================
//!  \fn    ChipCfgLock(
//!           OMAP_SYSC_GENERAL_REGS* pSysConfRegs, BOOL lock
//!         )
//!  \brief Lock or unlock the Chip Cfg MMR registers.
//=======================================================================
#if 0
void ChipCfgLock(OMAP_SYSC_GENERAL_REGS* pSysConfRegs, BOOL lock)
{
	UNREFERENCED_PARAMETER(pSysConfRegs);
	UNREFERENCED_PARAMETER(lock);

    // Not available on AM3517
}
#endif
//========================================================================
//!  \fn UsbPhyPowerCtrl ()
//!  \brief Controls the USB PHY Control Register. Depending on the value
//!         of the second argument, it either enables or disables the
//!         USB PHY.
//!  \param USBFNPDDCONTEXT *pPddContext - PDD context
//!         BOOL         enable - Flag to enable or disable
//!  \return None.
//========================================================================
static
void
UsbPhyPowerCtrl (
    USBFNPDDCONTEXT *pPddContext,
    BOOL           enable
    )
{

//RETAILMSG(1,(L"<><><><> UsbPhyPowerCtrl %08X en-%d\r\n", pPddContext, enable));

#if 0
    OMAP_SYSC_GENERAL_REGS *pSysConfRegs = pPddContext->pSysConfRegs;

    /* start the on-chip PHY and its PLL */
    if (pSysConfRegs != NULL)
    {
        UINT32 nPhyCtl = INREG32(&pSysConfRegs->CONTROL_DEVCONF2);

        if (enable == TRUE)
        {
            // Unlock USBPHY_CTL reg
            ChipCfgLock(pSysConfRegs, FALSE);

			MASKREG32(&pSysConfRegs->CONTROL_DEVCONF2, DEVCONF2_USBOTG_REFFREQ, DEVCONF2_USBOTG_REFFREQ_13MHZ);

            // Take Phy out of reset
            nPhyCtl &= ~(DEVCONF2_USBOTG_PHY_RESET);
            OUTREG32(&pSysConfRegs->CONTROL_DEVCONF2, nPhyCtl);

            // Start on-chip PHY and PLL's
            nPhyCtl |= (DEVCONF2_USBOTG_PHY_PLLON     | 
                        DEVCONF2_USBOTG_SESSENDEN     |
                        DEVCONF2_USBOTG_VBUSDETECTEN  |
						DEVCONF2_USBOTG_REFFREQ_13MHZ |
                        DEVCONF2_USBOTG_DATAPOLARITY  );
            nPhyCtl &= ~(DEVCONF2_USBOTG_POWERDOWNOTG |
						 DEVCONF2_USBPHY_GPIO_MODE	  |
						 DEVCONF2_USBOTG_OTGMODE	  |
                         DEVCONF2_USBOTG_PHY_PD       );

            OUTREG32(&pSysConfRegs->CONTROL_DEVCONF2, nPhyCtl);
            //Sleep(1);

            // wait until ready
            while ((INREG32(&pSysConfRegs->CONTROL_DEVCONF2) & DEVCONF2_USBOTG_PWR_CLKGOOD) != DEVCONF2_USBOTG_PWR_CLKGOOD)
            {
                //Sleep(20);
            }
        }
        else
        {
            // Unlock USBPHY_CTL reg
            ChipCfgLock(pSysConfRegs, FALSE);

            // Only power down the USB2.0 PHY if USB1.1 PHY not in use
            if (!pPddContext->fUSB11Enabled)
            {
                nPhyCtl |= DEVCONF2_USBOTG_PHY_PD;
            }

            // Power down the OTG
            nPhyCtl |= DEVCONF2_USBOTG_POWERDOWNOTG;
			nPhyCtl &= ~DEVCONF2_USBOTG_PHY_PLLON;
            OUTREG32(&pSysConfRegs->CONTROL_DEVCONF2, nPhyCtl);
        }

        // Lock USBPHY_CTL reg
        ChipCfgLock(pSysConfRegs, TRUE);
    }
#endif

}

//========================================================================
//!  \fn void
//!       DumpUsbRegisters (
//!         USBFNPDDCONTEXT *pPdd
//!         )
//!  \brief Routine used to Dump the Interrupt related Registers of
//!         the USB Controller. It dumps only those registers related
//!         to the PDR Block and does not dump the Mentor Graphics Registers
//!  \param USBFNPDDCONTEXT *pPdd - USB Context Struct Pointer
//!  \return None.
//========================================================================
void
DumpUsbRegisters (
    USBFNPDDCONTEXT *pPdd
    )
{
	UNREFERENCED_PARAMETER(pPdd);

    return;
}

//========================================================================
//!  \fn void
//!         HandleRxEndPoint (
//!         USBFNPDDCONTEXT     *pPddContext,
//!         UINT16              endPoint
//!         )
//!  \brief This routine is used to Handle the Interrupts from the
//!         Receive EndPoints. This routine is invoked from the
//!         UsbPddTxRxIntrHandler() Routine for a specific EndPoint.
//!  \param USBFNPDDCONTEXT *pPddContext - PDD Context Struct Pointer
//!         UINT16 endPoint - EndPoint for which Interrupt handling is
//!                           required.
//!  \return DWORD - ERROR_SUCCESS if successful.
//=======================================================================
DWORD
HandleRxEndPoint (
    USBFNPDDCONTEXT     *pPddContext,
    UINT16               endPoint
    )
{
    CSL_UsbRegs *pBase = pPddContext->pUsbdRegs;
    UsbFnEp  *pEP      = &pPddContext->ep[endPoint];
    UINT16 epCsrVal    = 0;
    BOOL reportPkt     = FALSE;
    DWORD rc           = ERROR_SUCCESS;
    USB_DEVICE_REQUEST udr;

    LOCK_ENDPOINT (pPddContext);

    /* Read the EP Status and Count Registers */
    epCsrVal = pBase->EPCSR[ endPoint ].PERI_RXCSR;

    PRINTMSG(FALSE, /*TRUE, */
             (L"EP %u RXSCR 0x%08x\r\n",
              endPoint, epCsrVal));

    /* Check if the Host has sent a Stall on this Ep */
    if ((epCsrVal & MGC_M_RXCSR_P_SENTSTALL) != 0)
    {
        /*
         * Host has sent a STALL on this EP. Clear it and do not
         * overwrite the SENDSTALL BIT.
         */
        epCsrVal &= ~MGC_M_RXCSR_P_SENTSTALL;
        pBase->EPCSR[ endPoint ].PERI_RXCSR = epCsrVal;

        /*
         * Should we inform the MDD about the EndPoint Stall
         * by simulating a SETUP Packet of ENDPOINT CLEAR FEATURE
         */
        udr.bmRequestType = USB_REQUEST_FOR_ENDPOINT;
        udr.bRequest = USB_REQUEST_CLEAR_FEATURE;
        udr.wValue   = USB_FEATURE_ENDPOINT_STALL;
        udr.wIndex   = (BYTE) endPoint;
        udr.wLength  = 0;
        PRINTMSG(ZONE_PDD_RX,
                 (_T("Got IN_SENT_STALL EP %u \r\n"), endPoint));
        reportPkt    = TRUE;
    }
    else if ((epCsrVal & MGC_M_RXCSR_P_OVERRUN) != 0)
    {
        /* Overrun bit set. The Host cannot load data into the
         * OUT FIFO. Clear this Bit
         */
        epCsrVal &= ~MGC_M_RXCSR_P_OVERRUN ;
        pBase->EPCSR[ endPoint ].PERI_RXCSR = epCsrVal;
        PRINTMSG(ZONE_PDD_RX,
                 (L"RxPkt Overrun on EP %u\r\n",
                  endPoint));
    }
    /* Else Check if RxPktRdy is Set */
    else if ((epCsrVal & MGC_M_RXCSR_RXPKTRDY) != 0)
    {
        /* Move into RX Mode by updating the EP Status Struct */
        pEP->epStage = MGC_END0_STAGE_RX;
    }
    UNLOCK_ENDPOINT(pPddContext);

    if (!pEP->usingDma)
        ContinueRxTransfer (pPddContext, endPoint) ;

    /* Final Check: If the Host has requested for a Stall on any endPoint,
     * we will be informing to the Host through constructing a Clear
     * EndPoint feature SETUP Pkt. This needs to be notified to MDD
     */
    if (reportPkt == TRUE)
    {
        pPddContext->pfnNotify(pPddContext->pMddContext, UFN_MSG_SETUP_PACKET,
                               (DWORD) &udr);
    }
    return (rc);

}

//========================================================================
//!  \fn void
//!         HandleTxEndPoint (
//!         USBFNPDDCONTEXT     *pPddContext,
//!         UINT16              endPoint
//!         )
//!  \brief This routine is used to Handle the Interrupts from the
//!         Transmit EndPoints. This routine is invoked from the
//!         UsbPddTxRxIntrHandler() Routine for a specific EndPoint.
//!  \param USBFNPDDCONTEXT *pPddContext - PDD Context Struct Pointer
//!         UINT16 endPoint - EndPoint for which Interrupt handling is
//!                           required.
//!  \return DWORD - ERROR_SUCCESS if successful.
//=======================================================================
DWORD
HandleTxEndPoint (
    USBFNPDDCONTEXT     *pPddContext,
    UINT16               endPoint
    )
{
    CSL_UsbRegs  *pBase = pPddContext->pUsbdRegs;
    UsbFnEp  *pEP        = &pPddContext->ep[endPoint];
    USB_DEVICE_REQUEST udr;
    UINT16 epCsrVal;
    BOOL reportPkt = FALSE;
    DWORD rc = ERROR_SUCCESS;

    LOCK_ENDPOINT (pPddContext);

    /* Read the EP Status and Count Registers */
    epCsrVal = pBase->EPCSR[ endPoint ].PERI_TXCSR;

    PRINTMSG(FALSE, /*ZONE_SEND ZONE_FUNCTION, TRUE,  */
             (L"EP %u: TXCSR 0x%08x\r\n",
              endPoint, epCsrVal));

    /* Check if we have received a Stall on this EP */
    if ((epCsrVal & MGC_M_TXCSR_P_SENTSTALL) != 0)
    {
        /* Host has sent a Stall to the Tx EP.Clear it */
        epCsrVal &= ~MGC_M_TXCSR_P_SENTSTALL;
        pBase->EPCSR[ endPoint ].PERI_TXCSR = epCsrVal;

        /* Should we inform the MDD about the EndPoint Stall
         * by simulating a SETUP Packet of ENDPOINT CLEAR FEATURE
         */
        udr.bmRequestType = USB_REQUEST_FOR_ENDPOINT;
        udr.bRequest = USB_REQUEST_CLEAR_FEATURE;
        udr.wValue = USB_FEATURE_ENDPOINT_STALL;
        udr.wIndex = USB_ENDPOINT_DIRECTION_MASK | (BYTE) endPoint;
        udr.wLength = 0;

        PRINTMSG(ZONE_PDD_TX,
                 (_T("Got OUT_SENT_STALL EP %u \r\n"), endPoint));
        reportPkt = TRUE;
    }

    if ((epCsrVal & MGC_M_TXCSR_P_UNDERRUN) != 0)
    {
        /* Received next IN token already - just clear the bit */
        epCsrVal &= ~( MGC_M_TXCSR_P_UNDERRUN);
        pBase->EPCSR[ endPoint ].PERI_TXCSR = epCsrVal;
    }

    if ((epCsrVal & MGC_M_TXCSR1_FIFONOTEMPTY) != 0)
    {
        epCsrVal &= ~( MGC_M_TXCSR1_FIFONOTEMPTY);
        pBase->EPCSR[ endPoint ].PERI_TXCSR = epCsrVal;
    }

    UNLOCK_ENDPOINT(pPddContext);

    if (pEP->epStage == MGC_END0_STAGE_TX || pEP->epStage == MGC_END0_STAGE_ACKWAIT)
    {
        // If using DMA we were waiting for the packet to go out of FIFO onto the bus
        if (pEP->usingDma)
            TxDmaFifoComplete(pPddContext, endPoint) ;
        else
            ContinueTxTransfer (pPddContext, endPoint) ;
    }

    /* Final Check: If the Host has requested for a Stall on any endPoint,
     * we will be informing to the Host through constructing a Clear
     * EndPoint feature SETUP Pkt. This needs to be notified to MDD
     */
    if (reportPkt == TRUE)
    {
        pPddContext->pfnNotify(pPddContext->pMddContext, UFN_MSG_SETUP_PACKET,
                               (DWORD) &udr);
    }

    return (rc);
}

//========================================================================
//!  \fn void
//!         UsbPddEp0IntrHandler (
//!         USBFNPDDCONTEXT     *pPddContext
//!         )
//!  \brief This routine is used to Handle the Zero data Length SETUP
//!         packets received from the Host.
//!  \param USBFNPDDCONTEXT *pPddContext - PDD Context Struct Pointer
//!         PUSB_DEVICE_REQUEST pControlRequest - USB SETUP Request Pkt
//!  \return None
//=======================================================================

void
UsbPddEp0IntrHandler (
    USBFNPDDCONTEXT *pPddContext
    )
{
    USBFNPDDCONTEXT *pPdd = (USBFNPDDCONTEXT *)pPddContext;
    CSL_UsbRegs *pBase = pPdd->pUsbdRegs;
    UsbFnEp     *pEP = NULL;
    volatile UCHAR *pFifoReg = NULL;
    UINT8       epNum = 0;
    UINT16      ep0Csr;
    UINT16      ep0Count;
    BOOL        zeroLenSetup = FALSE;
    BOOL        reportPkt = FALSE;
    STransfer   *pTransfer;

    LOCK_ENDPOINT(pPdd);

    pEP = &pPdd->ep[epNum];
    pTransfer = pEP->pTransfer;
    pFifoReg = (UCHAR *)(&pBase->FIFO[ epNum ]);

    ep0Csr = pBase->EPCSR[ epNum ].PERI_TXCSR;
    ep0Count = pBase->EPCSR[ epNum ].RXCOUNT;

    PRINTMSG(ZONE_PDD_EP0, (L"UsbPddEp0IntrHandler: CSR 0x%08x, COUNT %u\r\n",
                           ep0Csr, ep0Count));

    /* Check if controller has sent a STALL condition to the host */
    if ((ep0Csr & MGC_M_CSR0_P_SENTSTALL) != 0)
    {
        PRINTMSG(ZONE_PDD_EP0, (L"UsbPddEp0IntrHandler: SENTSTALL indicated\r\n"));
        ep0Csr &= ~MGC_EP0_STALL_BITS;
        pBase->EPCSR[ epNum ].PERI_TXCSR = ep0Csr;
        Sleep(1);
        pEP->epStage = MGC_END0_START;
        pPdd->fWaitingForHandshake = FALSE;
        goto done;
    }

    /* Received a SETUPEND from host? */
    if ((ep0Csr & MGC_M_CSR0_P_SETUPEND) != 0)
    {
        PRINTMSG(ZONE_PDD_EP0, (L"UsbPddEp0IntrHandler: SETUPEND received\r\n"));

        /* Acknowledge the SETUPEND Request From host  */
        ep0Csr &= ~MGC_M_CSR0_P_SETUPEND;
        pBase->EPCSR[ epNum ].PERI_TXCSR = MGC_M_CSR0_P_SVDSETUPEND;
        Sleep(1);

        /* Now Tell MDD also that the current Transaction is cancelled */
        pEP->epStage = MGC_END0_START;

        if (pPdd->fHasQueuedSetupRequest)
        {
            PRINTMSG(ZONE_PDD_EP0, (L"UsbPddEp0IntrHandler: SETUPEND - Cancelling queued setup pkt\r\n"));
            pPdd->fHasQueuedSetupRequest = FALSE;
        }
        else if (pTransfer)
        {
            PRINTMSG(ZONE_PDD_EP0, (L"UsbPddEp0IntrHandler: SETUPEND - Cancelling transfer\r\n"));
            pTransfer->dwUsbError = UFN_CANCELED_ERROR;
            pEP->pTransfer = NULL;
            UNLOCK_ENDPOINT(pPdd);
            pPdd->pfnNotify(pPdd->pMddContext, UFN_MSG_TRANSFER_COMPLETE, (DWORD)pTransfer);
            LOCK_ENDPOINT(pPdd);
            pPdd->fWaitingForHandshake = FALSE;
        }

        /* Received new setup? */
        ep0Csr = pBase->EPCSR[ epNum ].PERI_TXCSR;
        ep0Count = pBase->EPCSR[ epNum ].RXCOUNT;
        if (ep0Csr == 0)
            goto done;
    }

    /* Handle Zero Packet case and mid-transfer states.
     * Zero packet case: This can happen when the Host has sent a Zero
     * Length packet Interrupt on the EP0. This is typically used when:
     * 1. The Host has received the previous data correctly and has
     * acknowledged the same.
     * 2. There is a huge amount of transfer to be done and we are
     * responding back to the Host in chunks. After the transfer of
     * each chunk, the host will send an interrupt with zero packet
     * length. And the RXPKTRDY or TXPKTRDY will not be set.
     */
    if (((ep0Csr == 0x00) && (ep0Count == 0x00)) ||
        pEP->epStage == MGC_END0_STAGE_STATUSIN ||
        pEP->epStage == MGC_END0_STAGE_STATUSOUT ||
        pEP->epStage == MGC_END0_STAGE_TX ||
        pEP->epStage == MGC_END0_STAGE_RX ||
        pEP->epStage == MGC_END0_STAGE_ACKWAIT)
    {
        switch (pEP->epStage)
        {
        case MGC_END0_STAGE_TX:
        case MGC_END0_STAGE_STATUSOUT:
            /* Continue with the Previous Transfer */
            ContinueEp0TxTransfer(pPdd, epNum);
            break;
        case MGC_END0_STAGE_RX:
        case MGC_END0_STAGE_STATUSIN:
            /* Continue with the Previous Transfer */
            ContinueEp0RxTransfer(pPdd, epNum) ;
            break;
        case MGC_END0_STAGE_ACKWAIT:
            /* Can now report zero data setup */
            pEP->epStage = MGC_END0_START;
            UNLOCK_ENDPOINT(pPdd);
            pPdd->pfnNotify(pPdd->pMddContext, UFN_MSG_SETUP_PACKET, (DWORD)&pPdd->setupRequest);
            LOCK_ENDPOINT(pPdd);
            break;
        default:
            PRINTMSG(ZONE_PDD_EP0, (L"UsbPddEp0IntrHandler: ZLP on EP0 stage %d\r\n", pEP->epStage));
            break;
        }
        /* If still in a mid-transfer state then we are done */
        if (pEP->epStage != MGC_END0_START)
            goto done;

        /* Received new setup? */
        ep0Csr = pBase->EPCSR[ epNum ].PERI_TXCSR;
        ep0Count = pBase->EPCSR[ epNum ].RXCOUNT;
        if (ep0Csr == 0)
            goto done;
    }

    DEBUGCHK(pEP->epStage == MGC_END0_START);

    /* Received new setup packet? */
    if ((ep0Csr & MGC_M_CSR0_RXPKTRDY) != 0)
    {
        if (ep0Count != 8)
        {
            PRINTMSG(ZONE_ERROR, (L"UsbPddEp0IntrHandler: Invalid SETUP pkt len %d\r\n", ep0Count));
        }
        else if (pPdd->fWaitingForHandshake)
        {
            /* The RNDIS client driver (and probably others) must not be sent a new
             * SETUP packet until it calls UfnPdd_SendControlStatusHandshake() for the
             * previous setup.  Queue a new setup pkt until this happens.
             */
            if (pPdd->fHasQueuedSetupRequest)
            {
                PRINTMSG(ZONE_ERROR, (L"UsbPddEp0IntrHandler: Received setup pkt before previous setup completed!"));
                DEBUGCHK(0);
                goto done;
            }

            SetupUsbRequest(pPdd, &pPdd->queuedSetupRequest, epNum, ep0Count);
            pPdd->fHasQueuedSetupRequest = TRUE;
            PRINTMSG(ZONE_PDD_EP0, (L"UsbPddEp0IntrHandler: Queuing setup pkt, len %d",
                                    pPdd->queuedSetupRequest.wLength));

            /* Complete the setup transfer */
            if (pPdd->queuedSetupRequest.wLength == 0)
            {
                zeroLenSetup = TRUE;
            }
        }
        else
        {
            SetupUsbRequest(pPdd, &pPdd->setupRequest, epNum, ep0Count);

            PRINTMSG(ZONE_PDD_EP0, (L"UsbPddEp0IntrHandler received setup pkt, len %d",
                                    pPdd->setupRequest.wLength));
            pPdd->fWaitingForHandshake = TRUE;

            if (pPdd->setupRequest.wLength == 0)
            {
                /* Zero Data Length SETUP Pkt */
                zeroLenSetup = TRUE;
                pEP->epStage = MGC_END0_STAGE_ACKWAIT;
            }
            else if ((pPdd->setupRequest.bmRequestType & USB_ENDPOINT_DIRECTION_MASK) != 0)
            {
                pEP->epStage = MGC_END0_STAGE_TX;
                reportPkt = TRUE;
            }
            else
            {
                pEP->epStage = MGC_END0_STAGE_RX;
                reportPkt = TRUE;
            }
        }

        /* Complete the setup transfer */
        if (zeroLenSetup)
        {
            /* Zero Data Length SETUP Pkt - clear PKTRDY and set DATAEND */
            pBase->EPCSR[ epNum ].PERI_TXCSR = MGC_M_CSR0_P_SVDRXPKTRDY | MGC_M_CSR0_P_DATAEND;
        }
        else
        {
            /* Clear PKTRDY */
            pBase->EPCSR[ epNum ].PERI_TXCSR = MGC_M_CSR0_P_SVDRXPKTRDY;
        }
    }

done:

    UNLOCK_ENDPOINT(pPdd);

    /* Report setup pkt to MDD */
    if (reportPkt == TRUE)
    {
        pPdd->pfnNotify(pPdd->pMddContext, UFN_MSG_SETUP_PACKET, (DWORD)&pPdd->setupRequest);
    }

    return;
}

//========================================================================
//!  \fn UsbPddTxRxIntrHandler ()
//!  \brief Interrupt Service Routine to record USB EndPoint interrupts.
//!         This routine is invoked from the MAIN ISR Handler Thread.
//!         AM35x does not support multiple Interrupt Sources for the
//!         USB Interrupt. Hence if the Common USB Interrupt Register
//!         is set, then this routine is invoked from the InterruptThread.
//!         There is an ORDER to perform the tests check p35 of
//!         the MUSBHDRC manual.
//!
//!  \param USBFNPDDCONTEXT *pPddContext - PDD Context Struct Pointer
//!         UINT16           intrTxReg   - EP TX Interrupt Status Reg
//!         UINT16           intrRxReg   - EP RX Interrupt Status Reg
//!  \return None.
//=======================================================================
void
UsbPddTxRxIntrHandler(
    USBFNPDDCONTEXT *pPddContext,
    UINT16           intrTxReg,
    UINT16           intrRxReg
    )
{
    USBFNPDDCONTEXT *pPdd = (USBFNPDDCONTEXT *)pPddContext;

    PRINTMSG(ZONE_FUNCTION, (L"+UsbPddTxRxIntrHandler TxIntr 0x%04x RxIntr 0x%04x\r\n",
                             intrTxReg, intrRxReg));

    /* Handle tx/rx on endpoints; each bit of intrTxReg is an endpoint,
     * endpoint 0 first (p35 of the manual) bc is "SPECIAL" treatment;
     * WARNING: when operating as device you might start receving traffic
     * to ep0 before anything else happens so be ready for it
     */
	{
        UINT  bShift = 0;
        UINT32 reg;

        /* RX on endpoints */
        reg = intrRxReg;
        bShift = 1;
        reg >>= 1;
        while (reg != 0)
        {
            if (reg & 1)
            {
                /* Working in DeviceMode, Invoke Rx EP Interrupt Handler */
                PRINTMSG(FALSE, /* ZONE_SEND TRUE */
                         (L"EP %u RX Handling IntrRxReg 0x%04x\r\n", bShift, intrRxReg));
                HandleRxEndPoint(pPdd, (UINT16)bShift);
            }

            reg >>= 1;
            bShift++;
        }
        /* TX on endpoints */
        reg = intrTxReg;
        bShift = 1;
        reg >>= 1;
        while (reg != 0)
        {
            if (reg & 1)
            {
                /* Working in DeviceMode, Invoke EP Interrupt Handler */
                PRINTMSG(FALSE, /*ZONE_SEND TRUE, */
                         (L"EP %u TX Handling IntrTxReg 0x%04x\r\n", bShift, intrTxReg));
                HandleTxEndPoint(pPdd, (UINT16)bShift);
            }
            reg >>= 1;
            bShift++;
        }
	}

    return;
}

//========================================================================
//!  \fn HandleUsbCoreInterrupt ()
//!  \brief Interrupt Service Routine to record USB "global" interrupts.
//!         This routine is invoked from the MAIN ISR Handler Thread.
//!         AM35x does not support multiple Interrupt Sources for the
//!         USB Interrupt. Hence if the Common USB Interrupt Register
//!         is set, then this routine is invoked from the InterruptThread.
//!         There is an ORDER to perform the tests check p35 of
//!         the MUSBHDRC manual.
//!
//!  \param USBFNPDDCONTEXT *pPddContext - PDD Context Struct Pointer
//!         UINT16           intrUsb     - Value of the Common USB
//!                                        Interrupt Status Reg
//!  \return None.
//=======================================================================
void
HandleUsbCoreInterrupt(
    USBFNPDDCONTEXT *pPddContext,
    UINT16           intrUsb
    )
{
    USBFNPDDCONTEXT *pPdd = (USBFNPDDCONTEXT *)pPddContext;

    PRINTMSG(ZONE_FUNCTION, (L"+UsbPdd_IntrStage0 INTRUSB 0x%04x\r\n",
                             intrUsb));

    if (intrUsb & (1 << 8))
    {
        /* once basic ID sensing behaves, this
         * will have work to do
         */
        DumpUsbRegisters (pPdd);
        PRINTMSG(ZONE_PDD_INIT, (L"InterruptThread: DRVVBUS 0x%04x\r\n",
                                ((CSL_UsbRegs *)pPdd->pUsbdRegs)->STATR));
    }
    else
    {
        /* As a Device, very little processing for RESUME, however
         * as a Host, we need to take up further actions
         */
        if (intrUsb & MGC_M_INTR_RESUME)
        {
            PRINTMSG(TRUE, /*ZONE_PDD_INIT, */ (L"+RESUME\r\n"));
        }
        /* p35 MUSBHDRC manual for the order of the tests */
        if (intrUsb & MGC_M_INTR_SESSREQ)
        {
            PRINTMSG(ZONE_PDD_INIT, (L"+SESSION_REQUEST (HOST_MODE)\r\n"));
        }
        /* VBUSError is bad, shutdown &  go to error mode and ignore
         * the other interrups; p35 MUSBHDRC manual for the order
         of the tests */
        if (intrUsb & MGC_M_INTR_VBUSERROR)
        {
            PRINTMSG(ZONE_PDD_INIT, (L"V_BUS ERROR??? stopping host\r\n"));
        }
        /* connect is valid only when in host mode; ignore it if in device mode;
           p35 MUSBHDRC manual for the order of the tests */
        if (intrUsb & MGC_M_INTR_CONNECT)
        {
            PRINTMSG(ZONE_PDD_INIT, (L"RECEIVED CONNECT (goto host mode)\r\n"));

        }
        if (intrUsb & MGC_M_INTR_RESET)
        {
            PRINTMSG(ZONE_PDD_INIT, /*TRUE, */ (L"BUS RESET\r\n"));
            /*PRINTMSG(TRUE, (L"+RESUME\r\n"));    */
        }
        /* As per the Mentor Graphics Documentation, USB Core
         * Interrupts like SOF, Disconnect and SUSPEND needs to
         * be addressed only after EndPoint Interrupt Processing
         */
    }
    return ;
}

//========================================================================
//!  \fn BOOL
//!         USBPeripheralInit (
//!         USBFNPDDCONTEXT *pPddContext
//!         )
//!  \brief USB Peripheral Controller Initialization. This routine is
//!         invoked during the regular start-up sequence. This routine
//!         allocates the Memory for the USB Controller and the Mentor
//!         Graphics Registers in Sofwtare. It clears all the CPPI
//!         registers and also disables all the interrupts.
//!  \param USBFNPDDCONTEXT *pPddContext - PDD Context Structure Pointer
//!  \return TRUE or FALSE.
//=======================================================================
BOOL
USBPeripheralInit (
    USBFNPDDCONTEXT *pPddContext
    )
{
    CSL_UsbRegs *pUsbRegs                = pPddContext->pUsbdRegs;
	//OMAP_SYSC_GENERAL_REGS *pSysConfRegs = pPddContext->pSysConfRegs;

    PRINTMSG(ZONE_FUNCTION, (L"+USBPeripheralInit\r\n"));

    // Determine if USB 1.1 is in use
	pPddContext->fUSB11Enabled = TRUE; // TODO : find out the corresponding register for AM3517

    /* VSbus off */
    USBFNPDD_PowerVBUS(FALSE);

    // Reset USB controller
    pUsbRegs->CTRLR |= BIT0;
    while((pUsbRegs->CTRLR & BIT0) != 0);

    // Start the on-chip PHY and its PLL
    UsbPhyPowerCtrl(pPddContext, TRUE);

    // Stop the on-chip PHY and its PLL
    UsbPhyPowerCtrl(pPddContext, FALSE);

#ifndef UFN_DISABLE_HIGH_SPEED
    /* Enable high speed */
    pUsbRegs->POWER = MGC_M_POWER_HSENAB;
#endif

    // If RNDIS Mode is required, then CTRL BIT 4 needs to be set
#ifdef USB_RNDIS_MODE
    pUsbRegs->CTRLR |= BIT4;
#endif

    /* start the on-chip PHY and its PLL */
    UsbPhyPowerCtrl(pPddContext, TRUE);

    /* Disable All Interrupts
     * NOTE: The USB Controller Definition of MASK Set and Clear
     * is different from the conventional meaning. Here MASK SET means
     * that the Interrupt is enabled and MASK Clear means Interrupts
     * are disabled. Hence do not write into the INTMSKSETR
     * Register of the controller during startup initialization
     */

	pUsbRegs->IRQ_EN_CLR_1 = USB_CORE_INTR_MASK_ALL ;
    pUsbRegs->IRQ_EN_CLR_0 = USB_EP_INTR_MASK_ALL ;
    pUsbRegs->EOIR = 0x00;

    /* Configure the Controller UINT BIT First */
    pUsbRegs->CTRLR &= ~BIT3;

    /* INTRUSBE Register Initialization for USB Core Interrupts */
    OUTREG8(&pUsbRegs->INTRUSBE, 0xf7);
    OUTREG8(&pUsbRegs->TESTMODE, 0);

    /* Clear BIT0 For Operating in Peripheral Mode */
    //pUsbRegs->DEVCTL = 0;

    /* Initial Function Address set to zero */
    OUTREG8(&pUsbRegs->FADDR, 0);

    /* SOFTCONN set in USBPeripheralStart() */

    PRINTMSG (ZONE_FUNCTION, (_T("Common USB IntrEnable 0x%02x OTG DEVCTL 0x%02x\r\n"),
                      pUsbRegs->INTRUSBE,
                      pUsbRegs->DEVCTL));

    DumpUsbRegisters(pPddContext);

    PRINTMSG(ZONE_FUNCTION, (L"-USBPeripheralInit\r\n"));

    return (TRUE) ;
}

//========================================================================
//!  \fn BOOL
//!         USBPeripheralStart (
//!         USBFNPDDCONTEXT *pPddContext
//!         )
//!  \brief Start the USB Peripheral Controller processing packets.
//!  \param USBFNPDDCONTEXT *pPddContext - PDD Context Structure Pointer
//!  \return TRUE or FALSE.
//=======================================================================
BOOL
USBPeripheralStart (
    USBFNPDDCONTEXT *pPddContext
    )
{
    CSL_UsbRegs *pUsbRegs = pPddContext->pUsbdRegs;

    PRINTMSG(ZONE_FUNCTION, (L"+USBPeripheralStart\r\n"));

    /* enable and start session */
#ifdef UFN_DISABLE_HIGH_SPEED
    pUsbRegs->POWER = MGC_M_POWER_SOFTCONN;
#else
    pUsbRegs->POWER = MGC_M_POWER_SOFTCONN | MGC_M_POWER_HSENAB;
#endif

    PRINTMSG(ZONE_FUNCTION, (L"-USBPeripheralStart\r\n"));

    return (TRUE) ;
}

//========================================================================
//!  \fn BOOL
//!         USBPeripheralDeinit (
//!         USBFNPDDCONTEXT *pPddContext
//!         )
//!  \brief USB Peripheral Controller Shutdown Routine. This routine is
//!         invoked during the regular shutdown sequence. This routine
//!         resets the USB Controller, Disables the USBPHYCTL Oscillator
//!         bit settings.
//!  \param USBFNPDDCONTEXT *pPddContext - PDD Context Structure Pointer
//!  \return TRUE or FALSE.
//=======================================================================
BOOL
USBPeripheralDeinit (
    USBFNPDDCONTEXT *pPddContext
    )
{
    CSL_UsbRegs  *pUsbRegs = pPddContext->pUsbdRegs;

    PRINTMSG(ZONE_FUNCTION, (L"+USBPeripheralDeinit\r\n"));

    /* End the session */
#ifdef UFN_DISABLE_HIGH_SPEED
    pUsbRegs->POWER = 0;
#else
    pUsbRegs->POWER = MGC_M_POWER_HSENAB;
#endif

    // For Peripheral mode of Operation, Switch OFF the VBUS
    USBFNPDD_PowerVBUS(FALSE);

    // Disable the on-chip PHY and its PLL
    UsbPhyPowerCtrl(pPddContext, FALSE);

	// Request pads
//	ReleaseDevicePads(OMAP_DEVICE_HSOTGUSB);

    PRINTMSG(ZONE_FUNCTION, (L"-USBPeripheralDeinit\r\n"));

    return (TRUE) ;
}

//========================================================================
//!  \fn BOOL
//!         USBPeripheralPowerDown (
//!         USBFNPDDCONTEXT *pPddContext
//!         )
//!  \brief Power down the USB Controller and PHY.
//!  \param USBFNPDDCONTEXT *pPddContext - PDD Context Structure Pointer
//!  \return TRUE or FALSE.
//=======================================================================
BOOL
USBPeripheralPowerDown (
    USBFNPDDCONTEXT *pPddContext
    )
{
    PRINTMSG(ZONE_FUNCTION, (L"+USBPeripheralPowerDown\r\n"));

    /* Disable the on-chip PHY and its PLL */
    UsbPhyPowerCtrl(pPddContext, FALSE);

	PRINTMSG(ZONE_FUNCTION, (L"-USBPeripheralPowerDown\r\n"));

    return (TRUE) ;
}

//========================================================================
//!  \fn BOOL
//!         USBPeripheralEndSession (
//!         USBFNPDDCONTEXT *pPddContext
//!         )
//!  \brief Set OTG controller to end current session
//!  \param USBFNPDDCONTEXT *pPddContext - PDD Context Structure Pointer
//!  \return TRUE or FALSE.
//=======================================================================
BOOL
USBPeripheralEndSession (
    USBFNPDDCONTEXT *pPddContext
    )
{
    CSL_UsbRegs  *pUsbRegs = pPddContext->pUsbdRegs;

    PRINTMSG(ZONE_FUNCTION, (L"+USBPeripheralPowerDown\r\n"));

    /* End the session */
#ifdef UFN_DISABLE_HIGH_SPEED
    pUsbRegs->POWER = 0;
#else
    pUsbRegs->POWER = MGC_M_POWER_HSENAB;
#endif

    /* Clear session bit */
    pUsbRegs->DEVCTL &= ~0x01;

    return (TRUE) ;
}

//========================================================================
//!  \fn BOOL
//!         USBPeripheralPowerUp (
//!         USBFNPDDCONTEXT *pPddContext
//!         )
//!  \brief Power down the USB Controller and PHY.
//!  \param USBFNPDDCONTEXT *pPddContext - PDD Context Structure Pointer
//!  \return TRUE or FALSE.
//=======================================================================
BOOL
USBPeripheralPowerUp (
    USBFNPDDCONTEXT *pPddContext
    )
{
    CSL_UsbRegs  *pUsbRegs = pPddContext->pUsbdRegs;

    PRINTMSG(ZONE_FUNCTION, (L"+USBPeripheralPowerUp\r\n"));
	RETAILMSG(TRUE, (L"+USBPeripheralPowerUp\r\n"));

    /* Start the on-chip PHY and its PLL */
    UsbPhyPowerCtrl(pPddContext, TRUE);

#ifndef UFN_DISABLE_HIGH_SPEED
    /* Enable high speed */
    pUsbRegs->POWER = MGC_M_POWER_HSENAB;
#endif

    /* Configure the Controller UINT BIT First */
    pUsbRegs->CTRLR &= ~BIT3;

    pUsbRegs->TESTMODE = 0;

    /* Initial Function Address set to zero */
    pUsbRegs->FADDR = 0;
    
    /* Set session bit */
    pUsbRegs->DEVCTL |= 0x01;

    PRINTMSG(ZONE_FUNCTION, (L"-USBPeripheralPowerUp\r\n"));

    return (TRUE) ;
}

#pragma warning(pop)
