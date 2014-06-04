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
//  File: fmd.c
//
//  This file implements NAND flash media PDD.
//
#include "omap.h"
#include "bsp_cfg.h"
#include "omap_gpmc_regs.h"
#include "omap_prcm_regs.h"
#include <ceddkex.h>
#pragma warning(push)
#pragma warning(disable: 4115)
#include <fmd.h>
#pragma warning(pop)
#include "nand.h"
#include "gpmc_ecc.h"
#include "soc_cfg.h"
#include "oalex.h"

//-----------------------------------------------------------------------------
#define FIFO_THRESHOLD          (64)            // pre-fetch fifo config

extern UCHAR g_ecctype;
//-----------------------------------------------------------------------------
// typedefs and enums
typedef enum {
    kPrefetchOff,
    kPrefetchRead,
    kPrefetchWrite,
} PrefetchMode_e;

//-----------------------------------------------------------------------------
// nand access definitions
#define WRITE_NAND(x,y)         OUTREG16(x,y)    // 16-bit access
#define READ_NAND(x)            INREG16(x)       // 16-bit access
typedef REG16 NANDREG;

typedef struct {
    CRITICAL_SECTION        cs;
    BOOL                    bNandLockMode;
    PrefetchMode_e          prefetchMode;
    NANDREG                 *pNandCmd;
    NANDREG                 *pNandAddress;
    NANDREG                 *pNandData;
    NANDREG                 *pFifo;
    OMAP_GPMC_REGS          *pGpmcRegs;
    DWORD                   memBase[2];
    DWORD                   memLen[2];
    DWORD                   timeout;
    NAND_INFO               nandInfo;
    DWORD                   ECCCfg;
    DWORD                   IrqWait;
    EccType_e               ECCtype;
    DWORD                   ECCsize; /* number of bytes */
} NandDevice_t;


//------------------------------------------------------------------------------
//  NAND Spare Area Format for x16 devices

/* 16 bit access, large page nand */
typedef struct
{
    UCHAR  hwBadBlock[2];           // Hardware bad block flag
    UCHAR  ecc[ECC_BYTES];         //BootROM expect ECC starts from the 3rd byte when bus width is 16
    UCHAR reserved1[4];         // Reserved - used by FAL
    UCHAR reserved2[2];         // Reserved - used by FAL
    UCHAR swBadBlock;           // Software bad block flag
    UCHAR oemReserved;          // For use by OEM
    UCHAR unused[2];           // Unused
}NAND_SPARE_AREA;



//-----------------------------------------------------------------------------
// global variables
// Note: global variable prevents to have twio instance of the driver loaded at the same time
static NandDevice_t s_Device;
static HANDLE s_hNand = NULL;


//------------------------------------------------------------------------------
//  Device registry parameters
#ifdef DEVICE_MODE
static const DEVICE_REGISTRY_PARAM g_deviceRegParams[] = {
    {
        L"MemBase", PARAM_MULTIDWORD, TRUE, offset(NandDevice_t, memBase),
        fieldsize(NandDevice_t, memBase), NULL
    }, {
        L"MemLen", PARAM_MULTIDWORD, TRUE, offset(NandDevice_t, memLen),
        fieldsize(NandDevice_t, memLen), NULL
    }, {
        L"Timeout", PARAM_DWORD, FALSE, offset(NandDevice_t, timeout),
        fieldsize(NandDevice_t, timeout), (VOID*)5000
    },{
        L"ECCtype", PARAM_DWORD, FALSE, offset(NandDevice_t, ECCtype),
        fieldsize(NandDevice_t, ECCtype), (VOID*)0
    }
};
#endif

//-----------------------------------------------------------------------------
static BOOL
NAND_LockBlocks(
    HANDLE hNand,
    UINT blockStart,
    UINT blockEnd,
    BOOL bLock
    );

//-----------------------------------------------------------------------------
static void
NAND_Uninitialize(
    HANDLE hNand
    );

//-----------------------------------------------------------------------------
static HANDLE 
NAND_Initialize(
    LPCTSTR szContext,
    PCI_REG_INFO *pRegIn,
    PCI_REG_INFO *pRegOut
    );

//-----------------------------------------------------------------------------
static BOOL
NAND_Seek(
    HANDLE hNand,
    SECTOR_ADDR sector,
    UINT offset
    );

//-----------------------------------------------------------------------------
static BOOL
NAND_Read(
    HANDLE hNand,
    BYTE *pData,
    int size,
    BYTE *pEcc
    );

//-----------------------------------------------------------------------------
static BOOL
NAND_Write(
    HANDLE hNand,
    BYTE *pData,
    int size,
    BYTE *pEcc
    );

//-----------------------------------------------------------------------------
static UINT16
NAND_GetStatus(
    HANDLE hNand
    );

//-----------------------------------------------------------------------------
static BOOL 
NAND_EraseBlock(
    HANDLE hNand,
    BLOCK_ID blockId
    );

//-----------------------------------------------------------------------------
static BOOL
NAND_Enable(
    HANDLE hNand,
    BOOL bEnable
    );

//-----------------------------------------------------------------------------
static UINT
NAND_MutexEnter(
    HANDLE hNand
    );

//-----------------------------------------------------------------------------
static UINT
NAND_MutexExit(
    HANDLE hNand
    );

//-----------------------------------------------------------------------------
static BOOL
NAND_SendCommand(
    HANDLE hNand,
    UINT cmd
    );

//-----------------------------------------------------------------------------
static BOOL
NAND_ConfigurePrefetch(
    HANDLE hNand,
    UINT accessType
    );


//-----------------------------------------------------------------------------
__inline
void
SectorAccess(
    NandDevice_t* pDevice, 
    SECTOR_ADDR sector,
    UINT offset
    )
{
    
    // Offset is provided to this function in bytes; NAND device requires words
    offset = offset / 2;
    WRITE_NAND(pDevice->pNandAddress, (offset & 0xFF));
    WRITE_NAND(pDevice->pNandAddress, ((offset >> 8) & 0xFF));
    WRITE_NAND(pDevice->pNandAddress, (sector & 0xFF));
    WRITE_NAND(pDevice->pNandAddress, ((sector >> 8) & 0xFF));
    WRITE_NAND(pDevice->pNandAddress, ((sector >> 16) & 0xFF));
}
       
