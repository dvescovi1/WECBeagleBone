// All rights reserved ADENEO EMBEDDED 2010
//===================================================================
//
//  Module Name:    BOOTLOADER
//  
//  File Name:      fileio.c
//  
//  Description:    FAT12/16/32 file system i/o
//
//===================================================================
// Copyright (c) 2007- 2009 BSQUARE Corporation. All rights reserved.
//===================================================================
#include "omap.h"

#define ATA_STATUS_OK           0

#include "fileio.h"
#include "filesys.h"

//----------------------------------------------------------------------------
//
// defines
//
//----------------------------------------------------------------------------

#define CURRENT_SECTOR_EOF_VALUE                            0xffff

// debug message enables
#ifdef DEBUG
    #define BOOTLOADER_DEBUG_DISPLAY_PARTITION_TABLE        1
    #define BOOTLOADER_DEBUG_DISPLAY_BPB                    0
    #define BOOTLOADER_DEBUG_DISPLAY_DIRECTORY_ENTRIES      0
    #define BOOTLOADER_DEBUG_DISPLAY_CALLS                  0
    #define BOOTLOADER_DEBUG_DUMP_SECTOR_ZERO               0
    #define BOOTLOADER_DEBUG_DISPLAY_ERRORS                 1
#else
    #define BOOTLOADER_DEBUG_DISPLAY_PARTITION_TABLE        0
    #define BOOTLOADER_DEBUG_DISPLAY_BPB                    0
    #define BOOTLOADER_DEBUG_DISPLAY_DIRECTORY_ENTRIES      0
    #define BOOTLOADER_DEBUG_DISPLAY_CALLS                  0
    #define BOOTLOADER_DEBUG_DUMP_SECTOR_ZERO               0
    #define BOOTLOADER_DEBUG_DISPLAY_ERRORS                 0
#endif

// file system options
#define BOOTLOADER_SUPPORTS_FAT12                           1
#define BOOTLOADER_SUPPORTS_FAT16                           1
#define BOOTLOADER_SUPPORTS_FAT32                           1
#define BOOTLOADER_SUPPORTS_EBPB                            0

// This uses a sector buffer to speed up disk read by avoiding reads of the same sector
#define FILEIO_USE_FAT_SECTOR_BUFFER                        1

// Enables the file system type from partition table to be overridden if
// the BPB specifies that the disk has less than 4096 clusters.
#define BOOTLOADER_SUPPORTS_SPECIAL_FAT12_CHECK             1


//----------------------------------------------------------------------------
//
// private data
//
//----------------------------------------------------------------------------

static FILESYS_INFO FilesysInfo;

#if FILEIO_USE_FAT_SECTOR_BUFFER
    static UCHAR FatBuffer[SECTOR_SIZE];
    static UINT32 FatBufferSectorNumber = 0;
#endif

UINT32 ExtendedPartionBaseSector;

//----------------------------------------------------------------------------
//
// Debug Data Display function
//
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//
// Bios Parameter Block Copy function
//
//----------------------------------------------------------------------------
static void BpbCopy(PBIOS_PARAMETER_BLOCK bpb, PBIOS_PARAMETER_BLOCK_PACKED bpbPacked)
{
    memcpy(&bpb->bytes_per_sector, &bpbPacked->bytes_per_sector, sizeof(bpb->bytes_per_sector));
    memcpy(&bpb->sectors_per_cluster, &bpbPacked->sectors_per_cluster, sizeof(bpb->sectors_per_cluster));
    memcpy(&bpb->reserved_sectors, &bpbPacked->reserved_sectors, sizeof(bpb->reserved_sectors));
    memcpy(&bpb->number_of_fats, &bpbPacked->number_of_fats, sizeof(bpb->number_of_fats));
    memcpy(&bpb->number_of_root_directory_entries, &bpbPacked->number_of_root_directory_entries, sizeof(bpb->number_of_root_directory_entries));
    memcpy(&bpb->total_sectors, &bpbPacked->total_sectors, sizeof(bpb->total_sectors));
    memcpy(&bpb->media_descriptor, &bpbPacked->media_descriptor, sizeof(bpb->media_descriptor));
    memcpy(&bpb->sectors_per_fat, &bpbPacked->sectors_per_fat, sizeof(bpb->sectors_per_fat));
    memcpy(&bpb->sectors_per_track, &bpbPacked->sectors_per_track, sizeof(bpb->sectors_per_track));
    memcpy(&bpb->number_of_heads, &bpbPacked->number_of_heads, sizeof(bpb->number_of_heads));
    memcpy(&bpb->number_of_hidden_sectors, &bpbPacked->number_of_hidden_sectors, sizeof(bpb->number_of_hidden_sectors));
    memcpy(&bpb->number_of_hidden_sectors_high, &bpbPacked->number_of_hidden_sectors_high, sizeof(bpb->number_of_hidden_sectors_high));
    memcpy(&bpb->big_total_sectors, &bpbPacked->big_total_sectors, sizeof(bpb->big_total_sectors));
    memcpy(&bpb->big_sectors_per_fat, &bpbPacked->big_sectors_per_fat, sizeof(bpb->big_sectors_per_fat));
    memcpy(&bpb->ext_flags, &bpbPacked->ext_flags, sizeof(bpb->ext_flags));
    memcpy(&bpb->fs_version, &bpbPacked->fs_version, sizeof(bpb->fs_version));
    memcpy(&bpb->root_dir_starting_cluster, &bpbPacked->root_dir_starting_cluster, sizeof(bpb->root_dir_starting_cluster));
    memcpy(&bpb->fs_info_sector, &bpbPacked->fs_info_sector, sizeof(bpb->fs_info_sector));
    memcpy(&bpb->backup_boot_sector, &bpbPacked->backup_boot_sector, sizeof(bpb->backup_boot_sector));
    memcpy(&bpb->reserved, &bpbPacked->reserved, sizeof(bpb->reserved));
}

#ifdef DEBUG
    #if BOOTLOADER_DEBUG_DUMP_SECTOR_ZERO || BOOTLOADER_DEBUG_DUMP_ALL_SECTORS

        static void DumpData(unsigned char *pData, int ByteCount)
        {
            unsigned long i, j, k;
            unsigned char *pCh;
        
            i = ByteCount;
            pCh = pData;
            while (i) 
            {
                OALMSG(OAL_INFO, (L"%x ", ByteCount - i));
                k = (i < 16) ? i : 16;
                for (j = 0; j < k; j++) 
                    OALMSG(OAL_INFO, (L"%x ", pCh[j]));
                // align last line
                if (k < 16) 
                    for (j = 0; j < 16 - k; j++) 
                        OALMSG(OAL_INFO, (L"   "));
                for (j = 0; j < k; j++) 
                {
                    if ((pCh[j] < ' ') || (pCh[j] > '~')) 
                        OALMSG(OAL_INFO, (L"."));
                    else 
                        OALMSG(OAL_INFO, (L"%c", pCh[j]));
                }
                OALMSG(OAL_INFO, (L"\r\n"));
                i -= k;
                pCh += k;
            }
        }
    #endif
