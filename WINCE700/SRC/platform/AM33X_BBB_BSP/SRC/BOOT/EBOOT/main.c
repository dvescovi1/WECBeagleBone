// All rights reserved ADENEO EMBEDDED 2010
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
//  File:  main.c
//
//  This file implements main bootloader functions called from blcommon
//  library.
//
#include "bsp.h"
#include <eboot.h>
#include "sdk_i2c.h"
#include "sdk_gpio.h"
#include "oal_i2c.h"
#include "kitl_cfg.h"
#include "boot_cfg.h"
#include "oal_alloc.h"
#include "ceddkex.h"

#include "bsp_cfg.h"
#include "bsp_def.h"
#include "bsp_padcfg.h"
#include "blcommon.h"
#include "am33x_dmtimer.h"
#include "am33x_clocks.h"
#include "am33x_base_regs.h"
#include "am33x_prcm.h"
#include "image_cfg.h"
#include "lcdc.h"

#define TODO_WARNING

//------------------------------------------------------------------------------
//  misc global variables
BOOL g_InvertDisplay = FALSE;

//------------------------------------------------------------------------------
//  This global variable is used to save information about downloaded regions.
EBOOT_CONTEXT g_eboot;

UINT16 DefaultMacAddress[] = {0x7B64,0x0CD4,0x55EA} /*TODO DO NOT USE THIS MAC FOR RELEASE*/;
UINT16 DefaultMacAddress1[] = {0x0000,0x0000,0x0000} /*TODO DO NOT USE THIS MAC FOR RELEASE*/;

//------------------------------------------------------------------------------
//  This global variable is used to save boot configuration. It is read from
//  flash memory or initialized to default values if flash memory doesn't
//  contain valid structure. It can be modified by user in bootloader
//  configuration menu invoked by BLMenu.
BOOT_CFG g_bootCfg;

//------------------------------------------------------------------------------
//  This global variable is used to save Device prefix according to CPU family.
CHAR  *gDevice_prefix;
const volatile DWORD dwEbootECCtype = (DWORD)-1;
UCHAR g_ecctype;

//------------------------------------------------------------------------------
//
//  Globals: g_Length and g_Offset
//
//  These global variables are used to provide download progress
//
DWORD g_Length = 0;
DWORD g_Offset = 0;

//------------------------------------------------------------------------------
// External Variables
extern DEVICE_IFC_GPIO Tps659xx_Gpio;
extern DWORD g_bootSlot;	//see sdhc.c
extern enum OMAP_LCD_DVI_RES g_dispRes;

//------------------------------------------------------------------------------
//  This global variable is used to save information about CPU family: 35x or 37x.
UINT32 g_CPUFamily=0;


extern DEVICE_IFC_GPIO Am3xx_Gpio;
extern OMAP_LCD_DVI_RES_MENU dispResMenu[];

/* TIMER Clock Source Select */
#define CLKSEL_TIMER2_CLK		(AM33X_PRCM_REGS_PA + 0x508)

//------------------------------------------------------------------------------
// External Functions

VOID Launch(UINT32 address);
VOID JumpTo(UINT32 address);
VOID OEMDeinitDebugSerial();
extern BOOL EnableDeviceClocks(UINT devId, BOOL bEnable);
void ReadMacAddressFromFuse(int cpgmac_num, UCHAR mac[6]);
extern BOOL detect_baseboard_id_info();
extern BOOL detect_daughter_board_profiles();
extern BOOL WriteFlashNK( UINT32 address, UINT32 size );


void configure_pin_mux()
{
    ConfigurePadArray(BSPGetDevicePadInfo(BSPGetDebugUARTConfig()->dev));
	
    // I2C0
    ConfigurePadArray(BSPGetDevicePadInfo(AM_DEVICE_I2C0));

    // I2C1
    ConfigurePadArray(BSPGetDevicePadInfo(AM_DEVICE_I2C1));

    // I2C2
    ConfigurePadArray(BSPGetDevicePadInfo(AM_DEVICE_I2C2));

	// NAND
    //ConfigurePadArray(BSPGetDevicePadInfo(AM_DEVICE_NAND));
    
	// Ethernet
    ConfigurePadArray(BSPGetDevicePadInfo(AM_DEVICE_CPGMAC0));
    
    // MMC0
    ConfigurePadArray(BSPGetDevicePadInfo(AM_DEVICE_MMCHS0));

    // MMC1
    ConfigurePadArray(BSPGetDevicePadInfo(AM_DEVICE_MMCHS1));

    // SPI0
    //ConfigurePadArray(BSPGetDevicePadInfo(AM_DEVICE_MCSPI0));
    
    // LCD
    ConfigurePadArray(BSPGetDevicePadInfo(AM_DEVICE_LCDC));

	// Frammer
	ConfigurePadArray(BSPGetDevicePadInfo(AM_DEVICE_FRAMER));
}


void BSPGpioInit()
{
   BSPInsertGpioDevice(0,&Am3xx_Gpio,NULL);
}

