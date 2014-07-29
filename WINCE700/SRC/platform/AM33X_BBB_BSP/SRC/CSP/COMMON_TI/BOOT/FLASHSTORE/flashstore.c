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
//  File:  flashstore.c
//
//
//  This file implements OALFlashStoreXXX interface
//
#include "omap.h"
#pragma warning(push)
#pragma warning(disable:4115)
#include <fmd.h>
#include <oal.h>
#pragma warning(pop)
#include "oalex.h"

//------------------------------------------------------------------------------

#define dimof(x)    (sizeof(x)/sizeof((x)[0]))

typedef struct OAL_FLASH_CONTEXT {
    VOID *pFmd;
    ULONG sectorSize;
    ULONG sectorsPerBlock;
    ULONG blocksOnFlash;
    ULONG reservedBlocks;
    ULONG blockPos;
    ULONG badBlocksAtPos;

    ULONG reservedRegions;
    ReservedEntry reservedRegion[16];
    ULONG regions;
    FlashRegion region[16];

    UCHAR sectorBuffer[2048];

} OAL_FLASH_CONTEXT;

//------------------------------------------------------------------------------

static
BOOL
SeekToBlock(
    HANDLE hFlash,
    ULONG block
    );

static
BOOL
IsSectorEmpty(
    UCHAR *pData,
    ULONG size,
    SectorInfo *pInfo
    );

static
BOOL
BuildLayoutInfo(
    HANDLE hFlash
    );

//------------------------------------------------------------------------------

HANDLE
OALFlashStoreOpen(
    DWORD address
    )
{
    static OAL_FLASH_CONTEXT flashContext;
    HANDLE hFlash = NULL;
    PCI_REG_INFO regInfo;
    FlashInfo flashInfo;
    ULONG block;
    DWORD status;


    OALMSG(OAL_FLASH&&OAL_FUNC, (L"+OALFlashStoreOpen(0x%08x)\r\n", address));

    // Open FMD to access NAND
    
    regInfo.MemBase.Reg[0] = address;
    flashContext.pFmd = FMD_Init(NULL, &regInfo, NULL);
    if (flashContext.pFmd == NULL)
        {
        OALMSG(OAL_ERROR, (L"ERROR: OALFlashStoreOpen: "
            L"FMD_Init call failed!\r\n"
            ));
        goto cleanUp;
        }

    // Get flash info
    if (!FMD_GetInfo(&flashInfo))
        {
        OALMSG(OAL_ERROR, (L"ERROR: OALFlashStoreOpen: "
            L"FMD_GetInfo call failed!\r\n"
            ));
        goto cleanUp;
        }
    flashContext.sectorSize = flashInfo.wDataBytesPerSector;
    flashContext.sectorsPerBlock = flashInfo.wSectorsPerBlock;
    flashContext.blocksOnFlash = flashInfo.dwNumBlocks;
    flashContext.blockPos = 0;
    flashContext.badBlocksAtPos = 0;
    flashContext.reservedBlocks = 0;

    // Get number of reserved blocks
    block = 0;
    while (block < flashContext.blocksOnFlash)
        {
        status = FMD_GetBlockStatus(block);
        if ((status & BLOCK_STATUS_BAD) != 0)
            {
            block++;
            continue;
            }
        if ((status & BLOCK_STATUS_RESERVED) == 0) break;
        flashContext.reservedBlocks++;
        block++;
        }

    OALMSG(OAL_INFO, (L"OALFlashStoreOpen: "
        L"%d blocks, %d sectors/block\r\n",
        flashContext.blocksOnFlash, flashContext.sectorsPerBlock
        ));
    OALMSG(OAL_INFO, (L"OALFlashStoreOpen: "
        L"%d bytes/sector, %d reserved blocks\r\n",
        flashContext.sectorSize, flashContext.reservedBlocks
        ));

    // Done
    hFlash = &flashContext;

cleanUp:
    OALMSG(OAL_FLASH&&OAL_FUNC, (
        L"-OALFlashStoreOpen(rc = 0x%08x)\r\n", hFlash
        ));
    return hFlash;
}

