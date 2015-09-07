// All rights reserved ADENEO EMBEDDED 2010
/*
===============================================================================
*             Texas Instruments OMAP(TM) Platform Software
* (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
*
* Use of this software is controlled by the terms and conditions found
* in the license agreement under which this software has been supplied.
*
===============================================================================
*/
//
//  This file contains OMAP specific oal extensions.
//
#ifndef __OALEX_H
#define __OALEX_H

#ifdef __cplusplus
extern "C" {
#endif

#include "omap_cpuver.h"
#include <oal_memory.h>
#include <oemglobal.h>
#include <omap_types.h>

//-----------------------------------------------------------------------------
//
//  Valid parameters for dwOEMHighSecurity
//
#define OEM_HIGH_SECURITY_HS            1
#define OEM_HIGH_SECURITY_GP            2

//-----------------------------------------------------------------------------
//
//  Define:  OAL_ARG_QUERY_xxx
//
#define OAL_ARGS_QUERY_COLDBOOT         (BSP_ARGS_QUERY + 0)
#define OAL_ARGS_QUERY_HWENTROPY        (BSP_ARGS_QUERY + 1)
#define OAL_ARGS_QUERY_RNDISMAC         (BSP_ARGS_QUERY + 2)
#define OAL_ARGS_QUERY_DEVICEID         (BSP_ARGS_QUERY + 3)
#define OAL_ARGS_QUERY_OALFLAGS         (BSP_ARGS_QUERY + 4)
#define OAL_ARGS_QUERY_ETHADDR_CPGMAC   (BSP_ARGS_QUERY + 5)
#define OAL_ARGS_QUERY_DEVICE_PREFIX    (BSP_ARGS_QUERY + 6)
#define OAL_ARGS_QUERY_DISP_RES         (BSP_ARGS_QUERY + 7)
#define OAL_ARGS_QUERY_ETHADDR_CPGMAC1  (BSP_ARGS_QUERY + 8)
#define OAL_ARGS_QUERY_ECC_TYPE         (BSP_ARGS_QUERY + 9)
#define OAL_ARGS_QUERY_OPP_MODE         (BSP_ARGS_QUERY + 10)
#define OAL_ARGS_QUERY_EBOOT_CFG        (BSP_ARGS_QUERY + 11)
#define OAL_ARGS_QUERY_EBOOT_CFGSZ      (BSP_ARGS_QUERY + 12)
#define OAL_ARGS_QUERY_BOOT_INFO        (BSP_ARGS_QUERY + 13)

// must match the BOOT_CFG_OALFLAGS_... defines
#define OAL_ARGS_OALFLAGS_RETAILMSG_ENABLE  (1 << 0)
#define OAL_ARGS_OALFLAGS_CFG_SAVE          (1 << 1)
#define OAL_ARGS_OALFLAGS_CLEAN_REGISTRY    (1 << 3)
#define OAL_ARGS_OALFLAGS_INVERT_DISPLAY    (1 << 4)
#define OAL_ARGS_OALFLAGS_PORTRAIT_DISPLAY  (1 << 5)


//-----------------------------------------------------------------------------
//
//  maximum number of domains which are DVFS capable, set to high value for
//  future compatability
//
#define MAX_DVFS_DOMAINS                (8)

//-----------------------------------------------------------------------------
//
//  Custom cache sync value to invalidate only data cache
//
#define TI_CACHE_SYNC_INVALIDATE        0x80000000

//-----------------------------------------------------------------------------
//
//  Custom CeSetMemoryAttributes
//
#define TI_PAGE_NOCACHE                 (PAGE_NOCACHE)
#define TI_PAGE_WRITECOMBINE            (PAGE_WRITECOMBINE)
#define TI_PAGE_WRITETHROUGH            (0x80000801)
#define TI_PAGE_WRITEBACK               (0x80000802)

#ifndef PAGE_WRITETHROUGH
#define PAGE_WRITETHROUGH               (0x100000)
#endif

//-----------------------------------------------------------------------------
//
//  Extern:  g_CpuFamily
//
//  Set during OEMInit to indicate CPU family.
//
extern DWORD g_dwCpuFamily;

// see IOCTL_HAL_GET_CPUFAMILY for values

//-----------------------------------------------------------------------------
//
//  Extern:  g_CpuRevision
//
//  Set during OEMInit to indicate CPU revision.
//
extern DWORD g_dwCpuRevision;

// see IOCTL_HAL_GET_CPUREVISION for values

//-----------------------------------------------------------------------------
//
//  Extern:  g_oalRetailMsgEnable
//
//  Used to enable retail messages
//
extern BOOL g_oalRetailMsgEnable;

//-----------------------------------------------------------------------------
//
//  Global:  g_ResumeRTC
//
//  Used to inform RTC code that a resume occured
//
extern BOOL g_ResumeRTC;

//-----------------------------------------------------------------------------
//
//  Extern:  g_oalAddressTable 
//
//  This table is used by kernel to establish the virtual to physical address
//  mapping. It is also used in OALPAtoVA and OALVAtoPA functions to allow
//  get virtual address for physical and vice versa.
//
extern OAL_ADDRESS_TABLE g_oalAddressTable[];

//-----------------------------------------------------------------------------
//
//  Global:  dwOEMTargetProject
//
//  This global variable specify target BSP project. It is used to modify
//  code behavior on different project types. Variable is modified by FIXUP
//  statement in config.bib.
//
extern const volatile DWORD dwOEMTargetProject;

//-----------------------------------------------------------------------------
//
//  Global:  dwOEMHighSecurity
//
//  This global variable to distinguish HS and GP devices. It is used to modify
//  code behavior on different security types. Variable is modified by FIXUP
//  statement in config.bib.
//
extern const volatile DWORD dwOEMHighSecurity;

//-----------------------------------------------------------------------------
//
//  Globals:  g_oalProfilerEnabled
//            g_oalPerfTimerIrq  
//
//  Profiler timer enabled flag and IRQ
//
extern BOOL   g_oalProfilerEnabled;
extern UINT32 g_oalPerfTimerIrq;

//-----------------------------------------------------------------------------
// Power related IOCTL's
#define FILE_DEVICE_PRCM            (501)
#define FILE_DEVICE_OMAPPOWEREXT    (601)
#define FILE_DEVICE_SMARTREFLEX     (701) // UNDONE: smartreflex should be autonomous

//-----------------------------------------------------------------------------
//
//  Define: INTERRUPTS_STATUS
//
//  returns if ARM interrupts are enabled or not
//
BOOL
INTERRUPTS_STATUS();

//-----------------------------------------------------------------------------
//
//  Define: IOCTL_SMARTREFLEX_CONTROL
//
//  This code is used to enable/disable smartreflex
//
#define IOCTL_SMARTREFLEX_CONTROL \
    CTL_CODE(FILE_DEVICE_SMARTREFLEX, 1001, METHOD_BUFFERED, FILE_ANY_ACCESS)

BOOL 
OALIoCtlSmartReflexControl(
    UINT32 code, 
    VOID  *pInBuffer,
    UINT32 inSize, 
    VOID  *pOutBuffer, 
    UINT32 outSize, 
    UINT32*pOutSize
    );

//-----------------------------------------------------------------------------
//
//  Define: IOCTL_UPDATE_RETENTION_VOLTAGES
//
//  This code is used to change VDD1 and VDD2 retention voltages
//
#define IOCTL_UPDATE_RETENTION_VOLTAGES \
    CTL_CODE(FILE_DEVICE_SMARTREFLEX, 1002, METHOD_BUFFERED, FILE_ANY_ACCESS)

BOOL
OALIoCtlUpdateRetentionVoltages(
    UINT32 code,
    VOID  *pInBuffer,
    UINT32 inSize,
    VOID  *pOutBuffer,
    UINT32 outSize,
    UINT32*pOutSize
    );

//-----------------------------------------------------------------------------
//
//  Define: IOCTL_RETENTION_VOLTAGES
//
//  Generic structure that contains the retention voltage values
//
typedef struct {
    UINT               retentionVoltage[2];
} IOCTL_RETENTION_VOLTAGES;

//-----------------------------------------------------------------------------
//
//  Define: IOCTL_PRCM_DEVICE_ENABLE_IN
// 
//  Generic structure used to enable/disable a particular functionality
//  for a given device
//
typedef struct {
    GUID               guid;
    UINT               devId;  
    BOOL               bEnable;
} IOCTL_PRCM_DEVICE_ENABLE_IN;

//-----------------------------------------------------------------------------
//
//  Define: IOCTL_INTERRUPT_LATENCY_CONSTRAINT
//
//  This code is used to set a constraint on the cpu idle code to what the
//  minimal wake-up latency is.
//
#define IOCTL_INTERRUPT_LATENCY_CONSTRAINT \
    CTL_CODE(FILE_DEVICE_OMAPPOWEREXT, 1001, METHOD_BUFFERED, FILE_ANY_ACCESS)

BOOL 
OALIoCtlInterruptLatencyConstraint(
    UINT32 code, 
    VOID  *pInBuffer,
    UINT32 inSize, 
    VOID  *pOutBuffer, 
    UINT32 outSize, 
    UINT32*pOutSize
    );

//-----------------------------------------------------------------------------
//
//  Define: IOCTL_PRCM_DEVICE_GET_DEVICEMANAGEMENTTABLE
//
//  This code is used to retrieve the ref count of the core clocks
//
#define IOCTL_PRCM_DEVICE_GET_DEVICEMANAGEMENTTABLE    \
    CTL_CODE(FILE_DEVICE_PRCM, 1001, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct {
    BOOL (*pfnEnableDeviceClocks)(UINT clkId, BOOL bEnable);
    BOOL (*pfnEnableDeviceIClock)(UINT clkId, BOOL bEnable);
    BOOL (*pfnEnableDeviceFClock)(UINT clkId, BOOL bEnable);
    BOOL (*pfnEnableDeviceClockAutoIdle)(UINT clkId, BOOL bEnable);
    BOOL (*pfnSetSourceDeviceClocks)(UINT devId, UINT count, UINT rgSourceClocks[]);
    BOOL (*pfnGetDeviceContextState)(UINT devId, BOOL bSet);
    DWORD (*pfnUpdateOnDeviceStateChange)(UINT devId, UINT oldState, UINT newState, BOOL bPreSetChange);
    float (*pfnGetSystemClockFrequency)();
	BOOL (*pfnPrcmDomainResetStatus)(UINT powerDomain, UINT *pResetStatus, BOOL	bClear);
	BOOL (*pfnPrcmDomainReset)(UINT powerDomain, UINT resetMask);
	BOOL (*pfnPrcmDomainResetRelease)(UINT powerDomain, UINT resetMask);
    UINT32 (*pfnPrcmClockGetClockRate)(OMAP_CLOCKID clock_id);
    VOID (*pfnPrcmCM3ResetAndHandshake)();
} OMAP_DEVCLKMGMT_FNTABLE;

BOOL 
OALIoCtlPrcmDeviceGetDeviceManagementTable(
    UINT32 code, 
    VOID  *pInBuffer,
    UINT32 inSize, 
    VOID  *pOutBuffer, 
    UINT32 outSize, 
    UINT32*pOutSize
    );

//-----------------------------------------------------------------------------
//
//  Define: IOCTL_PRCM_DEVICE_SET_AUTOIDLESTATE
//
//  This code is used to enable/disable device autoidle
//
#define IOCTL_PRCM_DEVICE_SET_AUTOIDLESTATE    \
    CTL_CODE(FILE_DEVICE_PRCM, 1002, METHOD_BUFFERED, FILE_ANY_ACCESS)

BOOL 
OALIoCtlPrcmDeviceSetAutoIdleState(
    UINT32 code, 
    VOID  *pInBuffer,
    UINT32 inSize, 
    VOID  *pOutBuffer, 
    UINT32 outSize, 
    UINT32*pOutSize
    );

//-----------------------------------------------------------------------------
//
//  Define: IOCTL_PRCM_CLOCK_GET_SOURCECLOCKINFO
//
//  Given a device id, information about the device's source clock will be 
//  returned
//
#define IOCTL_PRCM_CLOCK_GET_SOURCECLOCKINFO    \
    CTL_CODE(FILE_DEVICE_PRCM, 1003, METHOD_BUFFERED, FILE_ANY_ACCESS)

// Level definition:
//  1 = Source clock
//  2 = dpll clock out
//  3 = dpll
//  4 = Vdd
//
typedef struct {
    UINT                clockId;
    DWORD               clockLevel;
} IOCTL_PRCM_CLOCK_GET_SOURCECLOCKINFO_IN;

typedef struct {
    LONG                refCount;    
    UINT                parentId;
    UINT                parentLevel;
    UINT                parentRefCount;
} IOCTL_PRCM_CLOCK_GET_SOURCECLOCKINFO_OUT;

BOOL 
OALIoCtlPrcmClockGetSourceClockInfo(
    UINT32 code, 
    VOID *pInBuffer,
    UINT32 inSize, 
    VOID *pOutBuffer, 
    UINT32 outSize, 
    UINT32 *pOutSize
    );

//-----------------------------------------------------------------------------
//
//  Define: IOCTL_PRCM_DEVICE_GET_SOURCECLOCKINFO
//
//  Given a device id, information about the device's source clock will be 
//  returned
//
#define IOCTL_PRCM_DEVICE_GET_SOURCECLOCKINFO    \
    CTL_CODE(FILE_DEVICE_PRCM, 1004, METHOD_BUFFERED, FILE_ANY_ACCESS)
    
typedef struct {
    UINT                count;
    struct {
        UINT                clockId;
        UINT                nLevel;
        LONG                refCount;    
    }                   rgSourceClocks[3];
} IOCTL_PRCM_DEVICE_GET_SOURCECLOCKINFO_OUT;

BOOL 
OALIoCtlPrcmDeviceGetSourceClockInfo(
    UINT32 code, 
    VOID *pInBuffer,
    UINT32 inSize, 
    VOID *pOutBuffer, 
    UINT32 outSize, 
    UINT32 *pOutSize
    );

//-----------------------------------------------------------------------------
//
//  Define: IOCTL_PRCM_DEVICE_GET_DEVICESTATUS
//
//  retrieves the current status of the device.
//
#define IOCTL_PRCM_DEVICE_GET_DEVICESTATUS    \
    CTL_CODE(FILE_DEVICE_PRCM, 1005, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct {
    BOOL            bEnabled;
    BOOL            bAutoIdle;
} IOCTL_PRCM_DEVICE_GET_DEVICESTATUS_OUT;
    
BOOL 
OALIoCtlPrcmDeviceGetDeviceStatus(
    UINT32 code, 
    VOID *pInBuffer,
    UINT32 inSize, 
    VOID *pOutBuffer, 
    UINT32 outSize, 
    UINT32 *pOutSize
    );

//-----------------------------------------------------------------------------
//
//  Define: IOCTL_PRCM_CLOCK_SET_SOURCECLOCK
//
//  changes the source clock of a given functional clock.
//
#define IOCTL_PRCM_CLOCK_SET_SOURCECLOCK    \
    CTL_CODE(FILE_DEVICE_PRCM, 1006, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct {
    UINT            clkId;
    UINT            newParentClkId;
} IOCTL_PRCM_CLOCK_SET_SOURCECLOCK_IN;
    
BOOL 
OALIoCtlPrcmClockSetSourceClock(
    UINT32 code, 
    VOID *pInBuffer,
    UINT32 inSize, 
    VOID *pOutBuffer, 
    UINT32 outSize, 
    UINT32 *pOutSize
    );

//-----------------------------------------------------------------------------
//
//  Define: IOCTL_PRCM_CLOCK_SET_SOURCECLOCKDIVISOR
//
//  changes the source clock of a given functional clock.
//
#define IOCTL_PRCM_CLOCK_SET_SOURCECLOCKDIVISOR    \
    CTL_CODE(FILE_DEVICE_PRCM, 1007, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct {
    UINT            clkId;
    UINT            parentClkId;
    UINT            divisor;
} IOCTL_PRCM_CLOCK_SET_SOURCECLOCKDIVISOR_IN;
    
BOOL 
OALIoCtlPrcmClockSetSourceClockDivisor(
    UINT32 code, 
    VOID *pInBuffer,
    UINT32 inSize, 
    VOID *pOutBuffer, 
    UINT32 outSize, 
    UINT32 *pOutSize
    );

//-----------------------------------------------------------------------------
//
//  Define: IOCTL_PRCM_CLOCK_SET_DPLLSTATE
//
//  This code is used to update a dpll setting
//
#define IOCTL_PRCM_CLOCK_SET_DPLLSTATE    \
    CTL_CODE(FILE_DEVICE_PRCM, 1008, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct {
    UINT            size;
    UINT            dpllId;
    UINT            ffMask;
    UINT            lowPowerEnabled;
    UINT            rampTime;    
    UINT            driftGuardEnabled;
    UINT            dpllMode;
    UINT            dpllAutoidleState;
    UINT            rampLevel;    
    UINT            rampOnRelock;    
    UINT            bypassSelect;    
} IOCTL_PRCM_CLOCK_SET_DPLLSTATE_IN;

BOOL 
OALIoCtlPrcmClockSetDpllState(
    UINT32 code, 
    VOID *pInBuffer,
    UINT32 inSize, 
    VOID *pOutBuffer, 
    UINT32 outSize, 
    UINT32 *pOutSize
    );

//-----------------------------------------------------------------------------
//
//  Define: IOCTL_PRCM_DOMAIN_SET_WAKEUPDEP
//
//  This code is used to update a domains wake-up dependency
//
#define IOCTL_PRCM_DOMAIN_SET_WAKEUPDEP    \
    CTL_CODE(FILE_DEVICE_PRCM, 1009, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct {
    UINT            size;
    UINT            powerDomain;
    UINT            ffWakeDep;
    BOOL            bEnable;
} IOCTL_PRCM_DOMAIN_SET_WAKEUPDEP_IN;

BOOL 
OALIoCtlPrcmDomainSetWakeupDependency(
    UINT32 code, 
    VOID *pInBuffer,
    UINT32 inSize, 
    VOID *pOutBuffer, 
    UINT32 outSize, 
    UINT32 *pOutSize
    );

//-----------------------------------------------------------------------------
//
//  Define: IOCTL_PRCM_DOMAIN_SET_SLEEPDEP
//
//  This code is used to update a dpll setting
//
#define IOCTL_PRCM_DOMAIN_SET_SLEEPDEP    \
    CTL_CODE(FILE_DEVICE_PRCM, 1010, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct {
    UINT            size;
    UINT            powerDomain;
    UINT            ffSleepDep;
    BOOL            bEnable;
} IOCTL_PRCM_DOMAIN_SET_SLEEPDEP_IN;

BOOL 
OALIoCtlPrcmDomainSetSleepDependency(
    UINT32 code, 
    VOID *pInBuffer,
    UINT32 inSize, 
    VOID *pOutBuffer, 
    UINT32 outSize, 
    UINT32 *pOutSize
    );

//-----------------------------------------------------------------------------
//
//  Define: IOCTL_PRCM_DOMAIN_SET_POWERSTATE
//
//  This code is used to update a dpll setting
//
#define IOCTL_PRCM_DOMAIN_SET_POWERSTATE    \
    CTL_CODE(FILE_DEVICE_PRCM, 1011, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct {
    UINT            size;
    UINT            powerDomain;
    UINT            powerState;
    UINT            logicState;
} IOCTL_PRCM_DOMAIN_SET_POWERSTATE_IN;

BOOL 
OALIoCtlPrcmDomainSetPowerState(
    UINT32 code, 
    VOID *pInBuffer,
    UINT32 inSize, 
    VOID *pOutBuffer, 
    UINT32 outSize, 
    UINT32 *pOutSize
    );

//-----------------------------------------------------------------------------
//
//  Define: IOCTL_PRCM_SET_SUSPENDSTATE
//
//  Specifies to the kernel copy i2c function table
//
#define IOCTL_PRCM_SET_SUSPENDSTATE  \
    CTL_CODE(FILE_DEVICE_PRCM, 1012, METHOD_BUFFERED, FILE_ANY_ACCESS)

BOOL
OALIoCtlPrcmSetSuspendState(
    UINT32 code,
    VOID *pInBuffer,
    UINT32 inSize,
    VOID *pOutBuffer,
    UINT32 outSize,
    UINT32 *pOutSize
    );

//-----------------------------------------------------------------------------
//
//  Define: IOCTL_OPP_REQUEST
//
//  This code is used to change operating points
//
#define IOCTL_OPP_REQUEST    \
    CTL_CODE(FILE_DEVICE_PRCM, 1013, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct {
    DWORD       size;           // mask of system power state
    DWORD       dwCount;
    DWORD       rgDomains[MAX_DVFS_DOMAINS];
    DWORD       rgOpps[MAX_DVFS_DOMAINS];
} IOCTL_OPP_REQUEST_IN;

BOOL 
OALIoCtlOppRequest(
    UINT32 code, 
    VOID *pInBuffer,
    UINT32 inSize, 
    VOID *pOutBuffer, 
    UINT32 outSize, 
    UINT32 *pOutSize
    );

//-----------------------------------------------------------------------------
//

//  Define: IOCTL_CONTEXTSAVE_GETBUFFER
//
//  This code is used to notify kernel a register set has been modified
//  and to save context before OFF mode
//
#define HAL_CONTEXTSAVE_GPIO            0x00000001
#define HAL_CONTEXTSAVE_SCM             0x00000002
#define HAL_CONTEXTSAVE_DMA             0x00000004
#define HAL_CONTEXTSAVE_GPMC            0x00000010
#define HAL_CONTEXTSAVE_INTC            0x00000020
#define HAL_CONTEXTSAVE_PRCM            0x00000040
#define HAL_CONTEXTSAVE_VRFB            0x00000080
#define HAL_CONTEXTSAVE_PINMUX          0x00000100
#define HAL_CONTEXTSAVE_SMS             0x00000200

#define IOCTL_HAL_CONTEXTSAVE_GETBUFFER \
    CTL_CODE(FILE_DEVICE_PRCM, 1014, METHOD_BUFFERED, FILE_ANY_ACCESS)

BOOL 
OALIoCtlHalContextSaveGetBuffer(
    UINT32 code, 
    VOID *pInBuffer,
    UINT32 inSize, 
    VOID *pOutBuffer, 
    UINT32 outSize, 
    UINT32 *pOutSize
    );
/*
//-----------------------------------------------------------------------------
//
//  Define: IOCTL_PRCM_DOMAIN_SET_CLOCKSTATE
//
//  This code is used to update a dpll setting
//
#define IOCTL_PRCM_DOMAIN_SET_CLOCKSTATE    \
    CTL_CODE(FILE_DEVICE_PRCM, 1015, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct {
    UINT            size;
    UINT            powerDomain;
    UINT            clockDomain;
    UINT            clockState;
} IOCTL_PRCM_DOMAIN_SET_CLOCKSTATE_IN;

BOOL 
OALIoCtlPrcmDomainSetClockState(
    UINT32 code, 
    VOID *pInBuffer,
    UINT32 inSize, 
    VOID *pOutBuffer, 
    UINT32 outSize, 
    UINT32 *pOutSize
    );
*/

//-----------------------------------------------------------------------------
//
//  Define: IOCTL_PRCM_CLOCK_SET_DPLLCLKOUTSTATE
//
//  This code is used to update a dpll setting
//
#define IOCTL_PRCM_CLOCK_SET_DPLLCLKOUTSTATE    \
    CTL_CODE(FILE_DEVICE_PRCM, 1016, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct {
    UINT            size;
    UINT            dpllClkOutId;
    UINT            ffMask;
    UINT            autoGateEn;
    UINT            pwdnEn;    
    UINT            divisor;
} IOCTL_PRCM_CLOCK_SET_DPLLCLKOUTSTATE_IN;

BOOL 
OALIoCtlPrcmClockSetDpllClkOutState(
    UINT32 code, 
    VOID *pInBuffer,
    UINT32 inSize, 
    VOID *pOutBuffer, 
    UINT32 outSize, 
    UINT32 *pOutSize
    );

//-----------------------------------------------------------------------------
//
//  Define: IOCTL_HAL_POSTINIT
//
//  This code allows HAL to perform initialization after the kernel features
//  are enabled
//
BOOL 
OALIoCtlHALPostInit(
    UINT32 code, 
    VOID *pInBuffer,
    UINT32 inSize, 
    VOID *pOutBuffer, 
    UINT32 outSize, 
    UINT32 *pOutSize
    );

//-----------------------------------------------------------------------------
//
//  Define: IOCTL_HAL_OEM_PROFILER
//
//  This code enables/disables HAL specific profiling features
//
BOOL 
OALIoCtlHALProfiler(
    UINT32 code, 
    VOID *pInBuffer,
    UINT32 inSize, 
    VOID *pOutBuffer, 
    UINT32 outSize, 
    UINT32 *pOutSize
    );

//-----------------------------------------------------------------------------
//
//  Define: IOCTL_HAL_PROFILE
//
//  Deprecated IOCTL.  This handler is just to filter out the warnings.
//
BOOL 
OALIoCtlIgnore(
    UINT32 code, 
    VOID *pInBuffer,
    UINT32 inSize, 
    VOID *pOutBuffer, 
    UINT32 outSize, 
    UINT32 *pOutSize
    );

//-----------------------------------------------------------------------------
//
//  Define: IOCTL_HAL_IRQ2SYSINTR
//
//  This code is used to translate an IRQ to the SYSINTR
//
#define IOCTL_HAL_IRQ2SYSINTR  \
    CTL_CODE(FILE_DEVICE_HAL, 2048, METHOD_BUFFERED, FILE_ANY_ACCESS)

BOOL 
OALIoCtlHalIrq2Sysintr(
    UINT32 code, 
    VOID *pInBuffer,
    UINT32 inSize, 
    VOID *pOutBuffer, 
    UINT32 outSize, 
    UINT32 *pOutSize
    );

//-----------------------------------------------------------------------------
//
//  Define: IOCTL_HAL_GET_CPUID
//
//  This code is used to CPU CORE ID
//
#define IOCTL_HAL_GET_CPUID    \
    CTL_CODE(FILE_DEVICE_HAL, 2049, METHOD_BUFFERED, FILE_ANY_ACCESS)

BOOL 
OALIoCtlHalGetCpuID(
    UINT32 code, 
    VOID *pInBuffer,
    UINT32 inSize, 
    VOID *pOutBuffer, 
    UINT32 outSize, 
    UINT32 *pOutSize
    );

//-----------------------------------------------------------------------------
//
//  Define: IOCTL_HAL_GET_DIEID
//
//  This code is used to OMAP DIE ID
//
#define IOCTL_HAL_GET_DIEID    \
    CTL_CODE(FILE_DEVICE_HAL, 2050, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct {
    DWORD   dwIdCode;
    DWORD   dwProdID_0;       
    DWORD   dwProdID_1;       
    DWORD   dwProdID_2;       
    DWORD   dwProdID_3;       
    DWORD   dwDieID_0;       
    DWORD   dwDieID_1;       
    DWORD   dwDieID_2;       
    DWORD   dwDieID_3;       
} IOCTL_HAL_GET_DIEID_OUT;

BOOL 
OALIoCtlHalGetDieID(
    UINT32 code, 
    VOID *pInBuffer,
    UINT32 inSize, 
    VOID *pOutBuffer, 
    UINT32 outSize, 
    UINT32 *pOutSize
    );

//-----------------------------------------------------------------------------
//
//  Define: IOCTL_HAL_GET_CPUFAMILY
//
//  This code is used to get the CPU family
//
#define IOCTL_HAL_GET_CPUFAMILY    \
    CTL_CODE(FILE_DEVICE_HAL, 2051, METHOD_BUFFERED, FILE_ANY_ACCESS)

BOOL 
OALIoCtlHalGetCpuFamily(
    UINT32 code, 
    VOID *pInBuffer,
    UINT32 inSize, 
    VOID *pOutBuffer, 
    UINT32 outSize, 
    UINT32 *pOutSize
    );

//-----------------------------------------------------------------------------
//
//  Define: IOCTL_HAL_GET_CPUREVISION
//
//  This code is used to CPU revision
//
#define IOCTL_HAL_GET_CPUREVISION    \
    CTL_CODE(FILE_DEVICE_HAL, 2052, METHOD_BUFFERED, FILE_ANY_ACCESS)

BOOL 
OALIoCtlHalGetCpuRevision(
    UINT32 code, 
    VOID *pInBuffer,
    UINT32 inSize, 
    VOID *pOutBuffer, 
    UINT32 outSize, 
    UINT32 *pOutSize
    );

//-----------------------------------------------------------------------------
//
//  Define: IOCTL_HAL_GET_CPUSPEED
//
//  This code is used to get the CPU current speed in MHz
//
#define IOCTL_HAL_GET_CPUSPEED    \
    CTL_CODE(FILE_DEVICE_HAL, 3104, METHOD_BUFFERED, FILE_ANY_ACCESS)

BOOL 
OALIoCtlHalGetCpuSpeed(
    UINT32 code, 
    VOID *pInBuffer,
    UINT32 inSize, 
    VOID *pOutBuffer, 
    UINT32 outSize, 
    UINT32 *pOutSize
    );


//-----------------------------------------------------------------------------
//
//  Define: IOCTL_HAL_GET_BSP_VERSION
//
//  Retrieve the BSP version
//
#define IOCTL_HAL_GET_BSP_VERSION  \
    CTL_CODE(FILE_DEVICE_HAL, 3100, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct {
    DWORD   dwVersionMajor;
    DWORD   dwVersionMinor;   
	DWORD	dwVersionQFES;
    DWORD   dwVersionIncremental;       
} IOCTL_HAL_GET_BSP_VERSION_OUT;

BOOL 
OALIoCtlHalGetBspVersion(
    UINT32 code, 
    VOID *pInpBuffer,
    UINT32 inpSize, 
    VOID *pOutBuffer, 
    UINT32 outSize, 
    UINT32 *pOutSize
    );

//-----------------------------------------------------------------------------
//
//  Define: IOCTL_HAL_GET_DSP_INFO
//
//  This code is used to get the configured speed of the IVA2 (DSP) subsystem
//
#define IOCTL_HAL_GET_DSP_INFO    \
    CTL_CODE(FILE_DEVICE_HAL, 3101, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct {
    DWORD   dwSpeedMHz;
} IOCTL_HAL_GET_DSP_INFO_OUT;

BOOL 
OALIoCtlHalGetDspInfo(
    UINT32 code, 
    VOID *pInBuffer,
    UINT32 inSize, 
    VOID *pOutBuffer, 
    UINT32 outSize, 
    UINT32 *pOutSize
    );

//-----------------------------------------------------------------------------
//
//  Define: IOCTL_HAL_DUMP_REGISTERS
//
//  This code is used to dump CPU registers
//
#define IOCTL_HAL_DUMP_REGISTERS    \
    CTL_CODE(FILE_DEVICE_HAL, 3102, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_HAL_DUMP_REGISTERS_PRCM   0
#define IOCTL_HAL_DUMP_DEVICE_STATUS    1


BOOL OALIoCtlHalDumpRegisters(
    UINT32 code, 
	VOID *pInpBuffer,
    UINT32 inpSize, 
	VOID *pOutBuffer, 
	UINT32 outSize, 
	UINT32 *pOutSize
	);

//-----------------------------------------------------------------------------
//
//  Define: IOCTL_HAL_GET_ECC_TYPE
//
//  GET ECC type -Hamming 1 bit/BCH 4bit /BCH8 bit
//

#define IOCTL_HAL_GET_ECC_TYPE                \
    CTL_CODE(FILE_DEVICE_HAL, 3110, METHOD_BUFFERED, FILE_ANY_ACCESS)

BOOL OALIoctlHalGetEccType(
    UINT32 code, VOID *pInBuffer,  UINT32 inSize,  VOID *pOutBuffer, 
    UINT32 outSize, UINT32 *pOutSize);

//-----------------------------------------------------------------------------
//
//  Define: IOCTL_HAL_GET_BOOT_INFO
//
//  GET boot info data
//

#define IOCTL_HAL_GET_BOOT_INFO                \
    CTL_CODE(FILE_DEVICE_HAL, 3111, METHOD_BUFFERED, FILE_ANY_ACCESS)

BOOL OALIoctlHalGetBootInfo(
    UINT32 code, VOID *pInBuffer,  UINT32 inSize,  VOID *pOutBuffer, 
    UINT32 outSize, UINT32 *pOutSize);


//-----------------------------------------------------------------------------
//
//  Define: IOCTL_HAL_GET_PLATFORM_ID
//
//  GET platform ID
//

#define IOCTL_HAL_GET_PLATFORM_ID                \
    CTL_CODE(FILE_DEVICE_HAL, 3112, METHOD_BUFFERED, FILE_ANY_ACCESS)

BOOL OALIoctlHalGetPlatformID(
    UINT32 code, VOID *pInBuffer,  UINT32 inSize,  VOID *pOutBuffer, 
    UINT32 outSize, UINT32 *pOutSize);

//-----------------------------------------------------------------------------
//
//  Define: IOCTL_HAL_GET_IRQ_COUNTERS
//
//
#define IOCTL_HAL_GET_IRQ_COUNTERS    \
    CTL_CODE(FILE_DEVICE_HAL, 3202, METHOD_BUFFERED, FILE_ANY_ACCESS)


BOOL OALIoCtlHalGetIrqCounters(
    UINT32 code, 
	VOID *pInpBuffer,
    UINT32 inpSize, 
	VOID *pOutBuffer, 
	UINT32 outSize, 
	UINT32 *pOutSize
	);

//-----------------------------------------------------------------------------
//
//  Define: IOCTL_HAL_RTC_TIME
//
//  This code is used to indicate to kernel that a periodic time event from
//  RTC chip occurred
//
#define IOCTL_HAL_RTC_TIME    \
    CTL_CODE(FILE_DEVICE_HAL, 1024, METHOD_BUFFERED, FILE_ANY_ACCESS)

BOOL 
OALIoCtlHalRtcTime(
    UINT32 code,
    VOID *pInBuffer,
    UINT32 inSize,
    VOID *pOutBuffer,
    UINT32 outSize,
    UINT32 *pOutSize
    );

//-----------------------------------------------------------------------------
//
//  Define: IOCTL_HAL_RTC_ALARM
//
//  This code is used to call from TWL(Triton) chip driver when alarm occurs.
//
#define IOCTL_HAL_RTC_ALARM    \
    CTL_CODE(FILE_DEVICE_HAL, 1025, METHOD_BUFFERED, FILE_ANY_ACCESS)

BOOL 
OALIoCtlHalRtcAlarm(
    UINT32 code, 
    VOID *pInBuffer,
    UINT32 inSize, 
    VOID *pOutBuffer, 
    UINT32 outSize, 
    UINT32 *pOutSize
    );

//-----------------------------------------------------------------------------
//
//  Define: IOCTL_HAL_I2CMODE
//
//  Specifies to the kernel if a i2c is in hs mode
//
#define IOCTL_HAL_I2CHSMODE  \
    CTL_CODE(FILE_DEVICE_HAL, 1028, METHOD_BUFFERED, FILE_ANY_ACCESS)

BOOL 
OALIoCtlHalI2CMode(
    UINT32 code, 
    VOID *pInBuffer,
    UINT32 inSize, 
    VOID *pOutBuffer, 
    UINT32 outSize, 
    UINT32 *pOutSize
    );

//-----------------------------------------------------------------------------
//
//  Define: IOCTL_HAL_I2CCOPYFNTABLE
//
//  Specifies to the kernel copy i2c function table
//
#define IOCTL_HAL_I2CCOPYFNTABLE  \
    CTL_CODE(FILE_DEVICE_HAL, 1029, METHOD_BUFFERED, FILE_ANY_ACCESS)

BOOL 
OALIoCtlHalI2CCopyFnTable(
    UINT32 code, 
    VOID *pInBuffer,
    UINT32 inSize, 
    VOID *pOutBuffer, 
    UINT32 outSize, 
    UINT32 *pOutSize
    );

//-----------------------------------------------------------------------------
//
//  Define: IOCTL_HAL_PRCMCOPYFNTABLE
//
//  Specifies to the kernel copy PRCM function table
//
#define IOCTL_HAL_CLOCKCOPYFNTABLE  \
    CTL_CODE(FILE_DEVICE_HAL, 1030, METHOD_BUFFERED, FILE_ANY_ACCESS)

BOOL 
OALIoCtlHalClockCopyFnTable(
    UINT32 code, 
    VOID *pInBuffer,
    UINT32 inSize, 
    VOID *pOutBuffer, 
    UINT32 outSize, 
    UINT32 *pOutSize
    );



//-----------------------------------------------------------------------------
//
//  Define: IOCTL_HAL_PADCFGCOPYFNTABLE
//
//  Specifies to the kernel copy pad config function table
//
#define IOCTL_HAL_PADCFGCOPYFNTABLE  \
    CTL_CODE(FILE_DEVICE_HAL, 1031, METHOD_BUFFERED, FILE_ANY_ACCESS)

BOOL 
OALIoCtlHalPadCfgCopyFnTable(
    UINT32 code, 
    VOID *pInBuffer,
    UINT32 inSize, 
    VOID *pOutBuffer, 
    UINT32 outSize, 
    UINT32 *pOutSize
    );

//-----------------------------------------------------------------------------
//
//  Define: IOCTL_HAL_GET_MAC_ADDRESS
//
//
#define IOCTL_HAL_GET_MAC_ADDRESS  \
    CTL_CODE(FILE_DEVICE_HAL, 1032, METHOD_BUFFERED, FILE_ANY_ACCESS)

BOOL 
OALIoctlGetMacAddress(
    UINT32 code, 
    VOID *pInBuffer,
    UINT32 inSize, 
    VOID *pOutBuffer, 
    UINT32 outSize, 
    UINT32 *pOutSize
    );

//-----------------------------------------------------------------------------
//
//  Define: IOCTL_HAL_GET_MAC_ADDRESS1
//
//
#define IOCTL_HAL_GET_MAC_ADDRESS1  \
    CTL_CODE(FILE_DEVICE_HAL, 1033, METHOD_BUFFERED, FILE_ANY_ACCESS)

//-----------------------------------------------------------------------------
//
//  Define: IOCTL_HAL_GET_DISPLAY_RES
//
//
#define IOCTL_HAL_GET_DISPLAY_RES  \
    CTL_CODE(FILE_DEVICE_HAL, 1100, METHOD_BUFFERED, FILE_ANY_ACCESS)

BOOL 
OALIoctlGetDisplayRes(
    UINT32 code, 
    VOID *pInBuffer,
    UINT32 inSize, 
    VOID *pOutBuffer, 
    UINT32 outSize, 
    UINT32 *pOutSize
    );

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#define IOCTL_HAL_PHYS_TO_VIRT \
    CTL_CODE(FILE_DEVICE_HAL, 1150, METHOD_BUFFERED, FILE_ANY_ACCESS)

BOOL OALIoCtlNKPhysToVirt(UINT32 code, VOID *pInBuffer, UINT32 inSize, VOID *pOutBuffer, 
    UINT32 outSize, UINT32 *pOutSize);



//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//
//  Define: IOCTL_HAL_GET_DISPLAY_RES
//
//  Specifies to the kernel copy pad config function table
//
#define IOCTL_HAL_CONVERT_CA_TO_PA  \
    CTL_CODE(FILE_DEVICE_HAL, 1101, METHOD_BUFFERED, FILE_ANY_ACCESS)

BOOL 
OALIoctlConvertCAtoPA(
    UINT32 code, 
    VOID *pInBuffer,
    UINT32 inSize, 
    VOID *pOutBuffer, 
    UINT32 outSize, 
    UINT32 *pOutSize
    );

//-----------------------------------------------------------------------------
//
//  Define: IOCTL_HAL_GET_EBOOT_CFG
//
//  Queries the BSP arguments and get the eboot config
//
#define IOCTL_HAL_GET_EBOOT_CFG  \
    CTL_CODE(FILE_DEVICE_HAL, 1102, METHOD_BUFFERED, FILE_ANY_ACCESS)

BOOL 
OALIoctlGetEbootCfg(
    UINT32 code, 
    VOID *pInBuffer,
    UINT32 inSize, 
    VOID *pOutBuffer, 
    UINT32 outSize, 
    UINT32 *pOutSize
    );



//-----------------------------------------------------------------------------
//
//  Function:  OALIoCtlHalGetRNdisMacAddr
//
BOOL
OALIoCtlHalGetRNdisMacAddr(
    UINT32 code, 
    VOID *pInBuffer,
    UINT32 inSize, 
    VOID *pOutBuffer, 
    UINT32 outSize, 
    UINT32 *pOutSize
    );

//-----------------------------------------------------------------------------
//
//  Function:  OALIoCtlHalQueryFormatPartition
//
BOOL
OALIoCtlHalQueryFormatPartition(
    UINT32 code, 
    VOID *pInBuffer,
    UINT32 inSize, 
    VOID *pOutBuffer, 
    UINT32 outSize, 
    UINT32 *pOutSize
    );

//-----------------------------------------------------------------------------
//
//  Function:  OALPerformanceTimerInit
//
void 
OALPerformanceTimerInit(
    );

//-----------------------------------------------------------------------------
//
//  Function:  OALProfileTimerHit
//
void 
OALProfileTimerHit(
    UINT32 ra
    );


//-----------------------------------------------------------------------------
//
//  Functions: OALFlashStoreXXX
//
HANDLE
OALFlashStoreOpen(
    DWORD address
    );

BOOL
OALFlashStoreWrite(
    HANDLE hFlash,
    ULONG start,
    UCHAR *pData,
    ULONG dataSize,
    BOOL includeSectorInfo,
    BOOL offsetReservedBlocks
    );

BOOL
OALFlashStoreRead(
    HANDLE hFlash,
    ULONG start,
    UCHAR *pData,
    ULONG dataSize,
    BOOL includeSectorInfo,
    BOOL offsetReservedBlocks
    );

BOOL
OALFlashStoreErase(
    HANDLE hFlash,
    ULONG start,
    ULONG size,
    BOOL offsetReservedBlocks
    );

BOOL
OALFlashStoreBufferedRead(
    HANDLE hFlash,
    ULONG start,
    UCHAR *pData,
    ULONG size,
    BOOL offsetReservedBlocks
    );

BOOL
OALFlashStoreReadFromReservedRegion(
    HANDLE hFlash,
    LPCSTR name,
    ULONG start,
    UCHAR *pData,
    ULONG size
    );

BOOL
OALFlashStoreWriteToReservedRegion(
    HANDLE hFlash,
    LPCSTR name,
    ULONG start,
    UCHAR *pData,
    ULONG size
    );

ULONG
OALFlashStoreSectorSize(
    HANDLE hFlash
    );

ULONG
OALFlashStoreBlockSize(
    HANDLE hFlash
    );

VOID
OALFlashStoreClose(
    HANDLE hFlash
    );


//-----------------------------------------------------------------------------
//
//  Function: OALSRAMFnInit
//
BOOL OALSRAMFnInit(
    );

BOOL OALCPUSuspend(
    );

BOOL OALCPUEnd(
    );

BOOL OALCPURestoreContext(
    );

BOOL OALCPUStart(
    );



//-----------------------------------------------------------------------------
//
//  Function: OALWakeupLatency_xxx
//
BOOL
OALWakeupLatency_Initialize(
    );

DWORD
OALWakeupLatency_PushState(
    DWORD state
    );

void
OALWakeupLatency_PopState(
    );

DWORD
OALWakeupLatency_GetCurrentState(
    );

DWORD
OALWakeupLatency_GetSuspendState(
    );

BOOL
OALWakeupLatency_SetSuspendState(
    DWORD suspendState
    );

INT
OALWakeupLatency_GetDelayInTicks(
    DWORD state
    );

DWORD
OALWakeupLatency_FindStateByMaxDelayInTicks(
    INT delayTicks
    );

BOOL
OALWakeupLatency_UpdateOpp(
    DWORD *rgDomains,
    DWORD *rgOpps,
    DWORD  count
    );

BOOL
OALWakeupLatency_UpdateDomainState(
    DWORD powerDomain,
    DWORD powerState,
    DWORD logicState
    );

BOOL
OALWakeupLatency_DeviceEnabled(
    DWORD devId,
    BOOL bEnabled
    );

BOOL
OALWakeupLatency_IsChipOff(
    DWORD latencyState
    );

VOID 
OALWakeupLatency_SetOffModeConstraint(
    DWORD tcrr
    );

VOID 
OALWakeupLatency_SaveSnapshot(
    );

VOID 
OALWakeupLatency_DumpSnapshot(
    );

//-----------------------------------------------------------------------------
//
//  Function: OALMux_xxx
//
void
OALMux_InitMuxTable(
    );

DWORD
OALMux_UpdateOnDeviceStateChange(
    UINT devId,
    UINT oldState,
    UINT newState,
    BOOL bPreStateChange
    );

//-----------------------------------------------------------------------------
//
//  Function: OALSDRCRefreshCounter
//
void
OALSDRCRefreshCounter(
    UINT highFreq, 
    UINT lowFreq
    );

//-----------------------------------------------------------------------------
//
//  Function: OALPowerxxx
//
void 
OALPowerInit(
    );

void 
OALPowerPostInit(
    );

BOOL 
OALPowerVFP(
    DWORD dwCommand
    );

//-----------------------------------------------------------------------------
//
//  Function: OALPrcmIntrHandler
//
UINT32 
OALPrcmIntrHandler(
    );

//-----------------------------------------------------------------------------
//
//  Function: OALTickTimerIntr
//
UINT32 
OALTickTimerIntr(
    );

//-----------------------------------------------------------------------------
//
//  Function: OAL SmartReflex interrupt handlers
//
UINT32 
OALSmartReflex1Intr(
    );

UINT32 
OALSmartReflex2Intr(
    );

int PrcmCM3Isr();

//-----------------------------------------------------------------------------
//
//  Function: OALSetMemoryAttributes
//
BOOL 
OALSetMemoryAttributes(
    LPVOID pVirtAddr,           // Virtual address of region
    LPVOID pPhysAddrShifted,    // PhysicalAddress >> 8 (to support up to 40 bit address)
    DWORD  cbSize,              // Size of the region
    DWORD  dwAttributes         // attributes to be set
    );

//------------------------------------------------------------------------------
// Prototypes to control kitl related driver registry parameters
extern DWORD g_oalKitlEnabled;

void OEMEthernetDriverEnable(BOOL bEnable);
void OEMUsbDriverEnable(BOOL bEnable);


//------------------------------------------------------------------------------
// Prototypes to manage hardware watchdog
void OALWatchdogInit(DWORD dwPeriod, DWORD dwThreadPriority);

void OALWatchdogEnable(BOOL bEnable);

//-----------------------------------------------------------------------------
//
//
//  Define:  OUTSYSREG32
//
// Interface to update the GPMC, SDRC, MPUIC, CM register and corresponding
// shadow registers
//
#define OUTSYSREG32(RegGroup,offset,value) \
        OutShadowReg32((UINT32)RegGroup##_PA,(UINT32)(&(((RegGroup*)0)->offset)),(UINT32)value)

void 
OutShadowReg32(
    UINT32 deviceGroup, 
    UINT32 offset, 
    UINT32 value
    );

//-----------------------------------------------------------------------------
//
//  Define:  OUTSYSREG32
//
// Interface to update the GPMC, SDRC, MPUIC, CM register and corresponding
// shadow registers
//
#define SETSYSREG32(RegGroup,offset,value) \
        SetShadowReg32((UINT32)RegGroup##_PA,(UINT32)(&(((RegGroup*)0)->offset)),(UINT32)value)

void 
SetShadowReg32(
    UINT32 deviceGroup, 
    UINT32 offset, 
    UINT32 value
    );

//-----------------------------------------------------------------------------
//
//  Define:  OALContextRestore
//
// Restore the CORE context
//
VOID
OALContextRestore(
    UINT32 prevMpuState,
    UINT32 prevCoreState,
    UINT32 prevPerState
    );

//------------------------------------------------------------------------------
//
//  External: OALContextSave
//
//  Save the context of system modules just before wfi is called
//
BOOL 
OALContextSave(
    );

//-----------------------------------------------------------------------------
//
//  Function:  OALContextUpdateDirtyRegister
//
//  Identifies a register set as being dirty, ie register has been modified
//  and needs to be backed-up into SDRAM for off mode
//
void 
OALContextUpdateDirtyRegister(
    UINT32  ffRegisterSet
    );

//-----------------------------------------------------------------------------
//
//  Function:  OALContextUpdateCleanRegister
//
//  clears the dirty bit for a register set
//
void 
OALContextUpdateCleanRegister(
    UINT32  ffRegisterSet
    );

//------------------------------------------------------------------------------
//
//  Function:  OEMEnableIOPadWakeup
//
//  Enable/Disable IO PAD wakeup capability for a particular GPIO
//
//
void
OEMEnableIOPadWakeup(
    DWORD   gpio,
    BOOL    bEnable
    );

//------------------------------------------------------------------------------
//
//  Function:  OEMGetIOPadWakeupStatus
//
//       Checks for the IO PAD event and sets the corresponding bit according
//       to GPIO Bank ( array 0 -6  ) and pin ( 0 -31 )
//
//  Returns TRUE if an IO PAD wakeup event occured
//          FALSE no IO PAD wakeup event
BOOL
OEMGetIOPadWakeupStatus(UINT32 *pGpioPadIntrStatus);


//-----------------------------------------------------------------------------
//
//  Function:  OALEnableIOPadWakeup
//
//  Enables the configuration for IO PAD wakeup
//
VOID
OALEnableIOPadWakeup();

//-----------------------------------------------------------------------------
//
//  Function:  OALDisableIOPadWakeup
//
//  Disable the configuration for IO PAD wakeup
//
VOID
OALDisableIOPadWakeup();

//-----------------------------------------------------------------------------
//
//  Function:  OALEnableIOWakeupDaisyChain
//
//  Enables the IO Wakeup scheme for wakeup from OFF Mode
//
VOID
OALEnableIOWakeupDaisyChain();

//-----------------------------------------------------------------------------
//
//  Function:  OALDisableIOWakeupDaisyChain
//
//  Disables the IO Wakeup scheme for wakeup from OFF Mode
//
VOID
OALDisableIOWakeupDaisyChain();

//-----------------------------------------------------------------------------
//
//  Function:  OALIOPadWakeupEnableStatus
//
//  Returns IO Pad WakeupEnableStatus
//
BOOL
OALIOPadWakeupEnableStatus();


//------------------------------------------------------------------------------
//
//  Processor Feature Values used in IsProcessorFeaturePresent API
//

#define PF_ARM_THUMB2                   0x80000018      // thumb2 support
#define PF_ARM_T2EE                     0x80000019      // T2EE support
#define PF_ARM_VFP_V3                   0x8000001A      // VFPv3 hardware
#define PF_ARM_NEON                     0x8000001B      // NEON hardware
#define PF_ARM_UNALIGNED_ACCESS         0x8000001C      // can support unaligned access (need to call a KLIB_IOCTL to turn it on)
#define PF_ARM_VFP_SUPPORT              0x8000001D      // OS support for VFP (including OS VFP emulation, use PF_ARM_VFP_HARDWARE to query for VFP HW)
#define PF_ARM_VFP_V1                   0x8000001E      // VFPv1 hardware
#define PF_ARM_VFP_V2                   0x8000001F      // VFPv2 hardware
#define PF_ARM_VFP_ALL_ROUNDING_MODES   0x80000020      // VFP: All IEEE rounding modes are supported
#define PF_ARM_VFP_SHORT_VECTORS        0x80000021      // VFP: Support for short vector operations
#define PF_ARM_VFP_SQUARE_ROOT          0x80000022      // VFP: Square root implemented in hardware
#define PF_ARM_VFP_DIVIDE               0x80000023      // VFP: Divide operations implemented in hardware
#define PF_ARM_VFP_FP_EXCEPTIONS        0x80000024      // VFP: Support for trapped exceptions
#define PF_ARM_VFP_EXTENDED_REGISTERS   0x80000025      // VFP/Neon: 32 x 64bit register bank
#define PF_ARM_VFP_HALF_PRECISION       0x80000026      // VFP: support for half-precision floating point values
#define PF_ARM_NEON_HALF_PRECISION      0x80000027      // Neon: support for half-precision floating point values
#define PF_ARM_NEON_SINGLE_PRECISION    0x80000028      // Neon: support for float32 operations
#define PF_ARM_NEON_LOAD_STORE          0x80000029      // Neon: support for Neon SIMD loads and stores
#define PF_ARM_VFP_DENORMALS            0x8000002A      // VFP: hardware supports denormalized number arithmetic

//------------------------------------------------------------------------------
//
//  Function:  OALVFPXXX
//
void 
OALVFPEnable();

DWORD
OALVFPGetFPSID();

void 
OALVFPInitialize(OEMGLOBAL* pOemGlobal);

/*
// OEM defined VFP save/restore for implementation defined control registers
typedef void (* PFN_OEMSaveRestoreVFPCtrlRegs) (LPDWORD lpExtra, int nMaxRegs);
typedef BOOL (* PFN_OEMHandleVFPException) (EXCEPTION_RECORD *er, PCONTEXT pctx);

extern PFN_OEMSaveRestoreVFPCtrlRegs pOEMSaveVFPCtrlRegs;
extern PFN_OEMSaveRestoreVFPCtrlRegs pOEMRestoreVFPCtrlRegs;
extern PFN_OEMHandleVFPException     pOEMHandleVFPException;
*/

// OAL VFP handlers
BOOL OALHandleVFPException(
    EXCEPTION_RECORD* er,
    PCONTEXT pctx
);

void OALSaveVFPCtrlRegs(
    LPDWORD lpExtra,
    int nMaxRegs
);

void OALRestoreVFPCtrlRegs(
    LPDWORD lpExtra,
    int nMaxRegs
);


//-----------------------------------------------------------------------------
//
//  Function:  OALxxxNeonxxx
//

void OALInitNeonSaveArea(LPBYTE pArea);
void OALSaveNeonRegistersEx(LPBYTE pArea);
void OALRestoreNeonRegistersEx(LPBYTE pArea);
void OALSaveNeonRegisters(LPBYTE pArea);
void OALRestoreNeonRegisters(LPBYTE pArea);

//-----------------------------------------------------------------------------
//
//  Function:  OEMDeinitDebugSerial
//
//  used to disable the serial debug port after the Kernel startup
//
VOID OEMDeinitDebugSerial();

#ifdef __cplusplus
}
#endif

#endif
