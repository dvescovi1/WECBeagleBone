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
//  File: omap_hecc.cpp
//
//  This file implements device driver for CAN. 
//
#include "omap.h"
#include "ceddkex.h"
#include "soc_cfg.h"
#include "sdk_gpio.h"
#include "sdk_can.h"
#include "sdk_padcfg.h"
#include "oal_clock.h"
#define HECC
#include "omap_hecc_scc_regs.h"

#pragma warning(push)
#pragma warning(disable:4127)
#include <linklist.h>
#pragma warning(pop)
#include <cmnintrin.h>
#include <nkintr.h>

#define CANGIx_OTHER_INTERRUPTS (CANGIx_AAIM | CANGIx_WDIM | CANGIx_WUIM | CANGIx_RMLIM)

#define TX_MAILBOX2 (NB_MAILBOXES-11)
#define TX_MAILBOX1 (NB_MAILBOXES-12)
#define TX_MAILBOX0 (NB_MAILBOXES-13)

#define LAST_TX_MAILBOX TX_MAILBOX2
#define FIRST_TX_MAILBOX TX_MAILBOX0

static BYTE PriorityMapping[NB_TX_PRIORITIES] =
{
    3,  //TPL value for CRITICAL 
    2,  //TPL value for MEDIUM
    1   //TPL value for LOW
};
static const DWORD RxPriorityMlbxCount[NB_RX_PRIORITIES] =
{
    3,  //Critical Priority is assigned 3 mailboxes
    2,  //Medium Priority is assigned 2 mailboxes
    1  //Low Priority is assigned 1 mailbox
};

#define NB_MAX_CLASS_FILTER (32)


BOOL CANInitHardware(void *ctxt);
BOOL CANCommand(void* ctxt,CAN_COMMAND cmd);
BOOL CANConfigureBaudrate(void* ctxt,DWORD baudrate);
BOOL CANStatus(void* ctxt,CAN_STATUS* pStatus);
BOOL CANSend(void* ctxt,CAN_MESSAGE* msg,DWORD nbMsg,DWORD* nbMsgSent,TXCAN_PRIORITY priority,DWORD timeout);
BOOL CANReceive(void* ctxt,CAN_MESSAGE* msg,DWORD nbMaxMsg,DWORD* nbMsgReceived,DWORD timeout);


BOOL CANCreateClassFilter(void* ctxt,CLASS_FILTER* classFilter,RXCAN_PRIORITY priority ,BOOL fEnabled);
BOOL CANDeleteClassFilter(void* ctxt,CLASS_FILTER* classFilter);
BOOL CANEnableClassFilter(void* ctxt,CLASS_FILTER* classFilter, BOOL fEnabled);

BOOL CANAddSubClassFilter(void* ctxt,SUBCLASS_FILTER* subclassFilter);
BOOL CANRemoveSubClassFilter(void* ctxt,SUBCLASS_FILTER* subclassFilter);


BOOL CANConfigureAutoAnswer(void* ctxt,CAN_MESSAGE* msg, BOOL fEnabled);
BOOL CANRemoteRequest(void* ctxt, CAN_REMOTE_REQUEST* pRemoteRequest);

#define DEFAULT_THREAD_PRIORITY (250)

typedef struct _SLE{
    struct _SLE *pNext;
} SLE, *PSLE;

static _inline PSLE NextElement(PSLE pSle)
{
    return pSle->pNext;
}

static void PushElement(PSLE* head,PSLE pSle)
{
    PSLE current;
    PSLE previous;
    if (*head == NULL)
    {
        *head = pSle;
        return;
    }
    current = *head;
    do 
    {
        previous = current;
        current = NextElement(current);
    }
    while (current);
    pSle->pNext = NULL;
    previous->pNext = pSle;
}

static void RemoveElement(PSLE* head,PSLE pSle)
{
    PSLE current;
    PSLE next;
    if (*head == NULL)
    {        
        return;
    }
    current = *head;
    if (current == pSle)
    {
        *head = current->pNext;
        return;
    }
    while (current)
    {
        next = NextElement(current);
        if (next == pSle)
        {
            current->pNext = next->pNext;
            break;
        }
        current = next;
    }    
}


//------------------------------------------------------------------------------
//
//  Global:  dpCurSettings
//
//  Set debug zones names and initial setting for driver
//
#ifndef SHIP_BUILD

#undef ZONE_ERROR
#undef ZONE_WARN
#undef ZONE_FUNCTION
#undef ZONE_INFO

#define ZONE_ERROR          DEBUGZONE(0)
#define ZONE_WARN           DEBUGZONE(1)
#define ZONE_FUNCTION       DEBUGZONE(2)
#define ZONE_INFO           DEBUGZONE(3)
#define ZONE_IST            DEBUGZONE(4)

//------------------------------------------------------------------------------
//
//  Global:  dpCurSettings
//
DBGPARAM dpCurSettings = {
    L"CAN", {
        L"Errors",      L"Warnings",    L"Function",    L"Info",
        L"IST",         L"Undefined",   L"Undefined",   L"Undefined",
        L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined",
        L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined"
    },
    0x0003
};

#endif

typedef struct {
    SLE sle;
    CAN_ID id;
    CAN_ID mask;
    BOOL enabled;
}SUBCLASS_FILTER_INTERNAL;

typedef struct {
    SLE sle;
    CLASS_FILTER classFilter;
    SUBCLASS_FILTER_INTERNAL* subFilter;    
    BOOL enabled;
    DWORD mailboxMask;
} CLASS_FILTER_INTERNAL;

//------------------------------------------------------------------------------
//  Local Structures

typedef struct {
    DWORD dwSysintr;
    HANDLE hIntrEvent;
    HANDLE hIntrThread;
    BOOL fTerminateThread;

    DWORD ThreadPriority;    
    DWORD CanCtrlIndex;
    DWORD StrobePin;
    DWORD StrobePinPolarity;
    DWORD BaudRate;

    HANDLE hGpio;    
    OMAP_DEVICE device;
    OMAP_HECC_SCC_REGS *pCanRegs;    
    CRITICAL_SECTION regcs,filterscs;
        
    HANDLE RxMsgQ[2];
    HANDLE TxMsgQ[NB_TX_PRIORITIES][2];
        
    CAN_STATUS  canStatus;
    DWORD TxMailBoxAvailableMask;
    DWORD TxMailBoxMask;
    DWORD FreeMailboxMask;
    DWORD RxMailboxMask;
    DWORD AutoAnswerMask;
    DWORD RemoteRequestMask;
    HANDLE hRTREvent;

    CLASS_FILTER_INTERNAL* ClassFilterList;
    DWORD nbFreeClassFilter;
    
} T_CAN_DEVICE;

typedef struct {
    T_CAN_DEVICE* pDevice;
}T_CAN_OPEN_CTXT;

#define DFLT_LOW_TX_MAX_ELEM        (32)
#define DFLT_MEDIUM_TX_MAX_ELEM      (32)
#define DFLT_CRITICAL_TX_MAX_ELEM   (32)
#define DFLT_RX_MAX_ELEM            (256)

#define MSQ_READER  (0)
#define MSQ_WRITER  (1)

//------------------------------------------------------------------------------

static const DEVICE_REGISTRY_PARAM s_deviceRegParams[] = {
    {
        L"CanCtrlIndex", PARAM_DWORD, TRUE, offset(T_CAN_DEVICE, CanCtrlIndex),
            fieldsize(T_CAN_DEVICE, CanCtrlIndex), (VOID*)0
    }, {
        L"BaudRate", PARAM_DWORD, TRUE, offset(T_CAN_DEVICE, BaudRate),
            fieldsize(T_CAN_DEVICE, BaudRate), (VOID*)-1
    }, {
        L"StrobePin", PARAM_DWORD, FALSE, offset(T_CAN_DEVICE, StrobePin),
            fieldsize(T_CAN_DEVICE, StrobePin), (VOID*)-1
    },  {
        L"StrobePinPolarity", PARAM_DWORD, FALSE, offset(T_CAN_DEVICE, StrobePinPolarity),
            fieldsize(T_CAN_DEVICE, StrobePinPolarity), (VOID*)-1
    }, {
        L"Priority", PARAM_DWORD, FALSE, offset(T_CAN_DEVICE, ThreadPriority),
            fieldsize(T_CAN_DEVICE, ThreadPriority), (VOID*)DEFAULT_THREAD_PRIORITY
    },{
        L"maxRxMsg", PARAM_DWORD, FALSE, offset(T_CAN_DEVICE, canStatus.maxRxMsg),
            fieldsize(T_CAN_DEVICE, canStatus.maxRxMsg), (VOID*)DFLT_RX_MAX_ELEM
    },{
        L"TxLowMaxElem", PARAM_DWORD, FALSE, offset(T_CAN_DEVICE, canStatus.maxTxMsg[TXLOW]),
            fieldsize(T_CAN_DEVICE, canStatus.maxTxMsg[TXLOW]), (VOID*)DFLT_LOW_TX_MAX_ELEM
    }, {
        L"TxMediumMaxElem", PARAM_DWORD, FALSE, offset(T_CAN_DEVICE, canStatus.maxTxMsg[TXMEDIUM]),
            fieldsize(T_CAN_DEVICE, canStatus.maxTxMsg[TXMEDIUM]), (VOID*)DFLT_MEDIUM_TX_MAX_ELEM
    }, {
        L"TxCriticalMaxElem", PARAM_DWORD, FALSE, offset(T_CAN_DEVICE, canStatus.maxTxMsg[TXCRITICAL]),
            fieldsize(T_CAN_DEVICE, canStatus.maxTxMsg[TXCRITICAL]), (VOID*)DFLT_CRITICAL_TX_MAX_ELEM
    }, 
};


