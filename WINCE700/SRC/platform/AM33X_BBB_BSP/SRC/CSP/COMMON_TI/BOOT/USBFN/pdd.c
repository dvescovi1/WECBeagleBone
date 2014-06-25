// All rights reserved ADENEO EMBEDDED 2010
//------------------------------------------------------------------------------
//          Copyright (c) 2009, TI. All rights reserved.
//
// Use of this code is subject to the terms of the Software License
// Agreement (SLA) under which you licensed this software product.
// 
// If you did not accept the terms of the SLA, you are not authorized to
// use this code.
//
// This code and information is provided as is without warranty of any
// kind, either expressed or implied, including but not limited to the
// implied warranties of merchantability and/or fitness for a particular
// purpose.
//------------------------------------------------------------------------------
//
//  File:  pdd.c
//
//  This file contains USB function PDD implementation. Actual implementation
//  doesn't use DMA transfers and it doesn't support ISO endpoints.
//
#pragma warning (push)
#pragma warning (disable: 4115 4201 4214)
#include <windows.h>
#include <oal.h>
#include <oalex.h>
#undef ZONE_ERROR
#undef ZONE_INIT
#undef ZONE_WARNING
#undef ZONE_SEND
#include <usbfntypes.h>
#include <usbfn.h>

#include "omap_musbcore.h"
#include "soc_cfg.h"
#pragma warning (pop)
	
#pragma optimize ( "g", on )
#pragma warning  (push)
#pragma warning  (disable: 4213)

//------------------------------------------------------------------------------

#ifdef DEBUG
DBGPARAM dpCurSettings;
#endif

// PDD internal devState flags    
#define MUSB_DEVSTAT_B_HNP_ENABLE   (1 << 9)        // Host Enable HNP
#define MUSB_DEVSTAT_A_HNP_SUPPORT  (1 << 8)        // Host Support HNP
#define MUSB_DEVSTAT_A_ALT_HNP_SUPPORT (1 << 7)     // Host Support HNP on Alter Port.
#define MUSB_DEVSTAT_R_WK_OK        (1 << 6)        // Remote wakeup enabled
#define MUSB_DEVSTAT_USB_RESET      (1 << 5)        // USB device reset
#define MUSB_DEVSTAT_SUS            (1 << 4)        // Device suspended
#define MUSB_DEVSTAT_CFG            (1 << 3)        // Device configured
#define MUSB_DEVSTAT_ADD            (1 << 2)        // Device addresses       
#define MUSB_DEVSTAT_DEF            (1 << 1)        // Device defined
#define MUSB_DEVSTAT_ATT            (1 << 0)        // Device attached

//------------------------------------------------------------------------------
//
//  Type:  USBFN_EP
//
//  Internal PDD structure which holds info about endpoint direction, max packet
//  size and active transfer.
//
typedef struct {
    WORD maxPacketSize;
    BOOL dirRx;
    BOOL zeroLength;
    STransfer *pTransfer;
    BOOL bZeroLengthSent;
} USBFN_EP;

// ep0State values:  EP0 state
enum {
    EP0_ST_SETUP,
    EP0_ST_SETUP_PROCESSED,
    EP0_ST_STATUS_IN,
    EP0_ST_STATUS_OUT,
    EP0_ST_DATA_RX,
    EP0_ST_DATA_TX,
    EP0_ST_ERROR
};

// endPoint transfer type
enum {
    EP_TYPE_CTRL = 0,
    EP_TYPE_ISOCH,
    EP_TYPE_BULK,
    EP_TYPE_INTERRUPT
};
//------------------------------------------------------------------------------
//
//  Type:  USBFN_PDD
//
//  Internal PDD context structure.
//
typedef struct {

    VOID *pMddContext;
    PFN_UFN_MDD_NOTIFY pfnNotify;

    PCSP_MUSB_OTG_REGS   pUsbOtgRegs;   //  Pointer to USB OTG Reg
    PCSP_MUSB_CSR_REGS   pUsbCsrRegs;   //  Pointer to the CSR registers (0x100 - 0x1FF)
    PCSP_MUSB_GEN_REGS   pUsbGenRegs;   //  Pointer to USB General Reg

    HANDLE hClk;

    DWORD devState;
    BOOL selfPowered;

    BOOL setupDirRx;
    WORD setupCount;
    HANDLE hSetupEvent;

    USBFN_EP ep[USBD_EP_COUNT];

    UCHAR   ucAddress;
    BOOL    bSetAddress;
    
    BOOL fakeDsChange;                  // To workaround MDD problem
    DWORD ep0State;    
    DWORD intr_rx_data_avail ;
} USBFN_PDD;

USBFN_PDD g_usbfnpdd;

#ifdef DEBUG

#define ZONE_INTERRUPTS         DEBUGZONE(8)
#define ZONE_POWER              DEBUGZONE(9)
#define ZONE_PDD                DEBUGZONE(15)

#endif

#define USBFN_DEBUG             FALSE

extern BOOL EnableUSBClocks(BOOL bEnable);
extern void ConnectHardware();
extern void DisconnectHardware();
extern void ResetHardware();

void DumpUSBRegisters(PCSP_MUSB_OTG_REGS pOtg, PCSP_MUSB_GEN_REGS pGen)
{
	UNREFERENCED_PARAMETER(pOtg);
	UNREFERENCED_PARAMETER(pGen);

	OALMSG(TRUE, (TEXT("OTG_Rev        = 0x%x\r\n"), INREG32(&pOtg->OTG_Rev)));
	OALMSG(TRUE, (TEXT("OTG_FORCESTDBY = 0x%x\r\n"), INREG32(&pOtg->OTG_FORCESTDBY)));
	OALMSG(TRUE, (TEXT("OTG_INTERFSEL  = 0x%x\r\n"), INREG32(&pOtg->OTG_INTERFSEL)));
	OALMSG(TRUE, (TEXT("OTG_SYSCONFIG  = 0x%x\r\n"), INREG32(&pOtg->OTG_SYSCONFIG)));
	OALMSG(TRUE, (TEXT("OTG_SYSSTATUS  = 0x%x\r\n"), INREG32(&pOtg->OTG_SYSSTATUS)));
	OALMSG(TRUE, (TEXT("OTG_SIMENABLE  = 0x%x\r\n"), INREG32(&pOtg->OTG_SIMENABLE)));
	OALMSG(TRUE, (TEXT("FAddr          = 0x%x\r\n"), INREG8(&pGen->FAddr)));
	OALMSG(TRUE, (TEXT("DevCtl         = 0x%x\r\n"), INREG8(&pGen->DevCtl)));
	OALMSG(TRUE, (TEXT("IntrRxE        = 0x%x\r\n"), INREG16(&pGen->IntrRxE)));
	OALMSG(TRUE, (TEXT("IntrTxE        = 0x%x\r\n"), INREG16(&pGen->IntrTxE)));
	OALMSG(TRUE, (TEXT("IntrUSBE       = 0x%x\r\n"), INREG8(&pGen->IntrUSBE)));
	OALMSG(TRUE, (TEXT("Power          = 0x%x\r\n"), INREG8(&pGen->Power)));
	OALMSG(TRUE, (TEXT("RxFIFOsz       = 0x%x\r\n"), INREG8(&pGen->RxFIFOsz)));
	OALMSG(TRUE, (TEXT("RxFIFOadd      = 0x%x\r\n"), INREG16(&pGen->RxFIFOadd)));
	OALMSG(TRUE, (TEXT("TxFIFOsz       = 0x%x\r\n"), INREG8(&pGen->TxFIFOsz)));
	OALMSG(TRUE, (TEXT("TxFIFOadd      = 0x%x\r\n"), INREG16(&pGen->TxFIFOadd)));
}

