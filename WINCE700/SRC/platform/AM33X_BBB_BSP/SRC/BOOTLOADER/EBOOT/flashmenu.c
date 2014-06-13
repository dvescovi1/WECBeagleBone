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
//  File:  menu.c
//
#include <eboot.h>
#include <fmdext.h>

//------------------------------------------------------------------------------
//
//  Define:  dimof
//
#ifdef dimof
#undef dimof
#endif
#define dimof(x)                (sizeof(x)/sizeof(x[0]))

extern UCHAR g_ecctype;
//------------------------------------------------------------------------------

static VOID ShowFlashGeometry(OAL_BLMENU_ITEM *pMenu);
static VOID EraseFlash(OAL_BLMENU_ITEM *pMenu);
static VOID EraseBlock(OAL_BLMENU_ITEM *pMenu);
static VOID ReserveBlock(OAL_BLMENU_ITEM *pMenu);
static VOID SetBadBlock(OAL_BLMENU_ITEM *pMenu);
static VOID DumpFlash(OAL_BLMENU_ITEM *pMenu);
static VOID FormatFlash(OAL_BLMENU_ITEM *pMenu);
static VOID WriteFlash(OAL_BLMENU_ITEM *pMenu);
static VOID EnableFlashNK(OAL_BLMENU_ITEM *pMenu);
static VOID SetECCType(OAL_BLMENU_ITEM *pMenu);


//------------------------------------------------------------------------------

OAL_BLMENU_ITEM g_menuFlash[] = {
    {
        L'1', L"Show flash geometry", ShowFlashGeometry,
        NULL, NULL, NULL
    }, {
        L'2', L"Dump flash sector", DumpFlash,
        NULL, NULL, NULL
    }, {
        L'3', L"Erase flash", EraseFlash,
        NULL, NULL, NULL
    }, {
        L'4', L"Erase block range", EraseBlock,
        NULL, NULL, NULL
    }, {
        L'5', L"Reserve block range", ReserveBlock,
        NULL, NULL, NULL
    }, {
        L'6', L"Set bad block", SetBadBlock,
        NULL, NULL, NULL
    }, {
        L'7', L"Format flash", FormatFlash,
        NULL, NULL, NULL
    }, {
        L'8', L"Enable flashing NK.bin", EnableFlashNK,
        NULL, NULL, NULL
    }, {
        L'9', L"Set ECC mode", SetECCType,
        NULL, NULL, NULL
    }, {
        L'0', L"Exit and Continue", NULL,
        NULL, NULL, NULL
    }, {
        0, NULL, NULL,
        NULL, NULL, NULL
    }
};


//------------------------------------------------------------------------------

