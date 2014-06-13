// All rights reserved ADENEO EMBEDDED 2010
//===================================================================
//
//  Module Name:    BOOTLOADER
//  
//  File Name:      filesys.h
//  
//  Description:    FAT12/16/32 file system i/o for block devices
//
//===================================================================
// Copyright (c) 2007- 2009 BSQUARE Corporation. All rights reserved.
//===================================================================

#ifndef _FILESYS_H
#define _FILESYS_H

/*

    Overview:

    This header describes the MSDOS compatible FAT12 and FAT16 file systems.

    Basic Disk Format:

    Boot Sector
    Reserved Area
    FAT
    Additional copies of FAT
    Root Directory
    File Data Area


    Boot sector describes:
        sector size, reserved size, number and size of FATs, size of root directory, cluster size
    Boot sector also describes physical information about the disk:
        track size, head count, etc.

    The File Data Area is organized into clusters (see boot sector for size)
    each cluster sectors_per_cluster number of sectors (must be a power of 2). 
    
    Each copy of the FAT is (or at least should be) identical.
    The FAT is organized with one fat entry for each cluster in the File Data Area.
    Each entry in the FAT is a pointer to another FAT entry, forming a linked list.
    The linked list structure contains no data, only pointers. The location of each 
    FAT entry corresponds to a cluster location. 
    
    The first two FAT entries are reserved.

    A directory entry for a file contains the FAT entry number for the first sector
    of the file (and therefore also indicates the first cluster of the file). The
    first FAT entry points to the next, etc., until a special FAT entry marks the 
    end of the chain. Other special FAT entries are used to indicate bad or reserved
    sectors.

*/

// typedef for disk information as it will appear in the boot sector
typedef struct _BYTE_STRUCT_2 {
    UINT8   Byte0;
    UINT8   Byte1;
} BYTE_STRUCT_2;

typedef struct _BYTE_STRUCT_4 {
    UINT8   Byte0;
    UINT8   Byte1;
    UINT8   Byte2;
    UINT8   Byte3;
} BYTE_STRUCT_4;

#define BYTE_STRUCT_2_WRITE(x, value)\
{\
    (x).Byte0 = (value) & 0xFF;\
    (x).Byte1 = ((value) & 0xFF00) >> 8;\
}
#define BYTE_STRUCT_2_READ(x)   ((x).Byte0 | (((x).Byte1) << 8))

#define BYTE_STRUCT_4_WRITE(x, value)\
{\
    (x).Byte0 = (value) & 0xFF;\
    (x).Byte1 = ((value) & 0xFF00) >> 8;\
    (x).Byte2 = ((value) & 0xFF0000) >> 16;\
    (x).Byte3 = ((value) & 0xFF000000) >> 24;\
}
#define BYTE_STRUCT_4_READ(x)   ((x).Byte0 | (((x).Byte1) << 8) | (((x).Byte2) << 16) | (((x).Byte3) << 24))

#pragma pack(1)
typedef struct _PARTITION_TABLE_ENTRY {
    UINT8   ActivePartitionFlag;    // Active partition flag 0 = inactive, 80 = active
    UINT8   PartitionStartC;        // Partition start in CHS (BIOS int13) format
    UINT8   PartitionStartH;
    UINT8   PartitionStartS;
    UINT8   PartitionType;          // Partition type byte
    UINT8   PartitionEndC;          // Partition end in CHS (BIOS int13) format
    UINT8   PartitionEndH;
    UINT8   PartitionEndS;
    UINT32  PartitionStartLBA;      // Partition start in 32 bit LBA format
    UINT32  PartitionSize;          // Partition size (32 bit)
} PARTITION_TABLE_ENTRY;

typedef struct _PARTITION_TABLE {
    UINT8                   Reserved[446];
    PARTITION_TABLE_ENTRY   Entry[4];
} PARTITION_TABLE, * PPARTITION_TABLE;
#pragma pack()