#endif

static BOOL ByteIsPowerOfTwo(UINT8 b)
{
    return ( b == 1 || b == 2 || b == 4 || b == 8 || b == 16 || b == 32 || b == 64 || b == 128 || b == 256 );
}


/*
 *
 *  NAME: FileIoReadNextSectors()
 *
 *  PARAMETERS: pointer to file descriptor structure
 *              pointer to sector buffer (512 bytes)
 *              number of sectors to read
 *
 *  DESCRIPTION: reads next sequential sector(s) from file
 *
 *  RETURNS: 0 on success, error code on failure
 *
 */
static int FileIoReadNextSectors (S_FILEIO_OPERATIONS_PTR pfileio_ops, PFILEHANDLE pFile, void * pBuffer, UINT16 numSectors) 
{
    UINT32 SectorNumber;
    UINT16 FatOffsetInSector, FatSectorNumber;
    #if BOOTLOADER_SUPPORTS_FAT12
        UINT16 FatOffsetInByte;
    #endif

    int status;

    if (numSectors == 0)
        return FILEIO_STATUS_OK;  // done by default

    // check if is this an attempt to read past the end of the file
    if (pFile->current_sector_in_cluster == CURRENT_SECTOR_EOF_VALUE)
    {
        #if BOOTLOADER_DEBUG_DISPLAY_CALLS
            OALMSG(OAL_INFO, (L"BOOTLOADER: FileIoReadNextSectors EOF\r\n"));
        #endif
        return FILEIO_STATUS_READ_EOF;
    }
        
    // check for invalid current_cluster
    switch (FilesysInfo.FatType)
    {
        #if BOOTLOADER_SUPPORTS_FAT12
        case FAT_TYPE_FAT12:
            if (pFile->current_cluster < FAT12_CLUSTER_NEXT_START || (pFile->current_cluster > FAT12_CLUSTER_NEXT_END && pFile->current_cluster < FAT12_CLUSTER_LAST_START))
                return FILEIO_STATUS_READ_FAILED;
            break;
        #endif

        #if BOOTLOADER_SUPPORTS_FAT16
        case FAT_TYPE_FAT16:
            if (pFile->current_cluster < FAT16_CLUSTER_NEXT_START || (pFile->current_cluster > FAT16_CLUSTER_NEXT_END && pFile->current_cluster < FAT16_CLUSTER_LAST_START))
                return FILEIO_STATUS_READ_FAILED;
            break;
        #endif

        #if BOOTLOADER_SUPPORTS_FAT32
        case FAT_TYPE_FAT32:
            if (pFile->current_cluster < FAT32_CLUSTER_NEXT_START || (pFile->current_cluster > FAT32_CLUSTER_NEXT_END && pFile->current_cluster < FAT32_CLUSTER_LAST_START))
            {
                #if BOOTLOADER_DEBUG_DISPLAY_ERRORS
                    OALMSG(OAL_INFO, (L"BOOTLOADER: FileIoReadNextSectors failed, invalide current_cluster 0x%x\r\n", pFile->current_cluster));
                #endif
                return FILEIO_STATUS_READ_FAILED;
            }
            break;
        #endif
    }

    // calculate the sector number, used to read sector later
    // file data area starting sector
    SectorNumber = FilesysInfo.file_data_area_start;
    // plus first sector of current cluster 
    // Note: first 2 fat entries are reserved, making first cluster in file area = 2
    // ??? The reserved fat entry stuff is poorly documented, this code may be wrong
    #if BOOTLOADER_DEBUG_DISPLAY_ERRORS
        if (pFile->current_cluster < 2)
            OALMSG(OAL_INFO, (L"BOOTLOADER: FileIoReadNextSectors() called with current_cluster < 2!\r\n"));
    #endif
    SectorNumber += (pFile->current_cluster - 2) * (FilesysInfo.BiosParameterBlock.sectors_per_cluster);
    // plus sector offset within cluster
    SectorNumber += pFile->current_sector_in_cluster;

    // increment current_sector_in_cluster and check if this reads the last sector in the cluster
    pFile->current_sector_in_cluster = pFile->current_sector_in_cluster + numSectors;
    if (pFile->current_sector_in_cluster >= (FilesysInfo.BiosParameterBlock.sectors_per_cluster))
    {
        // the current read reads the last sector in the current cluster, 
        // get the next cluster number from the FAT
        switch (FilesysInfo.FatType)
        {
            #if BOOTLOADER_SUPPORTS_FAT12
            case FAT_TYPE_FAT12:
                if (pFile->current_cluster >= FAT12_CLUSTER_LAST_START)
                {
                    pFile->current_sector_in_cluster = CURRENT_SECTOR_EOF_VALUE;
                }
                else
                {
                    //   FatOffsetInByte is 0 if fat entry starts on byte boundary, 1 if in mid-byte
                    //   FatOffsetInSector is a byte pointer to byte containing first part of 12 bit fat entry
                    FatOffsetInByte = (UINT16) (pFile->current_cluster & 1);
                    FatOffsetInSector = (UINT16) (((pFile->current_cluster * 3) / 2) % SECTOR_SIZE);
                    FatSectorNumber = (UINT16) (((pFile->current_cluster * 3) / 2) / SECTOR_SIZE);

                    #if FILEIO_USE_FAT_SECTOR_BUFFER
                        // if not already contained in FatBuffer, read sector containing next fat entry.
                        // FatSectorNumber is an offset from the start of the partition + reserved area
                        if (FatBufferSectorNumber != FilesysInfo.partition_start + FatSectorNumber + FilesysInfo.BiosParameterBlock.reserved_sectors)
                        {
                            FatBufferSectorNumber = FilesysInfo.partition_start + FatSectorNumber + FilesysInfo.BiosParameterBlock.reserved_sectors;
                            if (pfileio_ops->read_sector(pfileio_ops->drive_info, FatBufferSectorNumber + ExtendedPartionBaseSector, FatBuffer) != ATA_STATUS_OK)
                            {
                                #if BOOTLOADER_DEBUG_DISPLAY_ERRORS
                                    OALMSG(OAL_INFO, (L"BOOTLOADER: read_sector %u (FAT) failed\r\n", FatBufferSectorNumber + ExtendedPartionBaseSector));
                                #endif
                                FatBufferSectorNumber = 0;
                                return FILEIO_STATUS_READ_FAILED;
                            }
                        }
                        // build current fat index from two bytes (which may be in different sectors)
                        // get data from first byte of containing current FAT entry
                        if (FatOffsetInByte)
                            pFile->current_cluster = (*(((UINT8 *)FatBuffer) + FatOffsetInSector) >> 4) & 0xf;  // lower 4 bits of 12 from upper half of byte
                        else
                            pFile->current_cluster = *(((UINT8 *)FatBuffer) + FatOffsetInSector);   // lower 8 bits of 12 from entire byte
                    #else
                        // read sector containing (at least the beginning of) the next fat entry
                        if (pfileio_ops->read_sector(pfileio_ops->drive_info, (FilesysInfo.partition_start + FatSectorNumber + FilesysInfo.BiosParameterBlock.reserved_sectors) + ExtendedPartionBaseSector, pBuffer) != ATA_STATUS_OK)
                        {
                            #if BOOTLOADER_DEBUG_DISPLAY_ERRORS
                                OALMSG(OAL_INFO, (L"BOOTLOADER: read_sector %u (FAT) failed\r\n", (FilesysInfo.partition_start + FatSectorNumber + FilesysInfo.BiosParameterBlock.reserved_sectors) + ExtendedPartionBaseSector));
                            #endif
                            return FILEIO_STATUS_READ_FAILED;
                        }

                        // build current fat index from two bytes (which may be in different sectors)
                        // get data from first byte of containing current FAT entry
                        if (FatOffsetInByte)
                            pFile->current_cluster = (*(((UINT8 *)pBuffer) + FatOffsetInSector) >> 4) & 0xf;    // lower 4 bits of 12 from upper half of byte
                        else
                            pFile->current_cluster = *(((UINT8 *)pBuffer) + FatOffsetInSector); // lower 8 bits of 12 from entire byte
                    #endif

                    // get data from second byte of FAT12 entry, first check if fat entry spans sector
                    if (FatOffsetInSector == 511)
                    {
                        // second byte spans sector, read next sector, reset offset
                        #if FILEIO_USE_FAT_SECTOR_BUFFER
                            FatBufferSectorNumber = FilesysInfo.partition_start + FatSectorNumber + FilesysInfo.BiosParameterBlock.reserved_sectors + 1;
                            if (pfileio_ops->read_sector(pfileio_ops->drive_info, FatBufferSectorNumber + ExtendedPartionBaseSector, FatBuffer) != ATA_STATUS_OK)
                            {
                                #if BOOTLOADER_DEBUG_DISPLAY_ERRORS
                                    OALMSG(OAL_INFO, (L"BOOTLOADER: read_sector %u (FAT) failed\r\n", FatBufferSectorNumber + ExtendedPartionBaseSector));
                                #endif
                                FatBufferSectorNumber = 0;
                                return FILEIO_STATUS_READ_FAILED;
                            }
                        #else
                            if (pfileio_ops->read_sector(pfileio_ops->drive_info, (FilesysInfo.partition_start + FatSectorNumber + FilesysInfo.BiosParameterBlock.reserved_sectors + 1) + ExtendedPartionBaseSector, pBuffer) != ATA_STATUS_OK)
                            {
                                #if BOOTLOADER_DEBUG_DISPLAY_ERRORS
                                    OALMSG(OAL_INFO, (L"BOOTLOADER: read_sector %u (FAT) failed\r\n", (FilesysInfo.partition_start + FatSectorNumber + FilesysInfo.BiosParameterBlock.reserved_sectors + 1) + ExtendedPartionBaseSector));
                                #endif
                                return FILEIO_STATUS_READ_FAILED;
                            }
                        #endif
                        FatOffsetInSector = 0;
                    }
                    else
                    {
                        // second byte is in current sector, just increment offset
                        FatOffsetInSector += 1;
                    }

                    //  build remainder of current fat index from the next byte
                    #if FILEIO_USE_FAT_SECTOR_BUFFER
                        if (FatOffsetInByte)
                            pFile->current_cluster |= *(((UINT8 *)FatBuffer) + FatOffsetInSector) << 4;         // upper 8 of 12 from entire byte
                        else
                            pFile->current_cluster |= (*(((UINT8 *)FatBuffer) + FatOffsetInSector) & 0xf) << 8; // upper 4 of 12 from lower half of byte
                    #else
                        if (FatOffsetInByte)
                            pFile->current_cluster |= *(((UINT8 *)pBuffer) + FatOffsetInSector) << 4;           // upper 8 of 12 from entire byte
                        else
                            pFile->current_cluster |= (*(((UINT8 *)pBuffer) + FatOffsetInSector) & 0xf) << 8;   // upper 4 of 12 from lower half of byte
                    #endif
                    //  reset current_sector_in_cluster
                    pFile->current_sector_in_cluster = 0;

                    // check to see if this was the last cluster, set flag so next read returns EOF
                    if (pFile->current_cluster >= FAT12_CLUSTER_LAST_START)
                    {
                        pFile->current_sector_in_cluster = CURRENT_SECTOR_EOF_VALUE;
                    }
                }
                break;
            #endif

            #if BOOTLOADER_SUPPORTS_FAT16
            case FAT_TYPE_FAT16:
                // check to see if this was the last cluster, set flag so next read returns EOF
                if (pFile->current_cluster >= FAT16_CLUSTER_LAST_START)
                {
                    pFile->current_sector_in_cluster = CURRENT_SECTOR_EOF_VALUE;
                }
                else
                {           
                    FatOffsetInSector = (UINT16) ((pFile->current_cluster * 2) % SECTOR_SIZE);
                    FatSectorNumber = (UINT16) ((pFile->current_cluster * 2) / SECTOR_SIZE);

                    #if FILEIO_USE_FAT_SECTOR_BUFFER
                        // if not already contained in FatBuffer,
                        // read sector containing next fat entry, FatSectorNumber is an
                        // offset from the start of the partition + reserved area
                        if (FatBufferSectorNumber != FilesysInfo.partition_start + FatSectorNumber + FilesysInfo.BiosParameterBlock.reserved_sectors)
                        {
                            FatBufferSectorNumber = FilesysInfo.partition_start + FatSectorNumber + FilesysInfo.BiosParameterBlock.reserved_sectors;
                            if (pfileio_ops->read_sector(pfileio_ops->drive_info, FatBufferSectorNumber + ExtendedPartionBaseSector, FatBuffer) != ATA_STATUS_OK)
                            {
                                #if BOOTLOADER_DEBUG_DISPLAY_ERRORS
                                    OALMSG(OAL_INFO, (L"BOOTLOADER: read_sector %u (FAT) failed\r\n", FatBufferSectorNumber + ExtendedPartionBaseSector));
                                #endif
                                FatBufferSectorNumber = 0;
                                return FILEIO_STATUS_READ_FAILED;
                            }
                        }
                        //  update current fat index
                        pFile->current_cluster = *(((UINT16 *)FatBuffer) + (FatOffsetInSector/2));
                    #else
                        // read sector containing next fat entry, FatSectorNumber is an
                        // offset from the start of the partition + reserved area
                        if (pfileio_ops->read_sector(pfileio_ops->drive_info, (FilesysInfo.partition_start + FatSectorNumber + FilesysInfo.BiosParameterBlock.reserved_sectors) + ExtendedPartionBaseSector, pBuffer) != ATA_STATUS_OK)
                        {
                            #if BOOTLOADER_DEBUG_DISPLAY_ERRORS
                                OALMSG(OAL_INFO, (L"BOOTLOADER: read_sector %u (FAT) failed\r\n", (FilesysInfo.partition_start + FatSectorNumber + FilesysInfo.BiosParameterBlock.reserved_sectors) + ExtendedPartionBaseSector));
                            #endif
                            return FILEIO_STATUS_READ_FAILED;
                        }
                        //  update current fat index
                        pFile->current_cluster = *(((UINT16 *)pBuffer) + (FatOffsetInSector/2));
                    #endif
                    //  reset current_sector_in_cluster
                    pFile->current_sector_in_cluster = 0;

                    // check to see if this was the last cluster, set flag so next read returns EOF
                    if (pFile->current_cluster >= FAT16_CLUSTER_LAST_START)
                    {
                        pFile->current_sector_in_cluster = CURRENT_SECTOR_EOF_VALUE;
                    }
                }
                break;

            #endif

            #if BOOTLOADER_SUPPORTS_FAT32
            case FAT_TYPE_FAT32:
                // check to see if this was the last cluster, set flag so next read returns EOF
                if (pFile->current_cluster >= FAT32_CLUSTER_LAST_START)
                {
                    pFile->current_sector_in_cluster = CURRENT_SECTOR_EOF_VALUE;
                }
                else
                {           
                    FatOffsetInSector = (UINT16) ((pFile->current_cluster * 4) % SECTOR_SIZE);
                    FatSectorNumber = (UINT16) ((pFile->current_cluster * 4) / SECTOR_SIZE);

                    #if FILEIO_USE_FAT_SECTOR_BUFFER
                        // if not already contained in FatBuffer,
                        // read sector containing next fat entry, FatSectorNumber is an
                        // offset from the start of the partition + reserved area
                        if (FatBufferSectorNumber != FilesysInfo.partition_start + FatSectorNumber + FilesysInfo.BiosParameterBlock.reserved_sectors)
                        {
                            FatBufferSectorNumber = FilesysInfo.partition_start + FatSectorNumber + FilesysInfo.BiosParameterBlock.reserved_sectors;
                            if (pfileio_ops->read_sector(pfileio_ops->drive_info, FatBufferSectorNumber + ExtendedPartionBaseSector, FatBuffer) != ATA_STATUS_OK)
                            {
                                #if BOOTLOADER_DEBUG_DISPLAY_ERRORS
                                    OALMSG(OAL_INFO, 
                                        (L"BOOTLOADER: FileIoReadNextSectors failed reading FAT: LBA 0x%x (cluster 0x%x, sector 0x%x)\r\n",
                                        FatBufferSectorNumber + ExtendedPartionBaseSector, 
                                        pFile->current_cluster, 
                                        pFile->current_sector_in_cluster)
                                          );
                                #endif
                                FatBufferSectorNumber = 0;
                                return FILEIO_STATUS_READ_FAILED;
                            }
                        }
                        //  update current fat index
                        pFile->current_cluster = (*(((UINT32 *)FatBuffer) + (FatOffsetInSector/4))) & FAT32_CLUSTER_MASK;
                    #else
                        // read sector containing next fat entry, FatSectorNumber is an
                        // offset from the start of the partition + reserved area
                        if (pfileio_ops->read_sector(pfileio_ops->drive_info, 
                                                     (FilesysInfo.partition_start + FatSectorNumber + 
                                                      FilesysInfo.BiosParameterBlock.reserved_sectors) + 
                                                      ExtendedPartionBaseSector, 
                                                     pBuffer
                                                    )
                                       != ATA_STATUS_OK
                           )
                        {
                            #if BOOTLOADER_DEBUG_DISPLAY_ERRORS
                                OALMSG(OAL_INFO, (L"BOOTLOADER: read_sector %u (FAT) failed\r\n", 
                                         (FilesysInfo.partition_start + FatSectorNumber + 
                                          FilesysInfo.BiosParameterBlock.reserved_sectors) + 
                                          ExtendedPartionBaseSector)
                                      );
                            #endif
                            return FILEIO_STATUS_READ_FAILED;
                        }

                        //  update current fat index
                        pFile->current_cluster = *(((UINT32 *)pBuffer) + (FatOffsetInSector/4)) & FAT32_CLUSTER_MASK;
                    #endif
                    //  reset current_sector_in_cluster
                    pFile->current_sector_in_cluster = 0;

                    if (pFile->current_cluster >= FAT32_CLUSTER_LAST_START)
                    {
                        // no more clusters, signal end of file or directory
                        pFile->current_sector_in_cluster = CURRENT_SECTOR_EOF_VALUE;
                    }
                }
                break;
            #endif
        }
    }

    if (numSectors > 1)
    {
        status = pfileio_ops->read_multi_sectors(
                             pfileio_ops->drive_info, 
                             (FilesysInfo.partition_start + SectorNumber) + 
                                 ExtendedPartionBaseSector, 
                             pBuffer,
                             numSectors);
    }
    else
    {
        status = pfileio_ops->read_sector(
                             pfileio_ops->drive_info, 
                             (FilesysInfo.partition_start + SectorNumber) + 
                                 ExtendedPartionBaseSector, 
                             pBuffer);
    }

    // read the requested sector
    if (status != ATA_STATUS_OK)
    {
        #if BOOTLOADER_DEBUG_DISPLAY_ERRORS
            OALMSG(OAL_INFO, 
              (L"BL: FileIoReadNextSectors failed reading: LBA 0x%x, cluster 0x%x, sec in cluster 0x%x, num sec %d\r\n", 
               (FilesysInfo.partition_start + SectorNumber) + ExtendedPartionBaseSector, 
               pFile->current_cluster - 2, 
               pFile->current_sector_in_cluster,
               numSectors
              ));
        #endif
        return FILEIO_STATUS_READ_FAILED;
    }

    // update sector count
    pFile->current_sector += numSectors;
    
    return FILEIO_STATUS_OK;
} // FileIoReadNextSectors  