//------------------------------------------------------------------------------

BOOL
OALFlashStoreWrite(
    HANDLE hFlash,
    ULONG start,
    UCHAR *pData,
    ULONG dataSize,
    BOOL includeSectorInfo,
    BOOL offsetReservedBlocks
    )
{
    BOOL rc = FALSE;
    OAL_FLASH_CONTEXT *pFlash = hFlash;
    ULONG offset, sector, block;
    ULONG sectorInBlock, sectorDataSize;
    ULONG startSector, startBlock;
    SectorInfo sectorInfo, *pSectorInfo;
    ULONG retry;


    OALMSG(OAL_FLASH&&OAL_FUNC, (
        L"+OALFlashStoreWrite(0x%08x, 0x%08x, 0x%08x, 0x%08x, %d, %d)\r\n",
        hFlash, start, pData, dataSize, includeSectorInfo, offsetReservedBlocks
        ));

    sectorDataSize = pFlash->sectorSize;
    if (includeSectorInfo) sectorDataSize += sizeof(SectorInfo);

    // First get socket start block number
    startSector = start/pFlash->sectorSize;
    startBlock = startSector/pFlash->sectorsPerBlock;

    // We support only write on sector boundary...
    offset = start - startSector * pFlash->sectorSize;
    if (offset > 0)
        {
        OALMSG(OAL_ERROR, (L"ERROR: OALFlashStoreWrite: "
            L"Flash write must start on sector boundary (offset %d)\r\n",
            offset
            ));
        goto cleanUp;
        }

    // And only full sectors
    offset = dataSize - (dataSize/sectorDataSize) * sectorDataSize;
    if (offset > 0)
        {
        OALMSG(OAL_ERROR, (L"ERROR: OALFlashStoreRead: "
            L"Flash read size must be multiple of sector size (offset %d)\r\n",
            offset
            ));
        goto cleanUp;
        }

    // Offset reserved blocks if required
    if (offsetReservedBlocks)
        {
        startBlock +=  pFlash->reservedBlocks;
        startSector += pFlash->reservedBlocks * pFlash->sectorsPerBlock;
        }

    // Seek to start block
    if (!SeekToBlock(pFlash, startBlock))
        {
        OALMSG(OAL_ERROR, (L"ERROR: OALFlashStoreWrite: "
            L"Failed seek to block %d (address 0x%08x, %d bad blocks)\r\n",
            startBlock, start, pFlash->badBlocksAtPos
            ));
        goto cleanUp;
        }

    // Start write to flash
    offset = 0;
    sector = startSector + pFlash->badBlocksAtPos * pFlash->sectorsPerBlock;
    block = startBlock + pFlash->badBlocksAtPos;
    sectorInBlock = sector - block * pFlash->sectorsPerBlock;
    while (offset < dataSize)
        {
        // If it is first sector in block, check and erase block
        if (sectorInBlock == 0)
            {
            // First check if block exists
            if (block >= pFlash->blocksOnFlash)
                {
                OALMSG(OAL_ERROR, (L"ERROR: OALFlashStoreWrite: "
                    L"Run out of blocks on flash memory\r\n"
                    ));
                goto cleanUp;
                }
            // Skip block if it is marked as bad
            if ((FMD_GetBlockStatus(block) & BLOCK_STATUS_BAD) != 0)
                {
                OALMSG(OAL_WARN, (L"WARN: "
                    L"Skip bad block %d\r\n", block
                    ));
                block++;
                pFlash->badBlocksAtPos++;
                sector += pFlash->sectorsPerBlock;
                continue;
                }
            // Erase block
            retry = 4;
            do
                {
                if (FMD_EraseBlock(block)) break;
                }
            while (--retry > 0);
            // If erase failed
            if (retry == 0)
                {
                OALMSG(OAL_WARN, (L"WARN: OALFlashStoreWrite: "
                    L"Failed erase block %d, mark it as bad\r\n", block
                    ));
                if (!FMD_SetBlockStatus(block, BLOCK_STATUS_BAD))
                    {
                    OALMSG(OAL_ERROR, (L"ERROR: OALFlashStoreWrite: "
                        L"Failed set block %d status as bad\r\n", block
                        ));
                    goto cleanUp;
                    }
                block++;
                pFlash->badBlocksAtPos++;
                sector += pFlash->sectorsPerBlock;
                continue;
                }
            }

        // Prepare sector info
        if (includeSectorInfo)
            {
            pSectorInfo = (SectorInfo*)(pData + offset + pFlash->sectorSize);
            if (pSectorInfo->bBadBlock != 0xFF)
                {
                OALMSG(OAL_ERROR, (L"ERROR: OALFlashStoreWrite: "
                    L"Corrupted data, sector info can't have set bad block\r\n"
                    ));
                goto cleanUp;
                }
            }
        else
            {
            memset(&sectorInfo, 0xFF, sizeof(sectorInfo));
            if (!offsetReservedBlocks) sectorInfo.bOEMReserved = (BYTE)
                ~(OEM_BLOCK_RESERVED|OEM_BLOCK_READONLY);
            pSectorInfo = &sectorInfo;
            }

        // Write sector only if it isn't empty
        if (!IsSectorEmpty(pData + offset, pFlash->sectorSize, pSectorInfo))
            {
            // Write sector
            if (!FMD_WriteSector(sector, pData + offset, pSectorInfo, 1))
                {
                OALMSG(OAL_WARN, (L"WARN: OALFlashStoreWrite: "
                    L"Failed write sector %d (%d sector in block %d)\r\n",
                    sector, sectorInBlock, block
                    ));
                // Try erase block and write again
                if (offset >= (sectorInBlock * sectorDataSize))
                    {
                    offset -= (sectorInBlock * sectorDataSize);
                    sector -= sectorInBlock;
                    sectorInBlock = 0;
                    continue;
                    }
                // If we get there we can't recover
                goto cleanUp;
                }
            }

        // Move to next sector
        sector++;
        sectorInBlock++;
        if (sectorInBlock  >= pFlash->sectorsPerBlock)
            {
            block++;
            pFlash->blockPos++;
            sectorInBlock = 0;
            }
        offset += sectorDataSize;
        }

    // Done
    rc = TRUE;

cleanUp:
    OALMSG(OAL_FLASH&&OAL_FUNC, (L"-OALFlashStoreWrite(rc = %d)\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------

BOOL
OALFlashStoreRead(
    HANDLE hFlash,
    ULONG start,
    UCHAR *pData,
    ULONG dataSize,
    BOOL includeSectorInfo,
    BOOL offsetReservedBlocks
    )
{
    BOOL rc = FALSE;
    OAL_FLASH_CONTEXT *pFlash = hFlash;
    ULONG offset, sector, block;
    ULONG sectorInBlock, sectorDataSize;
    ULONG startSector, startBlock;
    SectorInfo sectorInfo, *pSectorInfo;
    ULONG retry;


    OALMSG(OAL_FLASH&&OAL_FUNC, (
        L"+OALFlashStoreRead(0x%08x, 0x%08x, 0x%08x, 0x%08x, %d, %d)\r\n",
        hFlash, start, pData, dataSize, includeSectorInfo, offsetReservedBlocks
        ));

    sectorDataSize = pFlash->sectorSize;
    if (includeSectorInfo) sectorDataSize += sizeof(SectorInfo);

    // First get socket start block number
    startSector = start/pFlash->sectorSize;
    startBlock = startSector/pFlash->sectorsPerBlock;

    // We support only read on sector boundary...
    offset = start - startSector * pFlash->sectorSize;
    if (offset > 0)
        {
        OALMSG(OAL_ERROR, (L"ERROR: OALFlashStoreRead: "
            L"Flash read must start on sector boundary (offset %d)\r\n",
            offset
            ));
        goto cleanUp;
        }

    // And only full sectors
    offset = dataSize - (dataSize/sectorDataSize) * sectorDataSize;
    if (offset > 0)
        {
        OALMSG(OAL_ERROR, (L"ERROR: OALFlashStoreRead: "
            L"Flash read size must be multiple of sector size (offset %d)\r\n",
            offset
            ));
        goto cleanUp;
        }

    // Offset reserved blocks if required
    if (offsetReservedBlocks)
        {
        startBlock +=  pFlash->reservedBlocks;
        startSector += pFlash->reservedBlocks * pFlash->sectorsPerBlock;
        }

    // Seek to start block
    if (!SeekToBlock(pFlash, startBlock))
        {
        OALMSG(OAL_ERROR, (L"ERROR: OALFlashStoreRead: "
            L"Failed seek to block %d (address 0x%08x, %d bad blocks)\r\n",
            startBlock, start, pFlash->badBlocksAtPos
            ));
        goto cleanUp;
        }


    // Start read
    offset = 0;
    sector = startSector + pFlash->badBlocksAtPos * pFlash->sectorsPerBlock;
    block = startBlock + pFlash->badBlocksAtPos;
    sectorInBlock = sector - block * pFlash->sectorsPerBlock;
    while (offset < dataSize)
        {
        // If it is first sector in block, check and erase block
        if (sectorInBlock == 0)
            {
            // First check if block exists
            if (block >= pFlash->blocksOnFlash)
                {
                OALMSG(OAL_ERROR, (L"ERROR: OALFlashStoreRead: "
                    L"Run out of blocks on flash memory\r\n"
                    ));
                goto cleanUp;
                }
            // Skip block if it is marked as bad
            if ((FMD_GetBlockStatus(block) & BLOCK_STATUS_BAD) != 0)
                {
                OALMSG(OAL_WARN, (L"WARN: "
                    L"Skip bad block %d\r\n", block
                    ));
                block++;
                pFlash->badBlocksAtPos++;
                sector += pFlash->sectorsPerBlock;
                continue;
                }
            }

        // Prepare sector info location
        if (includeSectorInfo)
            {
            pSectorInfo = (SectorInfo*)(pData + offset + pFlash->sectorSize);
            }
        else
            {
            pSectorInfo = &sectorInfo;
            }

        // Read sector
        retry = 4;
        do
            {
            if (FMD_ReadSector(sector, pData + offset, pSectorInfo, 1)) break;
            }
        while (--retry > 0);

        // If read failed, exit...
        if (retry == 0)
            {
            OALMSG(OAL_ERROR, (L"ERROR: OALFlashStoreRead: "
                L"Failed read sector %d (%d sector in block %d)\r\n",
                sector, sectorInBlock, block
                ));
            goto cleanUp;
            }

        // Move to next sector
        sector++;
        sectorInBlock++;
        if (sectorInBlock  >= pFlash->sectorsPerBlock)
            {
            block++;
            pFlash->blockPos++;
            sectorInBlock = 0;
            }
        offset += sectorDataSize;
        }

    // Done
    rc = TRUE;

cleanUp:
    OALMSG(OAL_FLASH&&OAL_FUNC, (L"-OALFlashStoreRead(rc = %d)\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------

BOOL
OALFlashStoreErase(
    HANDLE hFlash,
    ULONG start,
    ULONG size,
    BOOL offsetReservedBlocks
    )
{
    BOOL rc = FALSE;
    OAL_FLASH_CONTEXT *pFlash = hFlash;
    ULONG sector, block, count, offset;
    ULONG retry;


    OALMSG(OAL_FLASH&&OAL_FUNC, (
        L"+OALFlashStoreErase(0x%08x, 0x%08x, 0x%08x, %d)\r\n",
        hFlash, start, size, offsetReservedBlocks
        ));

    // First get socket start block number
    sector = (start + pFlash->sectorSize - 1)/pFlash->sectorSize;
    block = (sector + pFlash->sectorsPerBlock - 1)/pFlash->sectorsPerBlock;

    // Offset reserved blocks
    if (offsetReservedBlocks) block += pFlash->reservedBlocks;

    // Find number of blocks to erase
    if (size == -1)
        {
        count = pFlash->blocksOnFlash - block;
        }
    else
        {
        offset = block * pFlash->sectorsPerBlock * pFlash->sectorSize - start;
        size -= offset;
        count = size/(pFlash->sectorsPerBlock * pFlash->sectorSize);
        }

    if (!SeekToBlock(pFlash, block)) goto cleanUp;

    // Compensate for bad blocks
    block += pFlash->badBlocksAtPos;
    while ((count > 0) && (block < pFlash->blocksOnFlash))
        {

        // Skip block if it is marked as bad
        if ((FMD_GetBlockStatus(block) & BLOCK_STATUS_BAD) != 0)
            {
            OALMSG(OAL_WARN, (L"WARN: OALFlashStoreErase: "
                L"Skip bad block %d\r\n", block
                ));
            block++;
            pFlash->badBlocksAtPos++;
            continue;
            }

        // Erase block
        retry = 4;
        do
            {
            if (FMD_EraseBlock(block)) break;
            }
        while (--retry > 0);

        // If erase failed
        if (retry == 0)
            {
            OALMSG(OAL_WARN, (L"WARN: OALFlashStoreErase: "
                L"Failed erase block %d, mark it as bad\r\n", block
                ));
            if (!FMD_SetBlockStatus(block, BLOCK_STATUS_BAD))
                {
                OALMSG(OAL_ERROR, (L"ERROR: OALFlashStoreErase: "
                    L"Failed set block %d status as bad\r\n", block
                    ));
                goto cleanUp;
                }
            block++;
            pFlash->badBlocksAtPos++;
            continue;
            }

        // Move to next block
        count--;
        block++;
        pFlash->blockPos++;
        }

    // Done
    rc = TRUE;

cleanUp:
    OALMSG(OAL_FLASH&&OAL_FUNC, (L"-OALFlashStoreErase(rc = %d)\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------

BOOL
OALFlashStoreBufferedRead(
    HANDLE hFlash,
    ULONG start,
    UCHAR *pData,
    ULONG size,
    BOOL offsetReservedBlocks
    )
{
    BOOL rc = FALSE;
    OAL_FLASH_CONTEXT *pFlash = hFlash;
    ULONG offset, copySize;


    OALMSG(OAL_FLASH&&OAL_FUNC, (
        L"+OALFlashStoreBufferedRead(0x%08x, 0x%08x, 0x%08x, %d)\r\n",
        hFlash, start, size, offsetReservedBlocks
        ));

    //  Calculate offset into starting block
    offset = start - (start / pFlash->sectorSize) * pFlash->sectorSize;

    //  Read data from flash without sector boundary and size restrictions
    while( size > 0 )
    {
        //  Read whole sector into temp buffer
        rc = OALFlashStoreRead(
            hFlash, start - offset, pFlash->sectorBuffer, pFlash->sectorSize,
            FALSE, offsetReservedBlocks
            );
        if (!rc)
            {
            goto cleanUp;
            }
        
        // Determine amount to copy
        copySize = (size > pFlash->sectorSize - offset) ? pFlash->sectorSize - offset : size;

        // Copy data to given address
        memcpy(
            pData, pFlash->sectorBuffer + offset, copySize
            );

        // Shift pointers
        pData += copySize;
        start += copySize;
        size  -= copySize;
        offset = 0;
    }

    // Done
    rc = TRUE;
    
cleanUp:
    OALMSG(OAL_FLASH&&OAL_FUNC, (L"-OALFlashStoreBufferedRead(rc = %d)\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------

BOOL
OALFlashStoreReadFromReservedRegion(
    HANDLE hFlash,
    LPCSTR name,
    ULONG start,
    UCHAR *pData,
    ULONG size
    )
{
    BOOL rc = FALSE;
    OAL_FLASH_CONTEXT *pFlash = hFlash;
    ULONG regionSize, offset, ix;


    // Try build layout info if there are no reserved regions yet...
    if ((pFlash->reservedRegions == 0) && !BuildLayoutInfo(hFlash))
        {
        goto cleanUp;
        }

    // Find reserved region with given name
    for (ix = 0; ix < pFlash->reservedRegions; ix++)
        {
        if (strcmp(name, pFlash->reservedRegion[ix].szName) == 0) break;
        }

    // If there isn't one we are done
    if (ix >= pFlash->reservedRegions) goto cleanUp;

    // Get reserved partition size
    regionSize =  pFlash->reservedRegion[ix].dwNumBlocks;
    regionSize *= pFlash->sectorsPerBlock * pFlash->sectorSize;

    // Check if there is enough space for request
    if ((start >= regionSize) || ((regionSize - start) < size))
        {
        goto cleanUp;
        }

    // Calculate read start
    offset  = pFlash->reservedRegion[ix].dwStartBlock;
    offset *= pFlash->sectorsPerBlock * pFlash->sectorSize;
    start += offset;

    // Read partial sector
    offset = start - (start / pFlash->sectorSize) * pFlash->sectorSize;
    if (offset > 0)
        {
        rc = OALFlashStoreRead(
            hFlash, start - offset, pFlash->sectorBuffer, pFlash->sectorSize,
            FALSE, TRUE
            );
        if (!rc)
            {
            goto cleanUp;
            }
        // Copy data
        memcpy(
            pData, pFlash->sectorBuffer + offset, pFlash->sectorSize - offset
            );
        // Shift pointers
        pData += pFlash->sectorSize - offset;
        start += pFlash->sectorSize - offset;
        size -= pFlash->sectorSize - offset;
        }

    // Calculate last partial sector
    offset = size - (size / pFlash->sectorSize) * pFlash->sectorSize;
    if (offset > 0) size -= offset;
    
    // Read data
    rc = OALFlashStoreRead(hFlash, start, pData, size, FALSE, TRUE);
    if (!rc)
        {
        goto cleanUp;
        }

    // Read partial sector
    if (offset > 0)
        {
        // Shift pointers
        pData += size;
        start += size;
        // Read last sector
        rc = OALFlashStoreRead(
            hFlash, start, pFlash->sectorBuffer, pFlash->sectorSize, FALSE, TRUE
            );
        if (!rc)
            {
            goto cleanUp;
            }
        // Copy data
        memcpy(pData, pFlash->sectorBuffer, offset);
        }

    // Done
    rc = TRUE;
    
cleanUp:
    return rc;
}

//------------------------------------------------------------------------------

BOOL
OALFlashStoreWriteToReservedRegion(
    HANDLE hFlash,
    LPCSTR name,
    ULONG start,
    UCHAR *pData,
    ULONG size
    )
{
    BOOL rc = FALSE;
    OAL_FLASH_CONTEXT *pFlash = hFlash;
    ULONG regionSize, offset, ix;


    // Start must be sector aligned
    if ((start % pFlash->sectorSize) != 0)
        {
        goto cleanUp;
        }

    // Try build layout info if there are no reserved regions yet...
    if ((pFlash->reservedRegions == 0) && !BuildLayoutInfo(hFlash))
        {
        goto cleanUp;
        }

    // Find reserved region with given name
    for (ix = 0; ix < pFlash->reservedRegions; ix++)
        {
        if (strcmp(name, pFlash->reservedRegion[ix].szName) == 0) break;
        }

    // If there isn't one we are done
    if (ix >= pFlash->reservedRegions) goto cleanUp;

    // Get reserved partition size
    regionSize =  pFlash->reservedRegion[ix].dwNumBlocks;
    regionSize *= pFlash->sectorsPerBlock * pFlash->sectorSize;

    // Check if there is enough space for request
    if ((start >= regionSize) || ((regionSize - start) < size))
        {
        goto cleanUp;
        }

    // Calculate read start
    offset  = pFlash->reservedRegion[ix].dwStartBlock;
    offset *= pFlash->sectorsPerBlock * pFlash->sectorSize;
    start += offset;

    // Calculate last partial sector
    offset = size - (size / pFlash->sectorSize) * pFlash->sectorSize;
    if (offset > 0) size -= offset;

    // Read data
    rc = OALFlashStoreWrite(hFlash, start, pData, size, FALSE, TRUE);
    if (!rc)
        {
        goto cleanUp;
        }

    // Write partial sector
    if (offset > 0)
        {
        // Shift pointers
        pData += size;
        start += size;
        // Fill buffer
        memset(pFlash->sectorBuffer, 0xFF, pFlash->sectorSize);
        memcpy(pFlash->sectorBuffer, pData, offset);
        // Write last sector
        rc = OALFlashStoreWrite(
            hFlash, start, pFlash->sectorBuffer, pFlash->sectorSize, FALSE, TRUE
            );
        if (!rc)
            {
            goto cleanUp;
            }
        }

    // Done
    rc = TRUE;

cleanUp:
    return rc;
}

//------------------------------------------------------------------------------

ULONG
OALFlashStoreBlockSize(
    HANDLE hFlash
    )
{
    OAL_FLASH_CONTEXT *pFlash = hFlash;
    return pFlash->sectorsPerBlock * pFlash->sectorSize;
}

//------------------------------------------------------------------------------

ULONG
OALFlashStoreSectorSize(
    HANDLE hFlash
    )
{
    OAL_FLASH_CONTEXT *pFlash = hFlash;
    return pFlash->sectorSize;
}

//------------------------------------------------------------------------------

VOID
OALFlashStoreClose(
    HANDLE hFlash
    )
{
    OAL_FLASH_CONTEXT *pFlash = hFlash;

    if (pFlash->pFmd != NULL)
        {
        FMD_Deinit(pFlash->pFmd);
        pFlash->pFmd = NULL;
        }
}

//------------------------------------------------------------------------------

static
BOOL
SeekToBlock(
    HANDLE hFlash,
    ULONG block
    )
{
    BOOL rc = FALSE;
    OAL_FLASH_CONTEXT *pFlash = hFlash;
    ULONG blockPos, badBlocks;

    if (block < pFlash->blockPos)
        {
        pFlash->blockPos = 0;
        pFlash->badBlocksAtPos = 0;
        }

    blockPos = pFlash->blockPos;
    badBlocks = pFlash->badBlocksAtPos;
    while (blockPos < block)
        {
        // Check if we don't run out of flash
        if ((blockPos + badBlocks) >= pFlash->blocksOnFlash)
            {
            pFlash->blockPos = blockPos;
            pFlash->badBlocksAtPos = badBlocks;
            goto cleanUp;
            }
        // If block is marked as bad, add bad blocks and try next one
        if ((FMD_GetBlockStatus(blockPos + badBlocks) & BLOCK_STATUS_BAD) != 0)
            {
            OALMSG(OAL_WARN, (L"WARN: "
                L"Skip bad block %d\r\n", blockPos + badBlocks
                ));
            badBlocks++;
            continue;
            }
        blockPos++;
        }

    // Done
    pFlash->blockPos = blockPos;
    pFlash->badBlocksAtPos = badBlocks;
    rc = TRUE;

cleanUp:
    return rc;
}

//------------------------------------------------------------------------------

static
BOOL
IsSectorEmpty(
    UCHAR *pData,
    ULONG sectorSize,
    SectorInfo *pSectorInfo
    )
{
    BOOL rc = FALSE;
    ULONG idx;

    if (pSectorInfo->dwReserved1 != 0xFFFFFFFF) goto cleanUp;
    if (pSectorInfo->wReserved2 != 0xFFFF) goto cleanUp;
    if (pSectorInfo->bOEMReserved != 0xFF) goto cleanUp;

    for (idx = 0; idx < sectorSize; idx++)
        {
        if (pData[idx] != 0xFF) goto cleanUp;
        }

    rc = TRUE;

cleanUp:
    return rc;
}

//------------------------------------------------------------------------------

static
BOOL
BuildLayoutInfo(
    HANDLE hFlash
    )
{
    BOOL rc = FALSE;
    OAL_FLASH_CONTEXT *pFlash = hFlash;
    SectorInfo sectorInfo;
    FlashLayoutSector *pSector;
    ULONG block, sector, sectorInBlock;
    ULONG mbrSector;
    ULONG ix;


    // First check if we support this flash
    if (pFlash->sectorSize > sizeof(pFlash->sectorBuffer))
        {
        goto cleanUp;
        }

    // Seek to first unreserved block
    SeekToBlock(hFlash, pFlash->reservedBlocks);

    // Parse flash memory
    block = pFlash->blockPos + pFlash->badBlocksAtPos;
    sector = block * pFlash->sectorsPerBlock;
    mbrSector = (ULONG) -1;
    sectorInBlock = 0;
    while (block < pFlash->blocksOnFlash)
        {
        // If we are at beginning of block, check if it isn't bad
        if (sectorInBlock == 0)
            {
            // Skip block if it is marked as bad
            if ((FMD_GetBlockStatus(block) & BLOCK_STATUS_BAD) != 0)
                {
                OALMSG(OAL_WARN, (L"WARN: "
                    L"Skip bad block %d\r\n", block
                    ));
                block++;
                pFlash->badBlocksAtPos++;
                sector += pFlash->sectorsPerBlock;
                continue;
                }
            }

        // Read sector
        if (!FMD_ReadSector(sector, pFlash->sectorBuffer, &sectorInfo, 1))
            {
            }

        // Did we found both (MBR + FLS)?
        if ((mbrSector != -1) && IS_VALID_FLS(pFlash->sectorBuffer)) break;

        // Check for MBR in sector...
        mbrSector = IS_VALID_BOOTSEC(pFlash->sectorBuffer) ? sector : -1;

        // Move to next sector
        sector++;
        sectorInBlock++;
        if (sectorInBlock  >= pFlash->sectorsPerBlock)
            {
            block++;
            pFlash->blockPos++;
            sectorInBlock = 0;
            }
        }

    // If we parse all flash without success, fail
    if (mbrSector == -1) goto cleanUp;

    // At this moment there is flash layout in buffer
    pSector = (FlashLayoutSector *)pFlash->sectorBuffer;

    // Get and check number of reserved regions
    pFlash->reservedRegions = pSector->cbReservedEntries/sizeof(ReservedEntry);
    if (pFlash->reservedRegions > dimof(pFlash->reservedRegion)) goto cleanUp;

    // Copy reserved regions info
    memcpy(
        pFlash->reservedRegion, (ReservedEntry*)&pSector[1],
        pFlash->reservedRegions * sizeof(ReservedEntry)
        );

    // Get and check number of regions
    pFlash->regions = pSector->cbRegionEntries/sizeof(FlashRegion);
    if (pFlash->regions > dimof(pFlash->region)) goto cleanUp;

    // Copy regions info
    memcpy(
        pFlash->region, (UCHAR *)&pSector[1] + pSector->cbReservedEntries,
        pFlash->regions * sizeof(FlashRegion)
        );

    // print flash regions to debug output
    OALMSG(OAL_LOG_INFO, (
        L"Flash contains %u reserved regions:\r\n", pFlash->reservedRegions
        ));
    for (ix = 0; ix < pFlash->reservedRegions; ix++)
        {
        OALMSG(OAL_LOG_INFO, (
            L"    Name=%s, Start block=%u, Blocks=%u\r\n",
            pFlash->reservedRegion[ix].szName,
            pFlash->reservedRegion[ix].dwStartBlock,
            pFlash->reservedRegion[ix].dwNumBlocks
            ));
        }

    OALMSG(OAL_LOG_INFO, (
        L"Flash contains %u regions:\r\n", pFlash->regions
        ));
    for (ix = 0; ix < pFlash->regions; ix++)
        {
        OALMSG(OAL_LOG_INFO, (
            L"    Type=%d, Start=0x%x, NumP=0x%x, NumL=0x%x, Sec/Blk=0x%x, "
            L"B/Blk=0x%x, Compact=%d\r\n",
            pFlash->region[ix].regionType,
            pFlash->region[ix].dwStartPhysBlock,
            pFlash->region[ix].dwNumPhysBlocks,
            pFlash->region[ix].dwNumLogicalBlocks,
            pFlash->region[ix].dwSectorsPerBlock,
            pFlash->region[ix].dwBytesPerBlock,
            pFlash->region[ix].dwCompactBlocks
            ));
        }

    rc = TRUE;

cleanUp:
    return rc;
}

//------------------------------------------------------------------------------    
    