#define ACTIVE_LOW (0)


BOOL CAN_Deinit(DWORD context);
DWORD CANIntrThread(VOID *pContext);
BOOL CANisIDAccepted(CAN_ID id,SUBCLASS_FILTER_INTERNAL* pList);
void HandleRxOverrun(T_CAN_DEVICE *pDevice);
void HandleBusOffInterrupt(T_CAN_DEVICE *pDevice);
void HandleErrorPassiveInterrupt(T_CAN_DEVICE *pDevice);
void HandleWarningLevelInterrupt(T_CAN_DEVICE *pDevice);
DWORD HandleOtherInterrupts(T_CAN_DEVICE *pDevice);

static BOOL IDequal(CAN_ID id1, CAN_ID id2)
{
    if (id1.s_standard.extended)
    {        
        if ((id2.s_extended.extended == 1) && (id1.s_extended.id == id2.s_extended.id))
        {
            return TRUE;
        }
    }
    else
    {
        if ((id2.s_extended.extended == 0) && (id1.s_standard.id == id2.s_standard.id))
        {
            return TRUE;
        }
    }
    return FALSE;    
}
UINT32 IDtoCANMID(CAN_ID id)
{
    if (id.s_extended.extended)
    {
        return CANMID_EXTENDED | id.s_extended.id;
    }
    else
    {
        return (id.s_standard.id & 0x7FF) << 18;
    }
}

void CANMIDtoCANID(UINT32 canmid, CAN_ID *pId)
{
    if (canmid & CANMID_EXTENDED)
    {
        pId->s_extended.extended = 1;
        pId->s_extended.id = canmid & CANMID_IDMASK;
    }
    else
    {
        pId->s_standard.extended = 0;
        pId->s_standard.id = (canmid >> 18) & 0x7FF;
    }
}


UINT32 MaskToCANLAM(CAN_ID id)
{
    if (id.s_extended.extended)
    {
        return (~id.s_extended.id) & CANMID_IDMASK;
    }
    else
    {
        return CANLAM_LAMI | (((~id.s_standard.id) & 0x7FF) << 18);
    }
}

void StrobeActive(T_CAN_DEVICE* pDevice,BOOL fActive)
{
    if ((pDevice->hGpio==NULL) || (pDevice->StrobePin == -1))
        return;
        
    if (fActive ^ (pDevice->StrobePinPolarity != ACTIVE_LOW))
    {
        GPIOClrBit(pDevice->hGpio,pDevice->StrobePin);
    }
    else
    {
        GPIOSetBit(pDevice->hGpio,pDevice->StrobePin);
    }
}
BOOL CreateMsgQueuePair(WCHAR* name,HANDLE* pReader,HANDLE* pWriter,DWORD MaxElem,DWORD ElemSize,DWORD flags)
{
    MSGQUEUEOPTIONS msqQueueOptions;

    if ((pReader == NULL) || (pWriter==NULL))
    {
        return FALSE;
    }

    msqQueueOptions.bReadAccess = TRUE;
    msqQueueOptions.dwMaxMessages = MaxElem;
    msqQueueOptions.cbMaxMessage = ElemSize;
    msqQueueOptions.dwFlags = flags;
    msqQueueOptions.dwSize = sizeof(msqQueueOptions);

    *pReader = CreateMsgQueue(name,&msqQueueOptions);
    if (*pReader != INVALID_HANDLE_VALUE)
    {
        msqQueueOptions.bReadAccess = FALSE;
        *pWriter = OpenMsgQueue(GetCurrentProcess(),*pReader,&msqQueueOptions);
        if (*pWriter != INVALID_HANDLE_VALUE)
        {
            return TRUE;
        }
        else
        {
            CloseHandle(*pReader);
        }
    }
    return FALSE;
}
//------------------------------------------------------------------------------
//
//  Function:  CAN_Init
//
//  Called by device manager to initialize device.
//
DWORD
CAN_Init(
         LPCTSTR szContext,
         LPCVOID pBusContext
         )
{
    DWORD rc = (DWORD)NULL;    
    DWORD irq[4];
    PHYSICAL_ADDRESS pa;    
    T_CAN_DEVICE* pDevice = (T_CAN_DEVICE*) malloc(sizeof(T_CAN_DEVICE));

    UNREFERENCED_PARAMETER(pBusContext);

    irq[0] = (DWORD) -1;
    irq[1] = OAL_INTR_STATIC;

    if (pDevice==NULL) return 0;
    memset(pDevice,0,sizeof(T_CAN_DEVICE));
    pDevice->dwSysintr = (DWORD) SYSINTR_UNDEFINED;
    
    

    // Read parameters from registry
    if (GetDeviceRegistryParams(
        szContext, 
        pDevice, 
        dimof(s_deviceRegParams), 
        s_deviceRegParams) != ERROR_SUCCESS)
    {
        DEBUGMSG(ZONE_ERROR, (TEXT("ERROR: CAN_Init: Error reading from Registry.\r\n")));
        goto cleanUp;
    }

    if (pDevice->StrobePin != (DWORD) -1)
    {
        if (pDevice->StrobePinPolarity == (DWORD) -1)
        {
            DEBUGMSG(ZONE_ERROR, (L"ERROR: CAN_Init: strobe without polarity\r\n"));
            goto cleanUp;
        }
        pDevice->hGpio = GPIOOpen();
        if (pDevice->hGpio == NULL)
        {
            DEBUGMSG(ZONE_ERROR, (L"ERROR: CAN_Init: unable to open GPIO\r\n"));
            goto cleanUp;
        }
        StrobeActive(pDevice,FALSE);
        GPIOSetMode(pDevice->hGpio,pDevice->StrobePin,GPIO_DIR_OUTPUT);
    }

    if (CreateMsgQueuePair(NULL,&pDevice->RxMsgQ[MSQ_READER],&pDevice->RxMsgQ[MSQ_WRITER],
        pDevice->canStatus.maxRxMsg,sizeof(CAN_MESSAGE),MSGQUEUE_ALLOW_BROKEN) == FALSE)
    {
        goto cleanUp;
    }
    for (int i=0;i<NB_TX_PRIORITIES;i++)
    {
        if (CreateMsgQueuePair(NULL,&pDevice->TxMsgQ[i][MSQ_READER],&pDevice->TxMsgQ[i][MSQ_WRITER],
            pDevice->canStatus.maxTxMsg[i],sizeof(CAN_MESSAGE),MSGQUEUE_ALLOW_BROKEN) == FALSE)
        {
            goto cleanUp;
        }
    }

    pDevice->device = SOCGetCANDevice(pDevice->CanCtrlIndex);
    if (pDevice->device == OMAP_DEVICE_NONE)
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: CAN_Init: unable to get associated Device (check the CAN CTRL index)\r\n"));
        goto cleanUp;
    }
    pa.QuadPart = GetAddressByDevice(pDevice->device);
    if (pa.QuadPart == 0)
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: CAN_Init: unable to get base address\r\n"));
        goto cleanUp;
    }

    irq[2] = GetIrqByDevice(pDevice->device,L"INT0");
    irq[3] = GetIrqByDevice(pDevice->device,L"INT1");
    if ((irq[2]==(DWORD)-1) || (irq[3]==(DWORD)-1))
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: CAN_Init: unable to get the interrupts\r\n"));
        goto cleanUp;
    }

    InitializeCriticalSection(&pDevice->regcs);
    InitializeCriticalSection(&pDevice->filterscs);
    pDevice->pCanRegs = (OMAP_HECC_SCC_REGS*) MmMapIoSpace(pa,sizeof(OMAP_HECC_SCC_REGS),FALSE);
    if (pDevice->pCanRegs == NULL)
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: CAN_Init: unable to remap the registers\r\n"));
        goto cleanUp;
    }    
    pDevice->hRTREvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (pDevice->hRTREvent == NULL)
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: CAN_Init: Failed create Remote Request event\r\n"));
        goto cleanUp;
    }

    // Create interrupt event
    pDevice->hIntrEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (pDevice->hIntrEvent == NULL)
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: CAN_Init: Failed create interrupt event\r\n"));
        goto cleanUp;
    }

    if (KernelIoControl(IOCTL_HAL_REQUEST_SYSINTR,irq,sizeof(irq),&pDevice->dwSysintr,sizeof(pDevice->dwSysintr),NULL)== FALSE)
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: CAN_Init: unable to request a SYSINTR\r\n"));
        goto cleanUp;
    }


    if (InterruptInitialize(pDevice->dwSysintr,pDevice->hIntrEvent,NULL,0)==FALSE)
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: CAN_Init: unable to initialize the interrupt\r\n"));
        goto cleanUp;
    }

    pDevice->hIntrThread = CreateThread(NULL, 0, CANIntrThread, pDevice, CREATE_SUSPENDED, NULL);
    if (!pDevice->hIntrThread)
    {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: CAN_Init: Failed create interrupt thread\r\n"));
        goto cleanUp;
    }          
    CeSetThreadPriority(pDevice->hIntrThread,pDevice->ThreadPriority);

    pDevice->canStatus.BusState = ERROR_ACTIVE;
    pDevice->canStatus.CtrlState = STOPPED;
    
    pDevice->FreeMailboxMask = (DWORD) (((UINT64)1<<NB_MAILBOXES)-1);
    pDevice->TxMailBoxMask = 0;
    pDevice->RxMailboxMask = 0;
    pDevice->AutoAnswerMask = 0;
    pDevice->RemoteRequestMask = 0;
    for (int TxMailBox=FIRST_TX_MAILBOX;TxMailBox<=LAST_TX_MAILBOX;TxMailBox++)
    {
        pDevice->TxMailBoxMask |= (1<<TxMailBox);
        pDevice->FreeMailboxMask &= ~(1<<TxMailBox);
    }
    pDevice->TxMailBoxAvailableMask = pDevice->TxMailBoxMask;
    

    if (!RequestDevicePads(pDevice->device))
    {
        DEBUGMSG (ZONE_ERROR, (L"ERROR: CAN_Init: RequestDevicePads failed\r\n"));
        goto cleanUp;
    }
    EnableDeviceClocks(pDevice->device,TRUE);
    //initialize the hardware    
    CANInitHardware(pDevice);   
    EnableDeviceClocks(pDevice->device,FALSE);

    ResumeThread(pDevice->hIntrThread);
    // Return non-null value
    rc = (DWORD)pDevice;