void main()
{
    
    /* hard coded to AM33x, run time detection can be added later */
    g_CPUFamily = CPU_FAMILY_AM33X;
    
    /* setup pinmux and clocks for Debug UART */
    EnableDeviceClocks(BSPGetDebugUARTConfig()->dev,TRUE);
    
    BootloaderMain();
}

BOOL OEMPlatformInit()
//  This function provide platform initialization functions. It is called
//  from boot loader after OEMDebugInit is called.  Note that boot loader
//  BootloaderMain is called from  s/init.s code which is run after reset.
{
    const PAD_INFO I2CPads[]   =   {I2C0_PADS I2C1_PADS I2C2_PADS END_OF_PAD_ARRAY};
	
	AM33X_DMTIMER_REGS* pTimerRegs = OALPAtoUA(AM33X_GPTIMER2_REGS_PA);

    static UCHAR allocationPool[512];
    
    OALLocalAllocInit(allocationPool,sizeof(allocationPool));
    
	OALLog(L"\r\nTexas Instruments Windows CE EBOOT for AM33x, "
		L"Built %S at %S\r\n", __DATE__, __TIME__);

	OALLog(L"EBOOT Version %d.%d.%d, BSP " BSP_VERSION_STRING L"\r\n", 
		EBOOT_VERSION_MAJOR, EBOOT_VERSION_MINOR, EBOOT_VERSION_BUILD);

   // configure i2c devices
	OALI2CInit(AM_DEVICE_I2C0);    
    OALI2CInit(AM_DEVICE_I2C1);
    OALI2CInit(AM_DEVICE_I2C2);

	ConfigurePadArray(I2CPads);

	detect_baseboard_id_info(); 

	if (g_dwBoardId == AM33X_BOARDID_BBONEBLACK_BOARD)
	{		 
		 OALLog(L"\r\nTI BeagleBone (Black)\r\n");
	}
	else if (g_dwBoardId == AM33X_BOARDID_BBONE_BOARD)
	{
		 OALLog(L"\r\nTI BeagleBone (White)\r\n");
	}

	g_dwBoardHasDcard = FALSE;
	g_dwBoardProfile = (DWORD)PROFILE_0;

	if (g_dwBoardId == AM33X_BOARDID_BBONEBLACK_BOARD)
    {
        g_dwBoardProfile |= (DWORD)PROFILE_1;	// BBB specific pads
    }

	detect_daughter_board_profiles();

	if (g_dwBoardHasDcard & (HASDCARD_DVI | HASDCARD_LCD4 | HASDCARD_LCD7 | HASDCARD_LCD7_4D))
	{
        g_dwBoardProfile |= PROFILE_2;			// LCD color and sync pads
	}

	configure_pin_mux();

    /* Select the 32khz osc as Timer2 clock source */
    OUTREG32(OALPAtoUA(CLKSEL_TIMER2_CLK), 0x2);

	EnableDeviceClocks(AM_DEVICE_TIMER2, TRUE);    

    OALLogSetZones( 
//               (1<<OAL_LOG_VERBOSE)  |
               (1<<OAL_LOG_INFO)     |
               (1<<OAL_LOG_ERROR)    |
               (1<<OAL_LOG_WARN)     |
//               (1<<OAL_LOG_FUNC)     |
//               (1<<OAL_LOG_IO)     |
               0);

    //OALLogSetZones( (1<<OAL_LOG_VERBOSE)|(1<<OAL_LOG_INFO)|(1<<OAL_LOG_ERROR)|
    //                /*(1<<OAL_LOG_WARN)   |(1<<OAL_LOG_FUNC)|*/(1<<OAL_LOG_IO)     );

	// TIMER
    OUTREG32(&pTimerRegs->TCLR, 0);						// stop timer
    OUTREG32(&pTimerRegs->TIOCP, DMTIMER_TIOCP_SOFTRESET);	    // Soft reset GPTIMER
	while ((INREG32(&pTimerRegs->TIOCP) & DMTIMER_TIOCP_SOFTRESET) != 0);	// While until done
    OUTREG32(&pTimerRegs->TSICR, DMTIMER_TSICR_POSTED);	// Enable posted mode
    // Start timer
    OUTREG32(&pTimerRegs->TCLR, DMTIMER_TCLR_AR|DMTIMER_TCLR_ST);
    while ((INREG32(&pTimerRegs->TWPS) & DMTIMER_TWPS_TCLR) != 0); // Wait until write is done

    // Enable device clocks used by the bootloader
	// ...lets do them all
    EnableDeviceClocks(AM_DEVICE_GPIO0,TRUE);
    EnableDeviceClocks(AM_DEVICE_GPIO1,TRUE);
    EnableDeviceClocks(AM_DEVICE_GPIO2,TRUE);
    EnableDeviceClocks(AM_DEVICE_GPIO3,TRUE);
    EnableDeviceClocks(AM_DEVICE_MMCHS0,TRUE);
    EnableDeviceClocks(AM_DEVICE_MMCHS1,TRUE);

    GPIOInit();
 	
	/* Initialize Device Prefix */
    gDevice_prefix = BSP_DEVICE_AM33x_PREFIX;
    // Done
    return TRUE;
}

static VOID OEMPlatformDeinit( )
{
#if 0
    OMAP_DMTIMER_REGS *pTimerRegs = OALPAtoUA(OMAP_GPTIMER2_REGS_PA);

    // Soft reset GPTIMER
    OUTREG32(&pTimerRegs->TIOCP, SYSCONFIG_SOFTRESET);
    // While until done
    while ((INREG32(&pTimerRegs->TISTAT) & DMTIMER_TISTAT_RESETDONE) == 0);
#endif    

	// Disable device clocks that were used by the bootloader/xldr and not needed for kernel to start
    //EnableDeviceClocks(AM_DEVICE_DEBUGSS,FALSE);
    EnableDeviceClocks(AM_DEVICE_GPIO0,FALSE);
    EnableDeviceClocks(AM_DEVICE_GPIO1,FALSE);
    EnableDeviceClocks(AM_DEVICE_GPIO2,FALSE);
    EnableDeviceClocks(AM_DEVICE_GPIO3,FALSE);
    EnableDeviceClocks(AM_DEVICE_I2C0,FALSE);
    EnableDeviceClocks(AM_DEVICE_I2C1,FALSE);
    EnableDeviceClocks(AM_DEVICE_I2C2,FALSE);
    EnableDeviceClocks(AM_DEVICE_MCSPI0,FALSE);
//    EnableDeviceClocks(AM_DEVICE_TIMER0,FALSE);
    EnableDeviceClocks(AM_DEVICE_UART0,FALSE);
    EnableDeviceClocks(AM_DEVICE_WDT0,FALSE);
    EnableDeviceClocks(AM_DEVICE_WDT1,FALSE);
   // EnableDeviceClocks(AM_DEVICE_CPGMAC0,FALSE);    
    EnableDeviceClocks(AM_DEVICE_ELM,FALSE);
    EnableDeviceClocks(AM_DEVICE_GPMC,FALSE);
    EnableDeviceClocks(AM_DEVICE_IEEE5000,FALSE);
    EnableDeviceClocks(AM_DEVICE_MMCHS0,FALSE);
    EnableDeviceClocks(AM_DEVICE_MMCHS1,FALSE);
    EnableDeviceClocks(AM_DEVICE_TIMER2,FALSE);

//    HideLogo();	
}

