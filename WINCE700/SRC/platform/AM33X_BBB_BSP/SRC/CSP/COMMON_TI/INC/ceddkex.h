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
//  File:  ceddkex.h
//
//  This file contains OMAP specific ceddk extensions.
//
#ifndef __CEDDKEX_H
#define __CEDDKEX_H

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------
#define FILE_DEVICE_DMA                 0x000001DA
#define FILE_DEVICE_I2C                 0x000001DB
#define FILE_DEVICE_TIPMX               0x00008001
#define FILE_DEVICE_DVFS                0x00008002
#define FILE_DEVICE_LIST                0x00008003

//------------------------------------------------------------------------------

#define dimof(x)            (sizeof(x)/sizeof(x[0]))
#define offset(s, f)        FIELD_OFFSET(s, f)
#define fieldsize(s, f)     sizeof(((s*)0)->f)

//------------------------------------------------------------------------------
//
//  Function:  ReadRegistryParams
//
//  This function initializes driver default settings from registry based on
//  table passed as argument.
//
#define PARAM_DWORD             1
#define PARAM_STRING            2
#define PARAM_MULTIDWORD        3
#define PARAM_BIN               4

typedef struct {
    LPTSTR name;
    DWORD  type;
    BOOL   required;
    DWORD  offset;
    DWORD  size;
    PVOID  pDefault;
} DEVICE_REGISTRY_PARAM;

DWORD
GetDeviceRegistryParams(
    LPCWSTR szContext,
    VOID *pBase,
    DWORD count, 
    const DEVICE_REGISTRY_PARAM params[]
    );

DWORD
SetDeviceRegistryParams(
    LPCWSTR szContext,
    VOID *pBase,
    DWORD count, 
    const DEVICE_REGISTRY_PARAM params[]
    );

//------------------------------------------------------------------------------
//
//  Function:  HalContextUpdateDirtyRegister
//
//  update context save mask to indicate registers need to be saved before
//  off
//
void
HalContextUpdateDirtyRegister(
    UINT32 ffRegister
    );

//------------------------------------------------------------------------------
//
//  Type:  DmaChannelContext_t
//
//  contains device channel specifics.
//
typedef struct
{
    int                 index;
    DWORD               type;
    DWORD               channelAddress;
    DWORD               registerSize;
}
DmaChannelContext_t;