//----------------------------------------------------------------------------
//
// public functions
//
//----------------------------------------------------------------------------

// create 8+3 FAT file system directory entry strings from pFileName
void FileNameToDirEntry(LPCWSTR pFileName, PCHAR pName, PCHAR pExtension)
{
    int i, j;

    // fill name and extension with blanks
    strcpy(pName, "        ");
    strcpy(pExtension, "   ");

    //OALMSG(1, (L"FileNameToDirEntry: \""));

    // copy name
    for (i = 0; i < 8; i++)
    {
        if (pFileName[i] && pFileName[i] != L'.')
        {
            pName[i] = (CHAR) toupper((CHAR)(pFileName[i]));
            //OALMSG(1, (L"%c", (CHAR)(pName[i])));
        }
        else
            break;
    }

    // check for extension
    if (pFileName[i] == L'.')
    {
        // skip period
        i++;
        // copy extension
        for (j = 0; j < 3; j++)
        {
            if (pFileName[i])
            {
                pExtension[j] = (CHAR)toupper((BYTE)(pFileName[i++]));
                //OALMSG(1, (L"%c", (CHAR)(pExtension[j])));
            }
            else
                break;
        }
    }

    //OALMSG(1, (L"\"\r\n"));
}

/*
 *
 *  NAME: FileIoInit()
 *
 *  PARAMETERS: pointer to base of ATA disk drive registers
 *
 *  DESCRIPTION: initializes fileio subsystem
 *
 *  RETURNS: 0 on success, error code on failure
 *
 */