ULONG OEMPreDownload( )
//  This function is called before downloading an image. There is place
//  where user can be asked about device setup.
{
    ULONG rc = (ULONG) BL_ERROR;
	BSP_ARGS *pArgs = OALCAtoUA(IMAGE_SHARE_ARGS_CA);
    BOOL bForceBootMenu;
    AM33X_PRCM_REGS* pPrcmRegs = OALPAtoUA(AM33X_PRCM_REGS_PA);
    UINT32 *pStatusControlAddr = OALPAtoUA(AM33X_DEVICE_BOOT_REGS_PA);
    UINT32 dwSysBootCfg;
    UINT32 dwRSTST;

    BOOL bl_rc = FALSE;

    OALLog(L"INFO: Predownload....\r\n");

	// default g_bootSlot slot is device from which we booted
	// this info is stored at first location at bottom of xldr stack
	// cfg file (if present) is read from this boot device
	// we may change g_bootSlot later to force NK.bin load from alternate device
	g_bootSlot = *((DWORD *)OALPAtoUA(IMAGE_XLDR_STACK_PA));
    
    g_pOEMMultiBINNotify = OEMMultiBinNotify; // We need to support multi bin notify
	g_pOEMVerifyMemory   = OEMVerifyMemory;

    BLReserveBootBlocks();    // Ensure bootloader blocks are marked as reserved

	dwSysBootCfg = INREG32(pStatusControlAddr);

    // Read saved configration
    if ((bl_rc = BLReadBootCfg(&g_bootCfg)) && (g_bootCfg.signature == BOOT_CFG_SIGNATURE) &&
        (g_bootCfg.version == BOOT_CFG_VERSION)){
        g_bootCfg.ECCtype = (UCHAR)dwEbootECCtype;   // set in ebootxx.bib
        OALLog(L"INFO: Boot configuration found. Boot config Version %d, Signature %d\r\n", 
            g_bootCfg.version,g_bootCfg.signature );
    } else {
        if (bl_rc == FALSE)
            OALLog(L"BLReadBootCfg returns error\r\n");
        else if (g_bootCfg.signature != BOOT_CFG_SIGNATURE)
            OALLog(L"BOOT_CFG_SIGNATURE is different, read %d, expect %d\r\n",
                g_bootCfg.signature, BOOT_CFG_SIGNATURE);
        else if (g_bootCfg.version != BOOT_CFG_VERSION)
            OALLog(L"BOOT_CFG_VERSION is different: read %d, expect %d\r\n",
                g_bootCfg.version, BOOT_CFG_VERSION);
        
        OALLog(L"WARN: Boot config wasn't found, using defaults\r\n");
        memset(&g_bootCfg, 0, sizeof(g_bootCfg));
#ifndef BSP_READ_MAC_FROM_FUSE 
		memcpy(&g_bootCfg.mac,DefaultMacAddress,sizeof(g_bootCfg.mac));
		memcpy(&g_bootCfg.mac1,DefaultMacAddress1,sizeof(g_bootCfg.mac1));
#endif
        g_bootCfg.signature = BOOT_CFG_SIGNATURE;
        g_bootCfg.version = BOOT_CFG_VERSION;

        g_bootCfg.oalFlags = 0;
        g_bootCfg.flashNKFlags = 0;

        g_bootCfg.ECCtype =  (UCHAR)dwEbootECCtype;
        // To make it easier to select USB or EBOOT from menus when booting from SD card,
        // preset the kitlFlags. This has no effect if booting from SD card.
        g_bootCfg.kitlFlags = OAL_KITL_FLAGS_DHCP|OAL_KITL_FLAGS_ENABLED;
        g_bootCfg.kitlFlags |= OAL_KITL_FLAGS_VMINI;
        g_bootCfg.kitlFlags |= OAL_KITL_FLAGS_EXTNAME;

        g_bootCfg.displayRes = OMAP_RES_DEFAULT;	// no display by default

		if (g_dwBoardId == AM33X_BOARDID_BBONEBLACK_BOARD)
		{		 
			g_bootCfg.opp_mode = 4;		// OPM_SELECT-1 4 is 1GHz 
			g_bootCfg.displayRes = OMAP_DVI_800W_600H;
		}
		else if (g_dwBoardId == AM33X_BOARDID_BBONE_BOARD)
		{
			g_bootCfg.opp_mode = 3;		// OPM_SELECT-1 3 is 720Mhz
			if (HASDCARD_DVI == (g_dwBoardHasDcard & HASDCARD_DVI))
			{
				g_bootCfg.displayRes = OMAP_DVI_800W_600H;
			}
		}
		else
		{
			g_bootCfg.opp_mode = BSP_OPM_SELECT-1;
		}

		// check for LCD daughter card
		if (HASDCARD_LCD4 == (g_dwBoardHasDcard & HASDCARD_LCD4))
		{
			g_bootCfg.displayRes = OMAP_LCD_480W_272H;
		}
		if (HASDCARD_LCD7_4D == (g_dwBoardHasDcard & HASDCARD_LCD7_4D))
		{
			g_bootCfg.displayRes = OMAP_LCD_800W_480H4D;
		}

        // select default boot device based on boot select switch setting
        OALLog(L"INFO:Boot setting: 0x%02x\r\n", dwSysBootCfg & 0x1f);

        switch (dwSysBootCfg & 0x1f){
			case 0x17: // BB..force SDcard
			case 0x18: // BBB User PB pressed ...force SDcard first
				g_bootCfg.bootDevLoc.LogicalLoc = AM33X_MMCHS0_REGS_PA;
				OALLog(L"INFO:Boot setting: Using SDCard\r\n");
				break;
			case 0x1C: // eMMC ..the default first
				g_bootCfg.bootDevLoc.LogicalLoc = AM33X_MMCHS1_REGS_PA;
				OALLog(L"INFO:Boot setting: Using eMMC\r\n");
				break;          
			case 0x12: // NAND
			case 0x13:
			case 0x14:
				g_bootCfg.bootDevLoc.LogicalLoc = BSP_NAND_REGS_PA + 0x20;
				break;
	
			case 0x04:
			case 0x07:
				g_bootCfg.bootDevLoc.LogicalLoc = AM33X_EMACSW_REGS_PA;
				break;

			default:
				g_bootCfg.bootDevLoc.LogicalLoc = AM33X_EMACSW_REGS_PA;
				g_bootCfg.kitlDevLoc.LogicalLoc = AM33X_EMACSW_REGS_PA;
				break;
        }            
    
		if (g_bootCfg.kitlDevLoc.LogicalLoc == 0){
            g_bootCfg.kitlDevLoc.LogicalLoc = AM33X_EMACSW_REGS_PA;
        }

        g_bootCfg.deviceID = 0;
        g_bootCfg.osPartitionSize = IMAGE_WINCE_CODE_SIZE;
        wcscpy(g_bootCfg.filename, L"nk.bin");
    }

    // Initialize flash partitions if needed
    BLConfigureFlashPartitions(FALSE);

	// Save reset type
	dwRSTST = INREG32(&pPrcmRegs->PRM_RSTST);
	OALLog(L"INFO:PRM_RSTST: 0x%08x\r\n", dwRSTST);


    // Initialize ARGS structure
	// clear it if signature not valid
    if ((pArgs->header.signature != OAL_ARGS_SIGNATURE) ||
        (pArgs->header.oalVersion != OAL_ARGS_VERSION) ||
        (pArgs->header.bspVersion != BSP_ARGS_VERSION ||
		!(dwRSTST == RSTST_GLOBAL_WARM_SW_RST)))
//		!(dwRSTST & (RSTST_GLOBAL_WARM_SW_RST /* actually SW reset */ | RSTST_EXTERNAL_WARM_RST))))
    {
        memset(pArgs, 0, IMAGE_SHARE_ARGS_SIZE);
    }        
	else
	{
//        pArgs->coldBoot = TRUE;
        pArgs->coldBoot = FALSE;
		// RAM good
		// Print message, flush caches and jump to image
		OALLog(L"Warm boot, No download\r\n");
		OALLog(L"Launch Windows CE image by jumping to 0x%08x...\r\n\r\n", pArgs->imageLaunch);
		OEMDeinitDebugSerial();
		OEMPlatformDeinit();
		JumpTo(OALVAtoPA((UCHAR*)(pArgs->imageLaunch)));
    }
    
    // Don't force the boot menu, use default action unless user breaks
    // into menu
    bForceBootMenu = FALSE;
    
retryBootMenu:

	ReadMacAddressFromFuse(0, (UCHAR*)g_bootCfg.mac);
	ReadMacAddressFromFuse(1, (UCHAR*)g_bootCfg.mac1);

    // Call configuration menu
    BLMenu(bForceBootMenu);

	// check if we want to override boot device
	if ((g_bootSlot == 1 && g_bootCfg.bootDevLoc.LogicalLoc != AM33X_MMCHS0_REGS_PA) || 
		(g_bootSlot == 2 && g_bootCfg.bootDevLoc.LogicalLoc != AM33X_MMCHS1_REGS_PA))
	{
		OALLog(L"INFO:Boot device override\r\n");
		if (g_bootCfg.bootDevLoc.LogicalLoc == AM33X_MMCHS0_REGS_PA)
			g_bootSlot = 1;
		else if (g_bootCfg.bootDevLoc.LogicalLoc == AM33X_MMCHS1_REGS_PA)
			g_bootSlot = 2;
	}

#if BUILDING_EBOOT_SD
#ifndef BSP_SAVE_EBOOTCFG_TO_SD
    g_bootCfg.oalFlags &= ~BOOT_CFG_OAL_FLAGS_CFG_SAVE;
#endif
#else
    /* this flag is supported only for SD eboot */
    g_bootCfg.oalFlags &= ~BOOT_CFG_OAL_FLAGS_CFG_SAVE;
#endif
    
    pArgs->header.signature = OAL_ARGS_SIGNATURE;
    pArgs->header.oalVersion = OAL_ARGS_VERSION;
    pArgs->header.bspVersion = BSP_ARGS_VERSION;
    pArgs->kitl.flags = g_bootCfg.kitlFlags;
    pArgs->kitl.devLoc = g_bootCfg.kitlDevLoc;
    pArgs->kitl.ipAddress = g_bootCfg.ipAddress;
    pArgs->kitl.ipMask = g_bootCfg.ipMask;
    pArgs->kitl.ipRoute = g_bootCfg.ipRoute;
    memcpy(pArgs->kitl.mac,g_bootCfg.mac,sizeof(pArgs->kitl.mac)); 
    memcpy(pArgs->mac,g_bootCfg.mac,sizeof(pArgs->mac)); 
    memcpy(pArgs->mac1,g_bootCfg.mac1,sizeof(pArgs->mac1)); 
    pArgs->updateMode = FALSE;
    pArgs->deviceID = g_bootCfg.deviceID;
    pArgs->oalFlags = g_bootCfg.oalFlags;
    pArgs->dispRes = g_bootCfg.displayRes;
    pArgs->ECCtype = g_bootCfg.ECCtype; 
    pArgs->opp_mode = g_bootCfg.opp_mode;
	pArgs->coldBoot = TRUE;
	// boot cause in lower word, boot slot in upper word
	pArgs->bootInfo = (dwRSTST & 0x0000ffff) | g_bootSlot<<16;

    memcpy(pArgs->DevicePrefix, gDevice_prefix, sizeof(pArgs->DevicePrefix));
    /* unset the cfg save and clean reg flags in bootCfg as we do not 
	   want to persist these settings.
	   pArgs now contains the correct temp value for these flags */
    g_bootCfg.oalFlags &= ~(BOOT_CFG_OAL_FLAGS_CFG_SAVE | BOOT_CFG_OAL_FLAGS_CLEAN_REGISTRY);        
    memcpy(pArgs->ebootCfg,&g_bootCfg,sizeof(BOOT_CFG));
    pArgs->cfgSize = sizeof(BOOT_CFG);
    
	OALLog(dispResMenu[g_bootCfg.displayRes].resName);
    OALLog(L"\r\n");

	if (g_bootCfg.displayRes != OMAP_RES_DEFAULT)
	{
		if (pArgs->oalFlags & BOOT_CFG_OAL_FLAGS_INVERT_DISPLAY)
			g_InvertDisplay = TRUE;
		BLShowLogo(g_InvertDisplay);
	}

    // Image download depends on protocol
    g_eboot.bootDeviceType = OALKitlDeviceType(&g_bootCfg.bootDevLoc, g_bootDevices);

    switch (g_eboot.bootDeviceType){
        case BOOT_SDCARD_TYPE:
            OALMSG(OAL_ERROR, (L"OEMPreDownload: Filename %s\r\n", g_bootCfg.filename));
            rc = BLSDCardDownload(g_bootCfg.filename);
            break;
        case OAL_KITL_TYPE_FLASH:
            rc = BLFlashDownload(&g_bootCfg, g_bootDevices);
            break;
        case OAL_KITL_TYPE_ETH:
            rc = BLEthDownload(&g_bootCfg, g_bootDevices);
            break;
    }
        
    if (rc == BL_ERROR){
        // No automatic mode now, force the boot menu to appear
        bForceBootMenu = TRUE;
        goto retryBootMenu; 
    }   

    return rc;
}


VOID OEMLaunch( ULONG start, ULONG size, 
   ULONG launch, const ROMHDR *pRomHeader)
//  This function is the last one called by the boot framework and it is
//  responsible for to launching the image.
{
    BSP_ARGS *pArgs = OALCAtoUA(IMAGE_SHARE_ARGS_CA);

	UNREFERENCED_PARAMETER(size);
	UNREFERENCED_PARAMETER(pRomHeader);

    OALMSG(OAL_FUNC, (
        L"+OEMLaunch(0x%08x, 0x%08x, 0x%08x, 0x%08x - %d/%d)\r\n", start, size,
        launch, pRomHeader, g_eboot.bootDeviceType, g_eboot.type
        ));

	DrawProgressBar(100, 100, g_InvertDisplay);

#if 1
    // Depending on protocol there can be some action required
    switch (g_eboot.bootDeviceType)
    {
#if BUILDING_EBOOT_SD
        case BOOT_SDCARD_TYPE:            
            switch (g_eboot.type)
                {
                case DOWNLOAD_TYPE_RAM:
                    launch = (UINT32)OEMMapMemAddr(start, launch);
                    break;
                default:
                    OALMSG(OAL_ERROR, (L"ERROR: OEMLaunch: Unknown download type, spin forever\r\n"));
                    for(;;);
                    break;
                }
            break;

#endif

        case OAL_KITL_TYPE_ETH:
            BLEthConfig(pArgs);
            switch (g_eboot.type){
                case DOWNLOAD_TYPE_FLASHNAND:
				case DOWNLOAD_TYPE_FLASHNOR:
                    if (BLFlashDownload(&g_bootCfg, g_kitlDevices) != BL_JUMP){
                        OALMSG(OAL_ERROR, (L"ERROR: OEMLaunch: Image load from flash memory failed\r\n"));
                        goto cleanUp;
                    }
                    launch = g_eboot.launchAddress;
                    break;

                case DOWNLOAD_TYPE_RAM:
                    launch = (UINT32)OEMMapMemAddr(start, launch);
                    break;

                case DOWNLOAD_TYPE_EBOOT:
                case DOWNLOAD_TYPE_XLDR:
                    OALMSG(OAL_INFO, (L"INFO: XLDR/EBOOT/IPL downloaded, spin forever\r\n"));
                    for(;;);
                    break;

				case DOWNLOAD_TYPE_LOGO:
                    OALMSG(OAL_INFO, (L"INFO: Splashcreen logo downloaded, spin forever\r\n"));
                    for(;;);
                    break;

                default:
                    OALMSG(OAL_ERROR, (L"ERROR: OEMLaunch: Unknown download type, spin forever\r\n"));
                    for(;;);
                    break;
                }
            break;

        default:        
            launch = g_eboot.launchAddress;
            break;
    }

    
#ifndef BSP_NO_NAND_IN_SDBOOT	  
    if ((g_bootCfg.flashNKFlags & ENABLE_FLASH_NK) && 
        /* if loading from NAND then do not need to flash NAND again */      
        (g_eboot.bootDeviceType != OAL_KITL_TYPE_FLASH) && 
      (start != (IMAGE_WINCE_CODE_CA + NAND_ROMOFFSET)) &&
      (start != (IMAGE_WINCE_CODE_CA + NOR_ROMOFFSET))) {
        if( !WriteFlashNK(start, size))
            OALMSG(OAL_ERROR, (L"ERROR: OEMLaunch: "
                L"Flash NK.bin failed, start=%x, size=%x\r\n", start, size
                ));
    }
#endif
         

    // Check if we get launch address
    if (launch == (UINT32)INVALID_HANDLE_VALUE){
        OALMSG(OAL_ERROR, (L"ERROR: OEMLaunch: Unknown image launch address, spin forever\r\n"));
        for(;;);
    }        
	pArgs->imageLaunch = launch;

    // Print message, flush caches and jump to image
    OALLog(L"Launch Windows CE image by jumping to 0x%08x...\r\n\r\n", launch);

    OEMDeinitDebugSerial();
    OEMPlatformDeinit();
    JumpTo(OALVAtoPA((UCHAR*)launch));

#endif

cleanUp:
    return;
}

//------------------------------------------------------------------------------
//
//  Function:   OEMMultiBinNotify
//
VOID OEMMultiBinNotify( MultiBINInfo *pInfo )
{
    BOOL rc = FALSE;
    UINT32 base = OALVAtoPA((UCHAR*)IMAGE_WINCE_CODE_CA);
    UINT32 start, length;
    UINT32 ix;

    OALMSGS(OAL_INFO,(L"+OEMMultiBinNotify(0x%08x -> %d)\r\n", pInfo, pInfo->dwNumRegions));
    OALMSG(OAL_INFO, (L"Download file information:\r\n"));
    OALMSG(OAL_INFO, (L"-----------------------------------------------------------\r\n"));
    // Copy information to EBOOT structure and set also save address
    g_eboot.numRegions = pInfo->dwNumRegions;
    for (ix = 0; ix < pInfo->dwNumRegions; ix++){
        g_eboot.region[ix].start = pInfo->Region[ix].dwRegionStart;
        g_eboot.region[ix].length = pInfo->Region[ix].dwRegionLength;
        g_eboot.region[ix].base = base;
        base += g_eboot.region[ix].length;
        OALMSG(OAL_INFO, (L"[%d]: Address=0x%08x  Length=0x%08x  Save=0x%08x\r\n",
            ix, g_eboot.region[ix].start, g_eboot.region[ix].length,
            g_eboot.region[ix].base));
    }
    OALMSG(OAL_INFO, (L"-----------------------------------------------------------\r\n"));

    // Determine type of image downloaded
    if (g_eboot.numRegions > 1){
        OALMSG(OAL_ERROR, (L"ERROR: MultiXIP image is not supported\r\n"));
        goto cleanUp;
    }

    base = g_eboot.region[0].base;
    start = g_eboot.region[0].start;
    length = g_eboot.region[0].length; 
    
    if (start == IMAGE_XLDR_CODE_PA){
        g_eboot.type = DOWNLOAD_TYPE_XLDR;
        memset(OALPAtoCA(base), 0xFF, length);
    } else if (start == IMAGE_EBOOT_CODE_CA){
        g_eboot.type = DOWNLOAD_TYPE_EBOOT;
        memset(OALPAtoCA(base), 0xFF, length);
    } else if (start == (IMAGE_WINCE_CODE_CA + NAND_ROMOFFSET)){
        g_eboot.type = DOWNLOAD_TYPE_FLASHNAND;
        memset(OALPAtoCA(base), 0xFF, length);
    } else if (start == (IMAGE_WINCE_CODE_CA + NOR_ROMOFFSET)){
        g_eboot.type = DOWNLOAD_TYPE_FLASHNOR;
        memset(OALPAtoCA(base), 0xFF, length);
	} else if (start == 0) { // Probably a NB0 file, let's fint out
		// Convert the file name to lower case
		CHAR szFileName[MAX_PATH];
		int i = 0;
		int fileExtPos = 0;

		while ((pInfo->Region[0].szFileName[i] != '\0') && (i < MAX_PATH)){
			if((pInfo->Region[0].szFileName[i] >= 'A') && (pInfo->Region[0].szFileName[i] <= 'Z')) {
				szFileName[i] = (pInfo->Region[0].szFileName[i] - 'A' + 'a'); 
			} else {
				szFileName[i] = pInfo->Region[0].szFileName[i];
			}
			// Keep track of file extension position
			if (szFileName[i] == '.'){
				fileExtPos = i;
			}
			i++;
		}

		// Copy string terminator as well !!!!!!!!!!!! POSSIBLE BUG 
		szFileName[i] = pInfo->Region[0].szFileName[i];

		// Check if we support this file
		if (strncmp(szFileName, LOGO_NB0_FILE, LOGO_NB0_FILE_LEN) == 0){
			// Remap the start address to the correct NAND location of the logo
			g_eboot.region[0].start = IMAGE_XLDR_BOOTSEC_NAND_SIZE + IMAGE_EBOOT_BOOTSEC_NAND_SIZE;
			g_eboot.type = DOWNLOAD_TYPE_LOGO;
		} else {
		    OALMSG(OAL_ERROR, (L"Unsupported downloaded file\r\n"));
			goto cleanUp;
		}
	} else {
        g_eboot.type = DOWNLOAD_TYPE_RAM;
    }

    OALMSG(OAL_INFO, (L"Download file type: %d\r\n", g_eboot.type));
    rc = TRUE;

cleanUp:
    if (!rc){
        OALMSG(OAL_ERROR, (L"Spin for ever...\r\n"));
        for(;;);
    }
    OALMSGS(OAL_FUNC, (L"-OEMMultiBinNotify\r\n"));
}

//------------------------------------------------------------------------------
//
//  Function:  OEMMapMemAddr
//
//  This function maps image relative address to memory address. It is used
//  by boot loader to verify some parts of downloaded image.
//
//  EBOOT mapping depends on download type. Download type is
//  set in OMEMultiBinNotify.
//
UINT8* OEMMapMemAddr( DWORD image, DWORD address )
{
    UINT8 *pAddress = NULL;

    OALMSG(OAL_FUNC, (L"+OEMMapMemAddr(0x%08x, 0x%08x)\r\n", image, address));
    
    switch (g_eboot.type) {
        
    case DOWNLOAD_TYPE_XLDR:
    case DOWNLOAD_TYPE_EBOOT:   
	case DOWNLOAD_TYPE_LOGO:
        //  Map to scratch RAM prior to flashing
        pAddress = (UINT8*)g_eboot.region[0].base + (address - image);
        break;

    case DOWNLOAD_TYPE_RAM:            
        //  RAM based NK.BIN and EBOOT.BIN files are given in virtual memory addresses
        pAddress = (UINT8*)address;
        break;

    case DOWNLOAD_TYPE_FLASHNAND:
        pAddress = (UINT8*) (address/* - NAND_ROMOFFSET   FIXME!!!!*/);
        break;

	case DOWNLOAD_TYPE_FLASHNOR:
        pAddress = (UINT8*) (address/* - NOR_ROMOFFSET   FIXME!!!!*/);
        break;
        
    default:
        OALMSG(OAL_ERROR, (L"ERROR: OEMMapMemAddr: "
            L"Invalid download type %d\r\n", g_eboot.type
        ));

    }

    OALMSGS(OAL_FUNC, (L"-OEMMapMemAddr(pAddress = 0x%08x)\r\n", pAddress));
    return pAddress;
}