/*
typedef struct _BIOS_PARAMETER_BLOCK {
    UINT16  bytes_per_sector;
    UINT8   sectors_per_cluster;    // cluster size in sectors must be power of 2
    UINT16  reserved_sectors;
    UINT8   number_of_fats;
    UINT16  number_of_root_directory_entries;
    UINT16  total_sectors;
    UINT8   media_descriptor;
    UINT16  sectors_per_fat;
    UINT16  sectors_per_track;
    UINT16  number_of_heads;
    UINT16  number_of_hidden_sectors;
} BIOS_PARAMETER_BLOCK, * pBIOS_PARAMETER_BLOCK;

typedef struct _EXTENDED_BIOS_PARAMETER_BLOCK {
    UINT16  bytes_per_sector;
    UINT8   sectors_per_cluster;    // cluster size in sectors must be power of 2
    UINT16  reserved_sectors;
    UINT8   number_of_fats;
    UINT16  number_of_root_directory_entries;
    UINT16  total_sectors;          // if 0, use _huge entries
    UINT8   media_descriptor;
    UINT16  sectors_per_fat;
    UINT16  sectors_per_track;
    UINT16  number_of_heads;
    UINT32  number_of_hidden_sectors;
    UINT32  number_of_sectors_huge;
    //UINT8 signature;
    //UINT32    volume_id;
    //UINT8 volume_label[11];
    //UINT8 file_system_type[8];
} EXTENDED_BIOS_PARAMETER_BLOCK, * pEXTENDED_BIOS_PARAMETER_BLOCK;
*/

// Two versions of the Bios Parameter Block structure are defined.  One is packed, and
// is used by the low level functions that read the data from the disk.  The other is not
// packed and is used by high level file system routines.  This is due to a compiler bug 
// that can exist when structure elements that are two bytes wide are placed on one byte
// boundaries.  The packed structure exhibits this bug, and therefore must be accessed in a 
// byte wide mode at all times (hence the BYTE_STRUCT_x entries).  

// NOTE:  Any changes to either of these two structures must be duplicated in the other
//          structure, and the bpbCopy() function must also be updated to reflect the change.
typedef struct _FAT32_BIOS_PARAMETER_BLOCK {
    // same as standard BPB
    UINT16  bytes_per_sector;
    UINT8   sectors_per_cluster;
    UINT16  reserved_sectors;
    UINT8   number_of_fats;
    UINT16  number_of_root_directory_entries;   // ignored
    UINT16  total_sectors;
    UINT8   media_descriptor;
    UINT16  sectors_per_fat;                    // ignored, always 0, see big_sectors_per_fat...
    UINT16  sectors_per_track;
    UINT16  number_of_heads;
    UINT16  number_of_hidden_sectors;

    // new information common to EBPB and FAT32 BPB
    UINT16  number_of_hidden_sectors_high;
    UINT32  big_total_sectors;

    // new information for FAT32 BPB only
    UINT32  big_sectors_per_fat;
    UINT16  ext_flags;
    UINT16  fs_version;
    UINT32  root_dir_starting_cluster;
    UINT16  fs_info_sector;
    UINT16  backup_boot_sector;
    UINT16  reserved[6];
} BIOS_PARAMETER_BLOCK, * PBIOS_PARAMETER_BLOCK;

#pragma pack(1)
typedef struct _FAT32_BIOS_PARAMETER_BLOCK_PACKED {
    // same as standard BPB
    BYTE_STRUCT_2   bytes_per_sector;
    UINT8           sectors_per_cluster;
    BYTE_STRUCT_2   reserved_sectors;
    UINT8           number_of_fats;
    BYTE_STRUCT_2   number_of_root_directory_entries;   // ignored
    BYTE_STRUCT_2   total_sectors;
    UINT8           media_descriptor;
    BYTE_STRUCT_2   sectors_per_fat;                    // ignored, always 0, see big_sectors_per_fat...
    BYTE_STRUCT_2   sectors_per_track;
    BYTE_STRUCT_2   number_of_heads;
    BYTE_STRUCT_2   number_of_hidden_sectors;

    // new information common to EBPB and FAT32 BPB
    BYTE_STRUCT_2   number_of_hidden_sectors_high;
    BYTE_STRUCT_4   big_total_sectors;

    // new information for FAT32 BPB only
    BYTE_STRUCT_4   big_sectors_per_fat;
    BYTE_STRUCT_2   ext_flags;
    BYTE_STRUCT_2   fs_version;
    BYTE_STRUCT_4   root_dir_starting_cluster;
    BYTE_STRUCT_2   fs_info_sector;
    BYTE_STRUCT_2   backup_boot_sector;
    BYTE_STRUCT_2   reserved[6];
} BIOS_PARAMETER_BLOCK_PACKED, * PBIOS_PARAMETER_BLOCK_PACKED;
#pragma pack()