//-----------------------------------------------------------------------------
__inline
void
BlockAccess(
    NandDevice_t* pDevice,
    BLOCK_ID blockId
    )
{
    blockId *= pDevice->nandInfo.sectorsPerBlock;
    WRITE_NAND(pDevice->pNandAddress, (blockId & 0xFF));
    WRITE_NAND(pDevice->pNandAddress, ((blockId >> 8) & 0xFF));
    WRITE_NAND(pDevice->pNandAddress, ((blockId >> 16) & 0xFF));
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
__inline
void
WaitOnEmptyWriteBufferStatus(
    NandDevice_t *pDevice
    )
{
    UINT status;
    do
    {
        status = INREG32(&pDevice->pGpmcRegs->GPMC_STATUS);
    } while ((status & GPMC_STATUS_EMPTYWRITEBUFFER) == 0);
}

//-----------------------------------------------------------------------------
BOOL 
InitializePointers(
    LPCTSTR szContext,
    NandDevice_t *pDevice 
    )
{
#ifdef BOOT_MODE
    UNREFERENCED_PARAMETER(szContext);
    UNREFERENCED_PARAMETER(pDevice);
#else
    PHYSICAL_ADDRESS pa;
    InitializeCriticalSection(&pDevice->cs);

    // Read device parameters
    if (GetDeviceRegistryParams(
            szContext, pDevice, dimof(g_deviceRegParams), g_deviceRegParams
            ) != ERROR_SUCCESS)
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: FMD_Init: "
            L"Failed read FMD registry parameters\r\n"
            ));
        return FALSE;
        }

    pa.QuadPart = pDevice->memBase[0];
    pDevice->pGpmcRegs = MmMapIoSpace(pa, pDevice->memLen[0], FALSE);
    if (pDevice->pGpmcRegs == NULL)
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: FMD_Init: "
            L"Failed map FMD registers (0x%08x/0x%08x)\r\n",
            pDevice->memBase[0], pDevice->memLen[0]
            ));
        return FALSE;
        }

    pa.QuadPart = pDevice->memBase[1];
    pDevice->pFifo = (NANDREG*)MmMapIoSpace(pa, pDevice->memLen[1], FALSE);
    if (pDevice->pFifo == NULL)
        {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: FMD_Init: "
            L"Failed map FMD registers (0x%08x/0x%08x)\r\n",
            pDevice->memBase[0], pDevice->memLen[0]
            ));
        return FALSE;
        }
#endif
     return TRUE;
}

//-----------------------------------------------------------------------------
__inline
OMAP_GPMC_REGS*
NAND_GetGpmcRegs(
    HANDLE hNand
    )
{
    return ((NandDevice_t*)hNand)->pGpmcRegs;
}


//-----------------------------------------------------------------------------
__inline
NAND_INFO const*
NAND_GetGeometry(
    HANDLE hNand
    )
{
    return &(((NandDevice_t*)hNand)->nandInfo);
}

//-----------------------------------------------------------------------------
BOOL
NAND_LockBlocks(
    HANDLE hNand,
    UINT blockStart,
    UINT blockEnd,
    BOOL bLock
    )
{
    BOOL rc = FALSE; 
    NandDevice_t *pDevice = (NandDevice_t*)hNand;

    UNREFERENCED_PARAMETER(blockEnd);
    UNREFERENCED_PARAMETER(blockStart);

    if (pDevice == NULL) goto cleanUp;

    if (bLock)
        {
        CLRREG32(&pDevice->pGpmcRegs->GPMC_CONFIG, GPMC_CONFIG_WRITEPROTECT);        
        }
    else
        {
        SETREG32(&pDevice->pGpmcRegs->GPMC_CONFIG, GPMC_CONFIG_WRITEPROTECT);
        }
    rc = TRUE;

cleanUp:
    return rc;
}

//-----------------------------------------------------------------------------
void
NAND_Uninitialize(
    HANDLE hNand
    )
{
#ifdef BOOT_MODE
    UNREFERENCED_PARAMETER(hNand);
#else
    NandDevice_t *pDevice = (NandDevice_t*)hNand;

    if (pDevice->pGpmcRegs != NULL)
        {
        MmUnmapIoSpace((void*)pDevice->pGpmcRegs, pDevice->memLen[0]);
        }

    if (pDevice->pFifo != NULL)
        {
        MmUnmapIoSpace((void*)pDevice->pFifo, pDevice->memLen[1]);
        }
#endif
}

