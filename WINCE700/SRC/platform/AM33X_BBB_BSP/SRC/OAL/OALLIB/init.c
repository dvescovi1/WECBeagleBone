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

#include <bsp.h>
#include <bldver.h>
#include "omap_cpuver.h"

#include "am33x.h"
#include "bsp_padcfg.h"


#if (_WINCEOSVER >= 700)
	#include <vfpSupport.h>
#endif

//------------------------------------------------------------------------------
//  External functions
//
extern DWORD GetCp15ControlRegister(void);
extern DWORD GetCp15AuxiliaryControlRegister(void);
extern void EnableUnalignedAccess(void);

extern LPCWSTR g_oalIoCtlPlatformManufacturer;
extern LPCWSTR g_oalIoCtlPlatformName;
VOID OEMDeinitDebugSerial();

extern BOOL detect_baseboard_id_info();
extern BOOL detect_daughter_board_profiles();


//------------------------------------------------------------------------------
//  Global FixUp variables
//
//  Note: This is workaround for makeimg limitation - no fixup on variables
//        initialized to zero, they must also be const.
//
const volatile DWORD dwOEMFailPowerPaging  = 1;
//const volatile DWORD dwOEMTargetProject    = OEM_TARGET_PROJECT_CEBASE;
const volatile DWORD dwOEMDrWatsonSize     = 0x0004B000;
const volatile DWORD dwOEMHighSecurity     = OEM_HIGH_SECURITY_GP;
const volatile DWORD dwRamdiskEnabled   = (DWORD)-1;

//------------------------------------------------------------------------------
//  Global variables

//-----------------------------------------------------------------------------
//
//  Global:  g_CpuFamily
//
//  Set during OEMInit to indicate CPU family.
//
DWORD g_dwCpuFamily = CPU_FAMILY_AM33X;

//-----------------------------------------------------------------------------
//
//  Global:  g_CpuFamily
//
//  Set during OEMInit to indicate CPU family.
//

DWORD g_dwCpuRevision = (DWORD)CPU_REVISION_UNKNOWN;

//------------------------------------------------------------------------------
//
//  Global:  dwOEMSRAMStartOffset
//
//  offset to start of SRAM where SRAM routines will be copied to. 
//
DWORD dwOEMSRAMStartOffset = 0x00002000;

//------------------------------------------------------------------------------
//
//  Global:  dwOEMMPUContextRestore
//
//  location to store context restore information from off mode (PA)
//
const volatile DWORD dwOEMMPUContextRestore = AM33X_OCMC0_PA;

//------------------------------------------------------------------------------
//
//  Global:  dwOEMVModeSetupTime
//
//  Setup time for DVS transitions. Reinitialized in config.bib (FIXUPVAR)
//
DWORD dwOEMVModeSetupTime = 2;

#if 0
//------------------------------------------------------------------------------
//  Time the PRCM waits for system clock stabilization. 
//  Reinitialized in config.bib (FIXUPVAR)
const volatile DWORD dwOEMPRCMCLKSSetupTime = 0x140;//0x2;

//------------------------------------------------------------------------------
//  location to store context restore information from off mode
const volatile DWORD dwOEMMPUContextRestore = CPU_INFO_ADDR_PA;  // ??
#endif
//------------------------------------------------------------------------------
//  maximum idle period during OS Idle in milliseconds
DWORD dwOEMMaxIdlePeriod = 1000;

//------------------------------------------------------------------------------
//  Save kitl state
DWORD g_oalKitlEnabled;

//-----------------------------------------------------------------------------
//
//  Global:  g_oalRetailMsgEnable
//
//  Used to enable retail messages
//
BOOL   g_oalRetailMsgEnable = FALSE;

//-----------------------------------------------------------------------------
//
//  Global:  g_ResumeRTC
//
//  Used to inform RTC code that a resume occured
//
BOOL g_ResumeRTC = FALSE;


//------------------------------------------------------------------------------
//  Local functions
//
static void OALGPIOSetDefaultValues();