int FileIoInit(S_FILEIO_OPERATIONS_PTR pfileio_ops) 
{
    UINT16 Sector[SECTOR_SIZE/2];
    UINT8 * pSector = (UINT8 *)Sector;
    int status;
    int i;
    UINT32 boot_sector = 0;
    UCHAR partition_type = 0;
    int ExtendedPartitionDepth = 0;
    int PartitionTableEntryCount = 4;
        
    // initialize device driver
    #if BOOTLOADER_DEBUG_DISPLAY_CALLS
        OALMSG(OAL_INFO, (L"BOOTLOADER: FileIoInit() calling init.\r\n"));
    #endif

    // assume no extented partion, no offset
    ExtendedPartionBaseSector = 0;

    if ((status = pfileio_ops->init(pfileio_ops->drive_info)) != ATA_STATUS_OK)
        return FILEIO_STATUS_INIT_FAILED;

    // the identify drive command is optional, used only to display information for debugging
    //if (pfileio_ops->identify && pfileio_ops->identify(pfileio_ops->drive_info, pSector) != ATA_STATUS_OK)
    //  return FILEIO_STATUS_INIT_FAILED;
    if (pfileio_ops->identify)
        pfileio_ops->identify(pfileio_ops->drive_info, pSector);

    #if BOOTLOADER_DEBUG_DISPLAY_CALLS
        OALMSG(OAL_INFO, (L"BOOTLOADER: FileIoInit() calling read_sector to get partition table\r\n"));
    #endif

CheckPartitionTable:

    // read in candidate partition table sector
    if (pfileio_ops->read_sector(pfileio_ops->drive_info, boot_sector + ExtendedPartionBaseSector, pSector) != ATA_STATUS_OK)
    {
        #if BOOTLOADER_DEBUG_DISPLAY_ERRORS
            OALMSG(OAL_INFO, (L"BOOTLOADER: read_sector %u (PT) failed\r\n", boot_sector + ExtendedPartionBaseSector));
        #endif
        return FILEIO_STATUS_INIT_FAILED;
    }

    #if BOOTLOADER_DEBUG_DUMP_SECTOR_ZERO || BOOTLOADER_DEBUG_DUMP_ALL_SECTORS
        DumpData((unsigned char *)pSector, 512);
    #endif
        
    // check for valid BPB (Bios Parameter Block) in sector zero (old DOS disk organization, no partition table)
    //if ( (*pSector == 0xe9 || *pSector == 0xeb) && (((PBOOT_SECTOR)pSector)->bpb.bytes_per_sector == 512) )
    //if ( (*pSector == 0xe9 || *pSector == 0xeb) && ((BYTE_STRUCT_2_READ(((PBOOT_SECTOR)pSector)->bpb.bytes_per_sector)) == 512) )
    if (   
        (*pSector == 0xe9 || *pSector == 0xeb) &&
        ((BYTE_STRUCT_2_READ(((PBOOT_SECTOR)pSector)->bpb.bytes_per_sector)) == 512) &&
        ByteIsPowerOfTwo(((PBOOT_SECTOR)pSector)->bpb.sectors_per_cluster) &&
        (((PBOOT_SECTOR)pSector)->bpb.media_descriptor == 0xf8)
    )
    {
        boot_sector = 0;

        if ( (BYTE_STRUCT_2_READ(((PBOOT_SECTOR)pSector)->bpb.sectors_per_fat)) == 0)
        {
            // assume FAT32 for now...
            FilesysInfo.FatType = FAT_TYPE_FAT32;
            #if BOOTLOADER_DEBUG_DISPLAY_BPB
                OALMSG(OAL_INFO, (L"BOOTLOADER: Found BPB in sector zero, assuming FAT32 with no partition table\r\n"));
            #endif
        }
        else
        {
            // assume FAT16 for now...
            FilesysInfo.FatType = FAT_TYPE_FAT16;
            #if BOOTLOADER_DEBUG_DISPLAY_BPB
                OALMSG(OAL_INFO, (L"BOOTLOADER: Found BPB in sector zero, assuming FAT16 with no partition table\r\n"));
            #endif
        }
        goto NoPartitionTable;
    }

    // assume that sector 0 contains a partition table
    #if BOOTLOADER_DEBUG_DISPLAY_PARTITION_TABLE
        for (i = 0; i < PartitionTableEntryCount; i++)
        {
            OALMSG(OAL_INFO, (L"BOOTLOADER: PartitionTable[%d] Flag:0x%x, Type:0x%x, Start:0x%X, Size:0x%X\r\n", 
                i,
                ((PPARTITION_TABLE)pSector)->Entry[i].ActivePartitionFlag, 
                ((PPARTITION_TABLE)pSector)->Entry[i].PartitionType, 
                ((PPARTITION_TABLE)pSector)->Entry[i].PartitionStartLBA, 
                ((PPARTITION_TABLE)pSector)->Entry[i].PartitionSize));
        }
    #endif

    // search the partition table for the active partition
    for (i = 0; i < PartitionTableEntryCount; i++)
    {
        if (((PPARTITION_TABLE)pSector)->Entry[i].ActivePartitionFlag == 0x80 || ((PPARTITION_TABLE)pSector)->Entry[i].PartitionType != 0x00)
        {
            boot_sector = ((PPARTITION_TABLE)pSector)->Entry[i].PartitionStartLBA;
            partition_type = ((PPARTITION_TABLE)pSector)->Entry[i].PartitionType;
            break;
        }
    }
    if (i == PartitionTableEntryCount)
    {
        #if BOOTLOADER_DEBUG_DISPLAY_CALLS
            OALMSG(OAL_INFO, (L"BOOTLOADER: FileIoInit() no active partition found\r\n"));
        #endif

        return FILEIO_STATUS_INIT_FAILED;
    }

    #if BOOTLOADER_DEBUG_DISPLAY_CALLS
        OALMSG(OAL_INFO, (L"BOOTLOADER: trying partition %d\r\n", i));
    #endif

    // check partition type
    switch (partition_type)
    {
        #if BOOTLOADER_SUPPORTS_FAT12
        case 0x01:
            #if BOOTLOADER_DEBUG_DISPLAY_BPB
                OALMSG(OAL_INFO, (L"BOOTLOADER: Active partition type is FAT12\r\n"));
            #endif
            FilesysInfo.FatType = FAT_TYPE_FAT12;
            break;
        #endif
        
        #if BOOTLOADER_SUPPORTS_FAT16
        case 0x04:
        case 0x06:
        case 0x0e:
            #if BOOTLOADER_DEBUG_DISPLAY_BPB
                OALMSG(OAL_INFO, (L"BOOTLOADER: Active partition type is FAT16\r\n"));
            #endif
            FilesysInfo.FatType = FAT_TYPE_FAT16;
            break;
        #endif

        #if BOOTLOADER_SUPPORTS_FAT32
        case 0x0b:
        case 0x0c:
            #if BOOTLOADER_DEBUG_DISPLAY_BPB
                OALMSG(OAL_INFO, (L"BOOTLOADER: Active partition type is FAT32\r\n"));
            #endif
            FilesysInfo.FatType = FAT_TYPE_FAT32;
            break;
        #endif

        case 0x05:
        case 0x0f:
            #if BOOTLOADER_DEBUG_DISPLAY_PARTITION_TABLE
                OALMSG(OAL_INFO, (L"BOOTLOADER: Active partition type is EXTENDED DOS, reading extended partition table\r\n"));
            #endif
            ExtendedPartitionDepth++;
            PartitionTableEntryCount = 2;
            ExtendedPartionBaseSector = boot_sector;
            // sector addressing now is relative to partition table sector
            boot_sector = 0;
            goto CheckPartitionTable;
            break;

        default:
            #if BOOTLOADER_DEBUG_DISPLAY_BPB
                OALMSG(OAL_INFO, (L"BOOTLOADER: Active partition type is not supported (0x%x)\r\n", partition_type));
            #endif
            return FILEIO_STATUS_INIT_FAILED;
            break;
    }                   

    #if BOOTLOADER_DEBUG_DISPLAY_CALLS
        OALMSG(OAL_INFO, (L"BOOTLOADER: FileIoInit() calling read_sector to read MBR for active partition\r\n"));
    #endif

    // read in the master boot record (MBR), fill in the BPB
    if (pfileio_ops->read_sector(pfileio_ops->drive_info, boot_sector + ExtendedPartionBaseSector, pSector) != ATA_STATUS_OK)
    {
        #if BOOTLOADER_DEBUG_DISPLAY_ERRORS
            OALMSG(OAL_INFO, (L"BOOTLOADER: read_sector %u (MBR) failed\r\n", boot_sector + ExtendedPartionBaseSector));
        #endif
        return FILEIO_STATUS_INIT_FAILED;
    }
    
NoPartitionTable:

    #if BOOTLOADER_DEBUG_DISPLAY_CALLS
        OALMSG(OAL_INFO, (L"BOOTLOADER: FileIoInit() copying BPB into FilesysInfo structure.\r\n"));
    #endif
    // copy bpb info from sector buffer to bpb
    //FilesysInfo.BiosParameterBlock = ((BOOT_SECTOR *)pSector)->bpb;
    BpbCopy(&FilesysInfo.BiosParameterBlock, &((BOOT_SECTOR *)pSector)->bpb);

    #if BOOTLOADER_DEBUG_DISPLAY_BPB
        OALMSG(OAL_INFO, (L"BOOTLOADER: boot sector BPB for valid partition\r\n"));
        OALMSG(OAL_INFO, (L"BOOTLOADER: sector size:     %X\r\n", FilesysInfo.BiosParameterBlock.bytes_per_sector));
        OALMSG(OAL_INFO, (L"BOOTLOADER: sec/cluster:     %X\r\n", FilesysInfo.BiosParameterBlock.sectors_per_cluster));
        OALMSG(OAL_INFO, (L"BOOTLOADER: rsvd sectors:    %X\r\n", FilesysInfo.BiosParameterBlock.reserved_sectors));
        OALMSG(OAL_INFO, (L"BOOTLOADER: # FATs:          %X\r\n", FilesysInfo.BiosParameterBlock.number_of_fats));
        OALMSG(OAL_INFO, (L"BOOTLOADER: media descipt:   %X\r\n", FilesysInfo.BiosParameterBlock.media_descriptor));
        OALMSG(OAL_INFO, (L"BOOTLOADER: sec/track        %X\r\n", FilesysInfo.BiosParameterBlock.sectors_per_track));
        OALMSG(OAL_INFO, (L"BOOTLOADER: # heads          %X\r\n", FilesysInfo.BiosParameterBlock.number_of_heads));

        #if BOOTLOADER_SUPPORTS_FAT32
        if (FilesysInfo.FatType != FAT_TYPE_FAT32)
        {
        #endif
            OALMSG(OAL_INFO, (L"BOOTLOADER: # root dir ent:  %X\r\n", FilesysInfo.BiosParameterBlock.number_of_root_directory_entries));
            OALMSG(OAL_INFO, (L"BOOTLOADER: sec/fat:         %X\r\n", FilesysInfo.BiosParameterBlock.sectors_per_fat));

            #if BOOTLOADER_SUPPORTS_EBPB || BOOTLOADER_SUPPORTS_FAT32
            if (FilesysInfo.BiosParameterBlock.total_sectors == 0)
            {
                OALMSG(OAL_INFO, (L"BOOTLOADER: total sectors:   %X\r\n", FilesysInfo.BiosParameterBlock.big_total_sectors));
                OALMSG(OAL_INFO, (L"BOOTLOADER: # hidden sec:    %X\r\n", (UINT32) FilesysInfo.BiosParameterBlock.number_of_hidden_sectors_high << 16 || (UINT32) FilesysInfo.BiosParameterBlock.number_of_hidden_sectors));
            }
            else
            #endif
            {
                OALMSG(OAL_INFO, (L"BOOTLOADER: total sectors:   %X\r\n", FilesysInfo.BiosParameterBlock.total_sectors));
                OALMSG(OAL_INFO, (L"BOOTLOADER: # hidden sec:    %X\r\n", FilesysInfo.BiosParameterBlock.number_of_hidden_sectors));
            }
        #if BOOTLOADER_SUPPORTS_FAT32
        }
        else
        {
            OALMSG(OAL_INFO, (L"BOOTLOADER: root dir str cl: %X\r\n", FilesysInfo.BiosParameterBlock.root_dir_starting_cluster));
            OALMSG(OAL_INFO, (L"BOOTLOADER: b# total sectors:%X\r\n", FilesysInfo.BiosParameterBlock.big_total_sectors));
            OALMSG(OAL_INFO, (L"BOOTLOADER: b# sec/fat:      %X\r\n", FilesysInfo.BiosParameterBlock.big_sectors_per_fat));
            OALMSG(OAL_INFO, (L"BOOTLOADER: b# hidden sec:   %X\r\n", (UINT32) FilesysInfo.BiosParameterBlock.number_of_hidden_sectors_high << 16 || (UINT32) FilesysInfo.BiosParameterBlock.number_of_hidden_sectors));
        }
        #endif
    #endif

    #if BOOTLOADER_SUPPORTS_SPECIAL_FAT12_CHECK
        // switch to FAT12 if total number of clusters is too small (under 4096)
        
        // divide by zero check
        if (FilesysInfo.BiosParameterBlock.sectors_per_cluster == 0)
        {
            #if BOOTLOADER_DEBUG_DISPLAY_BPB
                OALMSG(OAL_INFO, (L"BOOTLOADER: ERROR - sectors_per_cluster is 0!!\r\n"));
            #endif
            return FILEIO_STATUS_INIT_FAILED;
        }

        // check for partition type override due to small number of sectors
        if (FilesysInfo.BiosParameterBlock.total_sectors == 0)
        {
            if ((FilesysInfo.BiosParameterBlock.big_total_sectors / FilesysInfo.BiosParameterBlock.sectors_per_cluster) <= 4087)
            {
                #if BOOTLOADER_DEBUG_DISPLAY_BPB
                    OALMSG(OAL_INFO, (L"BOOTLOADER: Disk has under 4096 clusters, switching to FAT12\r\n"));
                #endif
                FilesysInfo.FatType = FAT_TYPE_FAT12;
            }
        }
        else            
        {
            if ((FilesysInfo.BiosParameterBlock.total_sectors / FilesysInfo.BiosParameterBlock.sectors_per_cluster) <= 4087)
            {
                #if BOOTLOADER_DEBUG_DISPLAY_BPB
                    OALMSG(OAL_INFO, (L"BOOTLOADER: Disk has under 4096 clusters, switching to FAT12\r\n"));
                #endif
                FilesysInfo.FatType = FAT_TYPE_FAT12;
            }
        }

    #endif

    // sanity check - sector size
    if (FilesysInfo.BiosParameterBlock.bytes_per_sector != SECTOR_SIZE)
    {
        return FILEIO_STATUS_INIT_FAILED;
    }

    #if BOOTLOADER_DEBUG_DISPLAY_CALLS
        OALMSG(OAL_INFO, (L"BOOTLOADER: FileIoInit() initializing data structures.\r\n"));
    #endif
    FilesysInfo.partition_start = boot_sector;

    switch (FilesysInfo.FatType)
    {
        #if BOOTLOADER_SUPPORTS_FAT12 || BOOTLOADER_SUPPORTS_FAT16
        case FAT_TYPE_FAT12:
        case FAT_TYPE_FAT16:
            // compute root directory starting sector
            FilesysInfo.root_directory_start = 
                FilesysInfo.BiosParameterBlock.reserved_sectors + 
                (FilesysInfo.BiosParameterBlock.number_of_fats * FilesysInfo.BiosParameterBlock.sectors_per_fat);

            // compute file data area starting sector
            FilesysInfo.file_data_area_start = 
                FilesysInfo.root_directory_start + 
                (FilesysInfo.BiosParameterBlock.number_of_root_directory_entries / (SECTOR_SIZE/sizeof(DIRECTORY_ENTRY)) );

            //FilesysInfo.number_of_hidden_sectors = FilesysInfo.BiosParameterBlock.number_of_hidden_sectors;
            //FilesysInfo.total_sectors = FilesysInfo.BiosParameterBlock.total_sectors;
            //FilesysInfo.sectors_per_fat = FilesysInfo.BiosParameterBlock.sectors_per_fat;

            break;
        #endif
        
        #if BOOTLOADER_SUPPORTS_FAT32
        case FAT_TYPE_FAT32:
            // compute file data area starting sector (first sector after FATs and reserved sectors)
            FilesysInfo.file_data_area_start = 
                FilesysInfo.BiosParameterBlock.reserved_sectors + 
               (FilesysInfo.BiosParameterBlock.number_of_fats * FilesysInfo.BiosParameterBlock.big_sectors_per_fat);

            // save root directory starting cluster
            FilesysInfo.root_directory_start = FilesysInfo.BiosParameterBlock.root_dir_starting_cluster;

            break;
        #endif
    }

    return FILEIO_STATUS_OK;
}

