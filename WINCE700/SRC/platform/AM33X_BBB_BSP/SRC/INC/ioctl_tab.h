//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this sample source code is subject to the terms of the Microsoft
// license agreement under which you licensed this sample source code. If
// you did not accept the terms of the license agreement, you are not
// authorized to use this sample source code. For the terms of the license,
// please see the license agreement between you and Microsoft or, if applicable,
// see the LICENSE.RTF on your install media or the root of your tools installation.
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
// Copyright (c) 2007, 2008 BSQUARE Corporation. All rights reserved.

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
//  File:  ioctl_tab.h
//
//  Configuration file for the OAL IOCTL component.
//
//  This file is included by the platform's ioctl.c file and defines the 
//  global IOCTL table, g_oalIoCtlTable[]. Therefore, this file may ONLY
//  define OAL_IOCTL_HANDLER entries. 
//
// IOCTL CODE,                          Flags   Handler Function
//------------------------------------------------------------------------------

#if (_WINCEOSVER >= 700)
{ IOCTL_HAL_GET_POWER_DISPOSITION,      0,  OALIoCtlHalGetPowerDisposition  },
#endif
{ IOCTL_HAL_TRANSLATE_IRQ,              0,  OALIoCtlHalRequestSysIntr       },
{ IOCTL_HAL_REQUEST_SYSINTR,            0,  OALIoCtlHalRequestSysIntr       },
{ IOCTL_HAL_RELEASE_SYSINTR,            0,  OALIoCtlHalReleaseSysIntr       },
{ IOCTL_HAL_REQUEST_IRQ,                0,  OALIoCtlHalRequestIrq           },
{ IOCTL_HAL_IRQ2SYSINTR,                0,  OALIoCtlHalIrq2Sysintr          },
{ IOCTL_HAL_ILTIMING,                   0,  OALIoCtlHalILTiming             },

{ IOCTL_HAL_INITREGISTRY,               0,  OALIoCtlHalInitRegistry         },
{ IOCTL_HAL_INIT_RTC,                   0,  OALIoCtlHalInitRTC              },
{ IOCTL_HAL_REBOOT,                     0,  OALIoCtlHalReboot               },

{ IOCTL_HAL_DDK_CALL,                   0,  OALIoCtlHalDdkCall              },
{ IOCTL_HAL_DISABLE_WAKE,               0,  OALIoCtlHalDisableWake          },
{ IOCTL_HAL_ENABLE_WAKE,                0,  OALIoCtlHalEnableWake           },
{ IOCTL_HAL_GET_WAKE_SOURCE,            0,  OALIoCtlHalGetWakeSource        },
{ IOCTL_HAL_GET_CACHE_INFO,             0,  OALIoCtlHalGetCacheInfo         },
{ IOCTL_HAL_GET_DEVICE_INFO,            0,  OALIoCtlHalGetDeviceInfo        },
{ IOCTL_HAL_GET_DEVICEID,               0,  OALIoCtlHalGetDeviceId          },
{ IOCTL_HAL_GET_UUID,                   0,  OALIoCtlHalGetUUID              },
{ IOCTL_PROCESSOR_INFORMATION,          0,  OALIoCtlProcessorInfo           },
{ IOCTL_HAL_GET_CPUID,                  0,  OALIoCtlHalGetCpuID             },
{ IOCTL_HAL_GET_CPUFAMILY,              0,  OALIoCtlHalGetCpuFamily         },
{ IOCTL_HAL_GET_CPUSPEED,               0,  OALIoCtlHalGetCpuSpeed          },
{ IOCTL_HAL_GET_MAC_ADDRESS,            0,  OALIoctlGetMacAddress			},
{ IOCTL_HAL_GET_MAC_ADDRESS1,           0,  OALIoctlGetMacAddress			},
{ IOCTL_HAL_GET_DISPLAY_RES,            0,  OALIoctlGetDisplayRes           },
{ IOCTL_HAL_PHYS_TO_VIRT,			    0,	OALIoCtlNKPhysToVirt   			},
{ IOCTL_HAL_POSTINIT,                   0,  OALIoCtlHALPostInit             },
{ IOCTL_HAL_I2CCOPYFNTABLE,             0,  OALIoCtlHalI2CCopyFnTable       },
{ IOCTL_HAL_PADCFGCOPYFNTABLE,          0,  OALIoCtlHalPadCfgCopyFnTable    },
{ IOCTL_HAL_GET_ECC_TYPE,               0,  OALIoctlHalGetEccType           },
{ IOCTL_HAL_GET_PLATFORM_ID,            0,  OALIoctlHalGetPlatformID        },
{ IOCTL_HAL_GET_BOOT_INFO,              0,  OALIoctlHalGetBootInfo          },
{ IOCTL_HAL_GET_EBOOT_CFG,              0,  OALIoctlGetEbootCfg             },


{ IOCTL_PRCM_DEVICE_GET_DEVICEMANAGEMENTTABLE, 0, OALIoCtlPrcmDeviceGetDeviceManagementTable},
{ IOCTL_PRCM_DEVICE_GET_SOURCECLOCKINFO,0,  OALIoCtlPrcmDeviceGetSourceClockInfo},
{ IOCTL_PRCM_CLOCK_GET_SOURCECLOCKINFO, 0,  OALIoCtlPrcmClockGetSourceClockInfo},
{ IOCTL_PRCM_CLOCK_SET_SOURCECLOCKDIVISOR, 0, OALIoCtlPrcmClockSetSourceClockDivisor},
{ IOCTL_PRCM_CLOCK_SET_DPLLCLKOUTSTATE, 0,  OALIoCtlPrcmClockSetDpllClkOutState   },
{ IOCTL_OPP_REQUEST,                    0,  OALIoCtlOppRequest},

{ IOCTL_HAL_PROFILE,                    0,  OALIoCtlIgnore                  },
{ IOCTL_HAL_DUMP_REGISTERS,             0,  OALIoCtlHalDumpRegisters        },
{ IOCTL_HAL_GET_IRQ_COUNTERS, 			0,	OALIoCtlHalGetIrqCounters       },
{ IOCTL_HAL_GET_HIVE_CLEAN_FLAG,        0,  OALIoCtlHalGetHiveCleanFlag     },


// Required Termination
{ 0,                                    0,  NULL                        }