cleanUp:
    if (rc == (DWORD)NULL)
    {
        CAN_Deinit((DWORD) pDevice);
    }
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  CAN_Deinit
//
//  Called by device manager to uninitialize device.
//
BOOL
CAN_Deinit(
           DWORD context
           )
{
    BOOL rc = FALSE;
    T_CAN_DEVICE *pDevice = (T_CAN_DEVICE*) context;

    if (pDevice == NULL)
    {
        goto cleanUp;
    }
    if (pDevice->hIntrThread)
    {
        pDevice->fTerminateThread = TRUE;
        SetEvent(pDevice->hIntrEvent);
        WaitForSingleObject(pDevice->hIntrThread,INFINITE);
    }
    if (pDevice->hIntrEvent)
    {
        CloseHandle(pDevice->hIntrEvent);
    }
    if (pDevice->dwSysintr != SYSINTR_UNDEFINED)
    {
        InterruptDisable(pDevice->dwSysintr);
        KernelIoControl(IOCTL_HAL_RELEASE_SYSINTR,&pDevice->dwSysintr,sizeof(pDevice->dwSysintr),NULL,0,NULL);        
    }
    if (pDevice->pCanRegs)
    {
        MmUnmapIoSpace(pDevice->pCanRegs,sizeof(OMAP_HECC_SCC_REGS));
    }
    DeleteCriticalSection(&pDevice->regcs);
    DeleteCriticalSection(&pDevice->filterscs);

    if (pDevice->hGpio)
    {
        StrobeActive(pDevice,FALSE);
        GPIOClose(pDevice->hGpio);
    }
    ReleaseDevicePads(pDevice->device);

    EnableDeviceClocks(pDevice->device,FALSE);

    LocalFree(pDevice);


    rc = TRUE;

cleanUp:

    return rc;
}
BOOL ConfigureTxMailbox(T_CAN_DEVICE* pDevice, DWORD index)
{
    EnterCriticalSection(&pDevice->regcs);
    CLRREG32(&pDevice->pCanRegs->CANME,(1<<index));  
    CLRREG32(&pDevice->pCanRegs->CANMIL,(1<<index));
    SETREG32(&pDevice->pCanRegs->CANMIM,(1<<index));    
    LeaveCriticalSection(&pDevice->regcs);
    return TRUE;
}


//------------------------------------------------------------------------------
//
//  Function:  CAN_Open
//
//  Called by device manager to open a device for reading and/or writing.
//
DWORD
CAN_Open(
         DWORD context, 
         DWORD accessCode, 
         DWORD shareMode
         )
{
    T_CAN_DEVICE *pDevice = (T_CAN_DEVICE *)context;
    T_CAN_OPEN_CTXT *pCtxt;

    UNREFERENCED_PARAMETER(accessCode);
    UNREFERENCED_PARAMETER(shareMode);

    if (pDevice == NULL) return 0;

    pCtxt = (T_CAN_OPEN_CTXT*) malloc(sizeof(T_CAN_OPEN_CTXT));
    if (pCtxt ==NULL) return 0;

    memset(pCtxt,0,sizeof(T_CAN_OPEN_CTXT));
    pCtxt->pDevice = pDevice;

    EnableDeviceClocks(pDevice->device,TRUE);
   
    return (DWORD) pCtxt;
}