/*
 *
 *  NAME: FileIoOpen()
 *
 *  PARAMETERS: pointer to FILEHANDLE structure, with name and extension fields filled in
 *
 *  DESCRIPTION: opens the file for sequential read
 *
 *  RETURNS: 0 on success, error code on failure
 *
 */
int FileIoOpen(S_FILEIO_OPERATIONS_PTR pfileio_ops, PFILEHANDLE pFile) 
{
    UINT32 sector_number = 0;
    int status;
    int entry;
    int i;
    UINT16 DirEntryCount = 0;
    void * pSector = &pFile->buffer;

    switch (FilesysInfo.FatType)
    {
        #if BOOTLOADER_SUPPORTS_FAT12 || BOOTLOADER_SUPPORTS_FAT16
        case FAT_TYPE_FAT12:
        case FAT_TYPE_FAT16:
            // starting sector of directory
            sector_number = FilesysInfo.root_directory_start;

            // read directory sector
            if ((status = pfileio_ops->read_sector(pfileio_ops->drive_info, (FilesysInfo.partition_start + sector_number) + ExtendedPartionBaseSector, pSector)) != ATA_STATUS_OK)
            {
                #if BOOTLOADER_DEBUG_DISPLAY_ERRORS
                    OALMSG(OAL_INFO, (L"BOOTLOADER: read_sector %u (DIR) failed\r\n", (FilesysInfo.partition_start + sector_number) + ExtendedPartionBaseSector));
                #endif
                return FILEIO_STATUS_OPEN_FAILED;
            }

            break;
        #endif

        #if BOOTLOADER_SUPPORTS_FAT32
        case FAT_TYPE_FAT32:
            // setup to read first directory sector
            pFile->current_cluster = FilesysInfo.root_directory_start;
            pFile->current_sector_in_cluster = 0;
            pFile->current_sector = 0;
            pFile->bytes_in_buffer = 0;

            if ((status = FileIoReadNextSectors(pfileio_ops, pFile, pSector, 1)) != FILEIO_STATUS_OK)
                return FILEIO_STATUS_OPEN_FAILED;
            
            break;
        #endif
    }

    // scan the entire root directory looking for the file
    for ( ; ; )
    {
        // check each directory entry in the sector
        #if BOOTLOADER_SUPPORTS_FAT32
        for (entry = 0; (entry < SECTOR_SIZE/sizeof(DIRECTORY_ENTRY)) && (FilesysInfo.FatType == FAT_TYPE_FAT32 ? 1 : (DirEntryCount < FilesysInfo.BiosParameterBlock.number_of_root_directory_entries)); entry++)
        #else
        for (entry = 0; (entry < SECTOR_SIZE/sizeof(DIRECTORY_ENTRY)) && (DirEntryCount < FilesysInfo.BiosParameterBlock.number_of_root_directory_entries); entry++)
        #endif      
        {
            UINT8 * pName = (((DIRECTORY_ENTRY *)pSector) + entry)->filename;
            UINT8 Attribute = (((DIRECTORY_ENTRY *)pSector) + entry)->attribute;
            
            // bump count of total directory entries scanned
            DirEntryCount++;

            // skip long file names, subdirectories and volume lable
            if (/* Attribute == 0x0f || */ (Attribute & 0x18))
                continue;

            // skip special directory entries
            if (pName[0] == 0x00 || pName[0] == 0xe5)
                continue;
            
            if (pName[0] == 0x05)
                pName[0] = 0xe5;
                
            #if BOOTLOADER_DEBUG_DISPLAY_DIRECTORY_ENTRIES
                OALMSG(OAL_INFO, (L"  Directory Entry #%d: <%c%c%c%c%c%c%c%c %c%c%c> ", DirEntryCount, pName[0], pName[1], pName[2], pName[3], pName[4], pName[5], pName[6], pName[7], pExtension[0], pExtension[1], pExtension[2]));
            #endif

            // compare name
            for (i = 0; i < 8; i++)
            {
                if ((((DIRECTORY_ENTRY *)pSector) + entry)->filename[i] != pFile->name[i])
                    break;
            }
            if (i == 8)
            {
                // compare extension
                for (i = 0; i < 3; i++) 
                {
                    if ((((DIRECTORY_ENTRY *)pSector) + entry)->extension[i] != pFile->extension[i])
                        break;
                }
                if (i == 3) 
                {
                    // save starting cluster information
                    
                    #if BOOTLOADER_SUPPORTS_FAT32
                    if (FilesysInfo.FatType == FAT_TYPE_FAT32)
                        pFile->current_cluster = ((UINT32)((((DIRECTORY_ENTRY *)pSector) + entry)->starting_cluster)) | (((UINT32)((((DIRECTORY_ENTRY *)pSector) + entry)->starting_cluster_high)) << 16);
                    else
                    #endif
                        pFile->current_cluster = (((DIRECTORY_ENTRY *)pSector) + entry)->starting_cluster;
                    pFile->current_sector_in_cluster = 0;
                    pFile->file_size = (((DIRECTORY_ENTRY *)pSector) + entry)->file_size;
                    // initialize other stuff in File handle
                    pFile->current_sector = 0;
                    pFile->bytes_in_buffer = 0;
                    #if BOOTLOADER_DEBUG_DISPLAY_DIRECTORY_ENTRIES
                        OALMSG(OAL_INFO, (L"Found file, starting cluster = %x, file size = %X\r\n", pFile->current_cluster, pFile->file_size));
                    #endif
                    return FILEIO_STATUS_OK;
                }
            }
            #if BOOTLOADER_DEBUG_DISPLAY_DIRECTORY_ENTRIES
                OALMSG(OAL_INFO, (L"\r\n"));
            #endif
        }                

        switch (FilesysInfo.FatType)
        {
            #if BOOTLOADER_SUPPORTS_FAT12 || BOOTLOADER_SUPPORTS_FAT16
            case FAT_TYPE_FAT12:
            case FAT_TYPE_FAT16:
                // point to next sector
                sector_number++;

                #if BOOTLOADER_DEBUG_DISPLAY_DIRECTORY_ENTRIES
                    OALMSG(OAL_INFO, (L"Next directory sector %d, file data area start %d\r\n", sector_number, FilesysInfo.file_data_area_start));
                #endif

                // check for end of directory
                if (sector_number >= FilesysInfo.file_data_area_start)
                    return FILEIO_STATUS_OPEN_FAILED;

                // read next directory sector
                if ((status = pfileio_ops->read_sector(pfileio_ops->drive_info, (FilesysInfo.partition_start + sector_number) + ExtendedPartionBaseSector, pSector)) != ATA_STATUS_OK)
                {
                    #if BOOTLOADER_DEBUG_DISPLAY_ERRORS
                        OALMSG(OAL_INFO, (L"BOOTLOADER: read_sector %u (next DIR) failed\r\n", (FilesysInfo.partition_start + sector_number) + ExtendedPartionBaseSector));
                    #endif
                    return FILEIO_STATUS_OPEN_FAILED;
                }

                break;
            #endif

            #if BOOTLOADER_SUPPORTS_FAT32
            case FAT_TYPE_FAT32:
                // try to read the next directory sector
                if ((status = FileIoReadNextSectors(pfileio_ops, pFile, pSector, 1)) != FILEIO_STATUS_OK)
                    return FILEIO_STATUS_OPEN_FAILED;

                break;
            #endif
        }
    }
}