//------------------------------------------------------------------------------
#define IOCTL_DMA_RESERVE_CHANNEL          \
    CTL_CODE(FILE_DEVICE_DMA, 0x0101, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef DWORD                   IOCTL_DMA_RESERVE_IN;
typedef DmaChannelContext_t     IOCTL_DMA_RESERVE_OUT;


//------------------------------------------------------------------------------
#define IOCTL_DMA_RELEASE_CHANNEL         \
    CTL_CODE(FILE_DEVICE_DMA, 0x0102, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef DmaChannelContext_t     IOCTL_DMA_RELEASE_IN;


//------------------------------------------------------------------------------
#define IOCTL_DMA_REGISTER_EVENTHANDLE         \
    CTL_CODE(FILE_DEVICE_DMA, 0x0103, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct
{
    DmaChannelContext_t        *pContext;
    HANDLE                      hEvent;
    DWORD                       processId;
} IOCTL_DMA_REGISTER_EVENTHANDLE_IN;


//------------------------------------------------------------------------------
#define IOCTL_DMA_INTERRUPTDONE \
    CTL_CODE(FILE_DEVICE_DMA, 0x0105, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef DmaChannelContext_t     IOCTL_DMA_INTERRUPTDONE_IN;


//------------------------------------------------------------------------------
#define IOCTL_DMA_DISABLESTANDBY \
    CTL_CODE(FILE_DEVICE_DMA, 0x0106, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct
{
    DmaChannelContext_t        *pContext;
    BOOL                       bNoStandby;
    BOOL                       bDelicatedChannel;
} IOCTL_DMA_DISABLESTANDBY_IN;


//------------------------------------------------------------------------------
#define IOCTL_DMA_REQUEST_DEDICATED_INTERRUPT \
    CTL_CODE(FILE_DEVICE_DMA, 0x0107, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef DWORD                   IOCTL_DMA_REQUEST_DEDICATED_INTERRUPT_IN;
typedef struct
{
    DWORD                       IrqNum;
    DWORD                       DmaControllerPhysAddr;
    DWORD                       ffDmaChannels; 
} IOCTL_DMA_REQUEST_DEDICATED_INTERRUPT_OUT;

//------------------------------------------------------------------------------
#define IOCTL_DMA_RELEASE_DEDICATED_INTERRUPT \
    CTL_CODE(FILE_DEVICE_DMA, 0x0108, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef DWORD                   IOCTL_DMA_RELEASE_DEDICATED_INTERRUPT_IN;


//------------------------------------------------------------------------------
//
//  Define: IOCTL_DVFS_OPPNOTIFY
//
//  Used to notify device drivers of a pending OPM change
//
#define IOCTL_DVFS_OPPNOTIFY  \
        ((DWORD)CTL_CODE(FILE_DEVICE_DVFS, 0x0201, METHOD_BUFFERED, FILE_ANY_ACCESS))

typedef struct {
    DWORD               domain; // process id the ack event originated from
    DWORD               newOpp;
    DWORD               oldOpp;
} DVFS_OPPNOTIFY_DATA;

typedef struct {
    DWORD               size;
    DWORD               ffInfo;     // process id the ack event originated from
    DWORD               dwCount;
    DVFS_OPPNOTIFY_DATA rgOppInfo[8];
} IOCTL_DVFS_OPPNOTIFY_IN;

//------------------------------------------------------------------------------
//
//  Define: IOCTL_TIPMX_CONTEXTPATH
//
//  Used to query for the context path of the device driver.  For some
//  drivers, ie Display Driver, there are no API's to get the registry
//  path for the driver so it's necessary to query the device driver 
//  for the path
//
#define IOCTL_TIPMX_CONTEXTPATH  \
        ((DWORD)CTL_CODE(FILE_DEVICE_TIPMX, 0x0101, METHOD_BUFFERED, FILE_ANY_ACCESS))

typedef struct {
    _TCHAR      szContext[MAX_PATH];
} IOCTL_TIPMX_CONTEXTPATH_OUT;

//------------------------------------------------------------------------------
//
//  Define:  IOCTL_DDK_GET_DRIVER_IFC
//
//  This IOCTL code is used to obtain device driver interface pointers used for
//  in-process calls between drivers. The input driver can contain interface
//  GUID. The driver should fill output buffer with interface structure based
//  on interface GUID.
//
#define IOCTL_DDK_GET_DRIVER_IFC        \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0100, METHOD_BUFFERED, FILE_ANY_ACCESS)

//------------------------------------------------------------------------------
//
//  This IOCTL code is used to send context Restore message to device driver 
//
//
#define IOCTL_CONTEXT_RESTORE  \
        CTL_CODE(FILE_DEVICE_LIST, 0x2050, ((DWORD)METHOD_BUFFERED), FILE_ANY_ACCESS)

//------------------------------------------------------------------------------
//  Function:  DmaAllocateChannel
//
//  allocates a dma channel of the requested type.  If successful a non-null
//  handle is returned
//
HANDLE 
DmaAllocateChannel(
    DWORD dmaType
    );

//------------------------------------------------------------------------------
//
//  Function:  DmaFreeChannel
//
//  free's a dma channel
//
BOOL 
DmaFreeChannel(
    HANDLE hDmaChannel
    );

//------------------------------------------------------------------------------
//
//  Function:  DmaEnableInterrupts
//
//  enables interrupts for a dma channel.  The client will be notified of
//  the interrupt by signaling the handle given.  Pass in NULL to disable
//  interrupts
//
BOOL 
DmaEnableInterrupts(
    HANDLE hDmaChannel, 
    HANDLE hEvent
    );

//------------------------------------------------------------------------------
//
//  Function:  DmaGetLogicalChannelId
//
//  returns the logical channel id if successful else -1 on failure
//
DWORD 
DmaGetLogicalChannelId(
    HANDLE hDmaChannel
    );

//------------------------------------------------------------------------------
//
//  Function:  DmaInterruptDone
//
//  reenables interrupts
//
BOOL 
DmaInterruptDone(
    HANDLE hDmaChannel
    );

//------------------------------------------------------------------------------
//
//  Function:  DmaGetLogicalChannel
//
//  returns a pointer to the logical dma channels registers.
//
void* 
DmaGetLogicalChannel(
    HANDLE hDmaChannel
    );

//------------------------------------------------------------------------------
//
//  Function:  DmaDisableStandby
//
//  disable standby in dma controller.
//
BOOL 
DmaDisableStandby(
    HANDLE hDmaChannel, 
    BOOL noStandby
    );

//------------------------------------------------------------------------------
//
//  Function:  DmaDiAllocateInterrupt
//
//  Allocates a dedicated DMA interrupt for a specified number of DMA channels.  
//  If successful a non-null handle is returned.
//  NumberOfChannels - number of DMA channels to allocate.
//  hInterruptEvent - handle to an event to be registered with the DMA hardware interrupt.
//  phChannelHandles - phChannelHandles array of handles to each DMA channel.
//                     handles should be used with functions in dma_utility.
//
HANDLE 
DmaDiAllocateInterrupt(
    DWORD   NumberOfChannels, 
    HANDLE  hInterruptEvent,
    HANDLE  *phChannelHandles
    );

//------------------------------------------------------------------------------
//
//  Function:  DmaDiDmaFreeInterrupt
//
//  Frees a dedicated DMA interrupt and all associated DMA channels.
//
//  hDmaInterrupt - Handle generated by "DmaDiAllocateInterrupt"
//
BOOL 
DmaDiDmaFreeInterrupt(
    HANDLE hDmaInterrupt
    );

//------------------------------------------------------------------------------
//
//  Function:  DmaDiFindInterruptChannelByIndex
//
//  Return Channel that is causing an interrupt
//
//  hDmaInterrupt - Handle generated by "DmaDiAllocateInterrupt"
//
DWORD 
DmaDiFindInterruptChannelByIndex(
    HANDLE hDmaInterrupt
    );

//------------------------------------------------------------------------------
//
//  Function:  DmaDiClearInterruptChannelByIndex
//
//  Clear the DMA Channel interrupt bit flag 
//
//  hDmaInterrupt  - Handle generated by "DmaDiAllocateInterrupt"
//  ChannelIndex   - Channel Index returned from call to DmaDiFindInterruptChannelByIndex
BOOL
DmaDiClearInterruptChannelByIndex(
    HANDLE hDmaInterrupt,
    DWORD ChannelIndex
    );

//------------------------------------------------------------------------------
//
//  Function:  DmaDiInterruptDone
//
//  Call interrupt done dedicated DMA interrupt
//
//  hDmaInterrupt - Handle generated by "DmaDiAllocateInterrupt"
//
BOOL
DmaDiInterruptDone(
    HANDLE hDmaInterrupt
    );

//------------------------------------------------------------------------------
// device mediator list identifiers
//
#define DEVICEMEDIATOR_DEVICE_LIST      (1)
#define DEVICEMEDIATOR_DVFS_LIST        (2)

//------------------------------------------------------------------------------
// constraint constants
//
#define CONSTRAINT_CLASS_DVFS           (0x00000001)
#define CONSTRAINT_CLASS_USER           (0x00010000)

#define CONSTRAINT_STATE_NULL           (0xFFFF0000)
#define CONSTRAINT_STATE_FLOOR          (0xFFFF0001)

#define CONSTRAINT_MSG_DVFS_REQUEST     (0x00000001)
#define CONSTRAINT_MSG_DVFS_NEWOPM      (0x00010001)

#define CONSTRAINT_MSG_INTRLAT_REQUEST  (0x00000001)

#define CONSTRAINT_MSG_POWERDOMAIN_REQUEST (0x00000001)
#define CONSTRAINT_MSG_POWERDOMAIN_UPDATE (0x00020001)

//------------------------------------------------------------------------------
// policy constants
//
#define SMARTREFLEX_POLICY_NAME (L"SMARTREFLEX")
#define SMARTREFLEX_LPR_MODE 0x80010001

typedef struct {
    UINT        size;
    DWORD       powerDomain;
    DWORD       state;
} POWERDOMAIN_CONSTRAINT_INFO;

//------------------------------------------------------------------------------
typedef BOOL (*ConstraintCallback)(HANDLE hRefContext, DWORD msg, void *pParam, UINT size);

//------------------------------------------------------------------------------
//
//  Function:  PmxSendDeviceNotification
//
//  Sends a notification to a given device.
//
DWORD
PmxSendDeviceNotification(
    UINT listId, 
    UINT dwParam, 
    DWORD dwIoControlCode, 
    LPVOID lpInBuf, 
    DWORD nInBufSize, 
    LPVOID lpOutBuf, 
    DWORD nOutBufSize, 
    LPDWORD pdwBytesRet
    );

//------------------------------------------------------------------------------
//
//  Function:  PmxSetConstraintByClass
//
//  Imposes a constraint using an adapter identified by class.
//
HANDLE
PmxSetConstraintByClass(
    DWORD classId,
    DWORD msg,
    void *pParam,
    UINT  size
    );

//------------------------------------------------------------------------------
//
//  Function:  PmxSetConstraintById
//
//  Imposes a constraint using an adapter identified by id.
//
HANDLE
PmxSetConstraintById(
    LPCWSTR szId,
    DWORD msg,
    void *pParam,
    UINT  size
    );

//------------------------------------------------------------------------------
//
//  Function:  PmxUpdateConstraint
//
//  Updates a constraint context.
//
BOOL
PmxUpdateConstraint(
    HANDLE hConstraintContext,
    DWORD msg,
    void *pParam,
    UINT  size
    );

//------------------------------------------------------------------------------
//
//  Function:  PmxReleaseConstraint
//
//  Releases a constraint.
//
void
PmxReleaseConstraint(
    HANDLE hConstraintContext
    );

//-----------------------------------------------------------------------------
// 
//  Function:  PmxRegisterConstraintCallback
//
//  Called by external components to register a callback
//
HANDLE
PmxRegisterConstraintCallback(
    HANDLE hConstraintContext,
    ConstraintCallback fnCallback, 
    void *pParam, 
    UINT  size, 
    HANDLE hRefContext
    );

//-----------------------------------------------------------------------------
// 
//  Function:  PmxUnregisterConstraintCallback
//
//  Called by external components to release a callback
//
BOOL
PmxUnregisterConstraintCallback(
    HANDLE hConstraintContext,
    HANDLE hConstraintCallback
    );

//-----------------------------------------------------------------------------
// 
//  Function:  PmxOpenPolicy
//
//  Called by external components to open an access to a policy
//
HANDLE
PmxOpenPolicy(
    LPCWSTR szId
    );

//-----------------------------------------------------------------------------
// 
//  Function:  PmxNotifyPolicy
//
//  Called by external components to notify a policy
//
BOOL
PmxNotifyPolicy(
    HANDLE hPolicyContext,
    DWORD msg,
    void *pParam,
    UINT  size
    );

//-----------------------------------------------------------------------------
// 
//  Function:  PmxClosePolicy
//
//  Called by external components to close a policy handle
//
void
PmxClosePolicy(
    HANDLE hPolicyContext
    );

//-----------------------------------------------------------------------------
// 
//  Function:  PmxGetPolicyInfo
//
//  Called by external components to impose a policy
//
HANDLE
PmxGetPolicyInfo(
    HANDLE hPolicyContext,
    void  *pParam,
    UINT   size
    );

//-----------------------------------------------------------------------------
// 
//  Function:  PreloadCacheLine
//
//  Preloads buffer into cache
//
VOID
PreloadCacheLine(
    void  *pBuffer,
    UINT   size
    );

//-----------------------------------------------------------------------------
// 
//  Function:  PmxLoadConstraint
//
//  Called by external components to dynamically load a constraint adapter
//
HANDLE
PmxLoadConstraint(
    void *context
    );

//-----------------------------------------------------------------------------
// 
//  Function:  PmxLoadConstraintAdapter
//
//  Called by external components to dynamically load a constraint adapter
//
HANDLE
PmxLoadPolicy(
    void *context
    );


//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
//  Function:  BusClkRelease
//
BOOL
BusClockRelease(
    HANDLE hBus, 
    UINT id
    );

//------------------------------------------------------------------------------
//
//  Function:  BusClockRequest
//
BOOL
BusClockRequest(
    HANDLE hBus, 
    UINT id
    );

//------------------------------------------------------------------------------
//
//  Function:  BusSourceClocks
//
BOOL
BusSourceClocks(
    HANDLE hBus, 
    UINT id,
    UINT count,
    UINT rgSourceClocks[]
    );
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
//  Function:  GetDisplayResolutionFromBootArgs
//
void
GetDisplayResolutionFromBootArgs(
    DWORD * pDispRes
    );

//------------------------------------------------------------------------------
//
//  Function:  ConvertCAtoPA
//
DWORD 
ConvertCAtoPA(DWORD * va);


//------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif
