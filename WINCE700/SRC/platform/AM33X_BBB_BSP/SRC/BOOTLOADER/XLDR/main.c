// Copyright (c) 2014, David Vescovi.  All Rights Reserved.
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
//  This file contains X-Loader implementation for AM33X
//
#include "bsp.h"
#include "bsp_base_regs.h"

#pragma warning(push)
#pragma warning(disable: 4115 4201)
#include <blcommon.h>
#include <fmd.h>
#pragma warning(pop)

#include "sdk_gpio.h"
#include "oalex.h"
#define OAL
#include "oal_alloc.h"
#include "oal_clock.h"
#include "bsp_cfg.h"

// !!! IMPORTANT !!!
// Check the .bib, image_cfg.h and image_cfg.inc files
// to make sure the addr and size match.
#ifdef UART_BOOT
// UART_BOOT
#define NAND_MAX_BLOCKS_TO_WRITE   19
extern int XReceive(unsigned char *p_data_buff, int buff_size, unsigned int *p_receive_size);
#else
// Not UART_BOOT
#undef UART_BOOT
#undef UART_DNLD_EBOOT_TO_RAM
#undef UART_DNLD_RAW_TO_NAND
#endif  /* UART_BOOT */

#if defined(FMD_ONENAND) && defined(FMD_NAND)
    #error FMD_ONENAND and FMD_NAND cannot both be defined.
#endif


extern VOID OEMWriteDebugHex(unsigned long n, long depth);
extern void XGetStats
(
	int *p_xblock_cnt,
	int *p_xack_cnt,
	int *p_xnak_cnt,
	int *p_xcan_cnt,
	int *p_xothers_cnt,
	int *p_checksum_error_cnt,
	int *p_dup_pkt_cnt
);

//------------------------------------------------------------------------------

#ifdef SHIP_BUILD
#define XLDRMSGINIT
#define XLDRMSGDEINIT
#define XLDRMSG(msg)
#else
#define XLDRMSGINIT         {EnableDeviceClocks(BSPGetDebugUARTConfig()->dev,TRUE); OEMInitDebugSerial();}
#define XLDRMSGDEINIT       OEMDeinitDebugSerial()
#define XLDRMSG(msg)        OEMWriteDebugString(msg)
#define XLDRHEX(val, len)   OEMWriteDebugHex(val, len)
#endif


//------------------------------------------------------------------------------
// External Variables
extern DEVICE_IFC_GPIO Am3xx_Gpio;

//------------------------------------------------------------------------------
//  Global variables
ROMHDR * volatile const pTOC = (ROMHDR *)-1;

unsigned int  gCPU_family;
const volatile DWORD dwEbootECCtype = (DWORD)-1;
UCHAR g_ecctype =4;

//------------------------------------------------------------------------------
//  External Functions

extern VOID OEMDeinitDebugSerial();
extern VOID PlatformSetup();
extern VOID JumpTo();
extern VOID EnableCache_GP();

//------------------------------------------------------------------------------
//  Local Functions

static BOOL SetupCopySection(ROMHDR *const pTableOfContents);

//#ifdef UART_BOOT
#if 1

VOID XLDRWriteHexString(UINT8 *string, INT len)
{
	while (len--) 
		OEMWriteDebugByte((UINT8)*string++);
}

VOID XLDRWriteCharString(UINT8 *string)
{
    while (*string != '\0') OEMWriteDebugByte((UINT8)*string++);
}

#define XLDR_READS_PER_SEC 0x17FFFF
INT XLDRReadCharMaxTime(UINT8 *uc, INT num_sec)
{
	INT c;
	UINT32 reads = num_sec * XLDR_READS_PER_SEC;

	do {
	    c = OEMReadDebugByte();
        if (c == OEM_DEBUG_READ_NODATA) {
			c=0; /* continue to read */
        } else if (c == OEM_DEBUG_COM_ERROR) {
			c=-1;
            break;
        } else {
			*uc = (UINT8)c;/* received one byte */
			c = 1;
            break;
        }
	}
	while (--reads);
	return (c);
}

VOID XLDRWriteChar(const UINT8 c)
{
	OEMWriteDebugByte(c);
}

//------------------------------------------------------------------------------
//
//  Function:  XLDRWriteChar
//
//  Output char to debug serial port
//
VOID XLDRPrintUint8(const UINT8 ui8, UINT32 num_of_times, UINT32 end_cr_lf)
{
	INT i, j;
	UINT8 msg_buff[24];
	UINT8 c;
	UINT32 n = 0;

	while (n++ < num_of_times){
		j=0;
		msg_buff[j++]='0';
		msg_buff[j++]='x';

		for(i=0; i<2; i++){
			// grab a half byte and move it to the far right
		    c = (UINT8)(((ui8 & (0xf0 >> i*4)) >> 4*(1-i)));
			if ((0 <= c) && (c <= 9))
				msg_buff[j++] = c + '0';
			else
				msg_buff[j++] = (c - 10) + 'A';
		}

		if (end_cr_lf){
			msg_buff[j++]='\r';
			msg_buff[j++]='\n';
		} else {
			msg_buff[j++]=' '; /* end with a space */
		}
		msg_buff[j++]='\0';
        XLDRWriteCharString(&(msg_buff[0]));
	}
}

//------------------------------------------------------------------------------
//
//  Function:  XLDRPrintUlong
//
//  Output unsigned long to debug serial port
//
VOID XLDRPrintUlong(const UINT32 ulong_to_write, UINT32 num_of_times, UINT32 end_cr_lf)
{
	INT i, j;
	UINT32 n;
	UINT8 msg_buff[24];
	UINT8 c;

	n = 0;
	while (n++ < num_of_times)
	{
		j=0;
		msg_buff[j++]='0';
		msg_buff[j++]='x';

		for(i=0; i<8; i++){
			// get a half byte and move it to the far right
		    c = (UINT8)(((ulong_to_write & (0xf0000000 >> i*4)) >> 4*(7-i)));
			if ((0 <= c) && (c <= 9))
				msg_buff[j++] = c + '0';
			else
				msg_buff[j++] = (c - 10) + 'A';
		}

		if (end_cr_lf){
			msg_buff[j++]='\r';
			msg_buff[j++]='\n';
		} else {
			msg_buff[j++]=' '; /* end with a space */
		}
		msg_buff[j++]='\0';
        XLDRWriteCharString(&(msg_buff[0]));
	}
}
#endif /*UART_BOOT*/

void BSPGpioInit()
{
//   BSPInsertGpioDevice(0,&Am3xx_Gpio,NULL);
}

#ifdef TEST_NAND_ACCESS

#define WRITE_WEIGHT 0xA5

VOID XLDRShowNandGeometry(FlashInfo *pflashInfo)
{
	XLDRMSG(L"\r\nNand Geometry: \r\n");
	XLDRMSG(L"NumBlocks ");          XLDRPrintUlong((UINT32)pflashInfo->dwNumBlocks, 1, 1);
	XLDRMSG(L"BytesPerBlock ");      XLDRPrintUlong((UINT32)pflashInfo->dwBytesPerBlock, 1, 1); 
	XLDRMSG(L"SectorsPerBlock ");    XLDRPrintUlong((UINT32)pflashInfo->wSectorsPerBlock, 1, 1); 
	XLDRMSG(L"BytesPerSector ");     XLDRPrintUlong((UINT32)pflashInfo->wDataBytesPerSector, 1, 1); 
}

VOID XLDRTestReadWrite(FlashInfo *pflashInfo)
{
    // SECTOR_ADDR sector;
    BLOCK_ID block;
    INT image_block_cnt, nand_blocks_to_write;
    DWORD rc = 0;
    UINT8 *pImage;
    SectorInfo sectorInfo;
    UINT8 i;
    SECTOR_ADDR sector;

    UINT8 w = 0;

    XLDRShowNandGeometry(pflashInfo);

    // Find a unknown status block
	block = 0;
    image_block_cnt = 0;

    while (block < pflashInfo->dwNumBlocks) { // Skip to a good block
        rc = FMD_GetBlockStatus(block);

        if (rc == BLOCK_STATUS_UNKNOWN) {
  			break; // A unknown status block
   		}
        block++;
    }
    
  	if (block >= pflashInfo->dwNumBlocks){
        XLDRMSG(L"No unknown status block found \r\n");
   	} else { 
        XLDRMSG(L"First unknown status block ");     XLDRPrintUlong((UINT32)block, 1, 1); 
    }

    // Find a bad block
	block = 0;
    image_block_cnt = 0;

    while (block < pflashInfo->dwNumBlocks) { // Skip to a good block
        rc = FMD_GetBlockStatus(block);

        if ((rc & BLOCK_STATUS_BAD) == 1) {
   			break; // A bad block
   		}
        block++;
    }
    
    if (block >= pflashInfo->dwNumBlocks){
        XLDRMSG(L"No bad block found \r\n");
   	} else { 
		XLDRMSG(L"First bad block ");     XLDRPrintUlong((UINT32)block, 1, 1); 
    }

    // Find a good block
	block = 0;
    image_block_cnt = 0;

    while (block < pflashInfo->dwNumBlocks) { // Skip to a good block
        if ((FMD_GetBlockStatus(block) & BLOCK_STATUS_BAD) == 0) {
   			break; // A good block
   		}
        block++;
    }
    
   	if (block >= pflashInfo->dwNumBlocks){
   		return; // No good block found!!
   	}

	XLDRMSG(L"First good block ");     XLDRPrintUlong((UINT32)block, 1, 1); 

#ifdef TEST_NAND_WRITE
	// Erase first good block
	if (!FMD_EraseBlock(block)) {
		for(;;) XLDRMSG(L"TestReadWrite: Erase FAILED\r\n");
	}

	XLDRMSG(L"Erased first good block ");     XLDRPrintUlong((UINT32)block, 1, 1); 

    // Write 16 bytes in one sector of the block

#ifndef TEST_NAND_READ
    // Write only, add some weight to what is saving to flash.
    // This is only for test purpose.  Save something different
    // those when both READ & WRITE test are enabled.
    w = WRITE_WEIGHT;
#endif

    // Set address where to place image
    pImage = (UINT8*)IMAGE_STARTUP_IMAGE_PA;

    memset (pImage, 0xff, pflashInfo->wDataBytesPerSector);
    for (i=0; i<16; i++)
        *(pImage+i) = i + w;

    // Set address to where to copy from
    pImage = (UINT8*)IMAGE_STARTUP_IMAGE_PA;

	// Calculate starting sector id of the good block
   	sector = block * pflashInfo->wSectorsPerBlock;

	// Copy 1 sectors to block
    memset(&sectorInfo, 0xFF, sizeof(sectorInfo));
	sectorInfo.bOEMReserved &= ~(OEM_BLOCK_READONLY|OEM_BLOCK_RESERVED);
	sectorInfo.dwReserved1 = 0;
	sectorInfo.wReserved2 = 0;
    
    // Write 1 sector
    if (!FMD_WriteSector(sector, pImage, &sectorInfo, 1)) {
		for(;;) XLDRMSG(L"Write FAILED\r\n");
	}

    XLDRMSG(L"Write 1 sector to first good block succeeded, first 16 bytes:\r\n");

    pImage = (UINT8*)IMAGE_STARTUP_IMAGE_PA;
	for (i = 0; i<16; i++)
		XLDRPrintUint8(*(pImage+i), 1, 0);
    XLDRMSG(L"\r\n");

#endif // TEST_NAND_WRITE

#ifdef TEST_NAND_READ
    // Read the 16 bytes back

    // Set address where to place data read
    // (+BytesPerSector to avoid the place wrote from)
    pImage = (UINT8*)IMAGE_STARTUP_IMAGE_PA + pflashInfo->wDataBytesPerSector;
    memset(pImage, 0xff, pflashInfo->wDataBytesPerSector);
    memset(&sectorInfo, 0xFF, sizeof(sectorInfo));

	// Calculate starting sector id of the good block
    sector = block * pflashInfo->wSectorsPerBlock;

    if (!FMD_ReadSector(sector, pImage, &sectorInfo, 1)){
        for(;;) XLDRMSG(L"read failed\r\n");
    }            
    
    XLDRMSG(L"Read 1 sector from first good block succeeded\r\n");
    XLDRMSG(L"2 status bytes + first 32 bytes:\r\n");

    // Print it out
    pImage = (UINT8*)IMAGE_STARTUP_IMAGE_PA + pflashInfo->wDataBytesPerSector; //print the first 16 bytes to debug port 

    XLDRPrintUint8(sectorInfo.bOEMReserved, 1, 0);
    XLDRPrintUint8(sectorInfo.bBadBlock, 1, 1);

	for (i = 0; i<16; i++)
		XLDRPrintUint8(*(pImage+i), 1, 0);
    XLDRMSG(L"\r\n");

	for (i = 16; i<32; i++)
		XLDRPrintUint8(*(pImage+i), 1, 0);
    XLDRMSG(L"\r\n");

#endif //TEST_NAND_READ

}
#endif  //TEST_NAND_ACCESS


//------------------------------------------------------------------------------
VOID XLDRMain()
{
#ifdef FMD_NAND
    HANDLE hFMD;
    PCI_REG_INFO regInfo;
    FlashInfo flashInfo;
    SECTOR_ADDR sector;
    BLOCK_ID block;
    SectorInfo sectorInfo;
#ifdef MEMORY_BOOT
    UINT32 count;
    SECTOR_ADDR ix;
#ifdef EBOOT_ONLY_IMAGE
    UINT32 xldr_size_to_skip = 0;
#else
    UINT32 xldr_size_to_skip = IMAGE_XLDR_BOOTSEC_NAND_SIZE;
#endif
#endif  // MEMORY_BOOT
#endif  // FMD_NAND

#ifdef UART_BOOT
    UINT32 dnld_size;
    INT xret;

#ifdef UART_DNLD_RAW_TO_NAND
    UINT32 offset;
    INT image_block_cnt, nand_blocks_to_write;

    INT xblock_cnt = 0;
    INT xack_cnt = 0;
    INT xnak_cnt = 0;
    INT xcan_cnt = 0;
    INT xothers_cnt = 0;

    INT checksum_error_cnt = 0;
    INT dup_pkt_cnt = 0;
#endif
#endif  // UART_BOOT

    UINT8 *pImage;

//    HANDLE hGpio;
    static UCHAR allocationPool[512];
    LPCWSTR ProcessorName   = L"335X";

    // Setup global variables
    if (!SetupCopySection(pTOC))
        goto cleanUp;

    OALLocalAllocInit(allocationPool,sizeof(allocationPool));

    EnableCache_GP();
    ProcessorName = L"335X";
    PlatformSetup();

    XLDRMSGINIT;   // Initialize debug serial output

    // Print information...
#ifdef FMD_NAND
#ifdef MEMORY_BOOT
    XLDRMSG( TEXT("\r\nTexas Instruments Windows CE NAND X-Loader for SubArctic "));
#endif
#ifdef TEST_NAND_ACCESS
    XLDRMSG( TEXT("\r\nX-Loader Test NAND Access for SubArctic "));
#endif
#endif  // FMD_NAND

#ifdef UART_BOOT
    XLDRMSG( TEXT("\r\nTexas Instruments Windows CE SD/UART X-Loader for SubArctic "));
#endif

    XLDRMSG( (UINT16 *)ProcessorName);
    XLDRMSG(TEXT("\r\nBuilt ") TEXT(__DATE__) TEXT(" at ") TEXT(__TIME__) TEXT("\r\n"));
    XLDRMSG(TEXT("Version ") BSP_VERSION_STRING TEXT("\r\n"));

#ifdef FMD_NAND
#ifdef MEMORY_BOOT
    XLDRMSG( TEXT("\r\nThis XLDR "));
    XLDRMSG( TEXT("\r\n  o  copies image file from NAND to RAM "));
    XLDRMSG( TEXT("\r\n  o  starts execution of the image file in RAM \r\n"));
#endif
#endif  // FMD_NAND

#ifdef UART_BOOT
    XLDRMSG( TEXT("\r\nThis XLDR "));
    XLDRMSG( TEXT("\r\n  o  dnld image file to RAM "));
#ifdef UART_DNLD_RAW_TO_NAND
    XLDRMSG( TEXT("\r\n  o  saves the image file to NAND flash \r\n"));
#endif
#ifdef UART_DNLD_EBOOT_TO_RAM
    XLDRMSG( TEXT("\r\n  o  starts execution of the image file \r\n"));
#endif
#endif  // UART_BOOT

    //GPIOInit();
    //hGpio = GPIOOpen();
    //GPIOSetBit(hGpio,BSP_LCD_POWER_GPIO);
    //GPIOSetMode(hGpio,BSP_LCD_POWER_GPIO,GPIO_DIR_OUTPUT);
    
#ifdef FMD_NAND
    // Open FMD to access NAND
    regInfo.MemBase.Reg[0] = BSP_NAND_REGS_PA;
XLDRMSG(L"\r\nBEFORE\r\n");

    hFMD = FMD_Init(NULL, &regInfo, NULL);
    if (hFMD == NULL){
        XLDRMSG(L"\r\nFMD_Init failed\r\n");
        goto cleanUp;
    }

   XLDRMSG(L"\r\nFMD_Init succeeded\r\n");

    if (!FMD_GetInfo(&flashInfo)) {
        XLDRMSG(L"\r\nFMD_GetInfo failed\r\n");
        goto cleanUp;
    }

   XLDRMSG(L"\r\nFMD_GetInfo succeeded\r\n");

#ifdef TEST_NAND_ACCESS
    XLDRTestReadWrite(&flashInfo);
#endif

#ifdef MEMORY_BOOT
    XLDRMSG(L"\r\nRetrieving EBOOT from flash memory ...\r\n");
    // Start from NAND start
    block  = 0;
    sector = 0;

    // First skip XLDR boot region.

    // NOTE - The bootrom will load the xldr from the first good block starting
    // at zero.  If an uncorrectable ECC error is encountered it will try the next
    // good block.  The last block attempted is the fourth physical block.  The first
    // block is guaranteed good when shipped from the factory, for the first 1000
    // erase/program cycles.

    // Our programming algorithm will place four copies of the xldr into the first
    // four *good* blocks.  If one or more of the first four physical blocks is marked
    // bad, the XLDR boot region will include the fifth physical block or beyond.  This
    // would result in a wasted block containing a copy of the XLDR that will never be
    // loaded by the bootrom, but it simplifies the flash management algorithms.
    count = 0;

    while (count < xldr_size_to_skip){
        if ((FMD_GetBlockStatus(block) & BLOCK_STATUS_BAD) == 0)
            count += flashInfo.dwBytesPerBlock;
        block++;
        sector += flashInfo.wSectorsPerBlock;
    }

    // get ECC type for EBOOT from FIXUP value
    g_ecctype = (UCHAR)(dwEbootECCtype & 0xff);

    FMD_Deinit(hFMD); 
    hFMD = FMD_Init(NULL, &regInfo, NULL);

    // Set address to where to place image
    pImage = (UINT8*)IMAGE_STARTUP_IMAGE_PA;

    // Read image to memory
    count = 0;
    while ((count < IMAGE_STARTUP_IMAGE_SIZE) && (block < flashInfo.dwNumBlocks)) {
        if ((FMD_GetBlockStatus(block) & BLOCK_STATUS_BAD) != 0) {// Skip bad blocks
            block++;
            sector += flashInfo.wSectorsPerBlock;
            XLDRMSG(L"#");
            continue;
        }
        
        XLDRMSG(L"\r\nReading block ");
	    XLDRPrintUlong((UINT32)block, 1, 1);

        ix = 0; // Read sectors in block
        while ((ix++ < flashInfo.wSectorsPerBlock) && (count < IMAGE_STARTUP_IMAGE_SIZE)) {
            // If a read fails, there is nothing we can do about it
            XLDRMSG(L"v");
            if (!FMD_ReadSector(sector, pImage, &sectorInfo, 1)) {
                XLDRMSG(L"$");
            }

            sector++;
            pImage += flashInfo.wDataBytesPerSector;
            count += flashInfo.wDataBytesPerSector;
		}
        XLDRMSG(L".\r\n");
        block++;
	}
    XLDRMSG(L"\r\nJumping to bootloader EBOOT @ ");
	XLDRPrintUlong(IMAGE_STARTUP_IMAGE_PA, 1, 1);
    XLDRMSGDEINIT; // Wait for serial port

    JumpTo((VOID*)IMAGE_STARTUP_IMAGE_PA);
#endif  /* MEMORY_BOOT */
#endif  /* FMD_NAND */

#ifdef UART_BOOT
    XLDRMSG(L"\r\nDNLD Image to RAM @ ");
	XLDRPrintUlong(IMAGE_STARTUP_IMAGE_PA, 1, 1);

    // Set address where to place download image
    pImage = (UINT8*)IMAGE_STARTUP_IMAGE_PA;
	//xret = XReceive(pImage, IMAGE_XLDR_BOOTSEC_NAND_SIZE+IMAGE_EBOOT_CODE_SIZE+8, &dnld_size);
	xret = XReceive(pImage, IMAGE_XLDR_BOOTSEC_NAND_SIZE+IMAGE_EBOOT_CODE_SIZE+IMAGE_BOOTLOADER_BITMAP_SIZE+8, &dnld_size);
	
	if(xret < 0)
		goto cleanUp;

    XLDRMSG(L"\r\nDownloaded size = ");
	XLDRPrintUlong(dnld_size, 1, 1);

#ifdef UART_DNLD_RAW_TO_NAND
    XLDRMSG(L"\r\nWrite Image to NAND\r\n");

	// How many nand blocks to write
	if (dnld_size < flashInfo.dwBytesPerBlock) {
		nand_blocks_to_write=0;
	} else {
		for(nand_blocks_to_write=0; nand_blocks_to_write <= NAND_MAX_BLOCKS_TO_WRITE; nand_blocks_to_write++)
			if ((nand_blocks_to_write * flashInfo.dwBytesPerBlock) >= dnld_size)
				break;
	}

	if ((nand_blocks_to_write == 0) || (nand_blocks_to_write > NAND_MAX_BLOCKS_TO_WRITE))
		goto cleanUp;

    pImage = (UINT8*)IMAGE_STARTUP_IMAGE_PA; // Set address to where to copy from

	// Write dnld image, starting from first good block (4 xldr block and 2 eboot block)
	block = 0;
    image_block_cnt = 0;

	while (image_block_cnt < nand_blocks_to_write) {

       /* writing Eboot: need to change ECC mode*/ 
        if(image_block_cnt == 4)
        {
              // get EBOOT ECC type from FIXUP value
              g_ecctype = (UCHAR)dwEbootECCtype;
                
              FMD_Deinit(hFMD); 
              hFMD = FMD_Init(NULL, &regInfo, NULL);
        }

        while (block < flashInfo.dwNumBlocks) { // Skip to a good block
            if ((FMD_GetBlockStatus(block) & BLOCK_STATUS_BAD) == 0) {
    			break; // A good block
    		}
            block++;
        }
    
    	if (block >= flashInfo.dwNumBlocks){
    		goto cleanUp; // No good block found!!
    	}

		// Erase block first
		if (!FMD_EraseBlock(block)) {
			for(;;) XLDRMSG(L"Erase FAILED\r\n");
		}

		// Calculate starting sector id of the good block
    	sector = block * flashInfo.wSectorsPerBlock;
		offset = 0;

		// Copy sectors in block
		while (offset < flashInfo.dwBytesPerBlock) {
        	memset(&sectorInfo, 0xFF, sizeof(sectorInfo));
	        sectorInfo.bOEMReserved &= ~(OEM_BLOCK_READONLY|OEM_BLOCK_RESERVED);
			sectorInfo.dwReserved1 = 0;
			sectorInfo.wReserved2 = 0;
    
    		// Write 1 sector
    		if (!FMD_WriteSector(sector, pImage + offset, &sectorInfo, 1)) {
				for(;;) XLDRMSG(L"Write FAILED\r\n");
				goto cleanUp;
			}

			sector++;
			offset += flashInfo.wDataBytesPerSector;
		}
	    XLDRMSG(L"\r\nWrote to block "); XLDRPrintUint8((UINT8)block, 1, 1);

		++image_block_cnt; // Written 1 block

		// Start from next block id and next block of data
		++block;
		pImage += offset;
	}

	XGetStats ( &xblock_cnt, &xack_cnt, &xnak_cnt,
		        &xcan_cnt, &xothers_cnt, &checksum_error_cnt, 
				&dup_pkt_cnt);

	XLDRMSG(L"\r\nReceive return code "); XLDRPrintUint8((UINT8)xret, 1, 1);
	XLDRMSG(L"blocks written ");          XLDRPrintUint8((UINT8)image_block_cnt, 1, 1);
	XLDRMSG(L"bytes rx ");                XLDRPrintUlong((UINT32)dnld_size, 1, 1);
	XLDRMSG(L"pkts rx ");                 XLDRPrintUlong((UINT32)xblock_cnt, 1, 1);
	XLDRMSG(L"acks sent ");               XLDRPrintUlong((UINT32)xack_cnt, 1, 1);
	XLDRMSG(L"naks sent ");               XLDRPrintUlong((UINT32)xnak_cnt, 1, 1);
	XLDRMSG(L"can sent ");                XLDRPrintUlong((UINT32)xcan_cnt, 1, 1);
	XLDRMSG(L"others sent ");             XLDRPrintUlong((UINT32)xothers_cnt, 1, 1);
	XLDRMSG(L"chksum errs ");             XLDRPrintUlong((UINT32)checksum_error_cnt, 1, 1);
	XLDRMSG(L"dup pkts ");                XLDRPrintUlong((UINT32)dup_pkt_cnt, 1, 1);

#endif  /* UART_DNLD_RAW_TO_NAND */

#ifdef UART_DNLD_EBOOT_TO_RAM
    XLDRMSG(L"\r\nJumping to Image @ ");
	XLDRPrintUlong(IMAGE_STARTUP_IMAGE_PA, 1, 1);
    XLDRMSGDEINIT; // Wait for serial port
	JumpTo((VOID*)IMAGE_STARTUP_IMAGE_PA);
#endif  /* UART_DNLD_EBOOT_TO_RAM */

	// Wait for serial port
    XLDRMSGDEINIT;

	// Done.
	return;
#endif  /* UART_BOOT */
    
cleanUp:
    XLDRMSG(L"\r\nHALT\r\n");
    for(;;);
}


VOID* OALPAtoVA(UINT32 address, BOOL cached)
{
    UNREFERENCED_PARAMETER(cached);
    return (VOID*)address;
}

UINT32 OALVAtoPA(VOID *pVA){return (UINT32)pVA;}

static BOOL SetupCopySection(ROMHDR *const pTableOfContents)
//  Copies image's copy section data (initialized globals) to the correct
//  fix-up location.  Once completed, initialized globals are valid. Optimized
//  memcpy is too big for X-Loader.
{
    BOOL rc = FALSE;
    UINT32 loop, count;
    COPYentry *pCopyEntry;
    UINT8 *pSrc, *pDst;

    if (pTableOfContents == (ROMHDR *const) -1) goto cleanUp;

    for (loop = 0; loop < pTableOfContents->ulCopyEntries; loop++)
        {
        pCopyEntry = (COPYentry *)(pTableOfContents->ulCopyOffset + loop*sizeof(COPYentry));

        count = pCopyEntry->ulCopyLen;
        pDst = (UINT8*)pCopyEntry->ulDest;
        pSrc = (UINT8*)pCopyEntry->ulSource;
        while (count-- > 0)
            *pDst++ = *pSrc++;
        count = pCopyEntry->ulDestLen - pCopyEntry->ulCopyLen;
        while (count-- > 0)
            *pDst++ = 0;
        }

    rc = TRUE;

cleanUp:
    return rc;
}

VOID NKDbgPrintfW(LPCWSTR pszFormat, ...){
    //  Stubbed out to shrink XLDR binary size
    UNREFERENCED_PARAMETER(pszFormat);
}

VOID NKvDbgPrintfW(LPCWSTR pszFormat, va_list pArgList){
    //  Stubbed out to shrink XLDR binary size
    UNREFERENCED_PARAMETER(pszFormat);
    UNREFERENCED_PARAMETER(pArgList);
}
//------------------------------------------------------------------------------