VOID 
ShowFlashGeometry(OAL_BLMENU_ITEM *pMenu)
{
    HANDLE hFMD;
    PCI_REG_INFO regInfo;
    FlashInfo flashInfo;
    LPCWSTR pszType;
    BLOCK_ID block;
    UINT32 status;
    UINT32 listmode=0;

    UNREFERENCED_PARAMETER(pMenu);

    regInfo.MemBase.Reg[0] = g_ulFlashBase;
    hFMD = FMD_Init(NULL, &regInfo, NULL);
    if (hFMD == NULL) 
        {
        OALLog(L" Oops, can't open FMD driver\r\n");
        goto cleanUp;
        }

    flashInfo.dwNumBlocks = 0x89ABCDEF;  // A magic number to signify getting nand manufa & dev ID
    if (!FMD_GetInfo(&flashInfo)) 
        {
        OALLog(L" Oops, can't get flash geometry info\r\n");
        goto cleanUp;
        }

    OALLog(L"\r\n");
    OALLog(L" Manufacturer ID:    0x%02X\r\n", (UINT8)((flashInfo.dwNumBlocks & 0x0000FF00) >> 8));
    OALLog(L" Device ID:          0x%02X\r\n", (UINT8)(flashInfo.dwNumBlocks & 0x000000FF));

    flashInfo.dwNumBlocks = 0;

    if (!FMD_GetInfo(&flashInfo)) 
        {
        OALLog(L" Oops, can't get flash geometry info\r\n");
        goto cleanUp;
        }

    switch (flashInfo.flashType) 
        {
        case NAND:
            pszType = L"NAND";
            break;
        case NOR:
            pszType = L"NOR";
            break;
        default:
            pszType = L"Unknown";
        }

    OALLog(L" Flash Type:    %s\r\n", pszType);
    OALLog(L" Blocks:        %d\r\n", flashInfo.dwNumBlocks);
    OALLog(L" Bytes/block:   %d\r\n", flashInfo.dwBytesPerBlock);
    OALLog(L" Sectors/block: %d\r\n", flashInfo.wSectorsPerBlock);
    OALLog(L" Bytes/sector:  %d\r\n", flashInfo.wDataBytesPerSector);
	
    switch (g_bootCfg.ECCtype) 
        {
        case 0:
            pszType = L"Hamming 1bit ECC";
            break;
        case 1:
            pszType = L"BCH 4bit ECC with SW error correction";
            break;
        case 2:
            pszType = L"BCH 8bit ECC with SW error correction";
            break;
        case 3:
            pszType = L"BCH 8bit ECC with HW error correction";
            break;
        case 4:
            pszType = L"BCH 8bit ECC-X with HW error correction";
            break;
			
        default:
            pszType = L"Unknown";
        }
	
    OALLog(L" ECC mode:  %s\r\n", pszType);

    // now list bad/reserved sectors

    // First offset given
    block = 0;
    while (block < flashInfo.dwNumBlocks) 
        {

        // If block is bad, we have to offset it
        status = FMD_GetBlockStatus(block);

        // bad block
        if ((status & BLOCK_STATUS_BAD) != 0) 
            {
            if (listmode!=1)
                {
                OALLog(L"\r\n[bad]     ");
                listmode=1;
                }

            OALLog(L" %d", block);

            block++;
            continue;
            }

        // reserved block
        if ((status & BLOCK_STATUS_RESERVED) != 0) 
            {
            if (listmode!=2)
                {
                OALLog(L"\r\n[reserved]");
                listmode=2;
                }

            OALLog(L" %d", block);

            block++;
            continue;
            }

        block++;
    }

    OALLog(L" Done\r\n");

cleanUp:
    if (hFMD != NULL) 
        {
        FMD_Deinit(hFMD);
        }
    return;
}

//------------------------------------------------------------------------------

VOID EraseFlash(OAL_BLMENU_ITEM *pMenu)
{
    WCHAR key;
    HANDLE hFMD = NULL;
    PCI_REG_INFO regInfo;
    FlashInfo flashInfo;
    BLOCK_ID block;
    UINT32 status;

    UNREFERENCED_PARAMETER(pMenu);


    OALLog(L" Do you want to erase unreserved blocks [-/y]? ");

    // Get key
    key = OALBLMenuReadKey(TRUE);
    OALLog(L"%c\r\n", key);

    // Depending on result
    if (key != L'y' && key != L'Y') goto cleanUp;

    // Open FMD
    regInfo.MemBase.Reg[0] = g_ulFlashBase;
    hFMD = FMD_Init(NULL, &regInfo, NULL);
    if (hFMD == NULL) 
        {
        OALLog(L" Oops, can't open FMD driver\r\n");
        goto cleanUp;
        }

    if (!FMD_GetInfo(&flashInfo)) 
        {
        OALLog(L" Oops, can't get flash geometry info\r\n");
        goto cleanUp;
        }

    // First offset given
    block = 0;
    while (block < flashInfo.dwNumBlocks) 
        {

        // If block is bad, we have to offset it
        status = FMD_GetBlockStatus(block);

        // Skip bad blocks
        if ((status & BLOCK_STATUS_BAD) != 0) 
            {
            OALLog(L" Skip bad block %d\r\n", block);
            block++;
            continue;
            }

        // Skip reserved blocks
        if ((status & BLOCK_STATUS_RESERVED) != 0) 
            {
            OALLog(L" Skip reserved block %d\r\n", block);
            block++;
            continue;
            }

        // Erase block
        if (!FMD_EraseBlock(block)) 
            {
            OALLog(L" Oops, can't erase block %d - mark as bad\r\n", block);
            FMD_SetBlockStatus(block, BLOCK_STATUS_BAD);
            }

        block++;
    }

    OALLog(L" Done\r\n");
    
    // Block until a keypress
    OALBLMenuReadKey(TRUE);
    
cleanUp:
    if (hFMD != NULL) FMD_Deinit(hFMD);
    return;
}


//------------------------------------------------------------------------------

VOID EraseBlock(OAL_BLMENU_ITEM *pMenu)
{
    WCHAR key;
    HANDLE hFMD = NULL;
    PCI_REG_INFO regInfo;
    FlashInfo flashInfo;
    BLOCK_ID firstblock, lastblock=0;
    WCHAR szInputLine[16];
    UINT32 status;

    UNREFERENCED_PARAMETER(pMenu);

    OALLog(L"\r\n First Block Number: ");

    if (OALBLMenuReadLine(szInputLine, dimof(szInputLine)) == 0) 
        {
        goto cleanUp;
        }

    // Get block number
    firstblock = OALStringToUINT32(szInputLine);

    OALLog(L"\r\n Last Block Number: ");

    if (OALBLMenuReadLine(szInputLine, dimof(szInputLine)) != 0) 
        {
        // Get block number
        lastblock = OALStringToUINT32(szInputLine);
        }

    if (lastblock < firstblock) 
        {
        lastblock=firstblock;
        }

    // Open FMD
    regInfo.MemBase.Reg[0] = g_ulFlashBase;
    hFMD = FMD_Init(NULL, &regInfo, NULL);
    if (hFMD == NULL) 
        {
        OALLog(L" Oops, can't open FMD driver\r\n");
        goto cleanUp;
        }

    if (!FMD_GetInfo(&flashInfo)) 
        {
        OALLog(L" Oops, can't get flash geometry info\r\n");
        goto cleanUp;
        }

    if (lastblock >= flashInfo.dwNumBlocks) 
        {
        OALLog(L" Oops, too big block number\r\n");
        goto cleanUp;
        }

    OALLog(L" Do you want erase block %d-%d [-/y]? ", firstblock, lastblock);

    // Get key
    key = OALBLMenuReadKey(TRUE);
    OALLog(L"%c\r\n", key);

    // Depending on result
    if (key != L'y' && key != L'Y') 
        {
        goto cleanUp;
        }

    while (firstblock<=lastblock)
        {

        // If block is bad, we have to offset it
        status = FMD_GetBlockStatus(firstblock);

        // ask before erasing reserved blocks
        if ((status & BLOCK_STATUS_RESERVED) != 0) 
            {

            OALLog(L" Do you want to erase reserved block %d [-/y]? ", firstblock);

            // Get key
            key = OALBLMenuReadKey(TRUE);
            OALLog(L"%c\r\n", key);

            // Depending on result
            if (key != L'y' && key != L'Y') 
                {
                firstblock++;
                continue;
                }

            }

        // ask before erasing bad blocks
        if ((status & BLOCK_STATUS_BAD) != 0) 
            {

            OALLog(L" Do you want to erase bad block %d [-/y]? ", firstblock);

            // Get key
            key = OALBLMenuReadKey(TRUE);
            OALLog(L"%c\r\n", key);

            // Depending on result
            if (key != L'y' && key != L'Y') 
                {
                firstblock++;
                continue;
                }

            }

        // Erase block
        if (!FMD_EraseBlock(firstblock)) 
            {
            OALLog(L" Oops, can't erase block %d - mark as bad\r\n", firstblock);
            FMD_SetBlockStatus(firstblock, BLOCK_STATUS_BAD);
            }

        firstblock++;
        OALLog(L".");
        }

    OALLog(L" Done\r\n");

cleanUp:
    if (hFMD != NULL) FMD_Deinit(hFMD);
    return;
}

//------------------------------------------------------------------------------

VOID ReserveBlock(OAL_BLMENU_ITEM *pMenu)
{
    WCHAR key;
    HANDLE hFMD = NULL;
    PCI_REG_INFO regInfo;
    FlashInfo flashInfo;
    BLOCK_ID firstblock, lastblock=0;
    WCHAR szInputLine[16];
    UINT32 status;

    UNREFERENCED_PARAMETER(pMenu);

    OALLog(L"\r\n First Block Number: ");

    if (OALBLMenuReadLine(szInputLine, dimof(szInputLine)) == 0) 
        {
        goto cleanUp;
        }

    // Get block number
    firstblock = OALStringToUINT32(szInputLine);

    OALLog(L"\r\n Last Block Number: ");

    if (OALBLMenuReadLine(szInputLine, dimof(szInputLine)) != 0) 
        {
        // Get block number
        lastblock = OALStringToUINT32(szInputLine);
        }

    if (lastblock < firstblock) 
        {
        lastblock=firstblock;
        }

    // Open FMD
    regInfo.MemBase.Reg[0] = g_ulFlashBase;
    hFMD = FMD_Init(NULL, &regInfo, NULL);
    if (hFMD == NULL) 
        {
        OALLog(L" Oops, can't open FMD driver\r\n");
        goto cleanUp;
        }

    if (!FMD_GetInfo(&flashInfo)) 
        {
        OALLog(L" Oops, can't get flash geometry info\r\n");
        goto cleanUp;
        }

    if (lastblock >= flashInfo.dwNumBlocks) 
        {
        OALLog(L" Oops, too big block number\r\n");
        goto cleanUp;
        }

    OALLog(L" Do you want mark as reserved block %d-%d [-/y]? ", firstblock, lastblock);

    // Get key
    key = OALBLMenuReadKey(TRUE);
    OALLog(L"%c\r\n", key);

    // Depending on result
    if (key != L'y' && key != L'Y') 
        {
        goto cleanUp;
        }

    while (firstblock<=lastblock)
        {

        // If block is bad, we have to offset it
        status = FMD_GetBlockStatus(firstblock);

#if 0
        // Skip bad blocks
        if ((status & BLOCK_STATUS_BAD) != 0) 
            {
            OALLog(L" Skip bad block %d\r\n", firstblock);
            // NOTE - this will cause a smaller number of blocks to actually be reserved...
            firstblock++;
            continue;
            }

        // Skip already reserved blocks
        if ((status & BLOCK_STATUS_RESERVED) != 0) 
            {
            OALLog(L" Skip reserved block %d\r\n", firstblock);
            firstblock++;
            continue;
            }
#endif

        // Mark block as read-only & reserved
        if (!FMD_SetBlockStatus(firstblock, BLOCK_STATUS_READONLY|BLOCK_STATUS_RESERVED)) 
            {
            OALLog(L" Oops, can't mark block %d - as reserved\r\n", firstblock);
            }

        firstblock++;
        OALLog(L".");
        }

    OALLog(L" Done\r\n");

cleanUp:
    if (hFMD != NULL) FMD_Deinit(hFMD);
    return;
}

//------------------------------------------------------------------------------

VOID DumpFlash(OAL_BLMENU_ITEM *pMenu)
{
    HANDLE hFMD = NULL;
    PCI_REG_INFO regInfo;
    FlashInfo flashInfo;
    SectorInfo sectorInfo;
    SECTOR_ADDR sector;
    WCHAR szInputLine[16];
    UINT8 buffer[2048], pOob[64];
    UINT32 i, j;

    UNREFERENCED_PARAMETER(pMenu);


    // Open FMD
    regInfo.MemBase.Reg[0] = g_ulFlashBase;
    hFMD = FMD_Init(NULL, &regInfo, NULL);
    if (hFMD == NULL) 
        {
        OALLog(L" Oops, can't open FMD driver\r\n");
        goto cleanUp;
        }

    if (!FMD_GetInfo(&flashInfo)) 
        {
        OALLog(L" Oops, can't get flash geometry info\r\n");
        goto cleanUp;
        }

    if (flashInfo.wDataBytesPerSector > sizeof(buffer)) 
        {
        OALLog(L" Oops, sector size larger than my buffer\r\n");
        goto cleanUp;
        }

        for(;;)
        {

        OALLog(L"\r\n Sector Number: ");

        if (OALBLMenuReadLine(szInputLine, dimof(szInputLine)) == 0) 
            {
            break;
            }

        // Get sector number
        sector = OALStringToUINT32(szInputLine);

        // Check sector number
        if (sector > flashInfo.dwNumBlocks * flashInfo.wSectorsPerBlock) 
            {
            OALLog(L" Oops, too big sector number\r\n");
            continue;
            }

        if (!FMD_ReadSector(sector, buffer, &sectorInfo, 1)) 
            {
            OALLog(L" Oops, sector read failed\r\n");
            continue;
            }

        OALLog(
            L"\r\nSector %d (sector %d in block %d)\r\n", sector,
            sector%flashInfo.wSectorsPerBlock, sector/flashInfo.wSectorsPerBlock
        );
        OALLog(
            L"Reserved1: %08x OEMReserved: %02x Bad: %02x Reserved2: %04x\r\n",
            sectorInfo.dwReserved1, sectorInfo.bOEMReserved,
            sectorInfo.bBadBlock, sectorInfo.wReserved2
        );

        for (i = 0; i < flashInfo.wDataBytesPerSector; i += 16) 
            {
            OALLog(L"%04x ", i);
            for (j = i; j < i + 16 && j < flashInfo.wDataBytesPerSector; j++) 
                {
                OALLog(L" %02x", buffer[j]);
                }
            OALLog(L"  ");
            for (j = i; j < i + 16 && j < flashInfo.wDataBytesPerSector; j++) 
                {
                if (buffer[j] >= ' ' && buffer[j] < 127) 
                    {
                    OALLog(L"%c", buffer[j]);
                    } 
                else 
                    {
                    OALLog(L".");
                    }
                }
            OALLog(L"\r\n");
            }
	//dump OOB data
        if (!FMD_ReadSectorOOB(sector, pOob)) 
            {
            OALLog(L" Oops, sector read failed\r\n");
            continue;
            }
        for (i = 0; i < 64; i += 16) 
            {
            OALLog(L"%04x ", i);
            for (j = i; j < i + 16 && j < 64; j++) 
                {
                OALLog(L" %02x", pOob[j]);
                }
                
            OALLog(L"\r\n");
            }

        }

cleanUp:

    if (hFMD != NULL) 
        {
        FMD_Deinit(hFMD);
        }

    return;
}

//------------------------------------------------------------------------------

VOID SetBadBlock(OAL_BLMENU_ITEM *pMenu)
{
    HANDLE hFMD = NULL;
    PCI_REG_INFO regInfo;
    FlashInfo flashInfo;
    BLOCK_ID blockId;
    WCHAR szInputLine[16];

    UNREFERENCED_PARAMETER(pMenu);

    // Open FMD
    regInfo.MemBase.Reg[0] = g_ulFlashBase;
    hFMD = FMD_Init(NULL, &regInfo, NULL);
    if (hFMD == NULL) 
        {
        OALLog(L" Oops, can't open FMD driver\r\n");
        goto cleanUp;
        }

    if (!FMD_GetInfo(&flashInfo)) 
        {
        OALLog(L" Oops, can't get flash geometry info\r\n");
        goto cleanUp;
        }

    OALLog(L"\r\n Block Number: ");

    if (OALBLMenuReadLine(szInputLine, dimof(szInputLine)) == 0) 
        {
        goto cleanUp;
        }

    // Get sector number
    blockId = OALStringToUINT32(szInputLine);

    // Check sector number
    if (blockId >= flashInfo.dwNumBlocks) 
        {
        OALLog(L" Oops, too big block number\r\n");
        goto cleanUp;
        }

    FMD_SetBlockStatus(blockId, BLOCK_STATUS_BAD);

    OALLog(L"\r\n Done\r\n");

cleanUp:
    if (hFMD != NULL) 
        {
        FMD_Deinit(hFMD);
        }

    return;
}

//------------------------------------------------------------------------------

VOID FormatFlash(OAL_BLMENU_ITEM *pMenu)
{
    WCHAR key;
    
    UNREFERENCED_PARAMETER(pMenu);

    OALLog(L" Do you want to format unreserved blocks [-/y]? ");

    // Get key
    key = OALBLMenuReadKey(TRUE);
    OALLog(L"%c\r\n", key);

    // Depending on result
    if (key != L'y' && key != L'Y') goto cleanUp;

    BLConfigureFlashPartitions(TRUE);
    
cleanUp:    
    
    return;
}

//------------------------------------------------------------------------------