//-----------------------------------------------------------------------------
HANDLE 
NAND_Initialize(
    LPCTSTR szContext,
    PCI_REG_INFO *pRegIn,
    PCI_REG_INFO *pRegOut
    )
{
    DWORD chipSelect = BSPGetNandCS();
    const NAND_INFO *  pBSPNandInfo;
    HANDLE hDevice = NULL;
    UINT ffPrefetchMode = 0;
    UINT8 manufacturer, device;
    NandDevice_t *pDevice = &s_Device;
#ifndef BOOT_MODE    
    DWORD dwKernelRet;
#endif

    UNREFERENCED_PARAMETER(pRegOut);
    UNREFERENCED_PARAMETER(szContext);
    // initialize structure
    memset(pDevice, 0, sizeof(NandDevice_t));

#ifdef BOOT_MODE    
    pDevice->pGpmcRegs = (OMAP_GPMC_REGS*)OALPAtoUA(SOCGetGPMCAddress(0));
    pDevice->pFifo = (NANDREG*)OALPAtoUA(pRegIn->MemBase.Reg[0]);

    /* Get ECC mode from BootCfg */
    pDevice->ECCtype = g_ecctype;
    if((pDevice->ECCtype > BCH8bit) || (pDevice->ECCtype < Hamming1bit))
    	{
            pDevice->ECCtype = Hamming1bit;
            RETAILMSG(TRUE, (L"Incorrect ECC type setting\r\n"));			
    	}
#else    
    if (szContext != NULL)
        {
        if (InitializePointers(szContext, pDevice) == FALSE) goto cleanUp;
        }
    else
        {
        PHYSICAL_ADDRESS pa;
        
        // if there's not context string then use global macros

        pa.QuadPart = pRegIn->MemBase.Reg[0];
        pDevice->memLen[0] = pRegIn->MemLen.Reg[0];
        pDevice->pGpmcRegs = MmMapIoSpace(pa, pDevice->memLen[0], FALSE);
        if (pDevice->pGpmcRegs == NULL) goto cleanUp;

        pa.QuadPart = pRegIn->MemBase.Reg[1];
        pDevice->memLen[1] = pRegIn->MemLen.Reg[1];
        pDevice->pFifo = MmMapIoSpace(pa, pDevice->memLen[1], FALSE);
        if (pDevice->pGpmcRegs == NULL) goto cleanUp;

        }


        if (!KernelIoControl(IOCTL_HAL_GET_ECC_TYPE,
                             NULL, 0, &pDevice->ECCtype, sizeof(DWORD), &dwKernelRet))
        {
            RETAILMSG( TRUE,(TEXT("Failed to read Ecc type\r\n")));
            pDevice->ECCtype = Hamming1bit;
        }   
	
        RETAILMSG(TRUE, (L"ECC TYPE is %s\r\n", (pDevice->ECCtype==Hamming1bit)? L"Hamming 1 bit" :
			                                                    (pDevice->ECCtype==BCH4bit)? L"BCH 4 bit" : L"BCH 8 bit"));
	
#endif

    pDevice->pNandCmd = (NANDREG*)((UINT32)pDevice->pGpmcRegs + offset(OMAP_GPMC_REGS, GPMC_NAND_COMMAND_0) + (0x30 * chipSelect));
    pDevice->pNandAddress= (NANDREG*)((UINT32)pDevice->pGpmcRegs + offset(OMAP_GPMC_REGS, GPMC_NAND_ADDRESS_0) + (0x30 * chipSelect));
    pDevice->pNandData= (NANDREG*)((UINT32)pDevice->pGpmcRegs + offset(OMAP_GPMC_REGS, GPMC_NAND_DATA_0) + (0x30 * chipSelect));
    // Enable GPMC wait-to-nowait edge detection mechanism on NAND R/B pin
    NAND_Enable(pDevice, TRUE);

    // Write RESET command
    // (a reset aborts any current READ, WRITE (PROGRAM) or ERASE operation)
    NAND_SendCommand(pDevice, NAND_CMD_RESET);

    // Wait for NAND
    while ((NAND_GetStatus(pDevice) & NAND_STATUS_READY) == 0);

    // Send Read ID Command
    NAND_SendCommand(pDevice, NAND_CMD_READID);

    // Send Address 00h
    WRITE_NAND(pDevice->pNandAddress, 0);

    // Read the manufacturer ID & device code
    manufacturer = (UINT8)READ_NAND(pDevice->pNandData);
    device = (UINT8)READ_NAND(pDevice->pNandData);

 
    if ((pBSPNandInfo = BSPGetNandInfo(manufacturer,device))==NULL)
    {                
        goto cleanUp;
    }
    if ((pBSPNandInfo->sectorSize != 2048) && (pBSPNandInfo->wordData != 2))
    {
        ERRORMSG(1,(TEXT("FMD driver supports only 16bits large page (2KB) devices\r\n")));
        goto cleanUp;
    }
    pDevice->nandInfo = *pBSPNandInfo;

    pDevice->IrqWait = BSPGetNandIrqWait();

    /* ECCCfg: 16bit bus width, cs0, 4 - 512 bytes blocks per page */
    pDevice->ECCCfg = (GPMC_ECC_CONFIG_16BIT | (chipSelect << 1) | (0x3<<4)); 
    pDevice->ECCsize = (pDevice->ECCtype == Hamming1bit ) ? ECC_BYTES_HAMMING : 
		                      (pDevice->ECCtype == BCH4bit ) ? ECC_BYTES_BCH4 : ECC_BYTES_BCH8; 
							  
    //  Enable and reset ECC engine (workaround for engine giving 0s first time)
    ECC_Init(pDevice->pGpmcRegs, pDevice->ECCCfg, pDevice->ECCtype, NAND_ECC_READ);
    ECC_Reset(pDevice->pGpmcRegs);

    //  Only enable during NAND read/write/erase operations
    NAND_Enable(pDevice, FALSE);

    // configure the prefetch engine
    pDevice->prefetchMode = kPrefetchOff;
    OUTREG32(&pDevice->pGpmcRegs->GPMC_PREFETCH_CONTROL, 0);

    // set prefetch mask
    ffPrefetchMode = GPMC_PREFETCH_CONFIG_SYNCHROMODE |
                     GPMC_PREFETCH_CONFIG_PFPWENROUNDROBIN |
                     GPMC_PREFETCH_CONFIG_ENABLEOPTIMIZEDACCESS |
                     GPMC_PREFETCH_CONFIG_WAITPINSELECTOR(chipSelect) |
                     GPMC_PREFETCH_CONFIG_FIFOTHRESHOLD(FIFO_THRESHOLD) |
                     GPMC_PREFETCH_CONFIG_ENGINECSSELECTOR(chipSelect);

    OUTREG32(&pDevice->pGpmcRegs->GPMC_PREFETCH_CONFIG1, ffPrefetchMode);

    // configure prefetch engine
    OUTREG32(&pDevice->pGpmcRegs->GPMC_PREFETCH_CONFIG2, 
        pBSPNandInfo->sectorSize
        );
        
    SETREG32(&pDevice->pGpmcRegs->GPMC_PREFETCH_CONFIG1, 
        GPMC_PREFETCH_CONFIG_ENABLEENGINE
        );

    // We are done
    hDevice = pDevice;

cleanUp:
    return hDevice;
}

//-----------------------------------------------------------------------------
BOOL
NAND_Seek(
    HANDLE hNand,
    SECTOR_ADDR sector,
    UINT offset
    )
{
    // seek to address
    NandDevice_t *pDevice = (NandDevice_t*)hNand;
    SectorAccess(pDevice, sector, offset);
    return TRUE;
}