//-----------------------------------------------------------------------------
// Global Variables
RAMTableEntry g_RamTableEntry[] = 
{
	// add another 256meg (512 total) at 0x90000000 physical
    //physical memory start address >> 8, memory size, attribute must be 0
    {(DEVICE_RAM_512_PA) >> 8, 256 * 1024 * 1024, 0},    
	// add back RAMDISK memory if not used
//#ifndef SYSGEN_RAMDISK
//    {(IMAGE_WINCE_RAM_DISK_PA) >> 8, IMAGE_WINCE_RAM_DISK_SIZE_PA, 0},    
//#endif    
	{(0xA0000000) >> 8, 0, 0}     
};

RamTable g_RAMTable = {MAKELONG(CE_MINOR_VER, CE_MAJOR_VER), sizeof(g_RamTableEntry) / sizeof(g_RamTableEntry[0]), g_RamTableEntry};


//-----------------------------------------------------------------------------
//
// Function: OEMGetRamTable
//      This function is implemented by the OEM to return the OEMRamTable structure, 
//      which allows your platform to support more than 512 MB of physical memory.
// Parameters:
//
// Returns: 
//      Returns an OEMRamTable structure, as defined in %_WINCEROOT%\Public\Common\Oak\Inc\OEMGlobal.h.
//
//
//-----------------------------------------------------------------------------
PCRamTable OEMGetRamTable(void)
{
    return &g_RAMTable;
}



DWORD OALMux_UpdateOnDeviceStateChange( UINT devId, UINT oldState, UINT newState, BOOL bPreStateChange )
{
	UNREFERENCED_PARAMETER(devId);
    UNREFERENCED_PARAMETER(oldState);
    UNREFERENCED_PARAMETER(newState);
    UNREFERENCED_PARAMETER(bPreStateChange);
    return (DWORD) -1;
}


extern DEVICE_IFC_GPIO Am3xx_Gpio;
void BSPGpioInit()
{
    BSPInsertGpioDevice(0,&Am3xx_Gpio,L"GIO1:");   // Am3xx GPIOs
}

//------------------------------------------------------------------------------
//
//  Function:  OEMInit
//
//  This is Windows CE OAL initialization function. It is called from kernel
//  after basic initialization is made.
//
static UCHAR allocationPool[2048];
VOID OEMInit()
{
    BOOL           *pColdBoot;
	AM33X_SYSC_PADCONFS_REGS *padconf_r;
	AM33X_PRCM_REGS          *prcm_r;
    
    BOOL           *pRetailMsgEnable;

    //----------------------------------------------------------------------
    // Initialize OAL log zones
    //----------------------------------------------------------------------

    OALLogSetZones( 
    //           (1<<OAL_LOG_VERBOSE)  |
    //           (1<<OAL_LOG_INFO)     |
               (1<<OAL_LOG_ERROR)    |
               (1<<OAL_LOG_WARN)     |
    //           (1<<OAL_LOG_IOCTL)    | 
    //           (1<<OAL_LOG_FUNC)     |
    //           (1<<OAL_LOG_INTR)     |
               0);
    OALMSG(OAL_FUNC, (L"+OEMInit\r\n"));

    //----------------------------------------------------------------------
    // Initialize the OAL memory allocation system (TI code)
    //----------------------------------------------------------------------
    OALLocalAllocInit(allocationPool,sizeof(allocationPool));

    //----------------------------------------------------------------------
    // Update kernel variables
    //----------------------------------------------------------------------
#if (_WINCEOSVER >= 700)
    CEProcessorType = PROCESSOR_ARM_CORTEX;
    CEInstructionSet = PROCESSOR_ARM_V7_INSTRUCTION;
	// Set values of globals used in IOCTL_HAL_GET_DEVICE_INFO handler
    g_oalIoCtlPlatformManufacturer  = L"Texas Instruments";
    g_oalIoCtlPlatformName          = L"BSP_AM33X";
#endif

    dwNKDrWatsonSize = dwOEMDrWatsonSize;
  //  gdwFailPowerPaging = dwOEMFailPowerPaging;
  //  cbNKPagingPoolSize = (dwOEMPagingPoolSize == -1) ? 0 : dwOEMPagingPoolSize;
    
    // Alarm has resolution 1 second
    dwNKAlarmResolutionMSec = 1000;

    // Set extension functions
    pOEMIsProcessorFeaturePresent = OALIsProcessorFeaturePresent;
    pfnOEMSetMemoryAttributes     = OALSetMemoryAttributes;

    // Profiling support
    g_pOemGlobal->pfnProfileTimerEnable  = OEMProfileTimerEnable;
    g_pOemGlobal->pfnProfileTimerDisable = OEMProfileTimerDisable;

    //----------------------------------------------------------------------
    // Initialize cache globals
    //----------------------------------------------------------------------

    OALCacheGlobalsInit();
        
    EnableUnalignedAccess();

    #ifdef DEBUG
        OALMSG(1, (L"CPU CP15 Control Register = 0x%x\r\n", GetCp15ControlRegister()));
        OALMSG(1, (L"CPU CP15 Auxiliary Control Register = 0x%x\r\n", GetCp15AuxiliaryControlRegister()));
    #endif
        
    //----------------------------------------------------------------------
    // Initialize Power Domains
    //----------------------------------------------------------------------
    
    // OALPowerInit();
    PrcmInit();
	OALI2CInit(AM_DEVICE_I2C0);
	OALI2CInit(AM_DEVICE_I2C1);
	OALI2CInit(AM_DEVICE_I2C2);

    detect_baseboard_id_info(); 

	g_dwBoardHasDcard = FALSE;
	g_dwBoardProfile = (DWORD)PROFILE_0;

	if (g_dwBoardId == AM33X_BOARDID_BBONEB_BOARD)
    {
        g_dwBoardProfile |= (DWORD)PROFILE_1;
    }

	detect_daughter_board_profiles();

	if (g_dwBoardHasDcard & HASDCARD_DVI)
	{
        g_dwBoardProfile |= PROFILE_2;
	}

	//----------------------------------------------------------------------
    // Initialize Vector Floating Point co-processor
    //----------------------------------------------------------------------

#if (_WINCEOSVER >= 700)
    VfpOemInit(g_pOemGlobal, VFP_AUTO_DETECT_FPSID);
#else
    OALVFPInitialize(g_pOemGlobal);
#endif

    //----------------------------------------------------------------------
    // Initialize 512 RAM 
    //----------------------------------------------------------------------
    if (g_dwBoardId == AM33X_BOARDID_BBONEB_BOARD)
    {
		//enable RAM >= 512M     
		g_pOemGlobal->pfnGetOEMRamTable = OEMGetRamTable;
	}
	
	//----------------------------------------------------------------------
    // Initialize interrupt
    //----------------------------------------------------------------------
    if (!OALIntrInit()) {
        OALMSG(OAL_ERROR, (L"ERROR: OEMInit: failed to initialize interrupts\r\n"));
        goto cleanUp;
    }
    INTERRUPTS_ON();
    //----------------------------------------------------------------------
    // Initialize system clock
    //----------------------------------------------------------------------
	PrcmClockSetParent(kTIMER3_GCLK, kSYS_CLKIN_CK);
	if (!OALTimerInit(1, 24000, 200)){
        OALMSG(OAL_ERROR, (L"ERROR: OEMInit: Failed to initialize system clock\r\n"));
        goto cleanUp;
    }    

	OAL3XX_RTCInit(AM33X_RTCSS_REGS_PA, IRQ_RTCALARM);

    //----------------------------------------------------------------------
    // Initialize PAD cfg
    //----------------------------------------------------------------------
    OALPadCfgInit();

    //----------------------------------------------------------------------
    // configure pin mux
    //----------------------------------------------------------------------
    ConfigurePadArray(BSPGetAllPadsInfo());
    if (!RequestDevicePads(AM_DEVICE_I2C0)) OALMSG(OAL_ERROR, (TEXT("Failed to request pads for I2C0\r\n")));
    if (!RequestDevicePads(AM_DEVICE_I2C1)) OALMSG(OAL_ERROR, (TEXT("Failed to request pads for I2C1\r\n")));
    if (!RequestDevicePads(AM_DEVICE_I2C2)) OALMSG(OAL_ERROR, (TEXT("Failed to request pads for I2C2\r\n")));
    if (!RequestDevicePads(AM_DEVICE_FRAMER)) OALMSG(OAL_ERROR, (TEXT("Failed to request pads for framer\r\n")));
	if (g_dwBoardHasDcard & HASDCARD_DVI)
	{
//		if (!RequestAndConfigurePad(PAD_ID(GPMC_CSN2),AM335X_PIN_OUTPUT_PULLUP))
		if (!RequestAndConfigurePad(PAD_ID(ECAP0_IN_PWM0_OUT),AM335X_PIN_OUTPUT_PULLUP))
			OALMSG(OAL_ERROR, (TEXT("Failed to request pads for DVI nPD\r\n")));
	}

	GPIOInit();
	// Set GPIOs default values (like the buffers' OE)
	OALGPIOSetDefaultValues();

    //----------------------------------------------------------------------
    // Initialize SRAM Functions
    //----------------------------------------------------------------------
    OALSRAMFnInit();

	//----------------------------------------------------------------------
    // Initialize high performance counter
    //----------------------------------------------------------------------
    PrcmClockSetParent(kTIMER2_GCLK, kSYS_CLKIN_CK);
    OALPerformanceTimerInit(0, 0);

	if (CurMSec == 0){
		OALMSG(OAL_ERROR,(L"!!! LOOKS LOOKS TIMER IS NOT RUNNING %d %d\r\n", __LINE__, CurMSec));
	}

    //----------------------------------------------------------------------
    // Initialize the KITL
    //----------------------------------------------------------------------
    g_oalKitlEnabled = KITLIoctl(IOCTL_KITL_STARTUP, NULL, 0, NULL, 0, NULL);

    //----------------------------------------------------------------------
    // Initialize the watchdog
    //----------------------------------------------------------------------
    
#ifdef BSP_AM33X_WATCHDOG
#ifdef BSP_TIMEBOMB
		OALMSG(1, (L"Not for sale. Evaluation only!!\r\n" ));
#endif
    OALWatchdogInit(BSP_WATCHDOG_TIMEOUTPERIOD_MILLISECONDS,BSP_WATCHDOG_THREAD_PRIORITY);
#endif

    //----------------------------------------------------------------------
    // Check for retail messages enabled
    //----------------------------------------------------------------------
    pRetailMsgEnable = OALArgsQuery(OAL_ARGS_QUERY_OALFLAGS);
    if (pRetailMsgEnable && (*pRetailMsgEnable & OAL_ARGS_OALFLAGS_RETAILMSG_ENABLE))
        g_oalRetailMsgEnable = TRUE;

    //----------------------------------------------------------------------
    // Deinitialize serial debug
    //----------------------------------------------------------------------

    if (!g_oalRetailMsgEnable)
        OEMDeinitDebugSerial();

// not available under CE6
#if (_WINCEOSVER >= 700)
    //----------------------------------------------------------------------
    // Make Page Tables walk L2 cacheable. There are 2 new fields in OEMGLOBAL
    // that we need to update:
    // dwTTBRCacheBits - the bits to set for TTBR to change page table walk
    //                   to be L2 cacheable. (Cortex-A8 TRM, section 3.2.31)
    //                   Set this to be "Outer Write-Back, Write-Allocate".
    // dwPageTableCacheBits - bits to indicate cacheability to access Level
    //                   L2 page table. We need to set it to "inner no cache,
    //                   outer write-back, write-allocate. i.e.
    //                      TEX = 0b101, and C=B=0.
    //                   (ARM1176 TRM, section 6.11.2, figure 6.7, small (4k) page)
    //----------------------------------------------------------------------
    g_pOemGlobal->dwTTBRCacheBits = 0x8;            // TTBR RGN set to 0b01 - outer write back, write-allocate
    g_pOemGlobal->dwPageTableCacheBits = 0x140;     // Page table cacheability uses 1BB/AA format, where AA = 0b00 (inner non-cached)
#endif
    //----------------------------------------------------------------------
    // Check for a clean boot of device
    //----------------------------------------------------------------------
    pColdBoot = OALArgsQuery(OAL_ARGS_QUERY_COLDBOOT);
    if ((pColdBoot == NULL)|| ((pColdBoot != NULL) && *pColdBoot))
        NKForceCleanBoot();

cleanUp:
    OALMSG(OAL_FUNC, (L"-OEMInit\r\n"));
}


void OALGPIOSetDefaultValues()
{
	HANDLE hGPIO = GPIOOpen();

//    GPIOSetBit(hGPIO, BSP_COM_WL_RST_GPIO);            
//    GPIOSetMode(hGPIO, BSP_COM_WL_RST_GPIO,GPIO_DIR_OUTPUT);
	GPIOClose(hGPIO);
}