/*
 *
 *  NAME: FileIoRead()
 *
 *  pFileio_ops Pointer to S_FILEIO_OPERATIONS structure
 *  pFile       Pointer to FILEHANDLE structure with 'name' and 'extension' 
 *              fields already initialized using FileNameToDirEntry function.
 *  pDest       Pointer to destination for data
 *  Count       Number of bytes to read
 *
 *  DESCRIPTION: reads next sequential sector from file
 *
 *  RETURNS: 0 on success, error code on failure
 *
 */
int FileIoRead(S_FILEIO_OPERATIONS_PTR pfileio_ops, PFILEHANDLE pFile, UINT8 * pDest, DWORD Count)
{
    int status;
    UINT8 * s;

    UINT32 numSectorsToRead = 0;
            
    while (Count)
    {
        // create pointer to start of valid data in buffer
        s = pFile->buffer + (SECTOR_SIZE - pFile->bytes_in_buffer); 

        // if any bytes are in the buffer copy them to destination
        while (Count && pFile->bytes_in_buffer)
        {
            *pDest = *s;
            pDest++;
            s++;
            Count--;
            pFile->bytes_in_buffer--;
        }

        // Note: After the above while loop, the read is sector aligned

        // if Count is at least one sector size then read sector directly into caller's buffer
        while (Count >= SECTOR_SIZE)
        {
            if (Count < 2*SECTOR_SIZE)
            {
                // can only read SINGLE sector
                if ((status = FileIoReadNextSectors(pfileio_ops, pFile, pDest, 1)) != FILEIO_STATUS_OK)
                    return status;
    
                pDest += SECTOR_SIZE;
                Count -= SECTOR_SIZE;
    
                break;
            }

            // Condition for multi sector read is met 
            // read sectors in the cluster
            numSectorsToRead = Count/SECTOR_SIZE;  // number of sectors to read

            // make sure numSectorsToRead is within the current cluster's limit
            // number of sectors remain in cluster to be read
            if (numSectorsToRead + pFile->current_sector_in_cluster > FilesysInfo.BiosParameterBlock.sectors_per_cluster)
            {
                // TODO: what if this is the last cluster of the file.
                //       in that case, the cluster may be partially filled.
                //Read all the remaining sectors in the cluster
                numSectorsToRead = (FilesysInfo.BiosParameterBlock.sectors_per_cluster - pFile->current_sector_in_cluster);
            }

            status = FileIoReadNextSectors(pfileio_ops, pFile, pDest, (UINT16)numSectorsToRead);
            if (status != FILEIO_STATUS_OK)
                return status;

            pDest += (SECTOR_SIZE * numSectorsToRead);
            Count -= (SECTOR_SIZE * numSectorsToRead);
        }

        // if Count is not zero, read a sector into the file structure sector buffer
        if (Count)
        {
            if ((status = FileIoReadNextSectors(pfileio_ops, pFile, pFile->buffer, 1)) != FILEIO_STATUS_OK)
                return status;
            pFile->bytes_in_buffer = SECTOR_SIZE;
        }
    }
    return FILEIO_STATUS_OK;
}