//-----------------------------------------------------------------------------
BOOL
NAND_Read(
    HANDLE hNand,
    BYTE *pData,
    int size,
    BYTE *pEcc
    )
{
    UINT32 fifoLevel;
    NandDevice_t *pDevice = (NandDevice_t*)hNand;

    // Start ECC if a valid ECC buffer is passed in
    if (pEcc != NULL)
        {
        ECC_Init(pDevice->pGpmcRegs, pDevice->ECCCfg, pDevice->ECCtype, NAND_ECC_READ);
        }

    // enable prefetch if it's been properly configured
    if (pDevice->prefetchMode == kPrefetchRead )
        {
        SETREG32(&pDevice->pGpmcRegs->GPMC_PREFETCH_CONTROL, 
            GPMC_PREFETCH_CONTROL_STARTENGINE
            );

        // start copying data into passed in buffer
        while (size > 0)
            {
            // wait for fifo threshold to be reached
            fifoLevel = 0;
            while (fifoLevel < FIFO_THRESHOLD)
                {
                fifoLevel = INREG32(&pDevice->pGpmcRegs->GPMC_PREFETCH_STATUS);
                fifoLevel &= GPMC_PREFETCH_STATUS_FIFOMASK;
                fifoLevel >>= GPMC_PREFETCH_STATUS_FIFOSHIFT;
                }

            // copy data to buffer
            memcpy(pData, (BYTE*)pDevice->pFifo, FIFO_THRESHOLD);
            pData += FIFO_THRESHOLD;
            size -= FIFO_THRESHOLD;        
            }

        // NOTE:
        //  Prefetch engine will automatically stop on the completion
        // of data transfer
        pDevice->prefetchMode = kPrefetchOff;
        }
    else
        {
        // NOTE:
        //  Code assumes the entire page is read at once
        while (size >= sizeof(NANDREG))
            {
            *(NANDREG*)pData = READ_NAND(pDevice->pNandData);
            pData += sizeof(NANDREG);
            size -= sizeof(NANDREG);

	     /* workaround for BCH engine when ECC is not put at the end of OOB area.  
		  the checking is based on puting ECC at the BootROM expected location -
		  with 2 bytes offset from the start of the OOB area */
            if(pDevice->ECCtype == BCH4bit || pDevice->ECCtype == BCH8bit)
                if (size == (int)(sizeof(NAND_SPARE_AREA) - ECC_OFFSET - pDevice->ECCsize))
        	         ECC_Reset(pDevice->pGpmcRegs);  
                }
        }

    // get ECC result
    if (pEcc != NULL)
        {
        // UNDONE:
        //  should pass in sector size???
        ECC_Result(pDevice->pGpmcRegs, pEcc, pDevice->ECCsize);
        }
    else
    	{
    	ECC_Reset(pDevice->pGpmcRegs);
    	}
    return TRUE;
}


//-----------------------------------------------------------------------------
BOOL
NAND_Write(
    HANDLE hNand,
    BYTE *pData,
    int size,
    BYTE *pEcc
    )
{
    UINT32 fifoLevel;
    NandDevice_t *pDevice = (NandDevice_t*)hNand;

    // Start ECC if a valid ECC buffer is passed in
    if (pEcc != NULL)
        {
        ECC_Init(pDevice->pGpmcRegs, pDevice->ECCCfg, pDevice->ECCtype, NAND_ECC_WRITE);
        }

    // enable prefetch if it's been properly configured
    if (pDevice->prefetchMode == kPrefetchWrite )
        {
        SETREG32(&pDevice->pGpmcRegs->GPMC_PREFETCH_CONTROL, 
            GPMC_PREFETCH_CONTROL_STARTENGINE
            );
        // start copying data into passed in buffer
        while (size > 0)
            {
            // copy data to buffer
            memcpy((BYTE*)pDevice->pFifo, pData, FIFO_THRESHOLD);
            pData += FIFO_THRESHOLD;
            size -= FIFO_THRESHOLD;  
            
            // wait for fifo threshold to be reached
            fifoLevel = 0;
            while (fifoLevel < FIFO_THRESHOLD)
                {
                fifoLevel = INREG32(&pDevice->pGpmcRegs->GPMC_PREFETCH_STATUS);
                fifoLevel &= GPMC_PREFETCH_STATUS_FIFOMASK;
                fifoLevel >>= GPMC_PREFETCH_STATUS_FIFOSHIFT;
                }
            }

        // NOTE:
        //  Prefetch engine will automatically stop on the completion
        // of data transfer
        pDevice->prefetchMode = kPrefetchOff;
        }
    else
        {
        int writeCount = 0;
		
        while (size >= sizeof(NANDREG))
            {
            WRITE_NAND(pDevice->pNandData, *(NANDREG*)pData);

            // Workaround Errata 1.53
            // need to check on EMPTYWRITEBUFFERSTATUS on every
            // 255 bytes
            if (++writeCount >= 255)
                {
                WaitOnEmptyWriteBufferStatus(pDevice);
                writeCount = 0;
                }
            
            pData += sizeof(NANDREG);
            size -= sizeof(NANDREG);
            }
		
        }

    // get ECC result
    if (pEcc != NULL)
        {
        ECC_Result(pDevice->pGpmcRegs, pEcc, pDevice->ECCsize);
        }
    else
    	{
	 ECC_Reset(pDevice->pGpmcRegs);
    	}
    return TRUE;
}

//-----------------------------------------------------------------------------
UINT16
NAND_GetStatus(
    HANDLE hNand
    )
{
    NandDevice_t *pDevice = (NandDevice_t*)hNand;

    // request status
    WRITE_NAND(pDevice->pNandCmd, NAND_CMD_STATUS);
    return READ_NAND(pDevice->pNandData);
}

//-----------------------------------------------------------------------------
BOOL 
NAND_EraseBlock(
    HANDLE hNand,
    BLOCK_ID blockId
    )
{    
    NandDevice_t *pDevice = (NandDevice_t*)hNand;
    
    // Calculate the sector number
    NAND_SendCommand(hNand, NAND_CMD_ERASE_SETUP);    
    BlockAccess(pDevice, blockId);
    NAND_SendCommand(hNand, NAND_CMD_ERASE_CONFIRM);
    
    return TRUE;
}