//------------------------------------------------------------------------------
//
//  Function:  CAN_Close
//
//  This function closes the device context.
//
BOOL
CAN_Close(
          DWORD context
          )
{
    T_CAN_OPEN_CTXT *pCtxt = (T_CAN_OPEN_CTXT*) context;
    if (pCtxt)
    {
        T_CAN_DEVICE *pDevice = pCtxt->pDevice;
        StrobeActive(pDevice,FALSE);
        EnableDeviceClocks(pDevice->device,FALSE);
        LocalFree(pCtxt);        
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

//------------------------------------------------------------------------------
//
//  Function:  CAN_PowerUp
//
//  Called on resume of device.  Current implementation of CAN driver
//  will disable the CAN interrupts before suspend.  Make sure the
//  CAN interrupts are re-enabled on resume.
//
void
CAN_PowerUp(
            DWORD context
            )
{
    UNREFERENCED_PARAMETER(context);
}

//------------------------------------------------------------------------------
//
//  Function:  CAN_PowerDown
//
//  Called on suspend of device.
//
void
CAN_PowerDown(
              DWORD context
              )
{
    UNREFERENCED_PARAMETER(context);
}

//------------------------------------------------------------------------------
//
//  Function:  CAN_IOControl
//
//  This function sends a command to a device.
//
BOOL
CAN_IOControl(
              DWORD context, 
              DWORD code, 
              UCHAR *pInBuffer, 
              DWORD inSize, 
              UCHAR *pOutBuffer,
              DWORD outSize, 
              DWORD *pOutSize
              )
{
    BOOL rc = FALSE;
    T_CAN_OPEN_CTXT *pCtxt = (T_CAN_OPEN_CTXT*) context;
    T_CAN_DEVICE *ctxt = pCtxt->pDevice;


    UNREFERENCED_PARAMETER(pOutSize);    

    //Parameter check
    switch (code)
    {
    case IOCTL_CAN_COMMAND: 
        if ((inSize != sizeof(IOCTL_CAN_COMMAND_IN)) || (pInBuffer == NULL))
        {
            return FALSE;
        }
        break;
    case IOCTL_CAN_STATUS: 
        if ((outSize < sizeof(IOCTL_CAN_STATUS_OUT)) || (pOutBuffer == NULL))
        {
            return FALSE;
        }
        break;
    case IOCTL_CAN_CONFIG: 
        {
            IOCTL_CAN_CONFIG_IN* pCfgIn;
            if ((inSize != sizeof(IOCTL_CAN_CONFIG_IN)) || (pInBuffer == NULL))
            {
                return FALSE;
            }
            pCfgIn = (IOCTL_CAN_CONFIG_IN*) pInBuffer;
            switch (pCfgIn->cfgType)
            {
            case BAUDRATE_CFG:
                break;
            default:
                return FALSE;
            }
        }
        break;
    case IOCTL_CAN_FILTER_CONFIG: 
        {
            IOCTL_CAN_CLASS_FILTER_CONFIG_IN* pCfgIn;

#pragma warning(push)
#pragma warning(disable:6287) //If sizeof(IOCTL_CAN_CLASS_FILTER_CONFIG_IN) == sizeof(IOCTL_CAN_SUBCLASS_FILTER_CONFIG_IN) we have a warning of redudant testing
            if (((inSize != sizeof(IOCTL_CAN_CLASS_FILTER_CONFIG_IN)) && 
                (inSize != sizeof(IOCTL_CAN_SUBCLASS_FILTER_CONFIG_IN)))
                || (pInBuffer == NULL))
#pragma warning(pop)

            {
                return FALSE;
            }
            pCfgIn = (IOCTL_CAN_CLASS_FILTER_CONFIG_IN*) pInBuffer;
            switch (pCfgIn->cfgType)
            {
            case CREATE_CLASS_FILTER_CFG:
            case ENABLE_DISABLE_CLASS_FILTER_CFG:
            case DELETE_CLASS_FILTER_CFG:
            case ADD_SUBCLASS_FILTER_CFG:
            case REMOVE_SUBCLASS_FILTER_CFG:
                break;
            default:
                return FALSE;
            }
        }
        break;
    case IOCTL_CAN_SEND: 
        if ((inSize != sizeof(IOCTL_CAN_SEND_IN)) || (pInBuffer == NULL))
        {
            return FALSE;
        }
        break;
    case IOCTL_CAN_REMOTE_CONFIGURE_AUTO_ANSWER: 
        if ((inSize != sizeof(IOCTL_CAN_REMOTE_CONFIGURE_AUTO_ANSWER_IN)) || (pInBuffer == NULL))
        {
            return FALSE;
        }
        break;
    case IOCTL_CAN_RECEIVE: 
        if ((outSize < sizeof(IOCTL_CAN_RECEIVE_OUT)) || (pOutBuffer == NULL))
        {
            return FALSE;
        }
        break;
    case IOCTL_CAN_REMOTE_REQUEST:
        if ((inSize < sizeof(IOCTL_CAN_REMOTE_REQUEST_IN)) || (pInBuffer == NULL))
        {
            return FALSE;
        }
        break;
    default:
        return FALSE;
    }

    switch (code)
    {
    case IOCTL_CAN_COMMAND: 
        rc = CANCommand(ctxt,*((IOCTL_CAN_COMMAND_IN*)pInBuffer));
        break;
    case IOCTL_CAN_STATUS: 
        rc = CANStatus(ctxt,(IOCTL_CAN_STATUS_OUT*)pOutBuffer);
        break;
    case IOCTL_CAN_CONFIG: 
        {
            IOCTL_CAN_CONFIG_IN* pCfgIn;
            pCfgIn = (IOCTL_CAN_CONFIG_IN*) pInBuffer;
            switch (pCfgIn->cfgType)
            {
            case BAUDRATE_CFG:
                rc = CANConfigureBaudrate(ctxt,pCfgIn->BaudRate);
                break;
            }
        }
        break;
    case IOCTL_CAN_FILTER_CONFIG:
        {
            IOCTL_CAN_CLASS_FILTER_CONFIG_IN* pClassCfgIn;
            IOCTL_CAN_SUBCLASS_FILTER_CONFIG_IN* pSubClassCfgIn;
            pClassCfgIn = (IOCTL_CAN_CLASS_FILTER_CONFIG_IN*) pInBuffer;
            pSubClassCfgIn = (IOCTL_CAN_SUBCLASS_FILTER_CONFIG_IN*) pInBuffer;
            switch (pClassCfgIn->cfgType)
            {
            case CREATE_CLASS_FILTER_CFG:
                rc = CANCreateClassFilter(ctxt,&pClassCfgIn->classFilter,pClassCfgIn->rxPriority,pClassCfgIn->fEnabled);
                break;
            case ENABLE_DISABLE_CLASS_FILTER_CFG:
                rc = CANEnableClassFilter(ctxt,&pClassCfgIn->classFilter,pClassCfgIn->fEnabled);
                break;
            case DELETE_CLASS_FILTER_CFG:
                rc = CANDeleteClassFilter(ctxt,&pClassCfgIn->classFilter);
                break;
            case ADD_SUBCLASS_FILTER_CFG:
                rc = CANAddSubClassFilter(ctxt,&pSubClassCfgIn->subClassFilter);
                break;
            case REMOVE_SUBCLASS_FILTER_CFG:
                rc = CANRemoveSubClassFilter(ctxt,&pSubClassCfgIn->subClassFilter);
                break;                
            }
        }    
        break;
    case IOCTL_CAN_SEND: 
        {
            IOCTL_CAN_SEND_IN* pSendIn = (IOCTL_CAN_SEND_IN*) pInBuffer;
            rc = CANSend(ctxt,pSendIn->msgArray,pSendIn->nbMsg,&pSendIn->nbMsgSent,pSendIn->priority,pSendIn->timeout);            
        }
        break;
    case IOCTL_CAN_REMOTE_CONFIGURE_AUTO_ANSWER:
        {
            IOCTL_CAN_REMOTE_CONFIGURE_AUTO_ANSWER_IN* pAutoAnswerIn = (IOCTL_CAN_REMOTE_CONFIGURE_AUTO_ANSWER_IN*) pInBuffer;
            switch (pAutoAnswerIn->cfgType)
            {
            case SET_AUTO_ANSWER:
                rc = CANConfigureAutoAnswer(ctxt,&pAutoAnswerIn->msg,TRUE);            
                break;
            case DELETE_AUTO_ANSWER:
                rc = CANConfigureAutoAnswer(ctxt,&pAutoAnswerIn->msg,FALSE);            
                break;
            }
        }
        break;
    case IOCTL_CAN_REMOTE_REQUEST:
        {
            IOCTL_CAN_REMOTE_REQUEST_IN* pRemoteRequest = (IOCTL_CAN_REMOTE_REQUEST_IN*) pInBuffer;
            rc = CANRemoteRequest(ctxt,pRemoteRequest);
        }
        break;
    case IOCTL_CAN_RECEIVE: 
        {
            IOCTL_CAN_RECEIVE_OUT* pRcvOut = (IOCTL_CAN_RECEIVE_OUT*) pOutBuffer;            
            rc = CANReceive(ctxt,pRcvOut->msgArray,pRcvOut->nbMaxMsg,&pRcvOut->nbMsgReceived,pRcvOut->timeout);
        }        
        break;
    }
    return rc;
}
void SendMsg(T_CAN_DEVICE *pDevice,CAN_MESSAGE* pMsg,DWORD mailbox,BYTE TPL)
{    
    OMAP_HECC_SCC_MAILBOX* pMbx;    
    pMbx = &pDevice->pCanRegs->MailBoxes[mailbox];
    pMbx->CANMDH = pMsg->MDH;
    pMbx->CANMDL = pMsg->MDL;
    pMbx->CANMID = IDtoCANMID(pMsg->id);
    pMbx->CANMCF = pMsg->length | (TPL << 8);        
}

void StartTransmit(T_CAN_DEVICE *pDevice)
{        
    DWORD trs;
    OMAP_HECC_SCC_REGS* pRegs = pDevice->pCanRegs;

    trs = INREG32(&pRegs->CANTRS);

    //If we still have some mailbox waiting to transmit, we do no nothing in order to
    //keep a correct transmit order
    if ((trs & pDevice->TxMailBoxMask) != 0)
    {          
        return;
    }



    // All TX mailboxes are free. we fill them all with data
    EnterCriticalSection(&pDevice->regcs);    
    CLRREG32(&pRegs->CANME,pDevice->TxMailBoxMask);
    for (int TxMailBox=LAST_TX_MAILBOX;TxMailBox>=FIRST_TX_MAILBOX;TxMailBox--)
    {            
        DWORD flags;
        DWORD nbRead;
        CAN_MESSAGE msg;               

        int priority=0;
        while ( (priority <NB_TX_PRIORITIES) && (ReadMsgQueue(pDevice->TxMsgQ[priority][MSQ_READER],&msg,sizeof(msg),&nbRead,0,&flags) == FALSE))
        {
            priority++;
        }

        if (priority <NB_TX_PRIORITIES)
        {                    
            SendMsg(pDevice,&msg,TxMailBox,PriorityMapping[priority]);
            pDevice->TxMailBoxAvailableMask &= ~(1<<TxMailBox);
            SETREG32(&pRegs->CANME,1<<TxMailBox);
            OUTREG32(&pRegs->CANTRS,1<<TxMailBox);
            
            InterlockedDecrement(&pDevice->canStatus.currentTxMsg[priority]);
            InterlockedIncrement(&pDevice->canStatus.TotalSent);
        }
        else
        {
            break;
        }                   
    }    
    LeaveCriticalSection(&pDevice->regcs);
        
}
void HandleTXMailboxInterrupt(T_CAN_DEVICE *pDevice)
{    
    DWORD canaa,canta;
    OMAP_HECC_SCC_REGS* pRegs = pDevice->pCanRegs;

    //Handle TX mailboxes
    canaa = INREG32(&pRegs->CANAA);
    if (canaa)
    {
        RETAILMSG(0,(TEXT("TX interrupt (abort) 0x%x\r\n"),canaa));
        OUTREG32(&pRegs->CANTA,canaa);
        EnterCriticalSection(&pDevice->regcs);
        pDevice->TxMailBoxAvailableMask |= canaa;
        LeaveCriticalSection(&pDevice->regcs);
    }        

    canta = INREG32(&pRegs->CANTA);
    if (canta)
    {
        RETAILMSG(0,(TEXT("TX interrupt (acknoledge) 0x%x\r\n"),canta));
        OUTREG32(&pRegs->CANTA,canta);
        EnterCriticalSection(&pDevice->regcs);
        pDevice->TxMailBoxAvailableMask |= canta;
        LeaveCriticalSection(&pDevice->regcs);
    }
    StartTransmit(pDevice);

}

void HandleErrors(T_CAN_DEVICE *pDevice, DWORD int_status, DWORD err_status)
{
	// Warning Level int
	if (int_status & CANGIx_WLIM)
	{
		HandleWarningLevelInterrupt(pDevice);
		OUTREG32(&pDevice->pCanRegs->CANGIF0, CANGIx_WLIM);
		SETREG32(&pDevice->pCanRegs->CANES, CANES_EW);
		CLRREG32(&pDevice->pCanRegs->CANMC, CANMC_CCR);
	}

	// Error Passive int
	if (int_status & CANGIx_EPIM)
	{
		HandleErrorPassiveInterrupt(pDevice);
		OUTREG32(&pDevice->pCanRegs->CANGIF0, CANGIx_EPIM);
		SETREG32(&pDevice->pCanRegs->CANES, CANES_EP);
		CLRREG32(&pDevice->pCanRegs->CANMC, CANMC_CCR);
	}

	// Need to check bus off condition in CANES as well to ensure warning interrupts don't hog the system
	if ((int_status & CANGIx_BOIM) || (err_status & CANES_BO)) 
	{
		HandleBusOffInterrupt(pDevice);
		OUTREG32(&pDevice->pCanRegs->CANGIF0, CANGIx_BOIM);
		SETREG32(&pDevice->pCanRegs->CANES, CANES_BO);
		CLRREG32(&pDevice->pCanRegs->CANMC, CANMC_CCR);

		// Stop the driver to avoir spurious interrupts
		CANCommand(pDevice, STOP);
	}
}

CLASS_FILTER_INTERNAL *FindClassFilterFromMailbox(T_CAN_DEVICE* pDevice,int mailbox)
{
    CLASS_FILTER_INTERNAL* pCurrent = pDevice->ClassFilterList;   
    while (pCurrent)
    {   
        if (pCurrent->mailboxMask & (1<<mailbox))
        {
            return pCurrent;
        }
        pCurrent = (CLASS_FILTER_INTERNAL*) NextElement(&pCurrent->sle);
    }
    return pCurrent;
}



void HandleRXMailboxInterrupt(T_CAN_DEVICE *pDevice)
{    
    OMAP_HECC_SCC_MAILBOX mailboxCopy[NB_MAILBOXES];
    CLASS_FILTER_INTERNAL* pClassFilter;
    DWORD dwRmpStatus;
    DWORD cpy;
    int i;
    DWORD LocalRemoteRequestMask;
    OMAP_HECC_SCC_REGS* pRegs = pDevice->pCanRegs;  
    DWORD dwRmlStatus = INREG32(&pRegs->CANRML);    



    EnterCriticalSection(&pDevice->regcs);

        
    dwRmpStatus = INREG32(&pRegs->CANRMP);
    
    //First of all make a copy of the mailbox as fast as possible.
    //We do this because filtering takes CPU time and thus interleaving mailbox mgnt and filtering
    //increase the risk of overruns and msg order change
    LocalRemoteRequestMask = pDevice->RemoteRequestMask;
    cpy = dwRmpStatus;
    i = NB_MAILBOXES-1;    
    while (cpy)
    {        
        register DWORD mask = 1<<i;
        if (cpy & mask)
        {
            mailboxCopy[i] = pRegs->MailBoxes[i];
            
  
            if (dwRmlStatus &(1<<i))
            {
//                RETAILMSG(1,(TEXT("mailbox %d generated a RX LOST interrup\r\n"),i));            
                OUTREG32(&pRegs->CANRML,(1<<i));
                InterlockedIncrement(&pDevice->canStatus.RxLost);
            }
            OUTREG32(&pRegs->CANRMP,mask);
            
            if (LocalRemoteRequestMask & mask)
            {                
                CLRREG32(&pDevice->pCanRegs->CANME,mask);
                pDevice->RemoteRequestMask &= ~mask;
                pDevice->FreeMailboxMask |= mask;
            }
            else
            {
               // SETREG32(&pDevice->pCanRegs->CANME,mask);
            }
            cpy &= ~mask;
        }
        i--;
    }

    LeaveCriticalSection(&pDevice->regcs);
    
    
    // We have a copy of the 'full' mailboxex, we can look at each of them now
    i = NB_MAILBOXES-1;
    while (dwRmpStatus)
    {
        DWORD mask = 1<<i;        
        if (dwRmpStatus & mask) //Is mailbox full ?
        {
            CAN_MESSAGE msg;
            OMAP_HECC_SCC_MAILBOX* pMbx;
            pMbx = &mailboxCopy[i];            
            
            RETAILMSG(0,(TEXT("mailbox %d generated a RX interrup\r\n"),i));       
            //Translate the mailbox content into a CAN_MESSAGE
            msg.MDH = pMbx->CANMDH;
            msg.MDL = pMbx->CANMDL;
            CANMIDtoCANID(pMbx->CANMID,&msg.id);            
            msg.length = (BYTE) (pMbx->CANMCF & 0xF);
            InterlockedIncrement(&pDevice->canStatus.TotalReceived);

            // Filter the message out
            BOOL fAccepted = TRUE;
            if (LocalRemoteRequestMask & mask)
            {
                RETAILMSG(1,(TEXT("remote request answer received\r\n")));
                RETAILMSG(1,(TEXT("Remote 0x%08x 0x%08x %d | %i\r\n"),msg.MDH,msg.MDL,msg.length,i));                
            }
            else
            {
                
                pClassFilter = FindClassFilterFromMailbox(pDevice,i);            
                if (pClassFilter == NULL)
                {
                    ERRORMSG(1,(TEXT("Received a message with no filter. Very strange. Accept it just in case\r\n")));                    
                    RETAILMSG(1,(TEXT("unknown  0x%08x 0x%08x %d | %i\r\n"),msg.MDH,msg.MDL,msg.length,i));
                }
                else  if (CANisIDAccepted(msg.id,pClassFilter->subFilter) == FALSE)
                {
                    fAccepted = FALSE;
                }
            }

            // If the message is accepted, then post it in the RX queue
            if (fAccepted)
            {
                if (WriteMsgQueue(pDevice->RxMsgQ[MSQ_WRITER],&msg,sizeof(msg),0,0) == FALSE)
                {
                    InterlockedIncrement(& pDevice->canStatus.RxDiscarded);
                }
                else
                {
                    InterlockedIncrement(& pDevice->canStatus.currentRxMsg);
                }
            }
            else
            {
                InterlockedIncrement(& pDevice->canStatus.FilteredOut);
            }            
        }
        //Process next message
        dwRmpStatus &= ~mask;
        i--;
    }
}
void HandleRxOverrun(T_CAN_DEVICE *pDevice)
{
    UNREFERENCED_PARAMETER(pDevice);

}
void HandleBusOffInterrupt(T_CAN_DEVICE *pDevice)
{
    RETAILMSG(1,(TEXT("BUS OFF. status 0x%x\r\n"),pDevice->pCanRegs->CANGIF0));
    pDevice->canStatus.BusState = BUS_OFF;

}
void HandleErrorPassiveInterrupt(T_CAN_DEVICE *pDevice)
{
    RETAILMSG(1,(TEXT("Error passive. status 0x%x\r\n"),pDevice->pCanRegs->CANGIF0));
    pDevice->canStatus.BusState = ERROR_PASSIVE;
}
void HandleWarningLevelInterrupt(T_CAN_DEVICE *pDevice)
{
    RETAILMSG(1,(TEXT("Warning level. status 0x%x\r\n"),pDevice->pCanRegs->CANGIF0));
    pDevice->canStatus.BusState = ERROR_ACTIVE_WARNED;
}

DWORD HandleOtherInterrupts(T_CAN_DEVICE *pDevice)
{
    DWORD clearMask = 0;
    DWORD dwStatus = INREG32(&pDevice->pCanRegs->CANGIF0);
    
    if (dwStatus & CANGIx_RMLIM)
    {        
        HandleRxOverrun(pDevice);
        clearMask |= CANGIx_RMLIM;
    }
    if (dwStatus & CANGIx_AAIM)
    {
        HandleTXMailboxInterrupt(pDevice);
        clearMask |= CANGIx_AAIM;
    }
    if (dwStatus & CANGIx_WDIM)
    {        
        RETAILMSG(1,(TEXT("HECC : write denied 0x%x\r\n"),dwStatus));
        clearMask |= CANGIx_WDIM;
    }
    if (dwStatus & CANGIx_WUIM)
    {        
        RETAILMSG(1,(TEXT("HECC : Wake Up interrutp 0x%x\r\n"),dwStatus));
        clearMask |= CANGIx_WUIM;
    }

#ifdef DEBUG
    if ((dwStatus & CANGIx_OTHER_INTERRUPTS) & ~clearMask)
    {        
        RETAILMSG(1,(TEXT("HECC : HandleOtherInterrupts : remaining bits 0x%x\r\n"),dwStatus & ~clearMask));
    }
#endif
    return clearMask;
}

    
void ShowErrors(OMAP_HECC_SCC_REGS* pRegs)
{
#ifdef DEBUG
    DWORD CANES1,CANES2;
    CANES1 = INREG32(&pRegs->CANES);
    OUTREG32(&pRegs->CANES,CANES1);
    CANES2 = INREG32(&pRegs->CANES);
    OUTREG32(&pRegs->CANES,CANES2);
    DEBUGMSG(1,(TEXT("CANES1 0x%x\tCANES2 0x%x\tCANREC %d\tCANTEC %d\r\n"),
        CANES1,CANES2,pRegs->CANREC,pRegs->CANTEC));
#else
    UNREFERENCED_PARAMETER(pRegs);
#endif
}

BOOL CANHandleIrq(T_CAN_DEVICE *pDevice)
{
    BOOL fSpurious = TRUE;
    OMAP_HECC_SCC_REGS* pRegs = pDevice->pCanRegs;
    DWORD status0,status1, err_status;
    status0 = INREG32(&pRegs->CANGIF0);
    status1 = INREG32(&pRegs->CANGIF1);
	err_status = INREG32(&pRegs->CANES);

	if (err_status & (CANES_BE | CANES_BO | CANES_EP | CANES_EW))
	{
		HandleErrors(pDevice, status0, err_status);
	}

    if (((status0 & CANGIx_BOIM) == 0) && ((status0 & CANGIx_EPIM) == 0))
    {
        if (pDevice->canStatus.BusState != ERROR_ACTIVE)
        {
            pDevice->canStatus.BusState = ERROR_ACTIVE;
            // In case the trasnmit is stopped, re-start it.
            StartTransmit(pDevice);
        }
    }

    if (status1 & CANGIF_GMIFx)
    {
        fSpurious = FALSE;
        HandleRXMailboxInterrupt(pDevice);
    }
    if (status0 & CANGIF_GMIFx)
    {
        fSpurious = FALSE;
        HandleTXMailboxInterrupt(pDevice);
    }    

    if (status0 & CANGIx_OTHER_INTERRUPTS)
    {
        DWORD clearMask;
        fSpurious = FALSE;
        clearMask = HandleOtherInterrupts(pDevice);
		OUTREG32(&pDevice->pCanRegs->CANGIF0, clearMask);
    }

    return !fSpurious;   
}
//------------------------------------------------------------------------------
//
//  Function:  CAN_InterruptThread
//
//  This function acts as the IST for the CAN.
//
//  Note: This function is more complex than it would be if there were any way to
//        directly read the state of the TPS659XX PWRON key status.
//
DWORD CANIntrThread(VOID *pContext)
{
    T_CAN_DEVICE *pDevice = (T_CAN_DEVICE*) pContext;
    while (!pDevice->fTerminateThread)
    {
        InterruptDone(pDevice->dwSysintr);
        // Wait for CAN interrupt
        if (WaitForSingleObject(pDevice->hIntrEvent, INFINITE) == WAIT_OBJECT_0)
        {
            if (pDevice->fTerminateThread)
            {
                break;
            }
            while (CANHandleIrq(pDevice)); 
        }

    }

    DEBUGMSG(ZONE_IST, (L"-CANIntrThread\r\n"));
    return ERROR_SUCCESS;
}

//------------------------------------------------------------------------------
//
//  Function:  DllMain
//
//  Standard Windows DLL entry point.
//
BOOL
__stdcall
DllMain(
        HANDLE hDLL,
        DWORD reason,
        VOID *pReserved
        )
{
    UNREFERENCED_PARAMETER(pReserved);
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        RETAILREGISTERZONES((HMODULE)hDLL);
        DisableThreadLibraryCalls((HMODULE)hDLL);
        break;
    }
    return TRUE;
}

//------------------------------------------------------------------------------



BOOL CANInitHardware(void *ctxt)
{
    T_CAN_DEVICE *pDevice = (T_CAN_DEVICE*) ctxt;
    int i;
        
    // Soft reset the CTRL
    OUTREG32(&pDevice->pCanRegs->CANMC,CANMC_SRES);
    Sleep(1);

    //Configure PIn as CAN pins (not IOs)


    pDevice->pCanRegs->CANTIOC = CANTIOC_TXFUNC;
    pDevice->pCanRegs->CANRIOC = CANRIOC_RXFUNC;


    OUTREG32(&pDevice->pCanRegs->CANMC,CANMC_SCM | CANMC_LNTC | CANMC_LNTM | CANMC_ABO | CANMC_DBO |/*CANMC_STM |*/ CANMC_WUBA);
    


    // Set default baudrate
    CANConfigureBaudrate(pDevice,pDevice->BaudRate);

    // Init Mailboxes
    OUTREG32(&pDevice->pCanRegs->CANME,0);
    for (i=0;i<NB_MAILBOXES;i++)
    {        
        pDevice->pCanRegs->MailBoxes[i].CANMCF = 0;
        pDevice->pCanRegs->MailBoxes[i].CANMID = 0;
    }

    pDevice->pCanRegs->CANGIM =  CANGIx_AAIM    |
                                 CANES_EP    |
                                 CANGIx_WUIM    |
                                 CANGIx_RMLIM   |
                                 CANGIx_BOIM    |
                                 CANGIx_EPIM    |
                                 CANGIx_WLIM    ;

    
    for (int TxMailBox=FIRST_TX_MAILBOX;TxMailBox<=LAST_TX_MAILBOX;TxMailBox++)
    {
        ConfigureTxMailbox(pDevice, TxMailBox);
    }
    
    return TRUE;
}
BOOL CANCommand(void* ctxt,CAN_COMMAND cmd)
{
    DWORD flags;
    BOOL rc = TRUE;
    T_CAN_DEVICE *pDevice = (T_CAN_DEVICE*) ctxt;
    switch(cmd)
    {
    case START:
        pDevice->canStatus.CtrlState = STARTED;
        
        EnterCriticalSection(&pDevice->regcs);        
        
        //Enable the interrupts       
        SETREG32(&pDevice->pCanRegs->CANGIM,CANGIM_I0EN | CANGIM_I1EN);
        
        LeaveCriticalSection(&pDevice->regcs);

        //assert the Strobe
        StrobeActive(pDevice,TRUE);

        break;
    case RESET:                
        
        //Clear the status
        memset(&pDevice->canStatus,0,sizeof(pDevice->canStatus));
        // move to the stopped state
        CANCommand(ctxt,STOP);

        CLASS_FILTER_INTERNAL* pClassFilter;        
        pClassFilter = pDevice->ClassFilterList;
        while (pClassFilter)
        {
            CLASS_FILTER_INTERNAL* pNext;
            SUBCLASS_FILTER_INTERNAL* pSubFilter;
            pNext = (CLASS_FILTER_INTERNAL*) NextElement(&pClassFilter->sle);
            pSubFilter = pClassFilter->subFilter;
            while (pSubFilter)
            {
                SUBCLASS_FILTER_INTERNAL* pNextSub;
                pNextSub = (SUBCLASS_FILTER_INTERNAL*) NextElement(&pSubFilter->sle);
                LocalFree(pSubFilter);
                pSubFilter = pNextSub;
            }
            LocalFree(pClassFilter);
            pClassFilter = pNext;
        }

        pDevice->ClassFilterList = NULL;
        pDevice->nbFreeClassFilter = NB_MAX_CLASS_FILTER; 
        
        pDevice->FreeMailboxMask = (DWORD) (((UINT64)1<<NB_MAILBOXES)-1);
        pDevice->TxMailBoxMask = 0;
        pDevice->AutoAnswerMask = 0;
        pDevice->RxMailboxMask = 0;
        pDevice->RemoteRequestMask = 0;
        for (int TxMailBox=FIRST_TX_MAILBOX;TxMailBox<=LAST_TX_MAILBOX;TxMailBox++)
        {
            pDevice->TxMailBoxMask |= (1<<TxMailBox);
            pDevice->FreeMailboxMask &= ~(1<<TxMailBox);
        }
        pDevice->TxMailBoxAvailableMask = pDevice->TxMailBoxMask;

        // Reinit the hardware
        CANInitHardware(pDevice);               

        //Reset the Messages Queues        
        CAN_MESSAGE msg;
        DWORD nbRead;
        for (int i=0;i<NB_TX_PRIORITIES;i++)
        {            
            while (ReadMsgQueue(pDevice->TxMsgQ[i][MSQ_READER],&msg,sizeof(msg),&nbRead,0,&flags) == TRUE) Sleep(1);
            pDevice->canStatus.currentTxMsg[i] = 0;
        }
        while (ReadMsgQueue(pDevice->RxMsgQ[MSQ_READER],&msg,sizeof(msg),&nbRead,0,&flags) == TRUE) Sleep(1);
                
        break;

    case STOP:
        pDevice->canStatus.CtrlState = STOPPED;

        //Deassert the Strobe
        StrobeActive(pDevice,FALSE);

        EnterCriticalSection(&pDevice->regcs);       

        //Disable the interrupts
        CLRREG32(&pDevice->pCanRegs->CANGIM,CANGIM_I0EN  | CANGIM_I1EN);        
        
        LeaveCriticalSection(&pDevice->regcs);

        break;
    default:
        SetLastError(ERROR_INVALID_PARAMETER);
        rc = FALSE;
        break;
    }
    return rc;
}
void CANAllowConfigChange(OMAP_HECC_SCC_REGS* pRegs,BOOL fAllowed)
{    
    if (fAllowed)
    {
        
        SETREG32(&pRegs->CANMC,CANMC_CCR);
        while ((INREG32(&pRegs->CANES) & CANES_CCE) == 0) Sleep(1);
    }
    else
    {
        CLRREG32(&pRegs->CANMC,CANMC_CCR);
        while ((INREG32(&pRegs->CANES) & CANES_CCE) == CANES_CCE) Sleep(1);
    }    
}

BOOL FindBestCanBitTiming(int Fin,int bitrate,int* pBrp,int* Tseg1,int* Tseg2)
{
#define MAX_ERROR_PERTHOUSAND   10
    int TSEG1,TSEG2,BRP;
    int TSEGTotal = 0;    
    BOOL found;
    //bitrate = Fin / (BRP *(TSEG1+TSEG2+1);

    found = FALSE;
    BRP = 255;
    while (BRP)
    {
        TSEGTotal = Fin / (bitrate * BRP);
        if (((Fin-(TSEGTotal * (bitrate * BRP))) / (Fin/1000)) < MAX_ERROR_PERTHOUSAND)
        {     
            if ((TSEGTotal < (16+8+1)) && (TSEGTotal >=4))
            {
                break;
            }   
        }
        BRP--;
    }
    if (BRP==0)
    {
        return FALSE;
    }
    TSEG2 = (TSEGTotal-1)/3; //sample point at 66%
    if (TSEG2 == 0) TSEG2 = 1;
    TSEG1 = (TSEGTotal-1)-TSEG2;


    *pBrp = BRP;
    *Tseg1 = TSEG1;
    *Tseg2 = TSEG2;
    return TRUE;
}


BOOL CANConfigureBaudrate(void* ctxt,DWORD baudrate)
{
    int brp,tseg1,tseg2;

    T_CAN_DEVICE *pDevice = (T_CAN_DEVICE*) ctxt;
    OMAP_HECC_SCC_REGS* pRegs = pDevice->pCanRegs;
    if (pDevice->canStatus.CtrlState != STOPPED)
    {
        return FALSE;
    }
    if (FindBestCanBitTiming(13000000,baudrate,&brp,&tseg1,&tseg2) == FALSE)
    {
        return FALSE;
    }

    EnterCriticalSection(&pDevice->regcs);
    CANAllowConfigChange(pRegs,TRUE);

#if 0    
    OUTREG32(&pRegs->CANBTC,
        ((brp-1) << 16) | ((tseg2-1) << 0) | ((tseg1-1) << 3)   |   //BRP | TSEG2 | TSEG1
        ((min(tseg2,4)-1) << 8)                                 |   //SJW
        (CANBTC_ERM )|                                                // resync on both edges
        ((brp >= 5) ? CANBTC_SAM : 0)                                // Use oversampling if possible
        );
#else
    /* changing the SJW and resync params below, since we are seeing the bit length vary for 1Mbit case */
    /* taking the new params from linux */
    OUTREG32(&pRegs->CANBTC,
        ((brp-1) << 16) | ((tseg2-1) << 0) | ((tseg1-1) << 3)   |   //BRP | TSEG2 | TSEG1
        ((min(tseg2,1)-1) << 8)                                 |   //SJW
        (0 )|                                                // resync on both edges
        ((brp >= 5) ? CANBTC_SAM : 0)                                // Use oversampling if possible
        );
#endif
    
    CANAllowConfigChange(pRegs,FALSE);
    LeaveCriticalSection(&pDevice->regcs);


    return TRUE;
}

BOOL CANStatus(void* ctxt,CAN_STATUS* pStatus)
{
    T_CAN_DEVICE *pDevice = (T_CAN_DEVICE*) ctxt;
    if (((pDevice->pCanRegs->CANGIF0 & CANGIx_BOIM) == 0) && 
        ((pDevice->pCanRegs->CANGIF0 & CANGIx_EPIM) == 0))
    {
        pDevice->canStatus.BusState = ERROR_ACTIVE;
    }
    if (pStatus)
    {
        memcpy(pStatus,&pDevice->canStatus,sizeof(CAN_STATUS));
        pStatus->CANREC = pDevice->pCanRegs->CANREC;
        pStatus->CANTEC = pDevice->pCanRegs->CANTEC;
        return TRUE;
    }
    return FALSE;
}

BOOL CANSend(void* ctxt,CAN_MESSAGE* msg,DWORD nbMsg,DWORD* nbMsgSent,TXCAN_PRIORITY priority,DWORD timeout)
{
    BOOL fFirstMsg = TRUE;
    DWORD nbMsgRemaining = nbMsg;
    DWORD nbSentOk = 0;
    T_CAN_DEVICE *pDevice = (T_CAN_DEVICE*) ctxt;
    if (priority < NB_TX_PRIORITIES)
    {                       
        while (nbMsgRemaining)
        {       
            if (msg->length > 8)
            {
                ERRORMSG(1,(TEXT("msg lenght (%d) is > 8. Message skipped\r\n"),msg->length));
            }
            else 
            {
                if (WriteMsgQueue(pDevice->TxMsgQ[priority][MSQ_WRITER],msg,sizeof(*msg),timeout,0) == FALSE)
                {
                    break;
                }
                if (fFirstMsg)
                {
                    timeout = 0; 
                }
                InterlockedIncrement(&pDevice->canStatus.currentTxMsg[priority]);                
                nbSentOk++;
            }
            nbMsgRemaining--;
            msg++;
        }
        
        // In case the trasnmit is stopped, re-start it.
        StartTransmit(pDevice);
    }
    *nbMsgSent = nbSentOk;
    return (nbSentOk != 0) ? TRUE : FALSE;
}
BOOL CANReceive(void* ctxt,CAN_MESSAGE* msg,DWORD nbMaxMsg,DWORD* nbMsgReceived,DWORD timeout)
{
    DWORD nbMsgRemaining = nbMaxMsg; 
    DWORD nbRead,flags;
    T_CAN_DEVICE *pDevice = (T_CAN_DEVICE*) ctxt;
    BOOL fFirstMsg = TRUE;

    while (nbMsgRemaining && ReadMsgQueue(pDevice->RxMsgQ[MSQ_READER],msg,sizeof(*msg),&nbRead,timeout,&flags))
    {
        if (fFirstMsg)
        {
            timeout = 0;
        }
        InterlockedDecrement(&pDevice->canStatus.currentRxMsg);    
        nbMsgRemaining--;
        msg++;
    }
    *nbMsgReceived = nbMaxMsg - nbMsgRemaining;

    return (*nbMsgReceived != 0) ? TRUE : FALSE;
}



BOOL CANConfigureAutoAnswer(void* ctxt,CAN_MESSAGE* msg, BOOL fEnabled)
{
    UINT32 AutoAnswerMask;    
    int mailbox = -1;
    int i;
    T_CAN_DEVICE *pDevice = (T_CAN_DEVICE*) ctxt;
    
    if ((msg->length > 8) && fEnabled)
    {
        return FALSE;
    }

    EnterCriticalSection(&pDevice->regcs);

    AutoAnswerMask = pDevice->AutoAnswerMask;
    //Find matching mailbox if it already exists
    i = 0;
    while (AutoAnswerMask)
    {
        if (AutoAnswerMask & 0x1)
        {
        CAN_ID mbxId;
        
            CANMIDtoCANID(pDevice->pCanRegs->MailBoxes[i].CANMID,&mbxId);

            if (IDequal(mbxId,msg->id))
            {
                mailbox = i;
                break;
            }
        }
        AutoAnswerMask>>=1;
        i++;
    }
            
    if (fEnabled && (mailbox == -1)) // no matching mailbox found ?
    {
        //Try to allocate a mailbox
        for (i=NB_MAILBOXES-1;i>=0;i--)
        {
            if (pDevice->FreeMailboxMask & (1<<i))
            {
                mailbox = i;
                pDevice->FreeMailboxMask &= ~(1<<mailbox);
                break;
            }
        }
    }

    if (mailbox == -1)
    {
        //no mailbox found
        LeaveCriticalSection(&pDevice->regcs);
        return FALSE;
    }
    else
    {
        BOOL rc = TRUE;
        OMAP_HECC_SCC_MAILBOX* pMbx = &pDevice->pCanRegs->MailBoxes[mailbox];
        if (fEnabled)
        {
            // configure mailbox for autoanswer
            pDevice->pCanRegs->CANME &= ~(1<<mailbox);
            pDevice->pCanRegs->CANMD &= ~(1<<mailbox);
            pMbx->CANMDH = msg->MDH;
            pMbx->CANMDL = msg->MDL;
            pMbx->CANMID = IDtoCANMID(msg->id) | CANMID_AMM;
            pMbx->CANMCF = msg->length | (0x3 << 8);    //use highest priority for Remote frames
            pDevice->pCanRegs->CANME |= (1<<mailbox);
            pDevice->AutoAnswerMask |= (1<<mailbox);
        }
        else
        {
            // unconfigure mailbox
            pDevice->pCanRegs->CANME &= ~(1<<mailbox);            
            // and release it
            pDevice->FreeMailboxMask |= (1<<mailbox);
            pDevice->AutoAnswerMask &= ~(1<<mailbox);
        }
        LeaveCriticalSection(&pDevice->regcs);
        return rc;
    }
}

void ConfigureMailboxAndAcceptanceFilter(T_CAN_DEVICE *pDevice,UINT32 filterMailboxMask,CLASS_FILTER* pClassFilter)
{    
    DWORD mask = filterMailboxMask;
    BOOL lowestMailbox=TRUE;
    DWORD index = 0;
    OMAP_HECC_SCC_REGS* pRegs = pDevice->pCanRegs;
    OMAP_HECC_SCC_MAILBOX* pMailbox;

    // Disable the mailboxes (although at this point, they should already be disabled)
    CLRREG32(&pRegs->CANME,filterMailboxMask);

    while(mask)
    {
        if (mask & 0x1)
        {            
            pMailbox = &pDevice->pCanRegs->MailBoxes[index];



            if (lowestMailbox)
            {
                // if this is the first mailbox of the chain, then we do not protect it against overwrite so that we're aware of lost messages
                lowestMailbox = FALSE;
                CLRREG32(&pRegs->CANOPC,(1<<index));                    //Do not rotect against overwrites    
            }
            else
            {
                // protecting against overwrites makes the ctrl look for another mailbox if this one is already full
                SETREG32(&pRegs->CANOPC,(1<<index));                    //Protect against overwrites
            }
            SETREG32(&pRegs->CANMD,(1<<index));                     //Configure for recpetion
            SETREG32(&pRegs->CANMIL,(1<<index));                    //RX mailboxes trigger CAN IRQ 1
            SETREG32(&pRegs->CANMIM,(1<<index));                    // Unmask the interrupt

            OUTREG32(&pMailbox->CANMID,CANMID_AME | IDtoCANMID(pClassFilter->id));
            OUTREG32(&pRegs->CANLAM[index], MaskToCANLAM(pClassFilter->mask));            
        }
        mask >>= 1;
        index++;
    }
    SETREG32(&pRegs->CANME,filterMailboxMask);                     //Finally enabel the mailbox
}


BOOL CANCreateClassFilter(void* ctxt,CLASS_FILTER* pClassFilterIn,RXCAN_PRIORITY priority ,BOOL fEnabled)
{
    T_CAN_DEVICE *pDevice = (T_CAN_DEVICE*) ctxt;
    CLASS_FILTER_INTERNAL* pNewClassFilter = NULL;
    DWORD filterMailboxMask;
    DWORD count;

    if (pDevice == NULL)
    {
        return FALSE;
    }
    if (!(NB_RX_PRIORITIES > priority))
    {
        DEBUGMSG(1,(TEXT("Invalid RX priority\r\n")));
        return FALSE;
    }
    EnterCriticalSection(&pDevice->regcs);
    

    if (pDevice->nbFreeClassFilter < RxPriorityMlbxCount[priority])
    {
        DEBUGMSG(1,(TEXT("Not enough Mailboxes to implement filter class\r\n")));
        goto error;
    }
    pNewClassFilter = (CLASS_FILTER_INTERNAL*) LocalAlloc(LPTR,sizeof(CLASS_FILTER_INTERNAL));
    if (pNewClassFilter==NULL)
    {
        DEBUGMSG(1,(TEXT("Unable to allocate  pNewClassFilter\r\n")));
        goto error;
    }
    pDevice->nbFreeClassFilter -= RxPriorityMlbxCount[priority];    
    pNewClassFilter->classFilter = *pClassFilterIn;    
    pNewClassFilter->subFilter = NULL;
    pNewClassFilter->enabled = fEnabled;    

    filterMailboxMask = 0;
    count = 0;

    for (int i=NB_MAILBOXES-1;i>=0;i--)
    {
        if (pDevice->FreeMailboxMask & (1<<i))
        {
            filterMailboxMask |= (1<<i);
            count++;
            if (count == RxPriorityMlbxCount[priority])
            {
                break;
            }
        }
    }
    if (count != RxPriorityMlbxCount[priority])
    {
        ERRORMSG(1,(TEXT("This should never happen\r\n")));
        ASSERT(0);
        goto error;
    }

    

    PushElement((PSLE*)&pDevice->ClassFilterList,&pNewClassFilter->sle);
    
    pDevice->RxMailboxMask |= filterMailboxMask;
    pDevice->FreeMailboxMask &= ~(filterMailboxMask);
    pNewClassFilter->mailboxMask = filterMailboxMask;

    if (fEnabled)
    {
        ConfigureMailboxAndAcceptanceFilter(pDevice,pNewClassFilter->mailboxMask,pClassFilterIn);
    }



    
    LeaveCriticalSection(&pDevice->regcs);
    return TRUE;
error:
    if (pNewClassFilter)
    {
        LocalFree(pNewClassFilter);
    }
    LeaveCriticalSection(&pDevice->regcs);
    return FALSE;

}


CLASS_FILTER_INTERNAL* FindClassFilterInstance(T_CAN_DEVICE *pDevice,CLASS_FILTER* pClassFilter)
{
    CLASS_FILTER_INTERNAL* pCurrent = pDevice->ClassFilterList;
    if (pClassFilter == NULL)
    {
        return NULL;
    }
    while (pCurrent)
    {
        if ((pCurrent->classFilter.id.u32 == pClassFilter->id.u32)&&(pCurrent->classFilter.mask.u32 == pClassFilter->mask.u32))
        {
            return pCurrent;
        }
        pCurrent = (CLASS_FILTER_INTERNAL*) NextElement(&pCurrent->sle);
      

    }    
    return NULL;
}


BOOL CANDeleteClassFilter(void* ctxt,CLASS_FILTER* pClassFilterIn)
{
    T_CAN_DEVICE *pDevice = (T_CAN_DEVICE*) ctxt;
    CLASS_FILTER_INTERNAL* pClassFilterInstance;
    SUBCLASS_FILTER_INTERNAL* pCurrentSubClassFilter;

    EnterCriticalSection(&pDevice->regcs);
    pClassFilterInstance = FindClassFilterInstance(pDevice,pClassFilterIn);
    if (pClassFilterInstance == NULL)
    {
        LeaveCriticalSection(&pDevice->regcs);
        return FALSE;
    }

    RemoveElement((PSLE*) &pDevice->ClassFilterList,&pClassFilterInstance->sle);
    
    CLRREG32(&pDevice->pCanRegs->CANME,pClassFilterInstance->mailboxMask);    
    pDevice->FreeMailboxMask |= (pClassFilterInstance->mailboxMask);
    pDevice->RxMailboxMask &= ~pClassFilterInstance->mailboxMask;
    LeaveCriticalSection(&pDevice->regcs);

   pCurrentSubClassFilter = pClassFilterInstance->subFilter;
    while (pCurrentSubClassFilter)
    {
        SUBCLASS_FILTER_INTERNAL* pNext;
        pNext = (SUBCLASS_FILTER_INTERNAL*) NextElement(&pCurrentSubClassFilter->sle);
        LocalFree(pCurrentSubClassFilter);
        pCurrentSubClassFilter = pNext;
    }
    LocalFree(pClassFilterInstance);
    return TRUE;
}

BOOL CANEnableClassFilter(void* ctxt,CLASS_FILTER* pClassFilterIn, BOOL fEnabled)
{
    T_CAN_DEVICE *pDevice = (T_CAN_DEVICE*) ctxt;
    CLASS_FILTER_INTERNAL* pClassFilterInstance;

    EnterCriticalSection(&pDevice->regcs);
    pClassFilterInstance = FindClassFilterInstance(pDevice,pClassFilterIn);
    if (pClassFilterInstance)
    {
        if (fEnabled)
        {
            ConfigureMailboxAndAcceptanceFilter(pDevice,pClassFilterInstance->mailboxMask,pClassFilterIn);
        }
        else
        {            
            CLRREG32(&pDevice->pCanRegs->CANME,pClassFilterInstance->mailboxMask);
            
        }
        LeaveCriticalSection(&pDevice->regcs);
        return TRUE;
    }
    else
    {
        LeaveCriticalSection(&pDevice->regcs);
        return FALSE;
    }

}


BOOL CANAddSubClassFilter(void* ctxt,SUBCLASS_FILTER* pSubclassFilter)
{    
    T_CAN_DEVICE *pDevice = (T_CAN_DEVICE*) ctxt;
    CLASS_FILTER_INTERNAL* pClassFilterInstance;
    SUBCLASS_FILTER_INTERNAL* pNewSubclassFilter;

    pNewSubclassFilter = (SUBCLASS_FILTER_INTERNAL*) LocalAlloc(LPTR,sizeof(SUBCLASS_FILTER_INTERNAL));
    if (pNewSubclassFilter == NULL)
    {
        return FALSE;
    }
    pNewSubclassFilter->enabled = TRUE;
    pNewSubclassFilter->id.u32 = pSubclassFilter->id.u32 & pSubclassFilter->mask.u32;
    pNewSubclassFilter->mask.u32 = pSubclassFilter->mask.u32;

    EnterCriticalSection(&pDevice->regcs);

    pClassFilterInstance = FindClassFilterInstance(pDevice,&pSubclassFilter->classFilter);
    if (pClassFilterInstance == NULL)
    {
        goto error;
    }
    PushElement((PSLE*)&pClassFilterInstance->subFilter,&pNewSubclassFilter->sle);

    LeaveCriticalSection(&pDevice->regcs);
    return TRUE;
error:    
    LeaveCriticalSection(&pDevice->regcs);
    if (pNewSubclassFilter)
    {
        LocalFree(pNewSubclassFilter);
    }
    return FALSE;
}
BOOL CANRemoveSubClassFilter(void* ctxt,SUBCLASS_FILTER* pSubclassFilter)
{
    T_CAN_DEVICE *pDevice = (T_CAN_DEVICE*) ctxt;
    CLASS_FILTER_INTERNAL* pClassFilterInstance;
    SUBCLASS_FILTER_INTERNAL* pCurrent;
    SUBCLASS_FILTER_INTERNAL* pPrevious;

    EnterCriticalSection(&pDevice->regcs);

    pClassFilterInstance = FindClassFilterInstance(pDevice,&pSubclassFilter->classFilter);
    if (pClassFilterInstance == NULL)
    {
        goto error;
    }

    if (pClassFilterInstance->subFilter == NULL)
    {
        goto error;
    }
    pPrevious = NULL;
    pCurrent = pClassFilterInstance->subFilter;
    while (pCurrent)
    {
        if ((pCurrent->id.u32 == (pSubclassFilter->id.u32 & pSubclassFilter->mask.u32)) && 
            (pCurrent->mask.u32 == pSubclassFilter->mask.u32))
        {
            if (pPrevious)
            {
                pPrevious->sle.pNext = NextElement(&pCurrent->sle);
                LocalFree(pCurrent);
                pCurrent = pPrevious;
            }
            else
            {
                pClassFilterInstance->subFilter =(SUBCLASS_FILTER_INTERNAL*) NextElement(&pCurrent->sle);
                LocalFree(pCurrent);
                pCurrent = NULL;
            }
        }
        pPrevious = pCurrent;
        if (pCurrent)
        {
            pCurrent = (SUBCLASS_FILTER_INTERNAL*) NextElement(&pCurrent->sle);
        }
    }

    
    LeaveCriticalSection(&pDevice->regcs);
    return TRUE;
error:    
    LeaveCriticalSection(&pDevice->regcs);
    return FALSE;
}

BOOL CANisIDAccepted(CAN_ID id,SUBCLASS_FILTER_INTERNAL* pList)
{    
    SUBCLASS_FILTER_INTERNAL* pCurrent = pList;
    if (pList == NULL)
    {
        return TRUE;
    }
    while (pCurrent)
    {   
        if (pCurrent->id.u32  == (id.u32 & pCurrent->mask.u32))
        {
            return TRUE;
        }
        pCurrent = (SUBCLASS_FILTER_INTERNAL*) NextElement(&pCurrent->sle);
    }
    return FALSE;
}

BOOL CANRemoteRequest(void* ctxt, CAN_REMOTE_REQUEST* pRemoteRequest)
{    
    int mailbox = -1;
    T_CAN_DEVICE *pDevice = (T_CAN_DEVICE*) ctxt;    
    OMAP_HECC_SCC_MAILBOX* pMbx;
    OMAP_HECC_SCC_REGS* pCanRegs = pDevice->pCanRegs;
    
    EnterCriticalSection(&pDevice->regcs);
    for (int i=NB_MAILBOXES-1;i>=0;i--)
    {
        if (pDevice->FreeMailboxMask & (1<<i))
        {
            pDevice->FreeMailboxMask &= ~(1<<i);      
            mailbox = i;
            break;
        }
    }
    
    if (mailbox == -1)
    {
        LeaveCriticalSection(&pDevice->regcs);
        RETAILMSG(1,(TEXT("unable to get a fere mailbox for remote request\r\n")));
        return FALSE;

    }

    pDevice->RemoteRequestMask |= (1<<mailbox);
    pMbx = &pDevice->pCanRegs->MailBoxes[mailbox];
    //Configure the mailbox for remote frame rquest     
    pCanRegs->CANME &= ~(1<<mailbox);            
    pMbx->CANMID = IDtoCANMID(pRemoteRequest->id);    
    pMbx->CANMCF = CANMCF_RTR | 8 |(0x3 << 8);    //use highest priority for Remote frames    
    pCanRegs->CANMD |=  (1<<mailbox);
    pCanRegs->CANMIM |= (1<<mailbox);
    SETREG32(&pCanRegs->CANMIL,(1<<mailbox));                    //Remote request mailboxes trigger CAN IRQ 1
    pCanRegs->CANME |= (1<<mailbox);    
    pCanRegs->CANTRS = (1<<mailbox);
    
    LeaveCriticalSection(&pDevice->regcs);
    return TRUE;    
}