VOID WriteFlash(OAL_BLMENU_ITEM *pMenu)
{
    HANDLE hFMD = NULL;
    PCI_REG_INFO regInfo;
    FlashInfo flashInfo;
    SectorInfo sectorInfo;
    SECTOR_ADDR sector;
    WCHAR szInputLine[16];
    UINT8 buffer[2048];
    UINT32 i;
    UINT8 pattern;

    UNREFERENCED_PARAMETER(pMenu);

    // Open FMD
    regInfo.MemBase.Reg[0] = g_ulFlashBase;
    hFMD = FMD_Init(NULL, &regInfo, NULL);
    if (hFMD == NULL) 
        {
        OALLog(L" Oops, can't open FMD driver\r\n");
        goto cleanUp;
        }

    if (!FMD_GetInfo(&flashInfo)) 
        {
        OALLog(L" Oops, can't get flash geometry info\r\n");
        goto cleanUp;
        }

    if (flashInfo.wDataBytesPerSector > sizeof(buffer)) 
        {
        OALLog(L" Oops, sector size larger than my buffer\r\n");
        goto cleanUp;
        }

        for(;;)
        {

        OALLog(L"\r\n Sector Number: ");

        if (OALBLMenuReadLine(szInputLine, dimof(szInputLine)) == 0) 
            {
            break;
            }

        // Get sector number
        sector = OALStringToUINT32(szInputLine);

        // Check sector number
        if (sector > flashInfo.dwNumBlocks * flashInfo.wSectorsPerBlock) 
            {
            OALLog(L" Oops, too big sector number\r\n");
            continue;
            }

        OALLog(L"\r\n Pattern: ");

        if (OALBLMenuReadLine(szInputLine, dimof(szInputLine)) == 0) 
            {
            break;
            }

        // Get pattern
        pattern = (UINT8)(OALHexStringToUINT32(szInputLine) & 0xFF);

        // Fill buffer with pattern
        for (i=0; i<flashInfo.wDataBytesPerSector; i++)
            buffer[i] = pattern;

        memset(&sectorInfo, 0, sizeof(sectorInfo));

        if (!FMD_WriteSector(sector, buffer, &sectorInfo, 1)) 
            {
            OALLog(L" Oops, sector write failed\r\n");
            }
        else
            {
            OALLog(L" write sector %d with pattern 0x%02X completed\r\n", sector, pattern);
            }
        }
         

cleanUp:

    if (hFMD != NULL) 
        {
        FMD_Deinit(hFMD);
        }

    return;
}


VOID EnableFlashNK(OAL_BLMENU_ITEM *pMenu)
{
    WCHAR key;

    UNREFERENCED_PARAMETER(pMenu);
    if (g_bootCfg.flashNKFlags & ENABLE_FLASH_NK) {
        OALLog(L" Disable Flashing NK.bin [y/-]: ");
    } else {
        OALLog(L" Enable Flashing NK.bin [y/-]: ");
    }    

    // Get key
    key = OALBLMenuReadKey(TRUE);
    OALLog(L"%c\r\n", key);

    if (key == L'y' || key == L'Y') {
        if (g_bootCfg.flashNKFlags & ENABLE_FLASH_NK) 
		{
            g_bootCfg.flashNKFlags &= ~ENABLE_FLASH_NK;
            OALLog(L" Flashing NK.bin is disabled\r\n");
        }
		else 
		{
            g_bootCfg.flashNKFlags |= ENABLE_FLASH_NK;
            OALLog(L" Flashing NK.bin is enabled\r\n");
        }    
    }

}

static VOID SetECCType(OAL_BLMENU_ITEM *pMenu)
{
    WCHAR key;
    UNREFERENCED_PARAMETER(pMenu);
    OALLog(L" This command is used to temporarily changing ECC mode in NK.bin, it is for test purpose only! \r\n");
    OALLog(L" ECC modes: \r\n");
    OALLog(L"   0(Hamming 1bit)\r\n");
    OALLog(L"   1(BCH 4bit)\r\n");
    OALLog(L"   2(BCH 8bit)\r\n");
    OALLog(L"   3(BCH 8bit ELM)\r\n");
    OALLog(L"   4(BCH 8bit ELM-X)\r\n");
    OALLog(L" Select ECC mode: ");

    // Get key
    key = OALBLMenuReadKey(TRUE);
    OALLog(L"%c\r\n", key);

    if (key == L'0' || key == L'1' || key == L'2' || key == L'3' || key == L'4')
    {
        g_bootCfg.ECCtype = (UCHAR)(key - '0');
    }
    else 
    {
        g_bootCfg.ECCtype = 3;
        OALLog(L" Invalid ECC mode, set ECC mode as BCH 8bit ELM\r\n");
    }    
    g_ecctype = g_bootCfg.ECCtype;
	
}