// typedef for file handle, includes sector buffer
typedef struct _FILESYS_INFO {
    BIOS_PARAMETER_BLOCK BiosParameterBlock;
    UINT32  partition_start;        // sector number of partition start w.r.t disk
    UINT32  root_directory_start;   // sector number for FAT12 and FAT16, cluster number for FAT32
    UINT32  file_data_area_start;   // sector offset w.r.t partition start
    //UINT32    number_of_hidden_sectors;
    //UINT32    total_sectors;
    //UINT32    sectors_per_fat;
    UINT32  FatType;
} FILESYS_INFO, * pFILESYS_INFO;

// defines for FAT types
#define FAT_TYPE_FAT12      1
#define FAT_TYPE_FAT16      2
#define FAT_TYPE_FAT32      3

// typedef for structure of boot sector
//
// Note: All known hard drives use 512 byte sectors

#pragma pack(1)
typedef struct _BOOT_SECTOR {
    UINT8   jump[3];
    UINT8   oemname[8];
    BIOS_PARAMETER_BLOCK_PACKED bpb;
    UINT8   bootstrap[SECTOR_SIZE - (sizeof(BIOS_PARAMETER_BLOCK_PACKED) + 3 + 8)];
} BOOT_SECTOR, *PBOOT_SECTOR;
#pragma pack()

#define MEDIA_DESCRIPTOR_FIXED_DISK 0xf8

// typedef for structure of 32 byte directory entry

typedef struct _DIRECTORY_ENTRY {
    UINT8   filename[8];
    UINT8   extension[3];
    UINT8   attribute;
    UINT8   reserved[8];
    UINT16  starting_cluster_high;      // FAT32 only
    UINT16  time;
    UINT16  date;
    UINT16  starting_cluster;   // index into FAT
    UINT32  file_size;
} DIRECTORY_ENTRY, *PDIRECTORY_ENTRY;

// FAT entry meanings 

#define FAT12_CLUSTER_AVAILABLE         0x000
#define FAT12_CLUSTER_NEXT_START        0x001
#define FAT12_CLUSTER_NEXT_END          0xfef
#define FAT12_CLUSTER_RESERVED_START    0xff0
#define FAT12_CLUSTER_RESERVED_END      0xff6
#define FAT12_CLUSTER_BAD               0xff7
#define FAT12_CLUSTER_LAST_START        0xff8
#define FAT12_CLUSTER_LAST_END          0xfff

#define FAT16_CLUSTER_AVAILABLE         0x0000
#define FAT16_CLUSTER_NEXT_START        0x0001
#define FAT16_CLUSTER_NEXT_END          0xffef
#define FAT16_CLUSTER_RESERVED_START    0xfff0
#define FAT16_CLUSTER_RESERVED_END      0xfff6
#define FAT16_CLUSTER_BAD               0xfff7
#define FAT16_CLUSTER_LAST_START        0xfff8
#define FAT16_CLUSTER_LAST_END          0xffff

// for FAT32 only bits 27:0 are used, bits 31:28 are undefined
#define FAT32_CLUSTER_MASK              0x0fffffff
#define FAT32_CLUSTER_AVAILABLE         0x00000000
#define FAT32_CLUSTER_NEXT_START        0x00000001
#define FAT32_CLUSTER_NEXT_END          0x0fffffef
#define FAT32_CLUSTER_RESERVED_START    0x0ffffff0
#define FAT32_CLUSTER_RESERVED_END      0x0ffffff6
#define FAT32_CLUSTER_BAD               0x0ffffff7
#define FAT32_CLUSTER_LAST_START        0x0ffffff8
#define FAT32_CLUSTER_LAST_END          0x0fffffff

#endif

