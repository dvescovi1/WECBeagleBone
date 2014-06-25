// All rights reserved ADENEO EMBEDDED 2010
//===================================================================
//
//  Module Name:    BOOTLOADER 
//  
//  File Name:      fileio.h
//  
//  Description:    FAT12/16/32 file system i/o for block devices
//
//===================================================================
// Copyright (c) 2007- 2009 BSQUARE Corporation. All rights reserved.
//===================================================================

#ifndef _FILEIO_H
#define _FILEIO_H

// sector size is currently fixed to 512 bytes
#define SECTOR_SIZE         512

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//
// File IO Data Structures
//
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

typedef struct _FILEHANDLE {
    char name[9];           // root of 8.3 file name, with terminating null character.
                            // This field is typically initialized with FileNameToDirEntry()
    char extension[4];      // extension of 8.3 file name, with terminating null character.
                            // This field is typically initialized with FileNameToDirEntry()
    UINT32 file_size;       // Size of the file, in bytes

    // The following variables are used internally by the fileio routines
    UINT32 current_sector;             // sector count
    UINT32 current_cluster;
    UINT16 current_sector_in_cluster;  // next sector in cluster to read
    int bytes_in_buffer;
    UINT8  buffer[SECTOR_SIZE];
} FILEHANDLE, * PFILEHANDLE;

// S_FILEIO_OPERATIONS
// This data structure contains a set of pointers to functions that provide 
// access to a block device.  This structure needs to be initialized with the
// correct functions before calling FileIoInit.  The fileio routines use
// these function pointers to access the block device.
// Note that the implementation of these functions is custom for each device.
typedef struct fileio_operations_t {
    // Pointer to an initialization function for the block device.  Function
    // takes a pointer to a data structure that describes the device.  
    // This function should initialize the device, making it ready for read
    // access.
    int (*init)(void *drive_info);

    // Pointer to a diagnostic function that can return information about the
    // block device.  This function takes a pointer to the device information
    // data structure, and a pointer to a user buffer with size equal to the 
    // sector size.  Note that the use of the user buffer is not specified.  
    // NOTE - This diagnostic function may not be called by all versions of 
    // the fileio library.
    int (*identify)(void *drive_info, void *Sector);

    // Pointer to a function that reads the specified logical sector into
    // the provided sector buffer.  Function also takes a pointer to the 
    // device specific information data structure.
    int (*read_sector)(void *drive_info, UINT32 LogicalSector, void *pSector);

    // Pointer to a function that reads the specified logical sector into
    // the provided sector buffer.  Function also takes a pointer to the 
    // device specific information data structure.
    int (*read_multi_sectors)(void *drive_info, UINT32 LogicalSector, void *pBuffer, UINT16 numSectors);

    // Pointer to a device specific data structure.  This data structure 
    // must contain whatever information is needed by other device specific
    // functions.  The device information data structure must be initialized 
    // with the correct values, and this pointer must be initialized to the 
    // data structure before calling any of the fileio routines.
    void *drive_info;

} S_FILEIO_OPERATIONS, *S_FILEIO_OPERATIONS_PTR;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//
// File IO Function Return Codes
//
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#define FILEIO_STATUS_OK            0
#define FILEIO_STATUS_INIT_FAILED   1
#define FILEIO_STATUS_OPEN_FAILED   2
#define FILEIO_STATUS_READ_FAILED   3
#define FILEIO_STATUS_READ_EOF      4

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//
// File IO Public Functions
//
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//
// NAME: FileIoInit()
//
// DESCRIPTION: Initializes fileio subsystem on any FAT12/16/32 formatted block 
// device.  
//
// PARAMETERS:
//  fileio_ops  Pointer to a preinitialized S_FILEIO_OPERATIONS structure
//
// RETURNS:     FILEIO_STATUS_OK on success, error code on failure
//
//----------------------------------------------------------------------------

int FileIoInit(S_FILEIO_OPERATIONS_PTR fileio_ops);

//----------------------------------------------------------------------------
//
// NAME: FileNameToDirEntry()
//
// DESCRIPTION: create 8+3 FAT file system directory entry strings from 8+3 
// file name.  This function must be used to create the proper filename root 
// and extension strings for use by FileIoOpen().  This function is typically
// used to initialize the 'name' and 'extension' fields of a FILEHANDLE structure.
//
// PARAMETERS:  
//  pFileName   Pointer to WCHAR string containing original file name
//  pName       Pointer to buffer that will contain resulting filename root.
//              This pointer is typically the 'name' field of a FILEHANDLE 
//              structure.
//  pExtension  Pointer to buffer that will contain resulting filename extension.
//              This pointer is typically the 'extension' field of a FILEHANDLE
//              structure.
//
// RETURNS:     Nothing
//
//----------------------------------------------------------------------------

void FileNameToDirEntry(LPCWSTR pFileName, PCHAR pName, PCHAR pExtension);

//----------------------------------------------------------------------------
//
// NAME: FileIoOpen()
//
// DESCRIPTION: Opens the specified file for sequential read access.
//
// PARAMETERS:  
//  pFileio_ops Pointer to S_FILEIO_OPERATIONS structure
//  pFile       Pointer to FILEHANDLE structure with 'name' and 'extension' 
//              fields already initialized using FileNameToDirEntry function.
//
// RETURNS:     FILEIO_STATUS_OK on success, error code on failure
//
//----------------------------------------------------------------------------

int FileIoOpen(S_FILEIO_OPERATIONS_PTR pFileio_ops, PFILEHANDLE pFile);

//----------------------------------------------------------------------------
//
// NAME: FileIoRead()
//
// DESCRIPTION: Reads specified number of bytes from file into user buffer. 
//              File read pointer is saved in the FILEHANDLE structure, subsequent
//              calls to this function will continue reading from the previous
//              location.
//
// PARAMETERS:  
//  pFileio_ops Pointer to S_FILEIO_OPERATIONS structure
//  pFile       Pointer to FILEHANDLE structure with 'name' and 'extension' 
//              fields already initialized using FileNameToDirEntry function.
//  pDest       Pointer to destination for data
//  Count       Number of bytes to read
//
// RETURNS:     FILEIO_STATUS_OK on success, error code on failure
//
//----------------------------------------------------------------------------

int FileIoRead(S_FILEIO_OPERATIONS_PTR pFileio_ops, PFILEHANDLE pFile, void *pDest, DWORD Count);

#endif