BOOL ReadFIFO(USBFN_PDD *pPdd, UCHAR endpoint, void *pData, DWORD size)
//ReadFIFO(DWORD* pFifoBase, void *pData, DWORD size)
{
    DWORD total  = size / 4;
    DWORD remain = size % 4;
    DWORD i		 = 0;
    PCSP_MUSB_GEN_REGS pGenRegs = pPdd->pUsbGenRegs;
    
    DWORD* pDword = (DWORD*)pData;
    DWORD* pFifoBase = (DWORD*) &pGenRegs->fifo[endpoint];

    volatile ULONG *pReg = (volatile ULONG*)pFifoBase;

    if (((DWORD)pDword) & 0x1)
    {
        __unaligned DWORD* pUnalignedDword = (DWORD*)pDword;
        RETAILMSG(1,(TEXT("unaligned readfifo\r\n")));
        // this is NOT 16-bit aligned
        for (i = 0; i < total; i++)
        {
            *pUnalignedDword++ = INREG32(pReg);
        }
#pragma warning(push)
#pragma warning(disable:4090)
        pDword = pUnalignedDword;
#pragma warning(pop)
    }
    else
    {
        // this is 16-bit aligned. 32 bits access are allowed
        for (i = 0; i < total; i++)
        {
            *pDword++ = INREG32(pReg);
        }        
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

//-------------------------------------------------------------------------
BOOL WriteFIFO(USBFN_PDD *pPdd, UCHAR endpoint, void *pData, DWORD size)
{
    PCSP_MUSB_GEN_REGS pGenRegs = pPdd->pUsbGenRegs;
    DWORD totalTx;
    DWORD i;
    BOOL rc = TRUE;
    
    OALMSG(OAL_ETHER&&OAL_FUNC, (TEXT("+WriteFIFO: size(0x%x)\r\n"), size));

    // Zero-length data transferred
    if (size == 0)
        goto ErrorReturn;

    // Note - transfers to/from FIFO must have consistent width to maintain
    // alignment.  The last few writes can be smaller to account for remaining bytes.
    // NOTE - pointers passed to this function are not guaranteed to be aligned!
    
    // Write DWORDs if possible
    if (((UINT32)pData & 0x3) == 0)
    {
        totalTx = size / 4;
        for (i=0; i<totalTx; i++)
        {
            OUTREG32(&pGenRegs->fifo[endpoint], (*(UINT32*)pData));        
            ((UINT32*)pData)++;
            size -= 4;
        }
    }
    
    // Write WORDS if possible
    if (((UINT32)pData & 0x1) == 0)
    {
        totalTx = size / 2;
        for (i=0; i<totalTx; i++)
        {
            OUTREG16(&pGenRegs->fifo[endpoint], (*(UINT16*)pData));        
            ((UINT16*)pData)++;
            size -= 2;
        }
    }
    
    // Write remaining bytes
    totalTx = size;
    for (i=0; i<totalTx; i++)
    {
        OUTREG8(&pGenRegs->fifo[endpoint], (*(UINT8*)pData));        
        ((UINT8*)pData)++;
        size -= 1;
    }

ErrorReturn:
    OALMSG(OAL_ETHER&&OAL_FUNC, (TEXT("-WriteFIFO\r\n")));
    return rc;       
}

//------------------------------------------------------------------------------
//
//  Function:  ContinueTxTransfer
//
//  This function sends next packet from transaction buffer. 
//
static DWORD WINAPI
ContinueTxTransfer(
    USBFN_PDD *pPdd,
    DWORD endpoint
)
{
    DWORD rc = ERROR_INVALID_PARAMETER;
    USBFN_EP *pEP = &pPdd->ep[endpoint];
    PCSP_MUSB_CSR_REGS pCsrRegs = pPdd->pUsbCsrRegs;
    STransfer *pTransfer = pEP->pTransfer;
    DWORD space, txcount;
    UCHAR *pBuffer;
    DWORD dwFlag;
    
    OALMSG(OAL_ETHER&&OAL_FUNC, (L"+UsbFnPdd!ContinueTxTransfer: "
        L"EP %d pTransfer 0x%08x (%d, %d, %d)\r\n",
        endpoint, pTransfer, pTransfer != NULL ? pTransfer->cbBuffer : 0,
        pTransfer != NULL ? pTransfer->cbTransferred : 0,
        pTransfer != NULL ? pTransfer->dwUsbError : -1
        ));

    if (pTransfer == NULL) 
    {
        OALMSG(OAL_ERROR, (L"Error! pTransfer is NULL\r\n"));
        goto CleanUp;
    }    
      
    {
       
        pBuffer =(UCHAR*)pTransfer->pvBuffer + pTransfer->cbTransferred;
        space = pTransfer->cbBuffer - pTransfer->cbTransferred;
                
        if (endpoint != 0)
        {
            if (INREG16(&pCsrRegs->ep[endpoint].CSR.TxCSR) & TXCSR_P_TXPKTRDY)
            {
                OALMSG(OAL_ETHER&&OAL_FUNC, (TEXT("endpoint(%d) cbBuffer(%d) cbTransferred(%d)\r\n"), endpoint, pTransfer->cbBuffer, pTransfer->cbTransferred));
                OALMSG(OAL_ETHER&&OAL_FUNC, (TEXT("TXPKTRdy bit is set ..\r\n")));
                
                goto CleanUp;
            }
            else
            {
                // How many bytes we can send just now?
                txcount = pEP->maxPacketSize;
                if (txcount > space)
                    txcount = space;
                
                OALMSG(OAL_ETHER&&OAL_FUNC, (TEXT("WriteFIFO EP %d, Size %d\r\n"), endpoint, txcount));
                //memdump((uchar *)pBuffer, (unsigned short)txcount, 0);
                // Write data to FIFO
                WriteFIFO(pPdd, (UCHAR) endpoint, pBuffer, txcount);
                
                // We transfered some data
                pTransfer->cbTransferred += txcount;                
                SETREG16(&pCsrRegs->ep[endpoint].CSR.TxCSR, TXCSR_P_TXPKTRDY);
            } 
        }
        else // endpoint == 0
        {
            // Zero endpoint: Zero length padding needed
            // if last packet is maxPacketSize.
            pEP->zeroLength =
                (space == pEP->maxPacketSize) &&
                (pPdd->setupCount > pTransfer->cbBuffer);
            
            if (pPdd->ep0State != EP0_ST_SETUP_PROCESSED)
            {
                OALMSG(OAL_ERROR, (TEXT("Invalid State = %d\r\n"), pPdd->ep0State));
                goto CleanUp;
            }
            else
            {
                pPdd->ep0State = EP0_ST_DATA_TX;
                if (INREG16(&pCsrRegs->ep[endpoint].CSR.CSR0) & CSR0_P_TXPKTRDY)
                {
                    OALMSG(OAL_ERROR, (TEXT("endpoint(%d) cbBuffer(%d) cbTransferred(%d)\r\n"), endpoint, pTransfer->cbBuffer, pTransfer->cbTransferred));
                    OALMSG(OAL_ERROR, (TEXT("TXPKTRdy bit is set ..\r\n")));
                    goto CleanUp;
                }
                else
                {
                    // How many bytes we can send just now?
                    txcount = pEP->maxPacketSize;
                    if (txcount > space)
                        txcount = space;
                    
                    OALMSG(OAL_ETHER&&OAL_FUNC, (L"EP0_ST_DATA_TX space_left_to_send=%d txcount=%d\r\n", space, txcount));
                    
                    // Write data to FIFO
                    WriteFIFO(pPdd, EP0, pBuffer, txcount);
                    
                    // We transfered some data
                    pTransfer->cbTransferred += txcount;
                    
                    OALMSG(OAL_ETHER&&OAL_FUNC, (L"Tx packet size: %d\r\n",
                        pTransfer->cbBuffer
                        ));
                                        
                    if (pTransfer->cbTransferred == pTransfer->cbBuffer)
                    {
                        OALMSG(OAL_ETHER&&OAL_FUNC, (TEXT("DATAEND add\r\n")));
                        pPdd->ep0State = EP0_ST_STATUS_OUT;
                        OALMSG(OAL_ETHER&&OAL_FUNC, (TEXT("Set OUT Msg => ep0State = EP0_ST_STATUS_OUT\r\n")));
                        dwFlag = CSR0_P_TXPKTRDY | CSR0_P_DATAEND;
                        pEP->pTransfer = NULL;
                        pTransfer->dwUsbError = UFN_NO_ERROR;
                        pPdd->pfnNotify(pPdd->pMddContext, UFN_MSG_TRANSFER_COMPLETE, (DWORD)pTransfer);   
                    }
                    else
                        dwFlag = CSR0_P_TXPKTRDY;
                    SETREG16(&pCsrRegs->ep[endpoint].CSR.CSR0, dwFlag);                    
                }                
            }
        } // if (endpoint != 0)
    }
    
    rc = ERROR_SUCCESS;
    
CleanUp:
    
    OALMSG(OAL_ETHER&&OAL_FUNC, (L"-UsbFnPdd!ContinueTxTransfer\r\n"));
    return rc;
}

//------------------------------------------------------------------------------

static DWORD WINAPI
StartRxTransfer(
    USBFN_PDD *pPdd,
    DWORD endpoint
)
{
    DWORD rc = ERROR_INVALID_PARAMETER;
    USBFN_EP *pEP = &pPdd->ep[endpoint];
    STransfer *pTransfer = pEP->pTransfer;
    PCSP_MUSB_CSR_REGS pCsrRegs = pPdd->pUsbCsrRegs;
    BOOL bNotifyDataReady = FALSE;
    
    OALMSG(OAL_ETHER&&OAL_FUNC, (L"UsbFnPdd!StartRxTransfer: "
        L"EP %d pTransfer 0x%08x (%d, %d, %d)\r\n",
        endpoint, pTransfer, pTransfer != NULL ? pTransfer->cbBuffer : 0,
        pTransfer != NULL ? pTransfer->cbTransferred : 0,
        pTransfer != NULL ? pTransfer->dwUsbError : -1
        ));

    if (pTransfer == NULL) 
    {
        OALMSG(OAL_ERROR, (L"StartRxTransfer:  error pTransfer is NULL\r\n"));
        return ERROR_INVALID_PARAMETER;
    }    
    
    if (endpoint != 0)
    {
        if (pPdd->intr_rx_data_avail &(1 << endpoint))
        {
            // rx FIFO has data.
            UCHAR *pBuffer;            
            DWORD space, maxSize;
            WORD rxcount;
            
            rxcount = INREG16(&pCsrRegs->ep[endpoint].Count.RxCount);
            
            pBuffer =(UCHAR*)pTransfer->pvBuffer + pTransfer->cbTransferred;               
            space = pTransfer->cbBuffer - pTransfer->cbTransferred;
            // Get maxPacketSize
            maxSize = pPdd->ep[endpoint].maxPacketSize;
            
            {
                USHORT dwFlag = RXCSR_P_RXPKTRDY;
                
                ReadFIFO(pPdd, (UCHAR)endpoint, pBuffer, (DWORD)rxcount);
                OALMSG(OAL_ETHER&&OAL_FUNC, (L"Received:\r\n"));
                
                // We transfered some data
                pTransfer->cbTransferred = pTransfer->cbBuffer - space;
                
                // Is this end of transfer?
                if (rxcount < maxSize)
                {
                    // received last block
                    pTransfer->dwUsbError = UFN_NO_ERROR;
                    bNotifyDataReady = TRUE;
                    // Don't clear the RxPktRdy until it is ready to further received
                }
                else
                    CLRREG16(&pCsrRegs->ep[endpoint].RxCSR, dwFlag);
            }
            
            // clear intr_rx_data_avail EP0 bit
            pPdd->intr_rx_data_avail &= ~(1 << endpoint);           
            
            if (bNotifyDataReady)
            {
                pEP->pTransfer = NULL;
                pTransfer->dwUsbError = UFN_NO_ERROR;
                OALMSG(OAL_ETHER&&OAL_FUNC, (TEXT("ACK: StartRxTransfer for EP %d\r\n"), endpoint));
                pPdd->pfnNotify(
                    pPdd->pMddContext, UFN_MSG_TRANSFER_COMPLETE, (DWORD)pTransfer);
            }
        }
        else
        {
            CLRREG16(&pCsrRegs->ep[endpoint].RxCSR, RXCSR_P_RXPKTRDY);
            OALMSG(OAL_ETHER&&OAL_FUNC, (TEXT("EP %d IssueTransfer(OUT) RxCSR=0x%x\r\n"), endpoint, INREG16(&pCsrRegs->ep[endpoint].RxCSR)));
        }
    }
    else // if (endpoint != 0)
    {
        if (pPdd->ep0State == EP0_ST_SETUP_PROCESSED)
        {
            pPdd->ep0State = EP0_ST_DATA_RX;
            
            if (pPdd->intr_rx_data_avail &(1 << EP0))
            {
                // rx FIFO has data.
                UCHAR *pBuffer;            
                DWORD space, maxSize;
                USHORT rxcount;
                
                rxcount = INREG8(&pCsrRegs->ep[EP0].Count.Count0);
                
                pBuffer =(UCHAR*)pTransfer->pvBuffer + pTransfer->cbTransferred;               
                space = pTransfer->cbBuffer - pTransfer->cbTransferred;
                // Get maxPacketSize
                maxSize = pPdd->ep[EP0].maxPacketSize;
                
                {
                    USHORT dwFlag = CSR0_P_SERVICEDRXPKTRDY;
                    
                    ReadFIFO(pPdd, (UCHAR)endpoint, pBuffer, (DWORD)rxcount);
                    OALMSG(OAL_ETHER&&OAL_FUNC, (L"Received:\r\n"));
                    //memdump((UCHAR *)pBuffer, (USHORT)rxcount, 0);
                    
                    // We transfered some data
                    pTransfer->cbTransferred = pTransfer->cbBuffer - space;
                    
                    // Is this end of transfer?
                    if (rxcount < maxSize)
                    {
                        // received last block
                        pTransfer->dwUsbError = UFN_NO_ERROR;
                        pPdd->ep0State = EP0_ST_STATUS_IN;
                        bNotifyDataReady = TRUE;
                        dwFlag |= CSR0_P_DATAEND;
                    }
                    SETREG16(&pCsrRegs->ep[EP0].CSR.CSR0, dwFlag);
                }

                // clear intr_rx_data_avail EP0 bit
                pPdd->intr_rx_data_avail &= ~(1 << EP0);            
                
                if (bNotifyDataReady)
                {
                    pEP->pTransfer = NULL;
                    pTransfer->dwUsbError = UFN_NO_ERROR;
                    pPdd->pfnNotify(
                        pPdd->pMddContext, UFN_MSG_TRANSFER_COMPLETE, (DWORD)pTransfer);
                }
            }
        }
    }       
    
    rc = ERROR_SUCCESS;
    
    OALMSG(OAL_ETHER&&OAL_FUNC, (L"-StartRxTransfer\r\n"));
    
    return rc;
}

static UINT CalcFIFOAddr(DWORD endpoint, BOOL bRxDir)
{
    UINT fifoAddr;

    fifoAddr =  endpoint *(1024/8);

    if (bRxDir)
       fifoAddr +=512/8;

    return fifoAddr;
}

//------------------------------------------------------------
//
//  Function: StartUSBClock
//
//  Routine Description:
//
//      Start the USB Interface Clock
//
static void StartUSBClock(USBFN_PDD *pPdd)
{
    PCSP_MUSB_GEN_REGS pGen = pPdd->pUsbGenRegs;

	UNREFERENCED_PARAMETER(pPdd);

	OALMSG(OAL_ETHER&&OAL_FUNC, (TEXT("StartUSBClock\r\n")));

	if (!EnableUSBClocks(TRUE))
	{
		OALMSG(OAL_ETHER, (TEXT("StartUSBClock : Failed to enable clocks\r\n")));
	}
	
	// Reset the hardware
	ResetHardware();

	OUTREG8(&pGen->Power, POWER_HSENABLE); 

    return;
}

//------------------------------------------------------------------------------
//
//  Function:  GetUniqueDeviceID
//

DWORD GetUniqueDeviceID()
{
    DWORD code;
    DWORD *pDieId = (DWORD *)OALPAtoUA(SOCGetIDCodeAddress());

    // Create unique part of name from SoC ID
    code  = INREG32(pDieId);

    OALMSG(OAL_ETHER&&OAL_FUNC, (L"+USBFN::Device ID = 0x%x\r\n", code));

    return 0x0B5D902F;  // hardcoded id
}

//------------------------------------------------------------------------------
//  Function:   Device_Disconnect
//
//  Routine description:
//
//      Handle of Disconnect interrupt
//
//  Arguments:
//
//      pHSMUSBContext  :   Handle of HSMUSB_T
//      others          :   TBD
//
//  Return values:
//
//      ERROR_SUCCESS
//
DWORD Device_Disconnect(USBFN_PDD *pPdd)
{
    OALMSG(OAL_ETHER&&OAL_FUNC, (L"Device_Disconnect\r\n"));

    // We are not configured anymore
    // Let MDD process change
    pPdd->pfnNotify(pPdd->pMddContext, UFN_MSG_BUS_EVENTS, UFN_DETACH);
    pPdd->devState &= ~MUSB_DEVSTAT_ATT;
    
    return ERROR_SUCCESS;
}

//------------------------- Interrupt Routine ----------------------------------
// 
//  Function:   Device_ResetIRQ
//
//  Routine description:
//
//      Handling of Reset interrupt
//
//  Arguments:
//      
//      pHSMUSBContext  :   Handle to HSMUSB_T
//      others          :   to be determined.
//
//  Return values:
//  
//      ERROR SUCCESS
//
DWORD Device_ResetIRQ(USBFN_PDD *pPdd)
{
    PCSP_MUSB_GEN_REGS pGenRegs = pPdd->pUsbGenRegs;
    
    OALMSG(OAL_ETHER&&OAL_FUNC, (TEXT("+Device_ResetIRQ\r\n")));
    
    // initialize Endpoint0 setup state 
    pPdd->ep0State = EP0_ST_SETUP;
    
    pPdd->intr_rx_data_avail = 0;
    
    if ((pPdd->devState & MUSB_DEVSTAT_ATT) == 0)
    {
        // MUSB doesn't generate an attach interrrupt.  Interpret bus reset event as attach.
        pPdd->devState |= MUSB_DEVSTAT_ATT | MUSB_DEVSTAT_USB_RESET;
        pPdd->pfnNotify(pPdd->pMddContext, UFN_MSG_BUS_EVENTS, UFN_DETACH);
        pPdd->pfnNotify(pPdd->pMddContext, UFN_MSG_BUS_EVENTS, UFN_ATTACH);
        if (INREG8(&pGenRegs->Power) & POWER_HSMODE)
        {
		    OALMSG(OAL_ETHER, (TEXT("BS_HIGH_SPEED\r\n")));
            pPdd->pfnNotify(pPdd->pMddContext, UFN_MSG_BUS_SPEED, BS_HIGH_SPEED);
        }
        else
        {
		    OALMSG(OAL_ETHER, (TEXT("BS_FULL_SPEED\r\n")));
            pPdd->pfnNotify(pPdd->pMddContext, UFN_MSG_BUS_SPEED, BS_FULL_SPEED);
        }           
        // Tell MDD about reset...
        pPdd->pfnNotify(pPdd->pMddContext, UFN_MSG_BUS_EVENTS, UFN_RESET);
    }
    else
        pPdd->pfnNotify(pPdd->pMddContext, UFN_MSG_BUS_EVENTS, UFN_RESET);

    OALMSG(OAL_ETHER&&OAL_FUNC, (TEXT("-Device_ResetIRQ\r\n")));
    return ERROR_SUCCESS;
}

//------------------------------------------------------------------------------
//
//  Function:   Device_ProcessEP0
//
//  Routine description:
//
//      Handling of endpoint 0 transfer
//
//  Arguments:
//
//      pHSMUSBContext  :   Handle of HSMUSB_T
//      others          :   TBD
//
//  Return values:
//      
//      ERROR_SUCCESS
//
DWORD Device_ProcessEP0(USBFN_PDD *pPdd)
{
    PCSP_MUSB_GEN_REGS pGenRegs = pPdd->pUsbGenRegs;
    PCSP_MUSB_CSR_REGS pCsrRegs = pPdd->pUsbCsrRegs;
    UINT16 csr0Val;
    UINT16 rxcount;
    DWORD data[2];
    
    OALMSG(OAL_ETHER&&OAL_FUNC, (L"+Device_ProcessEP0\r\n"));

    csr0Val = INREG16(&pCsrRegs->ep[EP0].CSR.CSR0);
    rxcount = INREG16(&pCsrRegs->ep[EP0].Count.Count0);
    OALMSG(OAL_ETHER&&OAL_FUNC, (TEXT("Interrupt receive with CSR0 = 0x%x, rxcount = 0x%x\r\n"), csr0Val, rxcount));
    
    if (csr0Val & CSR0_P_SENTSTALL)
    {
        // MUSB finished sending STALL 
        // clear SENTSTALL 
        OALMSG(OAL_ETHER&&OAL_FUNC, (TEXT("Stall\r\n")));
        CLRREG16(&pCsrRegs->ep[EP0].CSR.CSR0, CSR0_P_SENTSTALL);
        pPdd->ep0State = EP0_ST_SETUP;
        csr0Val = INREG16(&pCsrRegs->ep[EP0].CSR.CSR0);
    }
    
    if (csr0Val & CSR0_P_SETUPEND)
    {
        /* setup request ended "early" */
        // clear SETUPEND 
        OALMSG(OAL_ETHER&&OAL_FUNC, (TEXT("SetupEnd\r\n")));
        if (pPdd->bSetAddress)
        {
            OALMSG(OAL_ETHER&&OAL_FUNC, (L"SetAddress=%d\r\n", pPdd->ucAddress));
            OUTREG8(&pGenRegs->FAddr, pPdd->ucAddress); 
            pPdd->bSetAddress = FALSE;
        }
        
        SETREG16(&pCsrRegs->ep[EP0].CSR.CSR0, CSR0_P_SERVICEDSETUPEND);
        pPdd->ep0State = EP0_ST_SETUP;
        csr0Val = INREG16(&pCsrRegs->ep[EP0].CSR.CSR0);
        OALMSG(OAL_ETHER&&OAL_FUNC, (TEXT("CSR0 = 0x%x\r\n"), csr0Val));
    }
    
    switch (pPdd->ep0State)
    {
        case EP0_ST_STATUS_IN:
            // dummy state in processEP0
            if (pPdd->bSetAddress)
            {
                OALMSG(OAL_ETHER&&OAL_FUNC, (L"SetAddress=%d\r\n", pPdd->ucAddress));
                OUTREG8(&pGenRegs->FAddr, pPdd->ucAddress); 
                pPdd->bSetAddress = FALSE;
            }
            OALMSG(OAL_ETHER&&OAL_FUNC, (TEXT("EP0_ST_STATUS_IN => EP0 SETUP\r\n")));
            pPdd->ep0State = EP0_ST_SETUP;
            
            // if received packet while in EP0_ST_STATUS_IN, continue process SETUP packet.
        case EP0_ST_SETUP:
            if (rxcount < 8)
                break;
            if (!(csr0Val & CSR0_P_RXPKTRDY))
                break;        
            data[0] = INREG32(&pGenRegs->fifo[EP0]);
            data[1] = INREG32(&pGenRegs->fifo[EP0]);        
            OALMSG(OAL_ETHER&&OAL_FUNC, (L"EP0_ST_SETUP receives 0x%x 0x%x\r\n", data[0], data[1]));

            pPdd->ep0State = EP0_ST_SETUP_PROCESSED;
            pPdd->pfnNotify(pPdd->pMddContext, UFN_MSG_SETUP_PACKET, (DWORD)data);        
            break;
            
        case EP0_ST_SETUP_PROCESSED:
            pPdd->intr_rx_data_avail |=(1 << EP0);
            break;
            
        case EP0_ST_DATA_RX:
            if (!(csr0Val & CSR0_P_RXPKTRDY))
                break;
            
            if (pPdd->ep[EP0].pTransfer == NULL) 
            {
                // set intr_rxdata_avail and when IssueTransfer() is called retrieve data
                //          from FIFO and set CSR0 ServicedRxPkt bit and check for end of packet to set CSR0 DATAEND bit.
                pPdd->intr_rx_data_avail |=(1 << EP0);
                OALMSG(OAL_ETHER&&OAL_FUNC, (L"Device_ProcessEP0:  error EP0_ST_DATA_RX pTransfer is NULL\r\n"));
                break;
            }
            
            {
                USBFN_EP *pEP = &pPdd->ep[EP0];
                STransfer *pTransfer = pEP->pTransfer;
                UCHAR *pBuffer;            
                DWORD space, remain, maxSize;
                
                pBuffer =(UCHAR*)pTransfer->pvBuffer + pTransfer->cbTransferred;               
                space = pTransfer->cbBuffer - pTransfer->cbTransferred;
                // Get maxPacketSize
                maxSize = pPdd->ep[EP0].maxPacketSize;
                
                {
                    USHORT dwFlag = CSR0_P_SERVICEDRXPKTRDY;
                    remain = rxcount;
                    
                    ReadFIFO(pPdd, (UCHAR)EP0, pBuffer, (DWORD)rxcount);
                    
                    OALMSG(OAL_ETHER&&OAL_FUNC, (L"Device_ProcessEP0: EP0_ST_DATA_RX received data\r\n"));                    
                    
                    // We transfered some data
                    pTransfer->cbTransferred += rxcount;
                    
                    // Is this end of transfer?
                    if (rxcount < maxSize)
                    {
                        // received last block
                        pTransfer->dwUsbError = UFN_NO_ERROR;
                        pPdd->ep0State = EP0_ST_STATUS_IN;
                        pEP->pTransfer = NULL;
                        pPdd->pfnNotify(
                            pPdd->pMddContext, UFN_MSG_TRANSFER_COMPLETE, (DWORD)pTransfer);
                        dwFlag |= CSR0_P_DATAEND;
                    }
                    SETREG16(&pCsrRegs->ep[EP0].CSR.CSR0, dwFlag);
                }
                
                // clear intr_rx_data_avail EP0 bit
                pPdd->intr_rx_data_avail &= ~(1 << EP0);            
            }
            break;
            
        case EP0_ST_DATA_TX:
            if (csr0Val & CSR0_P_TXPKTRDY)
                break;
            
            {
                USBFN_EP *pEP = &pPdd->ep[EP0];
                STransfer *pTransfer = pEP->pTransfer;
                UCHAR *pBuffer;
                DWORD space, txcount;
                
                if (pTransfer == NULL) 
                {
                    OALMSG(OAL_ETHER&&OAL_FUNC, (L"Device_ProcessEP0:  error EP0_ST_DATA_TX pTransfer is NULL\r\n"));
                    break;
                }
                
                {
                    DWORD dwFlag = 0;
                    pBuffer =(UCHAR*)pTransfer->pvBuffer + pTransfer->cbTransferred;
                    space = pTransfer->cbBuffer - pTransfer->cbTransferred;
                    
                    
                    // How many bytes we can send just now?
                    txcount = pEP->maxPacketSize;
                    if (txcount > space)
                        txcount = space;
                    
                    OALMSG(OAL_ETHER&&OAL_FUNC, (L"Device_ProcessEP0: EP0_ST_DATA_TX bytes_left_to_send=%d sending=%d\r\n", space, txcount));
                    
                    // Write data to FIFO
                    WriteFIFO(pPdd, EP0, pBuffer, txcount);
                    
                    // We transfered some data
                    pTransfer->cbTransferred += txcount;
                    dwFlag = CSR0_P_TXPKTRDY;               
                    
                    if (pTransfer->cbTransferred == pTransfer->cbBuffer)
                    {
                        pPdd->ep0State = EP0_ST_STATUS_OUT;
                        pEP->pTransfer = NULL;
                        pTransfer->dwUsbError = UFN_NO_ERROR;
                        pPdd->pfnNotify(pPdd->pMddContext, UFN_MSG_TRANSFER_COMPLETE, (DWORD)pTransfer);   
                        OALMSG(OAL_ETHER&&OAL_FUNC, (TEXT("Device_Process0: final copy Notify - transfer completed\r\n")));
                        dwFlag |= CSR0_P_DATAEND;
                    }       
                    
                    OALMSG(OAL_ETHER&&OAL_FUNC, (TEXT("2. Device_ProcessEP0 Set TxPktRdy - 0\r\n")));
                    SETREG16(&pCsrRegs->ep[EP0].CSR.CSR0, dwFlag);
                }
            }
            break;            
            
        case EP0_ST_STATUS_OUT:
            // dummy state in processEP0
            break;
            
        case EP0_ST_ERROR:
            if (csr0Val & CSR0_P_SENTSTALL)
            {
                OALMSG(OAL_ETHER&&OAL_FUNC, (TEXT("ST_ERROR\r\n")));
                pPdd->ep0State = EP0_ST_SETUP;
                CLRREG16(&pCsrRegs->ep[EP0].CSR.CSR0, CSR0_P_SENTSTALL);
            }
            break;
            
        default:
            break;
        }
        
        OALMSG(OAL_ETHER&&OAL_FUNC, (L"-Device_ProcessEP0\r\n"));
        
        return ERROR_SUCCESS;
}

//------------------------------------------------------------------------------
//  Function:   Device_ProcessEPx_RX
//
//  Routine description:
//
//      Handle of Endpoint (1-15) receive (OUT transaction)
//      This function is called when IntrRX of corresponding endpoint is set.
//      This indicates the corresponding endpoint has data in corresponding FIFO/DMA.
//
//  Arguments:
//
//      pHSMUSBContext  :   Handle of HSMUSB_T
//      endpoint        :   endpoint to be processed
//      others          :   TBD
//
//  Return values:
//
//      ERROR_SUCCESS
//


DWORD Device_ProcessEPx_RX(USBFN_PDD *pPdd, DWORD endpoint)
{
    PCSP_MUSB_CSR_REGS pCsrRegs = pPdd->pUsbCsrRegs;
    UINT16 csrVal;
    USBFN_EP *pEP = &pPdd->ep[endpoint];
    UINT16 rxcount;
    STransfer *pTransfer = pEP->pTransfer;
    
    OALMSG(OAL_ETHER&&OAL_FUNC, (L"+Device_ProcessEPx_RX: endpoint=%d\r\n", endpoint));
    csrVal = INREG16(&pCsrRegs->ep[endpoint].RxCSR);
    
    if (csrVal & RXCSR_P_SENTSTALL)
    {
        // MUSB finished sending STALL 
        // clear SENTSTALL 
        OALMSG(OAL_ETHER&&OAL_FUNC, (TEXT("Stall\r\n")));
        CLRREG16(&pCsrRegs->ep[endpoint].RxCSR, RXCSR_P_SENTSTALL);
        csrVal = INREG16(&pCsrRegs->ep[endpoint].RxCSR);
    }
    
    if (csrVal & RXCSR_P_OVERRUN)
    {
        // clear OVERRUN 
        OALMSG(OAL_ERROR, (TEXT("Overrun\r\n")));
        CLRREG16(&pCsrRegs->ep[endpoint].RxCSR, RXCSR_P_OVERRUN);
        if (pTransfer != NULL)
        {
           pTransfer->dwUsbError = UFN_CLIENT_BUFFER_ERROR;
           pEP->pTransfer = NULL;
           pPdd->pfnNotify(
               pPdd->pMddContext, UFN_MSG_TRANSFER_COMPLETE, (DWORD)pTransfer);
        }
        
        pPdd->intr_rx_data_avail &=(1 << endpoint);
        goto cleanUp;
    }
    
    if (!(csrVal & RXCSR_P_RXPKTRDY))
        goto cleanUp;      
    
    if (pTransfer == NULL) 
    {
        pPdd->intr_rx_data_avail |=(1 << endpoint);
        OALMSG(OAL_ERROR, (L"!!! Device_ProcessEPx_RX:  pTransfer is NULL\r\n"));
        goto cleanUp;
    }
    
    rxcount = INREG16(&pCsrRegs->ep[endpoint].Count.RxCount);
    
    {
        UCHAR *pBuffer; 
        
        pBuffer =(UCHAR*)pTransfer->pvBuffer + pTransfer->cbTransferred;            
        ReadFIFO(pPdd, (UCHAR)endpoint, pBuffer, (DWORD)rxcount);
        
        // clear intr_rx_data_avail EP bit
        pPdd->intr_rx_data_avail &= ~(1 << endpoint);           
        
        // We transfered some data
        pTransfer->cbTransferred += rxcount;
        
        // Is this end of transfer?
        // I guess the system doesn't send out the zero-length packet from host.
        if ((rxcount < pPdd->ep[endpoint].maxPacketSize) ||(rxcount == 0) || (pTransfer->cbTransferred == pTransfer->cbBuffer))
        {
            // received last block
            OALMSG(OAL_ETHER&&OAL_FUNC, (L"Device_ProcessEPx_RX: call pfnNotify COMPLETE endp:%d +\r\n", endpoint));  
            pTransfer->dwUsbError = UFN_NO_ERROR;            
            pEP->pTransfer = NULL;
            //memdump((uchar *)pTransfer->pvBuffer, (unsigned short) pTransfer->cbTransferred, 0);
            // Don't clear RXCSR_P_PXPKTRDY here, let the next time it start to do that.

            pPdd->pfnNotify(
                pPdd->pMddContext, UFN_MSG_TRANSFER_COMPLETE, (DWORD)pTransfer);
            //OALMSG(OAL_ETHER&&OAL_FUNC, (L"Device_ProcessEPx_RX: call pfnNotify COMPLETE -\r\n"));              
        }
        else
            CLRREG16(&pCsrRegs->ep[endpoint].RxCSR, RXCSR_P_RXPKTRDY);
    }
    
cleanUp:

    OALMSG(OAL_ETHER&&OAL_FUNC, (L"-Device_ProcessEPx_RX\r\n"));
    
    return ERROR_SUCCESS;
}

//------------------------------------------------------------------------------
//  Function:   Device_ProcessEPx_TX
//
//  Routine description:
//
//      Handle of Endpoint (1-15) transmit (IN transaction)
//      This function is called when IntrTX of corresponding endpoint is set.
//      This indicates the TX has been completed successfully.
//
//  Arguments:
//
//      pHSMUSBContext  :   Handle of HSMUSB_T
//      endpoint        :   endpoint to be processed
//      others          :   TBD
//
//  Return values:
//
//      ERROR_SUCCESS
//
DWORD Device_ProcessEPx_TX(USBFN_PDD *pPdd, DWORD endpoint)
{
    PCSP_MUSB_CSR_REGS pCsrRegs = pPdd->pUsbCsrRegs;
    USBFN_EP *pEP = &pPdd->ep[endpoint];
    STransfer *pTransfer = pEP->pTransfer;
    UINT16 csrVal;
    
    OALMSG(OAL_ETHER&&OAL_FUNC, (L"+Device_ProcessEPx_TX: endpoint=%d\r\n", endpoint));
    csrVal = INREG16(&pCsrRegs->ep[endpoint].CSR.TxCSR);
    
    if (csrVal & TXCSR_P_SENTSTALL)
    {
        // MUSB finished sending STALL 
        // clear SENTSTALL 
        OALMSG(OAL_ETHER&&OAL_FUNC, (TEXT("Stall\r\n")));
        CLRREG16(&pCsrRegs->ep[endpoint].CSR.TxCSR, TXCSR_P_SENTSTALL);
        csrVal = INREG16(&pCsrRegs->ep[endpoint].CSR.TxCSR);
    }
    
    if (csrVal & TXCSR_P_UNDERRUN)
    {
        // clear UNDERRUN 
        OALMSG(OAL_ETHER&&OAL_FUNC, (TEXT("Underrun\r\n")));
        CLRREG16(&pCsrRegs->ep[endpoint].CSR.TxCSR, TXCSR_P_UNDERRUN);
    }
    
    // check if finished sending packet (TXPKTRDY bit clear when finished)
    if (csrVal & TXCSR_P_TXPKTRDY) 
    {
        OALMSG(OAL_ETHER&&OAL_FUNC, (TEXT("TXCSR_P_TXPKTRDY\r\n")));
        goto cleanUp;      
    }
    
    if (pTransfer == NULL) 
    {
        OALMSG(OAL_ERROR, (L"Device_ProcessEPx_TX:  pTransfer is NULL\r\n"));
        goto cleanUp;
    }
    
    {
        UCHAR *pBuffer;     
        DWORD space, txcount;
        
        pBuffer =(UCHAR*)pTransfer->pvBuffer + pTransfer->cbTransferred;
        space = pTransfer->cbBuffer - pTransfer->cbTransferred;
        
        
        // How many bytes we can send just now?
        txcount = pEP->maxPacketSize;
        if (txcount > space)
            txcount = space;
        
        OALMSG(OAL_ETHER&&OAL_FUNC, (L"Device_ProcessEPx_TX: bytes_left_to_send=%d sending=%d\r\n", space, txcount));
        
        if (txcount)
        {
            // Write data to FIFO
            WriteFIFO(pPdd, (UCHAR) endpoint, pBuffer, txcount);
            
            // We transfered some data
            pTransfer->cbTransferred += txcount;
            SETREG16(&pCsrRegs->ep[endpoint].CSR.TxCSR, TXCSR_P_TXPKTRDY);
            //OALMSG(OAL_ETHER&&OAL_FUNC, (L"EPX_TX sent %d bytes\r\n", txcount));  
        }
        
        else if (((pTransfer->cbTransferred % pEP->maxPacketSize) == 0) && !pEP->bZeroLengthSent)
        {
            // send zero-length end of packet
            SETREG16(&pCsrRegs->ep[endpoint].CSR.TxCSR, TXCSR_P_TXPKTRDY);
            pEP->bZeroLengthSent = TRUE;
        }
        else
        {
            // ACK_COMPLETE
            //OALMSG(OAL_ETHER&&OAL_FUNC, (L"Device_ProcessEPx_TX: call pfnNotify COMPLETE endp:%d +\r\n", endpoint));  

            pEP->pTransfer = NULL;
            pTransfer->dwUsbError = UFN_NO_ERROR;

            pPdd->pfnNotify(pPdd->pMddContext, UFN_MSG_TRANSFER_COMPLETE, (DWORD)pTransfer);   
            //OALMSG(OAL_ETHER&&OAL_FUNC, (L"Device_ProcessEPx_TX: call pfnNotify COMPLETE -\r\n"));  
            //OALMSG(OAL_ETHER&&OAL_FUNC, (TEXT("Device_ProcessEPx_TX: ACK_COMPLETE\r\n")));
        }       
    }
    
cleanUp:    

    OALMSG(OAL_ETHER&&OAL_FUNC, (L"-Device_ProcessEPx_TX\r\n"));
    
    return ERROR_SUCCESS;
}

//------------------------------------------------------------------------------
//
//  Function:  InterruptThread
//
//  This is interrupt thread. It controls responses to hardware interrupt. To
//  reduce code length it calls interrupt specific functions.
//
DWORD InterruptThread(VOID *pPddContext)
{
    USBFN_PDD *pPdd = pPddContext;
    PCSP_MUSB_GEN_REGS pGenRegs = pPdd->pUsbGenRegs;
    DWORD intr_usb, intr_tx, intr_rx;
    int i;
    
	for(;;) 
	{
        BOOL fInterrupt = FALSE;
    
        // Get interrupt source
        // Note that these bits are cleared on read
        intr_usb = INREG8(&pGenRegs->IntrUSB);
        intr_tx  = INREG16(&pGenRegs->IntrTx);
        intr_rx  = INREG16(&pGenRegs->IntrRx);
       
        OALMSG(OAL_ETHER&&OAL_FUNC, (
            L"UsbFnPdd!InterruptThread: Interrupts -> 0x%x (intr_usb), 0x%x (intr_tx), 0x%x (intr_rx)\r\n", intr_usb, intr_tx, intr_rx));

        // See MUSBMHDRC spec page 46
        // 1. Handle Reset interrupts (also does attach)
        if (intr_usb & INTRUSB_RESET)
        {
            Device_ResetIRQ(pPdd);
            intr_usb &= INTRUSB_RESET;
            fInterrupt = TRUE;
        }
        
        // 2. Handle Endpoint 0 interrupt
        if (intr_tx & INTR_EP(0))
        {
            Device_ProcessEP0(pPdd);
            intr_tx &= ~INTR_EP(0);
            fInterrupt = TRUE;
        }
        
        // 3. Handle RX Endpoint interrupt
        if (intr_rx)
        {
            i = 1;
            while (i <= 15)
            {
                if (intr_rx & INTR_EP(i))
                {
                    Device_ProcessEPx_RX(pPdd, i);
                    intr_rx &= ~INTR_EP(i);
                }
                i++;
            }
            fInterrupt = TRUE;
        }
        
        // 4. Handle TX Endpoint interrupt
        if (intr_tx)
        {
            i = 1;
            while (i <= 15)
            {
                if (intr_tx & INTR_EP(i))
                {
                    Device_ProcessEPx_TX(pPdd, i);
                    intr_tx &= ~INTR_EP(i);
                }
                i++;
            }
            fInterrupt = TRUE;
        }
        
        // 5. Handle Disconnect
        if (intr_usb & INTRUSB_DISCONN)
        {
            Device_Disconnect(pPdd);
            intr_usb &= INTRUSB_DISCONN;
            fInterrupt = TRUE;
        }
        
        if( !fInterrupt )
        {
            break;
        }
    }
   return ERROR_SUCCESS;
}

//------------------------------------------------------------------------------
//
//  Function:  UfnPdd_IssueTransfer
//
//  Routine description:
//
//      This function is the handling of transferring data from and to the host. 
//
//  Arguments:
//
//      pPddContext :: Handle to HSUSBFNPDD_T
//      endpoint    :: Endpoint to be used to transfer data
//      pTransfer   :: Pointer to STransfer passing from MDD, containing buffer to read/write.
//
//  Return values:
//
//      ERROR_SUCCESS
//
DWORD WINAPI
UfnPdd_IssueTransfer(
    VOID *pPddContext,
    DWORD endpoint,
    STransfer *pTransfer
)
{
    DWORD rc = ERROR_INVALID_PARAMETER;
    USBFN_PDD *pPdd = pPddContext;
    PCSP_MUSB_CSR_REGS pCsrRegs = pPdd->pUsbCsrRegs;
    
    
    OALMSG(OAL_ETHER&&OAL_FUNC, (L"+UfnPdd_IssueTransfer\r\n"));
    
    if (pTransfer == NULL) 
    {
        OALMSG(OAL_ETHER, (L"UfnPdd_IssueTransfer:  error pTransfer is NULL\r\n"));
        goto ErrorReturn;
    }    
    
    // Save transfer for interrupt thread
//    ASSERT(pPdd->ep[endpoint].pTransfer == NULL);
    pPdd->ep[endpoint].pTransfer = pTransfer;
    
    // If transfer buffer is NULL length must be zero
    if (pTransfer->pvBuffer == NULL)
        pTransfer->cbBuffer = 0;
    
//    OALMSG(OAL_ETHER&&OAL_FUNC, (TEXT("IssueTransfer: ep(%d), length(%d), (%s) max packet(%d)\r\n"), 
//OALMSG(1, (TEXT("IssueTransfer: ep(%d), length(%d), (%s) \r\n"), 
//        endpoint, pTransfer->cbBuffer, (TRANSFER_IS_IN(pTransfer)? TEXT("IN"): TEXT("OUT"))));
//    DEBUGCHK(pTransfer->dwUsbError == UFN_NOT_COMPLETE_ERROR);
    
    if ((endpoint == 0) &&(pPdd->ep0State == EP0_ST_SETUP_PROCESSED))
    {
        SETREG16(&pCsrRegs->ep[endpoint].CSR.CSR0, CSR0_P_SERVICEDRXPKTRDY);
    }
    
    // Depending on direction
    if (TRANSFER_IS_IN(pTransfer))
    {
        pPdd->ep[endpoint].zeroLength =(pTransfer->cbBuffer == 0);
        rc = ContinueTxTransfer(pPdd, endpoint);
    }
    else
    {
        pPdd->ep[endpoint].zeroLength = FALSE;
        rc = StartRxTransfer(pPdd, endpoint);
    }
    
ErrorReturn:
    OALMSG(OAL_ETHER&&OAL_FUNC, (L"-UfnPdd_IssueTransfer\r\n"));
    return rc;
    
    //    return ERROR_SUCCESS;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
//  Function:  UfnPdd_AbortTransfer
//
//  Routine description:
//
//      This routine is called when the system aborts the transferred.
//
//  Arguments:
//
//      pPddContext ::  Handle to HSUSBFNPDD_T
//      endpoint    ::  Endpoint in which the transfer to be aborted
//      pTransfer   ::  Pointer to STransfer passing from MDD, containing buffer to be aborted.
//
//  Return values:
//  
//      ERROR_SUCCESS
//
DWORD WINAPI
UfnPdd_AbortTransfer(
    VOID *pPddContext,
    DWORD endpoint,
    STransfer *pTransfer
)
{
    USBFN_PDD *pPdd = pPddContext;
    PCSP_MUSB_CSR_REGS pCsrRegs = pPdd->pUsbCsrRegs;
    
    OALMSG(OAL_ETHER&&OAL_FUNC, (L"UfnPdd_AbortTransfer ep %d\r\n", endpoint));
    
    if (endpoint == 0)
    {
        SETREG16(&pCsrRegs->ep[endpoint].CSR.CSR0, CSR0_P_FLUSHFIFO);
    }
    else
    {
        USBFN_EP *pEP = &pPdd->ep[endpoint];
        
        if (pEP->dirRx)
            SETREG16(&pCsrRegs->ep[endpoint].RxCSR, RXCSR_P_FLUSHFIFO);        
        else
            SETREG16(&pCsrRegs->ep[endpoint].CSR.TxCSR, TXCSR_P_FLUSHFIFO);        
    }
     
    // Finish transfer
    pPdd->ep[endpoint].pTransfer = NULL;
    if (pTransfer != NULL)
    {
        pTransfer->dwUsbError = UFN_CANCELED_ERROR;
        OALMSG(OAL_ETHER&&OAL_FUNC, (TEXT("UfnPdd_AbortTransfer for EP %d\r\n"), endpoint));
        pPdd->pfnNotify(
           pPdd->pMddContext, UFN_MSG_TRANSFER_COMPLETE, (DWORD)pTransfer
           );
    }
    
    
    // Done
    return ERROR_SUCCESS;
}
//------------------------------------------------------------------------------
//
//  Function:  UfnPdd_StallEndpoint
//
//  Routine description:
//
//      This function is called by MDD to set end point to stall mode (halted).
//
//  Arguments:
//
//      pPddContext ::  Handle to HSUSBFNPDD_T
//      endpoint    ::  The endpoint to which it is requested to stall
//
//  Return values:
//
//      ERROR_SUCCESS
//
DWORD WINAPI
UfnPdd_StallEndpoint(
    VOID *pPddContext,
    DWORD endpoint
)
{
    USBFN_PDD *pPdd = pPddContext;
    USBFN_EP *pEP = &pPdd->ep[endpoint];
    PCSP_MUSB_CSR_REGS pCsrRegs = pPdd->pUsbCsrRegs;
    
    OALMSG(OAL_ETHER&&OAL_FUNC, (L"UfnPdd_StallEndpoint\r\n"));
    
    if (endpoint == 0)
    {
        // Stall next EP0 transaction
        SETREG16(&pCsrRegs->ep[endpoint].CSR.CSR0, CSR0_P_SENDSTALL);        
    }
    else
    {
        
        // stall end point
        if (pEP->dirRx)
            SETREG16(&pCsrRegs->ep[endpoint].RxCSR, RXCSR_P_SENDSTALL);        
        else
            SETREG16(&pCsrRegs->ep[endpoint].CSR.TxCSR, TXCSR_P_SENDSTALL);        
        
    }
    
    // Done
    return ERROR_SUCCESS;
}

//------------------------------------------------------------------------------
//
//  Function:  UfnPdd_ClearEndpointStall
//
//  Routine descriptions:
//
//      This function is called by MDD to clear end point stall mode (halted).
//
//  Arguments:
//
//      pPddContext ::  Handle to HSUSBFNPDD_T
//      endpoint    ::  The endpoint to which it is requested to stall
//
//  Return values:
//
//      ERROR_SUCCESS
//      
DWORD WINAPI
UfnPdd_ClearEndpointStall(
    VOID *pPddContext,
    DWORD endpoint
)
{
    USBFN_PDD *pPdd = pPddContext;
    USBFN_EP *pEP = &pPdd->ep[endpoint];
    PCSP_MUSB_CSR_REGS pCsrRegs = pPdd->pUsbCsrRegs;
    
    OALMSG(OAL_ETHER&&OAL_FUNC, (L"UfnPdd_ClearEndpointStall for EP %d\r\n", endpoint));
    
    if (endpoint == 0)
    {
        CLRREG16(&pCsrRegs->ep[endpoint].CSR.CSR0, CSR0_P_SENTSTALL | CSR0_P_SENDSTALL);        
    }
    else
    {
        
        // stall end point
        if (pEP->dirRx)
        {
            CLRREG16(&pCsrRegs->ep[endpoint].RxCSR, RXCSR_P_SENTSTALL | RXCSR_P_SENDSTALL);        
        }
        else
        {
            CLRREG16(&pCsrRegs->ep[endpoint].CSR.TxCSR, TXCSR_P_SENTSTALL | TXCSR_P_SENDSTALL);        
        }
        
    }
    
    return ERROR_SUCCESS;
}

//------------------------------------------------------------------------------
//
//  Function:  UfnPdd_IsEndpointHalted
//
//  Routine descriptions:
//  
//      Check and see if the endpoint has been stalled.
//
//  Arguments:
//
//      pPddContext :: Handle to HSUSBFNPDD_T
//      endpoint    :: endpoint to check if it is halted.
//      pHalted     :: Pointer to which the status of endpoint to be returned.
//
DWORD WINAPI
UfnPdd_IsEndpointHalted(
    VOID *pPddContext,
    DWORD endpoint,
    BOOL *pHalted
)
{
   
    USBFN_PDD *pPdd = pPddContext;
    USBFN_EP *pEP = &pPdd->ep[endpoint];
    PCSP_MUSB_CSR_REGS pCsrRegs = pPdd->pUsbCsrRegs;
    
    OALMSG(OAL_ETHER&&OAL_FUNC, (L"UfnPdd_IsEndpointHalted\r\n"));
        
    if (endpoint == 0)
    {
        // CSR0_P_SENDSTALL is documented as selfclearing, this might not be sufficient
        *pHalted = INREG16(&pCsrRegs->ep[endpoint].CSR.CSR0) & CSR0_P_SENDSTALL;
    }
    else
    {
        // stall end point
        if (pEP->dirRx)
        {
            *pHalted = INREG16(&pCsrRegs->ep[endpoint].RxCSR) & RXCSR_P_SENDSTALL;
        }
        else
        {
            *pHalted = INREG16(&pCsrRegs->ep[endpoint].CSR.TxCSR) & TXCSR_P_SENDSTALL;
        }
    }
        
    // Done
    return ERROR_SUCCESS;
}

//------------------------------------------------------------------------------
//
//  Function:  UfnPdd_SendControlStatusHandshake
//
//  Routine descriptions:
//
//      This function is the handling of the status phase in Control Transfer. 
//  
//  Arguments:
//  
//      pPddContext ::  Handle of HSUSBFNPDD_T
//      endpoint    ::  the endpoint to be processed.
//
//  Return values:
//
//      ERROR_SUCCESS
//
DWORD WINAPI
UfnPdd_SendControlStatusHandshake(
    VOID *pPddContext,
    DWORD endpoint
)
{
    DWORD rc = ERROR_INVALID_PARAMETER;
    USBFN_PDD *pPdd = pPddContext;
    PCSP_MUSB_CSR_REGS pCsrRegs = pPdd->pUsbCsrRegs;
    
    OALMSG(OAL_ETHER&&OAL_FUNC, (L"+UfnPdd_SendControlStatusHandshake\r\n"));
    
    if (endpoint == 0)
    {
        if (pPdd->ep0State == EP0_ST_SETUP_PROCESSED)
        {
            OALMSG(OAL_ETHER&&OAL_FUNC, (TEXT("\tEP0_ST_SETUP_PROCESSED => EP0_ST_STATUS_IN\r\n")));
            SETREG16(&pCsrRegs->ep[endpoint].CSR.CSR0, CSR0_P_SERVICEDRXPKTRDY | CSR0_P_DATAEND);        
            // Send zero byte to ack
            SETREG16(&pCsrRegs->ep[endpoint].CSR.CSR0, CSR0_P_TXPKTRDY);
            pPdd->ep0State = EP0_ST_STATUS_IN;
        }
        
        if (pPdd->ep0State == EP0_ST_STATUS_OUT)
        {
            OALMSG(OAL_ETHER&&OAL_FUNC, (L"\tEP0_ST_STATUS_OUT => EP0_ST_SETUP\r\n"));
            pPdd->ep0State = EP0_ST_SETUP;
        }
        
        rc = ERROR_SUCCESS;
    }
    else
        OALMSG(OAL_ETHER&&OAL_FUNC, (L"\t!!! UfnPdd_SendControlStatusHandshake called with endpoint %d\r\n", endpoint));
    
    OALMSG(OAL_ETHER&&OAL_FUNC, (L"-UfnPdd_SendControlStatusHandshake\r\n"));
    
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  UfnPdd_InitiateRemoteWakeup
//
DWORD WINAPI UfnPdd_InitiateRemoteWakeup(VOID *pPddContext)
{
	UNREFERENCED_PARAMETER(pPddContext);

    OALMSG(OAL_ETHER&&OAL_FUNC, (L"UsbFnPdd_InitiateRemoteWakeup\r\n"));

    return ERROR_SUCCESS;
}

//------------------------------------------------------------------------------
//
//  Function:  UfnPdd_PowerDown
//
VOID WINAPI UfnPdd_PowerDown(VOID *pPddContext)
{
    /* If CORE goes to OFF during suspend when using KITL over RNDIS, then it doesnt resume
           because this mini version of rndis driver doesnt support complete sequence needed for 
           going to D4 and restoring back to D0. Since supporting CORE OFF for suspend when KITL 
           is over RNDIS is not a practical use case, let USB OTG clocks be enabled during suspend,
           so that we dont have to worry about restoring OTG context on resume */
#if 0    
	UNREFERENCED_PARAMETER(pPddContext);

	if (!EnableUSBClocks(FALSE))
	{
		OALMSG(OAL_ETHER, (TEXT("StartUSBClock : Failed to disable clocks\r\n")));
	}
#else
    USBFN_PDD *pPdd = pPddContext;
    CLRREG16(&pPdd->pUsbOtgRegs->OTG_SYSCONFIG, OTG_SYSCONF_AUTOIDLE);        
    
    // Set the MIDLEMODE to No Stdby
    CLRREG32(&pPdd->pUsbOtgRegs->OTG_SYSCONFIG, OTG_SYSCONF_M_MASK);
    SETREG32(&pPdd->pUsbOtgRegs->OTG_SYSCONFIG, OTG_SYSCONF_M_NOSTDBY);

    // Set the SIDLEMODE to No Idle
    CLRREG32(&pPdd->pUsbOtgRegs->OTG_SYSCONFIG, OTG_SYSCONF_S_MASK);
    SETREG32(&pPdd->pUsbOtgRegs->OTG_SYSCONFIG, OTG_SYSCONF_S_NOIDLE);
    
#endif
}

//------------------------------------------------------------------------------
//
//  Function:  UfnPdd_PowerUp
//
VOID WINAPI UfnPdd_PowerUp(VOID *pPddContext)
{
    /* see note for PowerDown */
#if 0
	UNREFERENCED_PARAMETER(pPddContext);

	if (!EnableUSBClocks(TRUE))
	{
		OALMSG(OAL_ETHER, (TEXT("StartUSBClock : Failed to enable clocks\r\n")));
	}
#else    
    USBFN_PDD *pPdd = pPddContext;
    
    CLRREG16(&pPdd->pUsbOtgRegs->OTG_SYSCONFIG, OTG_SYSCONF_AUTOIDLE);        
    
    // Set the MIDLEMODE to No Stdby
    CLRREG32(&pPdd->pUsbOtgRegs->OTG_SYSCONFIG, OTG_SYSCONF_M_MASK);
    SETREG32(&pPdd->pUsbOtgRegs->OTG_SYSCONFIG, OTG_SYSCONF_M_SMARTSTDBY);

    // Set the SIDLEMODE to No Idle
    CLRREG32(&pPdd->pUsbOtgRegs->OTG_SYSCONFIG, OTG_SYSCONF_S_MASK);
    SETREG32(&pPdd->pUsbOtgRegs->OTG_SYSCONFIG, OTG_SYSCONF_S_SMARTIDLE);
    
    SETREG16(&pPdd->pUsbOtgRegs->OTG_SYSCONFIG, OTG_SYSCONF_AUTOIDLE);        
#endif
}

//------------------------------------------------------------------------------
//
//  Function:  UfnPdd_IOControl
//
DWORD WINAPI UfnPdd_IOControl(
    VOID *pPddContext, IOCTL_SOURCE source, DWORD code, UCHAR *pInBuffer,
    DWORD inSize, UCHAR *pOutBuffer, DWORD outSize, DWORD *pOutSize
) {
    DWORD rc = ERROR_INVALID_PARAMETER;
    UFN_PDD_INFO *pInfo;

	UNREFERENCED_PARAMETER(pPddContext);
	UNREFERENCED_PARAMETER(pOutSize);
	UNREFERENCED_PARAMETER(inSize);
	UNREFERENCED_PARAMETER(pInBuffer);

    switch (code) {
    case IOCTL_UFN_GET_PDD_INFO:
        if (source != BUS_IOCTL) break;
        if (pOutBuffer == NULL || outSize < sizeof(UFN_PDD_INFO)) break;
        pInfo = (UFN_PDD_INFO*)pOutBuffer;
        pInfo->InterfaceType = Internal;
        pInfo->BusNumber = 0;
        pInfo->dwAlignment = sizeof(DWORD);
        rc = ERROR_SUCCESS;
        break;
    case IOCTL_BUS_GET_POWER_STATE:
        break;

    case IOCTL_BUS_SET_POWER_STATE:
        break;
    }
    
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  UfnPdd_Deinit
//
DWORD WINAPI UfnPdd_Deinit(VOID *pPddContext)
{
    USBFN_PDD *pPdd = pPddContext;

    // Unmap USB controller registers
    if (pPdd->pUsbGenRegs != NULL) {
        pPdd->pUsbGenRegs = NULL;
    }
    
    if (pPdd->pUsbCsrRegs != NULL) {
        pPdd->pUsbCsrRegs = NULL;
    }

    if (pPdd->pUsbOtgRegs != NULL) {
        pPdd->pUsbOtgRegs = NULL;
    }

    // Done
    return ERROR_SUCCESS;
}

//------------------------------------------------------------------------------
//
//  Function:  UfnPdd_DeregisterDevice
//
//  This function is called by MDD to move device to pre-registered state.
//
DWORD WINAPI UfnPdd_DeregisterDevice(VOID *pPddContext)
{
    // Should shut down the endpoint...
    // This function is not currently called

	UNREFERENCED_PARAMETER(pPddContext);

    return ERROR_SUCCESS;
}


//------------------------------------------------------------------------------
//
//  Function:  UfnPdd_Stop
//
//  This function is called before UfnPdd_DeregisterDevice. It should de-attach
//  device to USB bus (but we don't want disable interrupts because...)
//
DWORD WINAPI UfnPdd_Stop(VOID *pPddContext)
{
    USBFN_PDD *pPdd = pPddContext;

    OALMSG(OAL_ETHER&&OAL_FUNC, (L"UfnPdd_Stop\r\n"));

    // Disconnect hardware
    if ((INREG8(&pPdd->pUsbGenRegs->DevCtl) & DEVCTL_SESSION) == DEVCTL_SESSION)
        CLRREG8(&pPdd->pUsbGenRegs->DevCtl, DEVCTL_SESSION);
    CLRREG8(&pPdd->pUsbGenRegs->Power, POWER_SOFTCONN);        
    DisconnectHardware();

    // Done
    return ERROR_SUCCESS;
}

//------------------------------------------------------------------------------
//
//  Function:  UfnPdd_DeinitEndpoint
//
//  This function is called when pipe to endpoint is closed. 
//
DWORD WINAPI UfnPdd_DeinitEndpoint(VOID *pPddContext, DWORD endPoint)
{
	UNREFERENCED_PARAMETER(pPddContext);
	UNREFERENCED_PARAMETER(endPoint);

    // This function is not currently called
    return ERROR_SUCCESS;
}


//------------------------------------------------------------------------------
//
//  Function:  UfnPdd_InitEndpoint
//
//  Routine descriptions:
//
//      This function is called when pipe to endpoint is created. By default, control endpoint
//  (endpoint 0) would be created during the driver is loaded. Other endpoints would be created
//  during the connection.
//
//  NOTE - This function is not called in the current implementation.  All endpoints are 
// initialized during RegisterDevice
//
//  Arguments:
//  
//      pPddContext ::  Handle of HSUSBFNPDD_T
//      endpoint    ::  endpoint to be initialize
//      speed       ::  connection speed
//      pEPDesc     ::  pointer to endpoint descriptor
//      pReserved   ::  not used.
//      configValue ::  not used.
//      interfaceNumber     ::  not used
//      alternateSetting    ::  not used.
//
//  Return values:
//
//      ERROR_SUCCESS
//
DWORD WINAPI
UfnPdd_InitEndpoint(
    VOID *pPddContext,
    DWORD endpoint,
    UFN_BUS_SPEED speed,
    USB_ENDPOINT_DESCRIPTOR *pEPDesc,
    VOID *pReserved,
    UCHAR configValue,
    UCHAR interfaceNumber,
    UCHAR alternateSetting
)
{
	UNREFERENCED_PARAMETER(pPddContext);
	UNREFERENCED_PARAMETER(endpoint);
	UNREFERENCED_PARAMETER(speed);
	UNREFERENCED_PARAMETER(pEPDesc);
	UNREFERENCED_PARAMETER(pReserved);
	UNREFERENCED_PARAMETER(configValue);
	UNREFERENCED_PARAMETER(interfaceNumber);
	UNREFERENCED_PARAMETER(alternateSetting);

    return ERROR_SUCCESS;
}

//------------------------------------------------------------------------------
//
//  Function:  UfnPdd_SetAddress
//
//  Routine descriptions:
//
//      This function is called by MDD when configuration process assigned address
//  to device. In this case, set the FADDR register when EP0 state machine transitions to proper state.
//
//  Arguments:
//  
//      pPddContext ::  Handle of HSUSBFNPDD_T
//      address     ::  Device address to be set when receiving from host.
//
//  Return values:
//
//      ERROR_SUCCESS
//
DWORD WINAPI
UfnPdd_SetAddress(
    VOID *pPddContext,
    UCHAR address
)
{
    USBFN_PDD *pPdd = pPddContext;
    OALMSG(OAL_ETHER&&OAL_FUNC, (L"UfnPdd_SetAddress: (%d)\r\n", address));
    
    pPdd->bSetAddress = TRUE;
    pPdd->ucAddress = address;
    
    return ERROR_SUCCESS;
}

//------------------------------------------------------------------------------
//
//  Function:  UfnPdd_Start
//
//  This function is called after UfnPdd_RegisterDevice. It should attach
//  device to USB bus.
//
DWORD WINAPI UfnPdd_Start(VOID *pPddContext)
{
    USBFN_PDD *pPdd = pPddContext;

    OALMSG(OAL_ETHER&&OAL_FUNC, (L"+USBFN:: UfnPdd_Start\r\n"));

    // Disconnect hardware
    if ((INREG8(&pPdd->pUsbGenRegs->DevCtl) & DEVCTL_SESSION) == DEVCTL_SESSION)
        CLRREG8(&pPdd->pUsbGenRegs->DevCtl, DEVCTL_SESSION);

    CLRREG8(&pPdd->pUsbGenRegs->Power, POWER_SOFTCONN);        
    DisconnectHardware();

    // Wait for while
    OALStall(10000);

	ConnectHardware();

    OUTREG8(&pPdd->pUsbGenRegs->IntrUSBE, 0);
	OUTREG16(&pPdd->pUsbGenRegs->IntrRxE, 0xFFFC);
	OUTREG16(&pPdd->pUsbGenRegs->IntrTxE, 0xFFFC);
	OUTREG8(&pPdd->pUsbGenRegs->Testmode, 0);
    OUTREG8(&pPdd->pUsbGenRegs->FAddr, 0); 

    // Wait for while
    OALStall(10000);

    // Attach device to bus
	SETREG8(&pPdd->pUsbGenRegs->DevCtl, DEVCTL_SESSION);

    // Done
    return ERROR_SUCCESS;
}

//------------------------------------------------------------------------------
//
//  Function:  UfnPdd_RegisterDevice
//
//  Routine descriptions:
//
//      This function is called by MDD after device configuration was sucessfully
//  verified by UfnPdd_IsEndpointSupportable and
//  UfnPdd_IsConfigurationSupportable. It should initialize hardware for given
//  configuration. Depending on hardware endpoints can be initialized later in
//  UfnPdd_InitEndpoint. 
//
//  Arguments:
//      pPddContext - Handle of HSUSBFNPDD_T
//      pHighSpeedDeviceDesc - Pointer to High Speed USB Device descriptor
//      pHighSpeedConfig - Pointer to USB Configuration
//      pFullSpeedDeviceDesc - Pointer to Full Speed USB Device descriptor
//      pFullSpeedConfig - Pointer to USB Configuration
//      pFullSpeedConfigDesc - Pointer to USB Configuration descriptor
//  
//  Return:
//    ERROR_SUCCESS
//
DWORD WINAPI
UfnPdd_RegisterDevice(
    VOID *pPddContext,
    const USB_DEVICE_DESCRIPTOR *pHighSpeedDeviceDesc,
    const UFN_CONFIGURATION *pHighSpeedConfig,
    const USB_CONFIGURATION_DESCRIPTOR *pHighSpeedConfigDesc,
    const USB_DEVICE_DESCRIPTOR *pFullSpeedDeviceDesc,
    const UFN_CONFIGURATION *pFullSpeedConfig,
    const USB_CONFIGURATION_DESCRIPTOR *pFullSpeedConfigDesc,
    const UFN_STRING_SET *pStringSets,
    DWORD stringSets
)
{
    // NOTE: InitEndpoints is not called in this implementation.  All configuration must
    // be done in this function.  Only the full speed configuration is used.
        
    USBFN_PDD *pPdd = pPddContext;
    PCSP_MUSB_GEN_REGS pGenRegs = pPdd->pUsbGenRegs;
    PCSP_MUSB_CSR_REGS pCsrRegs = pPdd->pUsbCsrRegs;
    UFN_INTERFACE *pIFC;
    UFN_ENDPOINT *pEP;
    DWORD endpoint;
    DWORD ifc, epx;
    
	UNREFERENCED_PARAMETER(stringSets);
	UNREFERENCED_PARAMETER(pStringSets);
	UNREFERENCED_PARAMETER(pFullSpeedConfigDesc);
	UNREFERENCED_PARAMETER(pHighSpeedConfigDesc);
	UNREFERENCED_PARAMETER(pHighSpeedConfig);
	UNREFERENCED_PARAMETER(pHighSpeedDeviceDesc);

    OALMSG(OAL_ETHER&&OAL_FUNC, (L"UsbFnPdd_RegisterDevice\r\n"));
    
    // Remember self powered flag
    pPdd->selfPowered =(pFullSpeedConfig->Descriptor.bmAttributes & 0x20) != 0;
    
    pPdd->ep[EP0].maxPacketSize = pFullSpeedDeviceDesc->bMaxPacketSize0;
    
    // Configure EP0
    OUTREG8(&pGenRegs->Index, 0);  /* select endpoint to access FIFO registers */
    
    OUTREG8(&pGenRegs->RxFIFOsz, MUSB_FIFOSZ_128_BYTE);
    OUTREG16(&pGenRegs->RxFIFOadd, 0);
    
    OUTREG8(&pGenRegs->TxFIFOsz, MUSB_FIFOSZ_128_BYTE);
    OUTREG16(&pGenRegs->TxFIFOadd, 256/8);
                
    OALMSG(USBFN_DEBUG, (L"Setup endpoint\r\n"));   
    OALMSG(USBFN_DEBUG, (L"RxFIFOsz=%d\r\n", INREG8(&pGenRegs->RxFIFOsz)));
    OALMSG(USBFN_DEBUG, (L"RxFIFOadd=%d\r\n", INREG16(&pGenRegs->RxFIFOadd)));

    // Configure Rx EPs
    for (ifc = 0; ifc < pFullSpeedConfig->Descriptor.bNumInterfaces; ifc++)
    {
        // For each endpoint in interface
        pIFC = &pFullSpeedConfig->pInterfaces[ifc];
        for (epx = 0; epx < pIFC->Descriptor.bNumEndpoints; epx++)
        {
            pEP = &pIFC->pEndpoints[epx];
            // If it is Tx EP skip it
            if ((pEP->Descriptor.bEndpointAddress & 0x80) != 0)
                continue;
            // Get EP address
            endpoint = pEP->Descriptor.bEndpointAddress & 0x0F;
            // Save max packet size & direction
            pPdd->ep[endpoint].maxPacketSize = pEP->Descriptor.wMaxPacketSize;
            pPdd->ep[endpoint].dirRx = TRUE;
            
            OUTREG16(&pGenRegs->Index, endpoint);  /* select endpoint to access FIFO registers */
            
            // configure endpoint based on type
            switch(pEP->Descriptor.bmAttributes & 0x03)
            {
            case EP_TYPE_INTERRUPT:
                OALMSG(USBFN_DEBUG, (L"Interrupt endpoint\r\n"));
                OUTREG8(&pGenRegs->RxFIFOsz, MUSB_FIFOSZ_512_BYTE);
                OUTREG16(&pGenRegs->RxFIFOadd, CalcFIFOAddr(endpoint,pPdd->ep[endpoint].dirRx));
                OUTREG16(&pCsrRegs->ep[endpoint].RxMaxP, pPdd->ep[endpoint].maxPacketSize);
                OUTREG16(&pCsrRegs->ep[endpoint].RxCSR, RXCSR_P_FLUSHFIFO | RXCSR_P_CLRDATATOG);
                
                OALMSG(USBFN_DEBUG, (L"RxFIFOsz=%d\r\n", INREG8(&pGenRegs->RxFIFOsz)));
                OALMSG(USBFN_DEBUG, (L"RxFIFOadd=%d\r\n", INREG16(&pGenRegs->RxFIFOadd)));
                
                // Doesn't support PING in high speed mode
                // doesn't hurt to set in any speed mode
                SETREG16(&pCsrRegs->ep[endpoint].RxCSR, RXCSR_P_DISNYET);
                break;
                
            case EP_TYPE_BULK:
                OALMSG(USBFN_DEBUG, (L"Bulk endpoint\r\n"));
                OUTREG8(&pGenRegs->RxFIFOsz, MUSB_FIFOSZ_512_BYTE);
                OUTREG16(&pGenRegs->RxFIFOadd, CalcFIFOAddr(endpoint,pPdd->ep[endpoint].dirRx));
                OUTREG16(&pCsrRegs->ep[endpoint].RxMaxP, pPdd->ep[endpoint].maxPacketSize);
                OUTREG16(&pCsrRegs->ep[endpoint].RxCSR, RXCSR_P_FLUSHFIFO | RXCSR_P_CLRDATATOG);
                
                OALMSG(USBFN_DEBUG, (L"RxFIFOsz=%d\r\n", INREG8(&pGenRegs->RxFIFOsz)));
                OALMSG(USBFN_DEBUG, (L"RxFIFOadd=%d\r\n", INREG16(&pGenRegs->RxFIFOadd)));
                break;
            case EP_TYPE_CTRL:
            case EP_TYPE_ISOCH:
            default:
                OALMSG(OAL_ERROR, (L"UfnPdd_RegisterDevice: Unsupported endpoint Transfer type\r\n"));
                break;
            }
        }
    }
    
    // Configure Tx EPs
    for (ifc = 0; ifc < pFullSpeedConfig->Descriptor.bNumInterfaces; ifc++)
    {
        // For each endpoint in interface
        pIFC = &pFullSpeedConfig->pInterfaces[ifc];
        for (epx = 0; epx < pIFC->Descriptor.bNumEndpoints; epx++)
        {
            pEP = &pIFC->pEndpoints[epx];
            // If it is Rx EP skip it
            if ((pEP->Descriptor.bEndpointAddress & 0x80) == 0)
                continue;
            // Get EP address
            endpoint = pEP->Descriptor.bEndpointAddress & 0x0F;
            // Save max packet size & direction
            pPdd->ep[endpoint].maxPacketSize = pEP->Descriptor.wMaxPacketSize;
            pPdd->ep[endpoint].dirRx = FALSE;
            
            OUTREG16(&pGenRegs->Index, endpoint);  /* select endpoint to access rxFIFOxx registers */
            
            // configure endpoint based on type
            switch(pEP->Descriptor.bmAttributes & 0x03)
            {
            case EP_TYPE_INTERRUPT:
                OALMSG(USBFN_DEBUG, (L"Interrupt endpoint\r\n"));
                OUTREG8(&pGenRegs->TxFIFOsz, MUSB_FIFOSZ_512_BYTE);
                OUTREG16(&pGenRegs->TxFIFOadd, CalcFIFOAddr(endpoint,pPdd->ep[endpoint].dirRx));
                OUTREG16(&pCsrRegs->ep[endpoint].TxMaxP, pPdd->ep[endpoint].maxPacketSize);
                OUTREG16(&pCsrRegs->ep[endpoint].CSR.TxCSR, TXCSR_P_FLUSHFIFO | TXCSR_P_CLRDATATOG | TXCSR_P_FRCDATATOG);
                
                OALMSG(USBFN_DEBUG, (L"TxFIFOsz=%d\r\n", INREG8(&pGenRegs->TxFIFOsz)));
                OALMSG(USBFN_DEBUG, (L"TxFIFOadd=%d\r\n", INREG16(&pGenRegs->TxFIFOadd)));
                break;
            case EP_TYPE_BULK:
                OALMSG(USBFN_DEBUG, (L"Bulk endpoint\r\n"));
                OUTREG8(&pGenRegs->TxFIFOsz, MUSB_FIFOSZ_512_BYTE);
                OUTREG16(&pGenRegs->TxFIFOadd, CalcFIFOAddr(endpoint,pPdd->ep[endpoint].dirRx));
                OUTREG16(&pCsrRegs->ep[endpoint].TxMaxP, pPdd->ep[endpoint].maxPacketSize);
                OUTREG16(&pCsrRegs->ep[endpoint].CSR.TxCSR, TXCSR_P_FLUSHFIFO | TXCSR_P_CLRDATATOG);
                
                OALMSG(USBFN_DEBUG, (L"TxFIFOsz=%d\r\n", INREG8(&pGenRegs->TxFIFOsz)));
                OALMSG(USBFN_DEBUG, (L"TxFIFOadd=%d\r\n", INREG16(&pGenRegs->TxFIFOadd)));
                break;
            case EP_TYPE_CTRL:
            case EP_TYPE_ISOCH:
            default:
                OALMSG(OAL_ERROR, (L"UfnPdd_RegisterDevice: Unsupported endpoint Transfer type\r\n"));
                break;
            }
        }
    }

    // Done
    return ERROR_SUCCESS;
}

//------------------------------------------------------------------------------
//
//  Function:  UfnPdd_IsEndpointSupportable
//
//  Routine descriptions:
//
//      This function is called by MDD to verify if EP can be supported on
//  hardware. It is called after UfnPdd_IsConfigurationSupportable. We must
//  verify configuration in this function, so we already know that EPs
//  are valid. Only information we can update there is maximal packet
//  size for EP0.
//
//  Arguments:
//
//      pPddContext ::  Handle of HSUSBFNPDD_T
//      endpoint    ::  Endpoint to be queried
//      speed       ::  connection speed
//      pEPDesc     ::  pointer to endpoint descriptor
//      configurationValue  ::
//      interfaceNumber     ::
//      alternateSetting    ::
//
//  Return values:
//
//      ERROR_SUCCESS
//
DWORD WINAPI UfnPdd_IsEndpointSupportable(
    VOID *pPddContext, DWORD endPoint, UFN_BUS_SPEED speed,
    USB_ENDPOINT_DESCRIPTOR *pEPDesc, UCHAR configurationValue,
    UCHAR interfaceNumber, UCHAR alternateSetting
) {
	UNREFERENCED_PARAMETER(pPddContext);
	UNREFERENCED_PARAMETER(alternateSetting);
	UNREFERENCED_PARAMETER(interfaceNumber);
	UNREFERENCED_PARAMETER(configurationValue);
	UNREFERENCED_PARAMETER(speed);

    // Update maximal packet size for EP0
    if (endPoint == 0) {
//        DEBUGCHK(pEPDesc->wMaxPacketSize <= 64);
//        DEBUGCHK(pEPDesc->bmAttributes == USB_ENDPOINT_TYPE_CONTROL);
        pEPDesc->wMaxPacketSize = 64;

    }

    // Done
    return ERROR_SUCCESS;
}

//------------------------------------------------------------------------------
//
//  Function:  UfnPdd_IsConfigurationSupportable
//
//  Routine Descriptions:
//
//      This function is called before UfnPdd_RegisterDevice. It should verify
//  that USB device configuration can be supported on hardware. Function can
//  modify EP size and/or EP address.
//
//  For INVENTRA MUSB we should check if total descriptor size is smaller
/// than 1024 bytes. Unfortunately we don't get information
//  about EP0 max packet size. So we will assume maximal 64 byte size.
//
//  Arguments:
//
//      pPddContext ::  Handle of HSUSBFNPDD_T
//      speed       ::  connection speed
//      pConfig     ::  device configuration
//
//  Return values:
//
//      ERROR_SUCCESS
//
DWORD WINAPI
UfnPdd_IsConfigurationSupportable(
    VOID *pPddContext,
    UFN_BUS_SPEED speed,
    UFN_CONFIGURATION *pConfig
)
{
    DWORD rc = ERROR_INVALID_PARAMETER;
    UFN_INTERFACE *pIFC;
    UFN_ENDPOINT *pEP;
    WORD ifc, epx, count;
    WORD size;

	UNREFERENCED_PARAMETER(pPddContext);
	UNREFERENCED_PARAMETER(speed);

    OALMSG(OAL_ETHER&&OAL_FUNC, (L"UfnPdd_IsConfigurationSupportable\r\n"));
    
    // Clear number of end points
    count = 0;
    
    // For each interface in configuration
    for (ifc = 0; ifc < pConfig->Descriptor.bNumInterfaces; ifc++)
    {
        // For each endpoint in interface
        pIFC = &pConfig->pInterfaces[ifc];
        for (epx = 0; epx < pIFC->Descriptor.bNumEndpoints; epx++)
        {
            pEP = &pIFC->pEndpoints[epx];
            
            if (pEP->Descriptor.wMaxPacketSize > MAX_EPX_PKTSIZE) 
            {
                size = MAX_EPX_PKTSIZE;
            }
            else
            {
                size = pEP->Descriptor.wMaxPacketSize;
            }
            
            
            // Is it ISO end point?
            if ((pEP->Descriptor.bmAttributes & 0x03) == 0x01)
            {
                // Actual driver doesn't support ISO endpoints
                goto cleanUp;
            }
            // Update EP size
            pEP->Descriptor.wMaxPacketSize = size;
        }
        // Add number of end points to total count
        count = count + pIFC->Descriptor.bNumEndpoints;
    }
    
    // Can we support this configuration?
    if (count < USBD_EP_COUNT)
        rc = ERROR_SUCCESS;
    
cleanUp:
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  UfnPdd_Init
//
//  This function is called by MDD on driver load. It should reset driver,
//  fill PDD interface structure. It can also get SYSINTR, initialize interrupt
//  thread and start interrupt thread. It must not attach device to USB bus.
//
DWORD WINAPI UfnPdd_Init(
    LPCTSTR szActiveKey, VOID *pMddContext, UFN_MDD_INTERFACE_INFO *pMddIfc,
    UFN_PDD_INTERFACE_INFO *pPddIfc) 
{
    DWORD rc;
    USBFN_PDD *pPdd;

	UNREFERENCED_PARAMETER(szActiveKey);

    OALMSG(OAL_ETHER&&OAL_FUNC, (L"+USBFN:: PDD Init\r\n"));
    
    rc = ERROR_INVALID_PARAMETER;
    
    // Allocate and initialize the OHCD object.
    pPdd = &g_usbfnpdd;

    if (pPdd == NULL) goto clean;

    // Clear the allocated object.
    memset(pPdd, 0, sizeof(USBFN_PDD));

    // Map the USB registers
    pPdd->pUsbGenRegs = (PCSP_MUSB_GEN_REGS)OALPAtoUA( SOCGetUSBOTGAddress() + OMAP_MUSB_GEN_OFFSET );
    if (pPdd->pUsbGenRegs == NULL) {
        OALMSG(OAL_ERROR, (
            L"ERROR: UfnPdd_Init: Controller registers mapping failed\r\n"
        ));
        goto clean;
    }
    
    pPdd->pUsbCsrRegs = (PCSP_MUSB_CSR_REGS)OALPAtoUA( SOCGetUSBOTGAddress() + OMAP_MUSB_CSR_OFFSET );
    if (pPdd->pUsbCsrRegs == NULL) {
        OALMSG(OAL_ERROR, (
            L"ERROR: UfnPdd_Init: Controller registers mapping failed\r\n"
        ));
        goto clean;
    }
    
    pPdd->pUsbOtgRegs = (PCSP_MUSB_OTG_REGS)OALPAtoUA( SOCGetUSBOTGAddress() + OMAP_MUSB_OTG_OFFSET );
    if (pPdd->pUsbOtgRegs == NULL) {
        OALMSG(OAL_ERROR, (
            L"ERROR: UfnPdd_Init: Controller registers mapping failed\r\n"
        ));
        goto clean;
    }
    
    pPdd->devState = 0;
    
    // Enable USBHS clocks
    StartUSBClock(pPdd);
    
    // Set PDD interface
    pPddIfc->dwVersion = UFN_PDD_INTERFACE_VERSION;
    pPddIfc->dwCapabilities = UFN_PDD_CAPS_SUPPORTS_FULL_SPEED;
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

    // Save MDD context & notify function
    pPdd->pMddContext = pMddContext;
    pPdd->pfnNotify = pMddIfc->pfnNotify;
    CLRREG32(&pPdd->pUsbOtgRegs->OTG_FORCESTDBY, OTG_FORCESTDY_ENABLEFORCE);

    // Done
    rc = ERROR_SUCCESS;
clean:
    OALMSG(OAL_ETHER&&OAL_FUNC, (L"-USBFN:: PDD Init\r\n"));
    return rc;
}

//------------------------------------------------------------------------------

extern BOOL UfnPdd_DllEntry(
    HANDLE hDllHandle, DWORD reason, VOID *pReserved
) {
	UNREFERENCED_PARAMETER(hDllHandle);
	UNREFERENCED_PARAMETER(reason);
	UNREFERENCED_PARAMETER(pReserved);

    return TRUE;
}

#pragma warning (pop)
#pragma optimize ( "", on )