//-----------------------------------------------------------------------------
BOOL
NAND_Enable(
            HANDLE hNand,
            BOOL bEnable
            )
{
    NandDevice_t *pDevice = (NandDevice_t*)hNand;

    if (pDevice->IrqWait)
    {
        if( bEnable )
        {
            // Enable GPMC wait-to-nowait edge detection mechanism on NAND R/B pin
            SETREG32 (&pDevice->pGpmcRegs->GPMC_IRQENABLE, pDevice->IrqWait);

            //  Reset IRQ status
            SETREG32 (&pDevice->pGpmcRegs->GPMC_IRQSTATUS, pDevice->IrqWait);
        }
        else
        {
            //  Reset IRQ status
            SETREG32 (&pDevice->pGpmcRegs->GPMC_IRQSTATUS, pDevice->IrqWait);

            // Disable GPMC wait-to-nowait edge detection mechanism on NAND R/B pin
            CLRREG32 (&pDevice->pGpmcRegs->GPMC_IRQENABLE, pDevice->IrqWait);
        }
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
UINT
NAND_MutexEnter(
    HANDLE hNand
    )
{   
#ifdef DEVICE_MODE
    NandDevice_t *pDevice = (NandDevice_t*)hNand;
    EnterCriticalSection(&pDevice->cs); 
    return pDevice->cs.LockCount;
#else
    UNREFERENCED_PARAMETER(hNand);
    return 0;
#endif
}

//-----------------------------------------------------------------------------
UINT
NAND_MutexExit(
    HANDLE hNand
    )
{
#ifdef DEVICE_MODE
    NandDevice_t *pDevice = (NandDevice_t*)hNand;
    LeaveCriticalSection(&pDevice->cs); 
    return pDevice->cs.LockCount;
#else
    UNREFERENCED_PARAMETER(hNand);
    return 0;
#endif
}

//-----------------------------------------------------------------------------
BOOL
NAND_SendCommand(
    HANDLE hNand,
    UINT cmd
    )
{
    NandDevice_t *pDevice = (NandDevice_t*)hNand;
    WRITE_NAND(pDevice->pNandCmd, cmd);
    return TRUE;
}

//-----------------------------------------------------------------------------
BOOL
NAND_ConfigurePrefetch(
    HANDLE hNand,
    UINT accessType
    )
{
    DWORD chipSelect = BSPGetNandCS();
    UINT ffPrefetchMode = 0;
    NandDevice_t *pDevice = (NandDevice_t*)hNand;

    // disable prefetch engine
    pDevice->prefetchMode = kPrefetchOff;
    
    OUTREG32(&pDevice->pGpmcRegs->GPMC_PREFETCH_CONTROL, 0);

    // set prefetch mask
    ffPrefetchMode = GPMC_PREFETCH_CONFIG_PFPWENROUNDROBIN |
                     GPMC_PREFETCH_CONFIG_ENABLEOPTIMIZEDACCESS |
                     GPMC_PREFETCH_CONFIG_WAITPINSELECTOR(chipSelect) |
                     GPMC_PREFETCH_CONFIG_FIFOTHRESHOLD(FIFO_THRESHOLD) |
                     GPMC_PREFETCH_CONFIG_ENGINECSSELECTOR(chipSelect);

    if (accessType == NAND_DATA_WRITE)
        {
        pDevice->prefetchMode = kPrefetchWrite;
        ffPrefetchMode |= GPMC_PREFETCH_CONFIG_WRITEPOST;
        }
    else
        {
        pDevice->prefetchMode = kPrefetchRead;
        }
        
    OUTREG32(&pDevice->pGpmcRegs->GPMC_PREFETCH_CONFIG1, ffPrefetchMode);

    SETREG32(&pDevice->pGpmcRegs->GPMC_PREFETCH_CONFIG1, 
        GPMC_PREFETCH_CONFIG_ENABLEENGINE
        );

    return TRUE;
}


//-----------------------------------------------------------------------------
BOOL
NAND_CorrectEccData(
    HANDLE hNand,
    BYTE *pData,
    UINT size,
    BYTE const *pEccOld,          // Pointer to the ECC on flash
    BYTE const *pEccNew           // Pointer to the ECC the caller calculated
    )
{
    BOOL rc = FALSE;
    NandDevice_t *pDevice = (NandDevice_t*)hNand;
    
    // this call assumes the array size of pEccOld and pEccNew are of the 
    // correct size to hold all the parity bits of the given size    
    if (memcmp(pEccOld, pEccNew, pDevice->ECCsize) != 0)
        {
        
        // check if data is correctable        
        if (ECC_CorrectData(pDevice->pGpmcRegs, pData, size, pEccOld, pEccNew) == FALSE)
            {
            goto cleanUp;
            }
        }    

    rc = TRUE;
    
cleanUp:
    return rc;
}

//------------------------------------------------------------------------------
// Waits until the NAND status reads "ready"
// Note : a timout could be added but shouldn't be required
//
__inline 
void
WaitForReadyStatus(
    HANDLE hNand
    )
{
    //  Wait for a Ready status
    while ((NAND_GetStatus(hNand) & NAND_STATUS_READY) == 0); 
}

//------------------------------------------------------------------------------
//
//  Function:  FMD_Init
//
//  This function is called to initialize flash subsystem.
//
VOID*
FMD_Init(
    LPCTSTR szContext,
    PCI_REG_INFO *pRegIn,
    PCI_REG_INFO *pRegOut
    )
{
    HANDLE hRet = NULL;

    // Map NAND registers
    s_hNand = NAND_Initialize(szContext, pRegIn, pRegOut);
    if (s_hNand == NULL)
        {
        goto cleanUp;
        }
        
    // We are done
    hRet = s_hNand;

cleanUp:
    return hRet;
}

//------------------------------------------------------------------------------
//
//  Function:  FMD_Deinit
//
BOOL
FMD_Deinit(
    VOID *pContext
    )
{
    BOOL rc = FALSE;
    if (pContext != s_hNand) goto cleanUp;

    //  Only enable during NAND read/write/erase operations
    NAND_Enable(s_hNand, TRUE);

    // Wait for NAND    
    // the next command may not work if you remove this wait on the status, 
    // because if the R/B pin is asserted during the reset, its deassertion 
    // isn't guaranteed to mean that the device is ready
    WaitForReadyStatus(s_hNand);   

    // Write RESET command
    // (a reset aborts any current READ, WRITE (PROGRAM) or ERASE operation)
    NAND_SendCommand(s_hNand, NAND_CMD_RESET);

    // Wait for NAND
    WaitForReadyStatus(s_hNand);

    // Clear GPMC wait-to-nowait edge detection mechanism on NAND R/B pin
    NAND_Enable(s_hNand, FALSE);

    // uninitialize and release allocated resources
    NAND_Uninitialize(s_hNand);
    s_hNand = NULL;
    
cleanUp:
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  FMD_GetInfo
//
//  This function is call to get flash information
//
BOOL
FMD_GetInfo(
    FlashInfo *pFlashInfo
    )
{
    BOOL rc = FALSE;

    // If we don't support NAND, fail...
    if (s_hNand == NULL) goto cleanUp;

    // Memory type is NAND
    pFlashInfo->flashType = NAND;
    pFlashInfo->dwNumBlocks = NAND_GetGeometry(s_hNand)->blocks;
    pFlashInfo->wSectorsPerBlock = (WORD) NAND_GetGeometry(s_hNand)->sectorsPerBlock;
    pFlashInfo->wDataBytesPerSector = (WORD) NAND_GetGeometry(s_hNand)->sectorSize;
    pFlashInfo->dwBytesPerBlock = NAND_GetGeometry(s_hNand)->sectorSize;
    pFlashInfo->dwBytesPerBlock *= NAND_GetGeometry(s_hNand)->sectorsPerBlock;

    // Done
    rc = TRUE;

cleanUp:
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  FMD_ReadSectorOOB
//
//  Read the OOB content of the sector.
//
BOOL
FMD_ReadSectorOOB(
    SECTOR_ADDR sector,
    UCHAR *pBuffer
    )
{
    BOOL rc = FALSE;
    UINT32 oldIdleMode;
    UINT32 sectorSize;

    // Fail if FMD wasn't opened
    if (s_hNand == NULL) goto cleanUp;
    
    NAND_MutexEnter(s_hNand);

    //  Change idle mode to no-idle to ensure access to GPMC registers
    sectorSize = NAND_GetGeometry(s_hNand)->sectorSize;
    oldIdleMode = INREG32(&(NAND_GetGpmcRegs(s_hNand)->GPMC_SYSCONFIG));
    OUTREG32(&(NAND_GetGpmcRegs(s_hNand)->GPMC_SYSCONFIG), SYSCONFIG_NOIDLE);
    
        {
        //  Only enable during NAND read/write/erase operations
        NAND_Enable(s_hNand, TRUE);

        // Make sure of the NAND status
        WaitForReadyStatus(s_hNand);

        // Send the command
        NAND_SendCommand(s_hNand, NAND_CMD_READ1);

        // Send the address
        NAND_Seek(s_hNand, sector, sectorSize);

        // Send the command
        NAND_SendCommand(s_hNand, NAND_CMD_READ2);

        // Wait for the action to finish
        WaitForReadyStatus(s_hNand);

        //Force a read here, else we will read the status again
        NAND_SendCommand (s_hNand, NAND_CMD_READ1);

        // read spare area
        NAND_Read(s_hNand, (BYTE*)pBuffer, 64, NULL);
        
        //  Only enable during NAND read/write/erase operations
        NAND_Enable(s_hNand, FALSE);

        }

    //  Change idle mode back
    OUTREG32(&(NAND_GetGpmcRegs(s_hNand)->GPMC_SYSCONFIG), oldIdleMode);
    
    // Done
    rc = TRUE;

cleanUp:
    // Release hardware lock
    if (s_hNand != NULL) NAND_MutexExit(s_hNand);
    
    ASSERT(rc);
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  FMD_ReadSector
//
//  Read the content of the sector.
//
BOOL
FMD_ReadSector(
    SECTOR_ADDR sector,
    UCHAR *pBuffer,
    SectorInfo *pSectorInfo,
    DWORD sectors
    )
{
    BOOL rc = FALSE;
    NAND_SPARE_AREA sa;
    UINT32 oldIdleMode;
    UINT32 sectorSize;
    BYTE rgEcc[ECC_BYTES];

    // Fail if FMD wasn't opened
    if (s_hNand == NULL) goto cleanUp;
    
    NAND_MutexEnter(s_hNand);

    //  Change idle mode to no-idle to ensure access to GPMC registers
    sectorSize = NAND_GetGeometry(s_hNand)->sectorSize;
    oldIdleMode = INREG32(&(NAND_GetGpmcRegs(s_hNand)->GPMC_SYSCONFIG));
    OUTREG32(&(NAND_GetGpmcRegs(s_hNand)->GPMC_SYSCONFIG), SYSCONFIG_NOIDLE);
	
    while (sectors > 0)
        {
        //  Only enable during NAND read/write/erase operations
        NAND_Enable(s_hNand, TRUE);
        
        // Read sector from A
        if (pBuffer != NULL)
            {
            // be sure to do this before sending the READ command and not after ! or the
            // status register would remain at status read mode, which would have to be changed
            // before retreiving the data
            WaitForReadyStatus(s_hNand);

			
            // configure prefetch engine for input and start it
            NAND_ConfigurePrefetch(s_hNand, NAND_DATA_READ);
           
            // Send the command
            NAND_SendCommand (s_hNand, NAND_CMD_READ1);

            // Send the address
            NAND_Seek(s_hNand, sector, 0);

            // Send the command
            NAND_SendCommand(s_hNand, NAND_CMD_READ2);

            // Wait for the action to finish
            WaitForReadyStatus(s_hNand);

            //Force a read here, else we will read the status again
            NAND_SendCommand (s_hNand, NAND_CMD_READ1);

	     // read data
            NAND_Read(s_hNand, pBuffer, sectorSize, rgEcc);
			
            // read spare area
            NAND_Read(s_hNand, (BYTE*)&sa, sizeof(sa), NULL);

            // Make sure of the NAND status
            WaitForReadyStatus(s_hNand);
            }
        else
         {
            // Make sure of the NAND status
            WaitForReadyStatus(s_hNand);
            // Send the command
            NAND_SendCommand(s_hNand, NAND_CMD_READ1);
    
            // Send the address
            NAND_Seek(s_hNand, sector, sectorSize);
    
            // Send the command
            NAND_SendCommand(s_hNand, NAND_CMD_READ2);
    
            // Wait for the action to finish
            WaitForReadyStatus(s_hNand);
            //Force a read here, else we will read the status again
            NAND_SendCommand (s_hNand, NAND_CMD_READ1);
    
            // read spare area
            NAND_Read(s_hNand, (BYTE*)&sa, sizeof(sa), NULL);

            // Make sure of the NAND status
            WaitForReadyStatus(s_hNand);
    
        }        // Copy sector info
        if (pSectorInfo != NULL)
            {
            pSectorInfo->bBadBlock    = sa.hwBadBlock[0] & sa.hwBadBlock[1];    // HW bad block check
            pSectorInfo->bBadBlock    = pSectorInfo->bBadBlock & sa.swBadBlock; // SW bad block flag check
            pSectorInfo->bOEMReserved = sa.oemReserved;

            memcpy(
                &pSectorInfo->dwReserved1, sa.reserved1,
                sizeof(pSectorInfo->dwReserved1)
                );

            memcpy(
                &pSectorInfo->wReserved2, sa.reserved2,
                sizeof(pSectorInfo->wReserved2)
                );
            }
        
        //  Only enable during NAND read/write/erase operations
        NAND_Enable(s_hNand, FALSE);

        // perform ecc correction and correct data when possible
        if ((pBuffer != NULL) &&
            (NAND_CorrectEccData(s_hNand, pBuffer, sectorSize, sa.ecc, rgEcc) == FALSE))
            {
            UINT count;
            UCHAR *pData = pBuffer;
            DEBUGMSG (ZONE_ERROR, (L"NAND_CorrectEccData returns FALSE, sector=%d\r\n", sector));	

            for (count = 0; count < sizeof(sa); count++)
                {
                // Allow OEMReserved byte to be set to reserved/readonly
                if (&(((UINT8*)&sa)[count]) == &sa.oemReserved) continue;
                if (((UINT8*)&sa)[count] != 0xFF) goto cleanUp;
                }

            for (count = 0; count < sectorSize; count++)
                {
                if (*pData != 0xFF) goto cleanUp;
                ++pData;
                }
            }
        
        // Move to next sector
        sector++;
        if (pBuffer != NULL) pBuffer += sectorSize;
        pSectorInfo++;
        sectors--;
        }

    //  Change idle mode back
    OUTREG32(&(NAND_GetGpmcRegs(s_hNand)->GPMC_SYSCONFIG), oldIdleMode);
    
    // Done
    rc = TRUE;

cleanUp:
    // Release hardware lock
    if (s_hNand != NULL) NAND_MutexExit(s_hNand);
    
    ASSERT(rc);
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  FMD_WriteSector
//
BOOL
FMD_WriteSector(
    SECTOR_ADDR sector,
    UCHAR *pBuffer,
    SectorInfo *pSectorInfo,
    DWORD sectors
    )
{
    BOOL rc = FALSE;
    NAND_SPARE_AREA sa;
    UINT32 oldIdleMode;
    UINT32 sectorSize;
    SECTOR_ADDR startSector = sector;
    SECTOR_ADDR endSector = sector + sectors;
    NandDevice_t *pDevice = (NandDevice_t*)s_hNand;

    // Fail if FMD wasn't opened
    if (s_hNand == NULL) goto cleanUp;
    
    // Obtain hardware lock
    NAND_MutexEnter(s_hNand);

    //  Change idle mode to no-idle to ensure access to GPMC registers
    sectorSize = NAND_GetGeometry(s_hNand)->sectorSize;
    oldIdleMode = INREG32(&(NAND_GetGpmcRegs(s_hNand)->GPMC_SYSCONFIG));
    OUTREG32(&(NAND_GetGpmcRegs(s_hNand)->GPMC_SYSCONFIG), SYSCONFIG_NOIDLE);

    //  Only enable during NAND read/write/erase operations    
    NAND_Enable(s_hNand, TRUE);
    NAND_LockBlocks(s_hNand, startSector, endSector, FALSE);
    
    while (sectors > 0)
        {
        // Clear out spare area struct
        memset(&sa, 0xFF, sizeof(NAND_SPARE_AREA));

        // When there is buffer write data
        if (pBuffer != NULL)
            {
            // enable prefetch
            NAND_ConfigurePrefetch(s_hNand, NAND_DATA_WRITE);

            // send the write command
            NAND_SendCommand(s_hNand, NAND_CMD_WRITE1);

            // send the address to write to
            NAND_Seek(s_hNand, sector, 0);

            // send data to flash
            NAND_Write(s_hNand, pBuffer, sectorSize, sa.ecc);

            }
        else
            {
            // Send the command
            NAND_SendCommand(s_hNand, NAND_CMD_READ1);

            // Send the address
            NAND_Seek(s_hNand, sector, NAND_GetGeometry(s_hNand)->sectorSize);

            // Send the command
            NAND_SendCommand(s_hNand, NAND_CMD_READ2);

            // Wait for the action to finish
            WaitForReadyStatus(s_hNand);

            //Force a read here, else we will read the status again
            NAND_SendCommand (s_hNand, NAND_CMD_READ1);

            // read spare area
            NAND_Read(s_hNand, (BYTE*)&sa, sizeof(sa), NULL);


            // send the write command
            NAND_SendCommand(s_hNand, NAND_CMD_WRITE1);

            // send the address to write to
            NAND_Seek(s_hNand, sector, sectorSize);
			
            }
		
        /* used for test purpose */ 
        if((pSectorInfo != NULL) && (pSectorInfo->bOEMReserved == SKIP_ECC_WRITE_MAGIC_NUMBER) )
        {
            /* skip updating ECC */
            ECC_Reset(pDevice->pGpmcRegs);			
	     goto  skip_ecc; 	
        }

        if (pSectorInfo != NULL)
            {
            // Fill in rest of spare area info (we already have ECC from above)
            sa.swBadBlock     = pSectorInfo->bBadBlock;
            sa.oemReserved  = pSectorInfo->bOEMReserved;
            memcpy(sa.reserved1, &pSectorInfo->dwReserved1, sizeof(sa.reserved1));
            memcpy(sa.reserved2, &pSectorInfo->wReserved2, sizeof(sa.reserved2));
            }

        // write new spare info
        NAND_Write(s_hNand, (BYTE*)&sa, sizeof(sa), NULL);

skip_ecc:
        // initiate the data programming process :
        NAND_SendCommand(s_hNand, NAND_CMD_WRITE2);

        // wait until completion of the operation :
        WaitForReadyStatus(s_hNand);

        if ((NAND_GetStatus(s_hNand) & NAND_STATUS_ERROR) != 0)
            {
            break;
            }
        // Move to next sector
        sector++;
        if (pBuffer != NULL) pBuffer += sectorSize;
        pSectorInfo++;
        sectors--;
        }

    //  Only enable during NAND read/write/erase operations    
    NAND_LockBlocks(s_hNand, startSector, endSector, TRUE);
    NAND_Enable(s_hNand, FALSE);

    //  Change idle mode back
    OUTREG32(&(NAND_GetGpmcRegs(s_hNand)->GPMC_SYSCONFIG), oldIdleMode);

    // All is ok, when we read all sectors
    rc = (sectors == 0);

cleanUp:
    // Release hardware lock
    if (s_hNand != NULL) NAND_MutexExit(s_hNand);
    
    ASSERT(rc);
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  FMD_EraseBlock
//
//  Erase the given block
//
BOOL
FMD_EraseBlock(
    BLOCK_ID blockId
    )
{
    BOOL rc = FALSE;
    UINT32 oldIdleMode;

    // Check if we know flash geometry
    if (s_hNand == NULL) return rc;

    // Obtain hardware lock
    NAND_MutexEnter(s_hNand);

    //  Change idle mode to no-idle to ensure access to GPMC registers
    oldIdleMode = INREG32(&(NAND_GetGpmcRegs(s_hNand)->GPMC_SYSCONFIG));
    OUTREG32(&(NAND_GetGpmcRegs(s_hNand)->GPMC_SYSCONFIG), SYSCONFIG_NOIDLE);

    //  Only enable during NAND read/write/erase operations    
    NAND_Enable(s_hNand, TRUE);
    NAND_LockBlocks(s_hNand, blockId, blockId, FALSE);
    
    // send block id to erase
    NAND_EraseBlock(s_hNand, blockId);

    // wait for completion    
    WaitForReadyStatus(s_hNand);

    //Verify there wasn't any error by checking the NAND status register :
    if ((NAND_GetStatus(s_hNand) & NAND_STATUS_ERROR) != 0)
        {
        rc = FALSE;
        goto cleanUp;
        }

    // Done
    rc = TRUE;

cleanUp:
    //  Only enable during NAND read/write/erase operations    
    NAND_LockBlocks(s_hNand, blockId, blockId, TRUE);
    NAND_Enable(s_hNand, FALSE);
   

    //  Change idle mode back
    OUTREG32(&(NAND_GetGpmcRegs(s_hNand)->GPMC_SYSCONFIG), oldIdleMode);

    // Release hardware lock
    if (s_hNand != NULL) NAND_MutexExit(s_hNand);

    return rc;
}


//------------------------------------------------------------------------------
//
//  Function: FMD_GetBlockStatus
//
DWORD
FMD_GetBlockStatus(
    BLOCK_ID blockId
    )
{
    DWORD rc = 0;
    SECTOR_ADDR sector;
    SectorInfo sectorInfo[2];


    // Check if we know flash geometry
    if (s_hNand == NULL) goto cleanUp;

    // Calculate sector
    sector = blockId * NAND_GetGeometry(s_hNand)->sectorsPerBlock;

    if (!FMD_ReadSector(sector, NULL, sectorInfo, 2))
        {
        rc = BLOCK_STATUS_UNKNOWN;
        goto cleanUp;
        }

    if ((sectorInfo[0].bBadBlock != 0xFF) || (sectorInfo[1].bBadBlock != 0xFF))
        {
        rc |= BLOCK_STATUS_BAD;
        }

    if ((sectorInfo[0].bOEMReserved & OEM_BLOCK_READONLY) == 0)
        {
        rc |= BLOCK_STATUS_READONLY;
        }

    if ((sectorInfo[0].bOEMReserved & OEM_BLOCK_RESERVED) == 0)
        {
        rc |= BLOCK_STATUS_RESERVED;
        }

cleanUp:
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function: FMD_SetBlockStatus
//
BOOL
FMD_SetBlockStatus(
    BLOCK_ID blockId,
    DWORD status
    )
{
    BOOL rc = FALSE;
    SECTOR_ADDR sector;
    SectorInfo sectorInfo;

    // Check if we know flash geometry
    if (s_hNand == NULL) goto cleanUp;

    // Calculate sector
    sector = blockId * NAND_GetGeometry(s_hNand)->sectorsPerBlock;

    if ((status & BLOCK_STATUS_BAD) != 0)
        {
        // Read the sector info
        if (!FMD_ReadSector(sector, NULL, &sectorInfo, 1)) goto cleanUp;

        // Set the bad block marker
        sectorInfo.bBadBlock = 0;

        // Complete the write (no erase, we changed 0xFF -> 0x00)
        if (!FMD_WriteSector(sector, NULL, &sectorInfo, 1)) goto cleanUp;
        }

#ifdef BOOT_MODE
    if ( ((status & BLOCK_STATUS_READONLY) != 0) || ((status & BLOCK_STATUS_RESERVED) != 0) )
        {
        // Read the sector info
        if (!FMD_ReadSector(sector, NULL, &sectorInfo, 1)) goto cleanUp;

        // Set the OEM field
        sectorInfo.bOEMReserved &= ((status & BLOCK_STATUS_READONLY) != 0) ? ~(OEM_BLOCK_READONLY) : 0xFF;
        sectorInfo.bOEMReserved &= ((status & BLOCK_STATUS_RESERVED) != 0) ? ~(OEM_BLOCK_RESERVED) : 0xFF;
        
        // Complete the write (no erase, changed bits from 1s to 0s)
        if (!FMD_WriteSector(sector, NULL, &sectorInfo, 1)) goto cleanUp;
        }
#endif

    rc = TRUE;

cleanUp:
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  FMD_PowerUp
//
VOID
FMD_PowerUp( VOID
    )
{
    // exit if FMD wasn't opened
    if (s_hNand == NULL) return;
    
    // Obtain hardware lock
    NAND_MutexEnter(s_hNand);

    //  Only enable during NAND read/write/erase operations
    NAND_Enable(s_hNand, TRUE);

    // Wait for NAND
    WaitForReadyStatus(s_hNand);   

    // Write the reset command
    NAND_SendCommand(s_hNand, NAND_CMD_RESET);

    // Wait for NAND
    WaitForReadyStatus(s_hNand);

    //  Only enable during NAND read/write/erase operations
    NAND_Enable(s_hNand, FALSE);

    // Release hardware lock
    NAND_MutexExit(s_hNand);
}

//------------------------------------------------------------------------------
//
//  Function:  FMD_PowerDown
//
VOID
FMD_PowerDown( VOID
    )
{
    // exit if FMD wasn't opened
    if (s_hNand == NULL) return;
    
    // Obtain hardware lock
    NAND_MutexEnter(s_hNand);

    //  Only enable during NAND read/write/erase operations
    NAND_Enable(s_hNand, TRUE);

    // Wait for NAND
    WaitForReadyStatus(s_hNand);   

    // Write the reset command
    NAND_SendCommand(s_hNand, NAND_CMD_RESET);

    // Wait for NAND
    WaitForReadyStatus(s_hNand);

    //  Only enable during NAND read/write/erase operations
    NAND_Enable(s_hNand, FALSE);

    // Release hardware lock
    NAND_MutexExit(s_hNand);
}

//------------------------------------------------------------------------------
//
//  Function:  FMD_OEMIoControl
//
BOOL
FMD_OEMIoControl(
    DWORD code,
    UCHAR *pInBuffer,
    DWORD inSize,
    UCHAR *pOutBuffer,
    DWORD outSize,
    DWORD *pOutSize
    )
{

    UNREFERENCED_PARAMETER(code);

    UNREFERENCED_PARAMETER(pInBuffer);
    UNREFERENCED_PARAMETER(inSize);
    UNREFERENCED_PARAMETER(pOutBuffer);
    UNREFERENCED_PARAMETER(outSize);
    UNREFERENCED_PARAMETER(pOutSize);
    
    return FALSE;    
}