//------------------------------------------------------------------------------
//
//  Function:  OEMIsFlashAddr
//
//  This function determines whether the address provided lies in a platform's
//  flash or RAM address range.
//
//  EBOOT decision depends on download type. Download type is
//  set in OMEMultiBinNotify.
//
BOOL OEMIsFlashAddr( ULONG address )
{
    BOOL rc;

	UNREFERENCED_PARAMETER(address);

    OALMSG(1 /*OAL_FUNC*/, (L"+OEMIsFlashAddr(0x%08x) g_eboot.type %d\r\n", address, g_eboot.type));

    // Depending on download type
    switch (g_eboot.type)
        {
        case DOWNLOAD_TYPE_XLDR:
        case DOWNLOAD_TYPE_EBOOT:
		case DOWNLOAD_TYPE_LOGO:
		case DOWNLOAD_TYPE_FLASHNAND:
        case DOWNLOAD_TYPE_FLASHNOR:
            rc = TRUE;
            break;
        default:
            rc = FALSE;
            break;
        }

    OALMSG(OAL_FUNC, (L"-OEMIsFlashAddr(rc = %d)\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:   OEMReadData
//
//  This function is called to read data from the transport during
//  the download process.
//
BOOL OEMReadData( ULONG size, UCHAR *pData )
//  This function is called to read data from the transport during
//  the download process.
{
    BOOL rc = FALSE;
    switch (g_eboot.bootDeviceType)
        {
        #if BUILDING_EBOOT_SD
        case BOOT_SDCARD_TYPE:
            rc = BLSDCardReadData(size, pData);
            break;
        #endif
        case OAL_KITL_TYPE_ETH:
            rc = BLEthReadData(size, pData);
            break;
        }

	if (rc) 
        g_Offset += size;

    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  OEMShowProgress
//
//  This function is called during the download process to visualise
//  download progress.
//
VOID OEMShowProgress(ULONG packetNumber)
//  This function is called during the download process to visualise
//  download progress.
{
    UNREFERENCED_PARAMETER(packetNumber);
    DrawProgressBar(g_Length, g_Offset, g_InvertDisplay);
    RETAILMSG(1,(TEXT(".")));
}

//------------------------------------------------------------------------------
//
//  Function:  OEMVerifyMemory
//
//  We only snarf this hook to get values for the globals used to calculate 
// progress.
//
BOOL OEMVerifyMemory(DWORD dwStartAddr, DWORD dwLength)
{
    UNREFERENCED_PARAMETER(dwStartAddr);
    //
    // These variables are being used to calculate the percentage of download
    // in function OEMShowProgress. These must be initialized here else we can
    // get the divide by zero exception.
    //
    g_Length    = dwLength;
    g_Offset    = 0;

    return TRUE;
}

UINT32 OALGetTickCount( )
{
#define TICK_TO_MSEC(tick)     ((tick) / 32)    // msec / 32
	AM33X_DMTIMER_REGS* g_pTimerRegs = OALPAtoUA(AM33X_GPTIMER2_REGS_PA);
    UINT64 tickCount = INREG32(&g_pTimerRegs->TCRR);
    //  Returns number of 1 msec ticks
    return (UINT32) TICK_TO_MSEC(tickCount);
}

DWORD OEMEthGetSecs( )
//  This function returns relative time in seconds.
{
    return OALGetTickCount()/1000;
}

void GetDisplayResolutionFromBootArgs( DWORD * pDispRes )
{
    *pDispRes=g_bootCfg.displayRes;
}

BOOL IsDVIMode()
{
    DWORD dispRes;    
    GetDisplayResolutionFromBootArgs(&dispRes);
    return (dispRes >= OMAP_DVI_800W_600H);
}

//------------------------------------------------------------------------------

