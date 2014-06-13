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
#include <elm.h>
#include "oalex.h"
//#include "sdk_padcfg.h"
//#include <oal_clock.h>

extern void GpmcEnable(BOOL on);

//-----------------------------------------------------------------------------
#define FIFO_THRESHOLD          (64)            // pre-fetch fifo config

//-----------------------------------------------------------------------------
// typedefs and enums
typedef enum {
    kPrefetchOff,
    kPrefetchRead,
    kPrefetchWrite,
} PrefetchMode_e;

//-----------------------------------------------------------------------------
// nand access definitions

void FMD_ShowBytes(UINT8 *pBytes, UINT32 size);

typedef REG8  NANDREG8;
typedef REG16 NANDREG16;

typedef struct {
    NANDREG8               *pNandCmd;
    NANDREG8               *pNandAddress;
    NANDREG8               *pNandData;
    NANDREG8               *pFifo;
} NAND8_T;

typedef struct {
    NANDREG16               *pNandCmd;
    NANDREG16               *pNandAddress;
    NANDREG16               *pNandData;
    NANDREG16               *pFifo;
} NAND16_T;

typedef struct {
    CRITICAL_SECTION        cs;
    BOOL                    bNandLockMode;
    PrefetchMode_e          prefetchMode;
    OMAP_GPMC_REGS          *pGpmcRegs;
    DWORD                   memBase[2];
    DWORD                   memLen[2];
    DWORD                   timeout;
    NAND_INFO               nandInfo;
    DWORD                   ECCCfg;
    DWORD                   IrqWait;
    EccType_e               ECCtype;
    DWORD                   ECCsize; /* number of bytes */
    DWORD                   prefetch_enable;
    DWORD                   options;
#define NAND_DEVICE_OPT_REG_SIZE_MASK   0x00000003
#define NAND_DEVICE_OPT_8BIT_REG    0x00000000
#define NAND_DEVICE_OPT_16BIT_REG   0x00000001

    union {
        NAND8_T    nand8;
        NAND16_T   nand16;
    } u;

} NandDevice_t;

#define IS_NAND_16BIT(p)  ((p)->options & NAND_DEVICE_OPT_REG_SIZE_MASK)

#define NANDREG8_PTR(x)    ((NANDREG8 *)(x))
#define NAND_REG8_CMD(p)   ((p)->u.nand8.pNandCmd)
#define NAND_REG8_ADDR(p)  ((p)->u.nand8.pNandAddress)
#define NAND_REG8_DATA(p)  ((p)->u.nand8.pNandData)
#define NAND_REG8_FIFO(p)  ((p)->u.nand8.pFifo)

#define NANDREG16_PTR(p)    (((NANDREG16 *)(p)))
#define NAND_REG16_CMD(p)   ((p)->u.nand16.pNandCmd)
#define NAND_REG16_ADDR(p)  ((p)->u.nand16.pNandAddress)
#define NAND_REG16_DATA(p)  ((p)->u.nand16.pNandData)
#define NAND_REG16_FIFO(p)  ((p)->u.nand16.pFifo)

#define NANDREG_PTR(p, x) ((IS_NAND_16BIT(p))?((NANDREG16 *)(x)):((NANDREG8 *)(x)))

#define NANDREG_SIZE(p)   ((int)((IS_NAND_16BIT(p))?(sizeof(REG16)):(sizeof(REG8))))

#define WRITE_NAND_ADDR(p,y) { \
                              if(IS_NAND_16BIT((p))) \
                                  OUTREG16((NANDREG16 *)(NAND_REG16_ADDR(p)),y); \
                              else                              \
                                  OUTREG8((NANDREG8 *)(NAND_REG8_ADDR(p)),y); \
                             }

#define WRITE_NAND_CMD(p,y) { \
                              if(IS_NAND_16BIT((p))) \
                                  OUTREG16((NANDREG16 *)(NAND_REG16_CMD(p)),y); \
                              else                              \
                                  OUTREG8((NANDREG8 *)(NAND_REG8_CMD(p)),y); \
                             }

#define WRITE_NAND_DATA(p,y) { \
                              if(IS_NAND_16BIT((p))) \
                                  OUTREG16((NANDREG16 *)(NAND_REG16_DATA(p)),y); \
                              else                              \
                                  OUTREG8((NANDREG8 *)(NAND_REG8_DATA(p)),y); \
                             }

#define READ_NAND_DATA(p) ((IS_NAND_16BIT(p))? \
                              INREG16((NANDREG16 *)(NAND_REG16_DATA(p))) : \
                              INREG8((NANDREG8 *)(NAND_REG8_DATA(p))))

//------------------------------------------------------------------------------
//  NAND Spare Area Format for x16 devices

/* 16 bit access, large page nand */
#pragma pack (1)
typedef struct
{
    UCHAR hwBadBlock[2];           // Hardware bad block flag
    UCHAR ecc[56];
    UCHAR reserved1[4];         // Reserved - used by FAL
    UCHAR reserved2[1];         // Reserved - used by FAL
//    UCHAR swBadBlock;           // Software bad block flag
    UCHAR oemReserved;          // For use by OEM
//    UCHAR unused[2];           // Unused
}NAND_SPARE_AREA;
#pragma pack()
//-----------------------------------------------------------------------------
// global variables
// Note: global variable prevents to have twio instance of the driver loaded at the same time
static NandDevice_t s_Device;
static HANDLE s_hNand = NULL;
#ifdef BOOT_MODE
extern DWORD g_ecctype;
#endif
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
    }, {
        L"Prefetch", PARAM_DWORD, FALSE, offset(NandDevice_t, prefetch_enable),
        fieldsize(NandDevice_t, prefetch_enable), (VOID*)0
    }
};
#endif

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static BOOL NAND_LockBlocks(HANDLE hNand, BOOL bLock );
static void NAND_Uninitialize( HANDLE hNand );
static HANDLE NAND_Initialize( LPCTSTR szContext,PCI_REG_INFO *pRegIn,PCI_REG_INFO *pRegOut);
static BOOL NAND_Seek( HANDLE hNand, SECTOR_ADDR sector, UINT offset, UINT read_mode);
static BOOL NAND_ReadPageBch( HANDLE hNand, BYTE *pData, int size, BYTE *pEcc );
static BOOL NAND_ReadPageOOB( HANDLE hNand, BYTE *pOOB);
static BOOL NAND_WritePageBch( HANDLE hNand, BYTE *pData, int size, BYTE *pEcc );
static BOOL NAND_WritePageOOB( HANDLE hNand, BYTE *pOOB);
static UINT16 NAND_GetStatus( HANDLE hNand );
static BOOL NAND_EraseBlock( HANDLE hNand, BLOCK_ID blockId );
static BOOL NAND_Enable( HANDLE hNand, BOOL bEnable );
static UINT NAND_MutexEnter( HANDLE hNand );
static UINT NAND_MutexExit( HANDLE hNand );
static BOOL NAND_SendCommand( HANDLE hNand, UINT cmd );
static BOOL NAND_ConfigurePrefetch( HANDLE hNand, UINT accessType);

__inline void SectorAccess( NandDevice_t* pDevice, SECTOR_ADDR sector, UINT offset, UINT read_mode)
{
    // Offset is provided to this function in bytes; NAND device requires words
    offset = offset / NANDREG_SIZE(pDevice);

	if( read_mode == READ_FULL_PAGE )
	{
		WRITE_NAND_ADDR(pDevice, (offset & 0xFF));
		WRITE_NAND_ADDR(pDevice, ((offset >> 8) & 0xFF));
		WRITE_NAND_ADDR(pDevice, (sector & 0xFF));
		WRITE_NAND_ADDR(pDevice, ((sector >> 8) & 0xFF));
		WRITE_NAND_ADDR(pDevice, ((sector >> 16) & 0xFF));
	}
	else if( read_mode == READ_RANDOM_COLUMN )
	{
		WRITE_NAND_ADDR(pDevice, (offset & 0xFF));
		WRITE_NAND_ADDR(pDevice, ((offset >> 8) & 0xFF));
	}
	else
	{
		RETAILMSG(TRUE,(L"Read mode incorrect value"));
	}
}
       
__inline void BlockAccess( NandDevice_t* pDevice, BLOCK_ID blockId )
{
    blockId *= pDevice->nandInfo.sectorsPerBlock;
    WRITE_NAND_ADDR(pDevice, (blockId & 0xFF));
    WRITE_NAND_ADDR(pDevice, ((blockId >> 8) & 0xFF));
    WRITE_NAND_ADDR(pDevice, ((blockId >> 16) & 0xFF));
}

__inline void WaitOnEmptyWriteBufferStatus( NandDevice_t *pDevice )
{
    UINT status;
    do {
        status = INREG32(&pDevice->pGpmcRegs->GPMC_STATUS);
    } while ((status & GPMC_STATUS_EMPTYWRITEBUFFER) == 0);
}

//-----------------------------------------------------------------------------
BOOL GetNandBusWidth(NandDevice_t *pDevice)
{
    DWORD chipSelect = BSPGetNandCS();
    REG32 *pConfig1 = NULL;
    DWORD deviceSize = 0;

    pConfig1 = 
        (REG32 *)((UINT32)pDevice->pGpmcRegs +
        offset(OMAP_GPMC_REGS, GPMC_CONFIG1_0) +
        (0x30 * chipSelect));

    deviceSize = (INREG32(pConfig1) >> 12) & 0x00000003;
    pDevice->options |= deviceSize;

    return TRUE;
}


BOOL InitializeNandRegPointers(NandDevice_t *pDevice)
{
    DWORD chipSelect = BSPGetNandCS();

    if(IS_NAND_16BIT(pDevice))
    {
#ifdef DEVICE_MODE
        RETAILMSG(TRUE, (L"16Bit NAND device\r\n"));			
#endif

        NAND_REG16_CMD(pDevice) = 
            NANDREG16_PTR(((UINT32)pDevice->pGpmcRegs + 
                                 offset(OMAP_GPMC_REGS, GPMC_NAND_COMMAND_0) + 
                                 (0x30 * chipSelect)));
    
        NAND_REG16_ADDR(pDevice) = 
            NANDREG16_PTR(((UINT32)pDevice->pGpmcRegs +
                                 offset(OMAP_GPMC_REGS, GPMC_NAND_ADDRESS_0) +
                                 (0x30 * chipSelect)));
    
        NAND_REG16_DATA(pDevice) = 
            NANDREG16_PTR(((UINT32)pDevice->pGpmcRegs +
                                 offset(OMAP_GPMC_REGS, GPMC_NAND_DATA_0) +
                                 (0x30 * chipSelect)));
    }
    else
    {
#ifdef DEVICE_MODE
        RETAILMSG(TRUE, (L"8Bit NAND device\r\n"));			
#endif

        NAND_REG8_CMD(pDevice) = 
            NANDREG8_PTR(((UINT32)pDevice->pGpmcRegs + 
                                 offset(OMAP_GPMC_REGS, GPMC_NAND_COMMAND_0) + 
                                 (0x30 * chipSelect)));
    
        NAND_REG8_ADDR(pDevice) = 
            NANDREG8_PTR(((UINT32)pDevice->pGpmcRegs +
                                 offset(OMAP_GPMC_REGS, GPMC_NAND_ADDRESS_0) +
                                 (0x30 * chipSelect)));
    
        NAND_REG8_DATA(pDevice) = 
            NANDREG8_PTR(((UINT32)pDevice->pGpmcRegs +
                                 offset(OMAP_GPMC_REGS, GPMC_NAND_DATA_0) +
                                 (0x30 * chipSelect)));
    }

    return TRUE;
}


BOOL InitializePointers(LPCTSTR szContext, NandDevice_t *pDevice )
{
#ifdef BOOT_MODE
    UNREFERENCED_PARAMETER(szContext);
    UNREFERENCED_PARAMETER(pDevice);
#else
    PHYSICAL_ADDRESS pa;
    InitializeCriticalSection(&pDevice->cs);

    // Read device parameters
    if (GetDeviceRegistryParams(szContext, pDevice, dimof(g_deviceRegParams), g_deviceRegParams)
		!= ERROR_SUCCESS){
        DEBUGMSG(ZONE_ERROR, (L"ERROR: FMD_Init: Failed read FMD registry parameters\r\n"));
        return FALSE;
    }

    pa.QuadPart = pDevice->memBase[0];
    pDevice->pGpmcRegs = MmMapIoSpace(pa, pDevice->memLen[0], FALSE);
    if (pDevice->pGpmcRegs == NULL){
        DEBUGMSG(ZONE_ERROR, (L"ERROR: FMD_Init: Failed map FMD registers (0x%08x/0x%08x)\r\n",
            pDevice->memBase[0], pDevice->memLen[0]));
        return FALSE;
    }

    /* Find out whether it is 8/16 bit NAND from GPMC config and
       set the options field first before doing anything */
    GetNandBusWidth(pDevice);

    pa.QuadPart = pDevice->memBase[1];

    if (IS_NAND_16BIT(pDevice))
        NAND_REG16_FIFO(pDevice) = NANDREG16_PTR(MmMapIoSpace(pa, pDevice->memLen[1], FALSE));
    else
        NAND_REG8_FIFO(pDevice) = NANDREG8_PTR(MmMapIoSpace(pa, pDevice->memLen[1], FALSE));

    if ((NAND_REG16_FIFO(pDevice) == NULL) && (NAND_REG8_FIFO(pDevice) == NULL))
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: FMD_Init: Failed map FMD registers (0x%08x/0x%08x)\r\n",
            pDevice->memBase[0], pDevice->memLen[0]));
        return FALSE;
    }

#endif
	return TRUE;
}

__inline OMAP_GPMC_REGS* NAND_GetGpmcRegs( HANDLE hNand){ return ((NandDevice_t*)hNand)->pGpmcRegs;  }
__inline NAND_INFO const* NAND_GetGeometry(HANDLE hNand){ return &(((NandDevice_t*)hNand)->nandInfo);}

BOOL NAND_LockBlocks( HANDLE hNand, BOOL bLock )
{
    NandDevice_t *pDevice = (NandDevice_t*)hNand;

	if (pDevice == NULL) return FALSE;

    if (bLock) CLRREG32(&pDevice->pGpmcRegs->GPMC_CONFIG, GPMC_CONFIG_WRITEPROTECT);        
    else       SETREG32(&pDevice->pGpmcRegs->GPMC_CONFIG, GPMC_CONFIG_WRITEPROTECT);

	return TRUE;
}

void NAND_Uninitialize( HANDLE hNand )
{
#ifdef BOOT_MODE
    UNREFERENCED_PARAMETER(hNand);
#else
    NandDevice_t *pDevice = (NandDevice_t*)hNand;

    if (pDevice->pGpmcRegs != NULL)
        MmUnmapIoSpace((void*)pDevice->pGpmcRegs, pDevice->memLen[0]);

    if (IS_NAND_16BIT(pDevice))
    {
        if (NAND_REG16_FIFO(pDevice) != NULL)
            MmUnmapIoSpace((void*)NAND_REG16_FIFO(pDevice), pDevice->memLen[1]);
    }
    else
    {
        if (NAND_REG8_FIFO(pDevice) != NULL)
            MmUnmapIoSpace((void*)NAND_REG8_FIFO(pDevice), pDevice->memLen[1]);
    }

    GpmcEnable(FALSE);
#endif
}

HANDLE NAND_Initialize(LPCTSTR szContext, PCI_REG_INFO *pRegIn, PCI_REG_INFO *pRegOut)
{
    DWORD chipSelect = BSPGetNandCS();
    const NAND_INFO *  pBSPNandInfo;
    HANDLE hDevice = NULL;
    UINT ffPrefetchMode = 0;
    UINT8 manufacturer, device, dataToDiscard;
    NandDevice_t *pDevice = &s_Device;
#ifndef BOOT_MODE    
    DWORD dwKernelRet;
#endif

    UNREFERENCED_PARAMETER(pRegOut);
    UNREFERENCED_PARAMETER(szContext);
    UNREFERENCED_PARAMETER(ffPrefetchMode);

    // initialize structure
    memset(pDevice, 0, sizeof(NandDevice_t));

    // use prefetch by default
    pDevice->prefetch_enable = 1;

#ifdef BOOT_MODE    
    pDevice->pGpmcRegs = (OMAP_GPMC_REGS*)OALPAtoUA(SOCGetGPMCAddress(0));

    /* Find out whether it is 8/16 bit NAND from GPMC config and
       set the options field first before doing anything */
    GetNandBusWidth(pDevice);

    if (IS_NAND_16BIT(pDevice))
        NAND_REG16_FIFO(pDevice) = NANDREG16_PTR(OALPAtoUA(pRegIn->MemBase.Reg[0]));
    else
        NAND_REG8_FIFO(pDevice) = NANDREG8_PTR(OALPAtoUA(pRegIn->MemBase.Reg[0]));

    /* Get ECC mode from BootCfg */
    pDevice->ECCtype = (EccType_e)g_ecctype; 
    if((pDevice->ECCtype > EccTypeLast) || (pDevice->ECCtype < Hamming1bit))
    {
        pDevice->ECCtype = BCH8bitElm;
        RETAILMSG(TRUE, (L"Incorrect ECC type setting\r\n"));			
    }

    if((pDevice->ECCtype == BCH8bitElm) || (pDevice->ECCtype == BCH8bitElmX)) elm_init();	

#else    
    GpmcEnable(TRUE);

    if (szContext != NULL){
        if (InitializePointers(szContext, pDevice) == FALSE) goto cleanUp;
    } else {
        PHYSICAL_ADDRESS pa;
        
        // if there's no context string then use global macros
        pa.QuadPart = pRegIn->MemBase.Reg[0];
        pDevice->memLen[0] = pRegIn->MemLen.Reg[0];
        pDevice->pGpmcRegs = MmMapIoSpace(pa, pDevice->memLen[0], FALSE);
        if (pDevice->pGpmcRegs == NULL) goto cleanUp;
	
        /* Find out whether it is 8/16 bit NAND from GPMC config and
           set the options field first before doing anything */
        GetNandBusWidth(pDevice);

        pa.QuadPart = pRegIn->MemBase.Reg[1];
        pDevice->memLen[1] = pRegIn->MemLen.Reg[1];

        if (IS_NAND_16BIT(pDevice))
            NAND_REG16_FIFO(pDevice) = NANDREG16_PTR(MmMapIoSpace(pa, pDevice->memLen[1], FALSE));
        else
            NAND_REG8_FIFO(pDevice) = NANDREG8_PTR(MmMapIoSpace(pa, pDevice->memLen[1], FALSE));

        if (pDevice->pGpmcRegs == NULL) goto cleanUp;

    }

    if (!KernelIoControl(IOCTL_HAL_GET_ECC_TYPE,
                         NULL, 0, &pDevice->ECCtype, sizeof(DWORD), &dwKernelRet))
    {
        RETAILMSG( TRUE,(TEXT("Failed to read Ecc type\r\n")));
        pDevice->ECCtype = Hamming1bit;
    }   

    if((pDevice->ECCtype == BCH8bitElm) || (pDevice->ECCtype == BCH8bitElmX)) elm_init();	

    RETAILMSG(TRUE, (L"ECC TYPE is %s\r\n", 
        (pDevice->ECCtype==Hamming1bit)? L"Hamming 1 bit" : 
        (pDevice->ECCtype==BCH4bit)    ? L"BCH 4 bit" : 
        (pDevice->ECCtype==BCH8bit)    ? L"BCH 8 bit" :
        (pDevice->ECCtype==BCH8bitElm) ? L"BCH 8 bit with ELM" :
        (pDevice->ECCtype==BCH8bitElmX)? L"BCH 8 bit with ELM-X" : L"Unknown"	));

	 if (pDevice->ECCtype > EccTypeLast) goto cleanUp;
	 
#endif

    // Set up the NAND cmd, addr and data reg pointers
    InitializeNandRegPointers(pDevice);

    // Enable GPMC wait-to-nowait edge detection mechanism on NAND R/B pin
    NAND_Enable(pDevice, TRUE);

    // Write RESET command
    // (a reset aborts any current READ, WRITE (PROGRAM) or ERASE operation)
    NAND_SendCommand(pDevice, NAND_CMD_RESET);

	while ((NAND_GetStatus(pDevice) & NAND_STATUS_READY) == 0);

    // Send Read ID Command
    NAND_SendCommand(pDevice, NAND_CMD_READID);

    // Send Address 00h
    WRITE_NAND_ADDR(pDevice, 0);


    // Read the manufacturer ID & device code
    manufacturer = (UINT8)READ_NAND_DATA(pDevice);
    device = (UINT8)READ_NAND_DATA(pDevice);
	// Read the bytes 3 to 5 send by the NAND after a "Read ID" request to clear the status register
	dataToDiscard = (UINT8)READ_NAND_DATA(pDevice);
	dataToDiscard = (UINT8)READ_NAND_DATA(pDevice);
	dataToDiscard = (UINT8)READ_NAND_DATA(pDevice);

    if ((pBSPNandInfo = BSPGetNandInfo(manufacturer,device))==NULL)
    {
        RETAILMSG(1, (L"FMD driver does not support NAND manufacturer:0x%02X dev:0x%02X\r\n", 
            manufacturer, device));
        goto cleanUp;
    }

	if ((pBSPNandInfo->sectorSize != 2048) && (pBSPNandInfo->wordData != 2)){
        ERRORMSG(1,(TEXT("FMD driver supports only 16bits large page (2KB) devices\r\n")));
        goto cleanUp;
    }

    pDevice->nandInfo = *pBSPNandInfo;
    pDevice->IrqWait = BSPGetNandIrqWait();

    /* ECCCfg: cs0, 4 - 512 bytes blocks per page */
    pDevice->ECCCfg = ((chipSelect << 1) | (0x3<<4)); 

    /* ECCCfg: 8/16bit bus width */
    if(IS_NAND_16BIT(pDevice))
        pDevice->ECCCfg |= GPMC_ECC_CONFIG_16BIT;
    else
        pDevice->ECCCfg |= GPMC_ECC_CONFIG_8BIT;

    pDevice->ECCsize = (pDevice->ECCtype == Hamming1bit ) ? ECC_BYTES_HAMMING : 
                       (pDevice->ECCtype == BCH4bit)      ? ECC_BYTES_BCH4 : 
                       (pDevice->ECCtype == BCH8bit)      ? ECC_BYTES_BCH8 :
                       (pDevice->ECCtype == BCH8bitElm)   ? ECC_BYTES_BCH8_ELM :
                       (pDevice->ECCtype == BCH8bitElmX)  ? ECC_BYTES_BCH8_ELMX : ECC_BYTES_BCH8;  
							  
    //  Enable and reset ECC engine (workaround for engine giving 0s first time)
    ECC_Init(pDevice->pGpmcRegs, pDevice->ECCCfg, pDevice->ECCtype, NAND_ECC_READ);
    ECC_Reset(pDevice->pGpmcRegs);

    NAND_Enable(pDevice, FALSE);

    pDevice->prefetchMode = kPrefetchOff;
    OUTREG32(&pDevice->pGpmcRegs->GPMC_PREFETCH_CONTROL, 0);
    
    if (pDevice->prefetch_enable)
    {
        // set prefetch mask
        ffPrefetchMode = GPMC_PREFETCH_CONFIG_SYNCHROMODE |
                         GPMC_PREFETCH_CONFIG_PFPWENROUNDROBIN |
                         GPMC_PREFETCH_CONFIG_ENABLEOPTIMIZEDACCESS |
                         GPMC_PREFETCH_CONFIG_WAITPINSELECTOR(chipSelect) |
                         GPMC_PREFETCH_CONFIG_FIFOTHRESHOLD(FIFO_THRESHOLD) |
                         GPMC_PREFETCH_CONFIG_ENGINECSSELECTOR(chipSelect);
    
        OUTREG32(&pDevice->pGpmcRegs->GPMC_PREFETCH_CONFIG1, ffPrefetchMode);
    
        OUTREG32(&pDevice->pGpmcRegs->GPMC_PREFETCH_CONFIG2, pBSPNandInfo->sectorSize);
        SETREG32(&pDevice->pGpmcRegs->GPMC_PREFETCH_CONFIG1, GPMC_PREFETCH_CONFIG_ENABLEENGINE);
    }
    
    hDevice = pDevice;
cleanUp:
    return hDevice;
}

BOOL NAND_Seek( HANDLE hNand, SECTOR_ADDR sector, UINT offset, UINT read_mode )
{
    NandDevice_t *pDevice = (NandDevice_t*)hNand;
    SectorAccess(pDevice, sector, offset, read_mode);
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
            if (IS_NAND_16BIT(pDevice))
                memcpy(pData, (BYTE*)NAND_REG16_FIFO(pDevice), FIFO_THRESHOLD);
            else
                memcpy(pData, (BYTE*)NAND_REG8_FIFO(pDevice), FIFO_THRESHOLD);

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
        while (size >= NANDREG_SIZE(pDevice))
        	{
            if (IS_NAND_16BIT(pDevice))
                *((NANDREG16 *)(pData)) = READ_NAND_DATA(pDevice);
            else
				*((NANDREG8 *)(pData)) = (UINT8)READ_NAND_DATA(pDevice);

            pData += NANDREG_SIZE(pDevice);
            size -= NANDREG_SIZE(pDevice);

            if (pDevice->prefetch_enable)
                {
	            /* workaround for BCH engine when ECC is not put at the end of OOB area.  
		        the checking is based on puting ECC at the BootROM expected location -
		        with 2 bytes offset from the start of the OOB area */
                if( pDevice->ECCtype == BCH4bit    || pDevice->ECCtype == BCH8bit || 
                    pDevice->ECCtype == BCH8bitElm || pDevice->ECCtype == BCH8bitElmX )
                    if (size == (int)(sizeof(NAND_SPARE_AREA) - ECC_OFFSET - pDevice->ECCsize))
        	            ECC_Reset(pDevice->pGpmcRegs);  
                }

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
NAND_NoPrefetchReadECC(
    HANDLE hNand,
    BYTE *pData,
    int size,
    BYTE *pEcc
    )
{
    NandDevice_t *pDevice = (NandDevice_t*)hNand;

    while (size >= NANDREG_SIZE(pDevice))
    {
        if (IS_NAND_16BIT(pDevice))
            *(NANDREG16_PTR(pData)) = READ_NAND_DATA(pDevice);
        else
            *(NANDREG8_PTR(pData)) = (UINT8)READ_NAND_DATA(pDevice);

        pData += NANDREG_SIZE(pDevice);
        size -= NANDREG_SIZE(pDevice);

		/* workaround for BCH engine when ECC is not put at the end of OOB area.  
		  the checking is based on puting ECC at the BootROM expected location -
		  with 2 bytes offset from the start of the OOB area */
        if(pDevice->ECCtype == BCH4bit    || pDevice->ECCtype == BCH8bit || 
           pDevice->ECCtype == BCH8bitElm || pDevice->ECCtype == BCH8bitElmX)
            if (size == (int)(sizeof(NAND_SPARE_AREA) - ECC_OFFSET - pDevice->ECCsize))
                ECC_Reset(pDevice->pGpmcRegs);  

    }

    ECC_Reset(pDevice->pGpmcRegs);

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
            // copy data to FIFO
            if (IS_NAND_16BIT(pDevice))
                memcpy((BYTE*)NAND_REG16_FIFO(pDevice), pData, FIFO_THRESHOLD);
            else
                memcpy((BYTE*)NAND_REG8_FIFO(pDevice), pData, FIFO_THRESHOLD);
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
		
        while (size >= NANDREG_SIZE(pDevice))
            {
            if (IS_NAND_16BIT(pDevice))
            {
                WRITE_NAND_DATA(pDevice, *(NANDREG16_PTR(pData)));
            }
            else
            {
                WRITE_NAND_DATA(pDevice, *(NANDREG8_PTR(pData)));
            }

            // Workaround Errata 1.53
            // need to check on EMPTYWRITEBUFFERSTATUS on every
            // 255 bytes
            if (++writeCount >= 255)
                {
                WaitOnEmptyWriteBufferStatus(pDevice);
                writeCount = 0;
                }
            
            pData += NANDREG_SIZE(pDevice);
            size -= NANDREG_SIZE(pDevice);
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
UINT16 NAND_GetStatus(HANDLE hNand)
{
    NandDevice_t *pDevice = (NandDevice_t*)hNand;
    // request status
    WRITE_NAND_CMD(pDevice, NAND_CMD_STATUS);
    return READ_NAND_DATA(pDevice);
}

//-----------------------------------------------------------------------------
BOOL NAND_EraseBlock(HANDLE hNand,BLOCK_ID blockId)
{    
    NandDevice_t *pDevice = (NandDevice_t*)hNand;
    
    NAND_SendCommand(hNand, NAND_CMD_ERASE_SETUP);    
    BlockAccess(pDevice, blockId);
    NAND_SendCommand(hNand, NAND_CMD_ERASE_CONFIRM);
    return TRUE;
}

//-----------------------------------------------------------------------------
BOOL NAND_Enable( HANDLE hNand, BOOL bEnable )
{
    NandDevice_t *pDevice = (NandDevice_t*)hNand;

    if (pDevice->IrqWait){
        if( bEnable ){
            // Enable GPMC wait-to-nowait edge detection mechanism on NAND R/B pin
            SETREG32 (&pDevice->pGpmcRegs->GPMC_IRQENABLE, pDevice->IrqWait);
            //  Reset IRQ status
            SETREG32 (&pDevice->pGpmcRegs->GPMC_IRQSTATUS, pDevice->IrqWait);
        } else {
            //  Reset IRQ status
            SETREG32 (&pDevice->pGpmcRegs->GPMC_IRQSTATUS, pDevice->IrqWait);
            // Disable GPMC wait-to-nowait edge detection mechanism on NAND R/B pin
            CLRREG32 (&pDevice->pGpmcRegs->GPMC_IRQENABLE, pDevice->IrqWait);
        }
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
UINT NAND_MutexEnter( HANDLE hNand )
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
UINT NAND_MutexExit( HANDLE hNand )
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
BOOL NAND_SendCommand( HANDLE hNand, UINT cmd )
{
    NandDevice_t *pDevice = (NandDevice_t*)hNand;
    WRITE_NAND_CMD(pDevice, cmd);
    return TRUE;
}

//-----------------------------------------------------------------------------
BOOL NAND_ConfigurePrefetch( HANDLE hNand, UINT accessType )
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

    if (accessType == NAND_DATA_WRITE){
        pDevice->prefetchMode = kPrefetchWrite;
        ffPrefetchMode |= GPMC_PREFETCH_CONFIG_WRITEPOST;
    } else {
        pDevice->prefetchMode = kPrefetchRead;
    }
        
    OUTREG32(&pDevice->pGpmcRegs->GPMC_PREFETCH_CONFIG1, ffPrefetchMode);
    SETREG32(&pDevice->pGpmcRegs->GPMC_PREFETCH_CONFIG1, GPMC_PREFETCH_CONFIG_ENABLEENGINE);
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
//    if (memcmp(pEccOld, pEccNew, pDevice->ECCsize) != 0)
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
__inline void WaitForReadyStatus( HANDLE hNand )
{
    while ((NAND_GetStatus(hNand) & NAND_STATUS_READY) == 0); 
}


//------------------------------------------------------------------------------
//  This function is called to initialize flash subsystem.
VOID* FMD_Init( LPCTSTR szContext, PCI_REG_INFO *pRegIn, PCI_REG_INFO *pRegOut )
{
    s_hNand = NAND_Initialize(szContext, pRegIn, pRegOut);
    return s_hNand;
}

//------------------------------------------------------------------------------
BOOL FMD_Deinit( VOID *pContext )
{
    BOOL rc = FALSE;

    
    if (s_hNand == NULL) return TRUE;

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

    rc = TRUE;

cleanUp:
    return rc;
}

//------------------------------------------------------------------------------
BOOL FMD_GetInfo( FlashInfo *pFlashInfo )
//  This function is call to get flash information
{
    if (s_hNand == NULL) return FALSE;

    if (pFlashInfo->dwNumBlocks == 0x89ABCDEF)  // A magic number to signify getting manuf & dev ID
    {
        pFlashInfo->dwNumBlocks = (UINT32)((NAND_GetGeometry(s_hNand)->manufacturerId << 8) | (NAND_GetGeometry(s_hNand)->deviceId ));
        return TRUE;
    }

    // Memory type is NAND
    pFlashInfo->flashType = NAND;
    pFlashInfo->dwNumBlocks = NAND_GetGeometry(s_hNand)->blocks;
    pFlashInfo->wSectorsPerBlock = (WORD) NAND_GetGeometry(s_hNand)->sectorsPerBlock;
    pFlashInfo->wDataBytesPerSector = (WORD) NAND_GetGeometry(s_hNand)->sectorSize;
    pFlashInfo->dwBytesPerBlock = NAND_GetGeometry(s_hNand)->sectorSize;
    pFlashInfo->dwBytesPerBlock *= NAND_GetGeometry(s_hNand)->sectorsPerBlock;
    return TRUE;
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
        NAND_Seek(s_hNand, sector, sectorSize, READ_FULL_PAGE);

        // Send the command
        NAND_SendCommand(s_hNand, NAND_CMD_READ2);

        // Wait for the action to finish
        WaitForReadyStatus(s_hNand);

        //Force a read here, else we will read the status again
        NAND_SendCommand (s_hNand, NAND_CMD_READ1);

        // read spare area
        NAND_Read(s_hNand, (BYTE*)pBuffer, NAND_OOB_SIZE, NULL);
        
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
BOOL FMD_ReadSector( SECTOR_ADDR sector, UCHAR *pBuffer,
					SectorInfo *pSectorInfo, DWORD sectors)
{
    BOOL rc = FALSE;
    NAND_SPARE_AREA sa;
    UINT32 oldIdleMode;
    UINT32 sectorSize;
    BYTE rgEcc[ECC_BYTES];
    NandDevice_t *pDevice;  
	
    if (s_hNand == NULL) goto cleanUp;
    pDevice= (NandDevice_t*)s_hNand;    
	
    NAND_MutexEnter(s_hNand);
    sectorSize = NAND_GetGeometry(s_hNand)->sectorSize;
    oldIdleMode = INREG32(&(NAND_GetGpmcRegs(s_hNand)->GPMC_SYSCONFIG));
    OUTREG32(&(NAND_GetGpmcRegs(s_hNand)->GPMC_SYSCONFIG), SYSCONFIG_NOIDLE);
    
    while (sectors > 0){
        NAND_Enable(s_hNand, TRUE);
        
        // Read sector from A
        if (pBuffer != NULL){
            // be sure to do this before sending the READ command and not after ! or the
            // status register would remain at status read mode, which would have to be changed
            // before retreiving the data
            WaitForReadyStatus(s_hNand);

            if (pDevice->prefetch_enable)
            {
                NAND_ConfigurePrefetch(s_hNand, NAND_DATA_READ);
            }

            NAND_SendCommand (s_hNand, NAND_CMD_READ1);
			
            NAND_Seek(s_hNand, sector, 0, READ_FULL_PAGE);
            NAND_SendCommand(s_hNand, NAND_CMD_READ2);
            WaitForReadyStatus(s_hNand);
            NAND_SendCommand (s_hNand, NAND_CMD_READ1);
            // read data & ECC from GPMC
            NAND_Read(s_hNand, pBuffer, sectorSize, rgEcc);

            // Read ECC from spare area in NAND
/*            if(pDevice->ECCtype == BCH8bitElm) 
            {
                // read spare area
                NAND_Read(s_hNand, (BYTE*)&sa, sizeof(sa), NULL);
            }
            else */
            if (pDevice->ECCtype == BCH8bitElmX)
            {
                NAND_SendCommand (s_hNand, NAND_CMD_RANDOM_READ);

                // Send the address
                NAND_Seek(s_hNand, sector, sectorSize + ECC_OFFSET, READ_RANDOM_COLUMN);
                NAND_SendCommand (s_hNand, NAND_CMD_RANDOM_READ_CONFIRM); 
               
                // read spare area

                if (pDevice->prefetch_enable)
                {
                    //sizeof(sa) - ECC_OFFSET
                    NAND_Read(s_hNand, (BYTE*)&sa.ecc,  sizeof(sa) - ECC_OFFSET, NULL);
                }
                else
                {
                   //sizeof(sa) - ECC_OFFSET
                   NAND_NoPrefetchReadECC(s_hNand, (BYTE*)&sa.ecc,  sizeof(sa) - ECC_OFFSET, NULL);  
                }

                WaitForReadyStatus(s_hNand);

				// read bad block 2 bytes
                NAND_SendCommand (s_hNand, NAND_CMD_RANDOM_READ);

                // Send the address
                NAND_Seek(s_hNand, sector, sectorSize, READ_RANDOM_COLUMN);
                NAND_SendCommand (s_hNand, NAND_CMD_RANDOM_READ_CONFIRM);

                if (IS_NAND_16BIT(pDevice))
                    *(NANDREG16_PTR(&(sa.hwBadBlock))) = READ_NAND_DATA(pDevice);
                else
                    *(NANDREG8_PTR(&(sa.hwBadBlock))) = (UINT8)READ_NAND_DATA(pDevice);
		  
            }
			else
				NAND_Read(s_hNand, (BYTE*)&sa,  sizeof(sa), NULL); 	

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
            NAND_Seek(s_hNand, sector, sectorSize, READ_FULL_PAGE);
    
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
    
        }       

        // Copy sector info
        if (pSectorInfo != NULL){
            pSectorInfo->bBadBlock    = sa.hwBadBlock[0] & sa.hwBadBlock[1];    // HW bad block check
            pSectorInfo->bOEMReserved = sa.oemReserved;
            memset (&pSectorInfo->wReserved2, 0xFF, sizeof(pSectorInfo->wReserved2) );
            memcpy(
                &pSectorInfo->wReserved2, sa.reserved2,
                sizeof(sa.reserved2)
                );
#if 0 // this change is because only 6 bytes left for sector info.			
            pSectorInfo->bBadBlock    = pSectorInfo->bBadBlock & sa.swBadBlock; // SW bad block flag check

            memcpy(
                &pSectorInfo->wReserved2, sa.reserved2,
                sizeof(pSectorInfo->wReserved2)
                );
#endif			
            memcpy(
                &pSectorInfo->dwReserved1, sa.reserved1,
                sizeof(pSectorInfo->dwReserved1)
                );

       }
        
		NAND_Enable(s_hNand, FALSE);

        ECC_Result(pDevice->pGpmcRegs, rgEcc, pDevice->ECCsize);				  

        // perform ecc correction and correct data when possible
        if ((pBuffer != NULL) && 
            (NAND_CorrectEccData(s_hNand, pBuffer, sectorSize, sa.ecc, rgEcc) == FALSE)){
            UINT count;
            UCHAR *pData = pBuffer;
            DEBUGMSG (ZONE_ERROR, (L"NAND_CorrectEccData returns FALSE, sector=%d\r\n", sector));	
            for (count = 0; count < sizeof(sa); count++){
                // Allow OEMReserved byte to be set to reserved/readonly
                if (&(((UINT8*)&sa)[count]) == &sa.oemReserved) continue;
                if (((UINT8*)&sa)[count] != 0xFF)
                {
                    goto cleanUp;
                }
            }

            for (count = 0; count < sectorSize; count++){
                if (*pData != 0xFF)
                {
                    goto cleanUp;
                }
                ++pData;
            }
        }

        // Move to next sector
        sector++;
        if (pBuffer != NULL) pBuffer += sectorSize;
        pSectorInfo++;
        sectors--;
    }

    OUTREG32(&(NAND_GetGpmcRegs(s_hNand)->GPMC_SYSCONFIG), oldIdleMode);

	
    rc = TRUE;
cleanUp:
    if (s_hNand != NULL) NAND_MutexExit(s_hNand);
    ASSERT(rc);
    return rc;
}

//------------------------------------------------------------------------------
BOOL FMD_WriteSector( SECTOR_ADDR sector, UCHAR *pBuffer,
    SectorInfo *pSectorInfo, DWORD sectors )
{
    BOOL rc = FALSE;
    NAND_SPARE_AREA sa;
    UINT32 oldIdleMode;
    UINT32 sectorSize;
    NandDevice_t *pDevice = (NandDevice_t*)s_hNand;
	
    if (s_hNand == NULL) goto cleanUp;
    NAND_MutexEnter(s_hNand);

    //  Change idle mode to no-idle to ensure access to GPMC registers
    sectorSize = NAND_GetGeometry(s_hNand)->sectorSize;
    oldIdleMode = INREG32(&(NAND_GetGpmcRegs(s_hNand)->GPMC_SYSCONFIG));
    OUTREG32(&(NAND_GetGpmcRegs(s_hNand)->GPMC_SYSCONFIG), SYSCONFIG_NOIDLE);

    NAND_Enable(s_hNand, TRUE);
    NAND_LockBlocks(s_hNand, FALSE);
    
    while (sectors > 0){
        memset(&sa, 0xFF, sizeof(NAND_SPARE_AREA)); // Clear out spare area struct
        if (pBuffer != NULL) {// When there is buffer write data

            if (pDevice->prefetch_enable)
            {
                NAND_ConfigurePrefetch(s_hNand, NAND_DATA_WRITE);  // enable prefetch
            }

            NAND_SendCommand(s_hNand, NAND_CMD_WRITE1);        // send the write command
            NAND_Seek(s_hNand, sector, 0, READ_FULL_PAGE);                     // send the address to write to
            NAND_Write(s_hNand, pBuffer, sectorSize, sa.ecc);        
			
        } else {
            NAND_SendCommand(s_hNand, NAND_CMD_READ1);
            NAND_Seek(s_hNand, sector, NAND_GetGeometry(s_hNand)->sectorSize, READ_FULL_PAGE); // Send the address
            NAND_SendCommand(s_hNand, NAND_CMD_READ2);
            WaitForReadyStatus(s_hNand);
            NAND_SendCommand (s_hNand, NAND_CMD_READ1);//Force a read here, else we will read the status again
            NAND_Read(s_hNand, (BYTE*)&sa, sizeof(sa), NULL); 
        
            NAND_SendCommand(s_hNand, NAND_CMD_WRITE1);
            NAND_Seek(s_hNand, sector, sectorSize, READ_FULL_PAGE);//send the address to write to
    	}		

        /* used for test purpose */ 
        if((pSectorInfo != NULL) && (pSectorInfo->bOEMReserved == SKIP_ECC_WRITE_MAGIC_NUMBER) )
        {
            /* skip updating ECC */
            ECC_Reset(pDevice->pGpmcRegs);			
			goto  skip_ecc; 	
        }

        if (pSectorInfo != NULL){
            // Fill in rest of spare area info (we already have ECC from above)
//            sa.swBadBlock     = pSectorInfo->bBadBlock;
            memcpy(sa.reserved1, &pSectorInfo->dwReserved1, sizeof(sa.reserved1));
            memcpy(sa.reserved2, &pSectorInfo->wReserved2, sizeof(sa.reserved2));
            sa.oemReserved  = pSectorInfo->bOEMReserved;
        }
		
        NAND_Write(s_hNand, (BYTE*)&sa, sizeof(sa), NULL);

skip_ecc:
        NAND_SendCommand(s_hNand, NAND_CMD_WRITE2);// initiate the data programming process :
        WaitForReadyStatus(s_hNand);

        if ((NAND_GetStatus(s_hNand) & NAND_STATUS_ERROR) != 0)
            break;

        // Move to next sector
        sector++;
        if (pBuffer != NULL) pBuffer += sectorSize;
        pSectorInfo++;
        sectors--;
    }

    NAND_LockBlocks(s_hNand, TRUE);
    NAND_Enable(s_hNand, FALSE);
    OUTREG32(&(NAND_GetGpmcRegs(s_hNand)->GPMC_SYSCONFIG), oldIdleMode);
    
    rc = (sectors == 0); // All is ok, when we read all sectors

cleanUp:
    if (s_hNand != NULL) NAND_MutexExit(s_hNand);
    ASSERT(rc);
    return rc;
}

//------------------------------------------------------------------------------
BOOL FMD_EraseBlock( BLOCK_ID blockId )
{
    BOOL rc = FALSE;
    UINT32 oldIdleMode;

    if (s_hNand == NULL) return rc;
    NAND_MutexEnter(s_hNand);

    oldIdleMode = INREG32(&(NAND_GetGpmcRegs(s_hNand)->GPMC_SYSCONFIG));
    OUTREG32(&(NAND_GetGpmcRegs(s_hNand)->GPMC_SYSCONFIG), SYSCONFIG_NOIDLE);

    NAND_Enable(s_hNand, TRUE);
    NAND_LockBlocks(s_hNand, FALSE);
    NAND_EraseBlock(s_hNand, blockId);
    WaitForReadyStatus(s_hNand);

    //Verify there wasn't any error by checking the NAND status register :
	rc = ((NAND_GetStatus(s_hNand) & NAND_STATUS_ERROR) != 0) ? FALSE : TRUE;

    NAND_LockBlocks(s_hNand, TRUE);
    NAND_Enable(s_hNand, FALSE);

    //  Change idle mode back
    OUTREG32(&(NAND_GetGpmcRegs(s_hNand)->GPMC_SYSCONFIG), oldIdleMode);

    if (s_hNand != NULL) NAND_MutexExit(s_hNand);

    return rc;
}

//------------------------------------------------------------------------------
DWORD FMD_GetBlockStatus( BLOCK_ID blockId )
{
    DWORD rc = 0;
    SECTOR_ADDR sector;
    SectorInfo sectorInfo[2];

    if (s_hNand == NULL) 
		return 0;

    sector = blockId * NAND_GetGeometry(s_hNand)->sectorsPerBlock;

    if (!FMD_ReadSector(sector, NULL, sectorInfo, 2))
		return BLOCK_STATUS_UNKNOWN;	

    if ((sectorInfo[0].bBadBlock != 0xFF) || (sectorInfo[1].bBadBlock != 0xFF))
        rc |= BLOCK_STATUS_BAD;

    if ((sectorInfo[0].bOEMReserved & OEM_BLOCK_READONLY) == 0)
        rc |= BLOCK_STATUS_READONLY;

    if ((sectorInfo[0].bOEMReserved & OEM_BLOCK_RESERVED) == 0)
        rc |= BLOCK_STATUS_RESERVED;

    return rc;
}

//------------------------------------------------------------------------------
BOOL FMD_SetBlockStatus( BLOCK_ID blockId, DWORD status )
{
    BOOL rc = FALSE;
    SECTOR_ADDR sector;
    SectorInfo sectorInfo;

    if (s_hNand == NULL) goto cleanUp;

    // Calculate sector
    sector = blockId * NAND_GetGeometry(s_hNand)->sectorsPerBlock;
    if ((status & BLOCK_STATUS_BAD) != 0) {
        if (!FMD_ReadSector(sector, NULL, &sectorInfo, 1)) goto cleanUp;
        sectorInfo.bBadBlock = 0;
        // Complete the write (no erase, we changed 0xFF -> 0x00)
        if (!FMD_WriteSector(sector, NULL, &sectorInfo, 1)) goto cleanUp;
    }
#ifdef BOOT_MODE
    if ( ((status & BLOCK_STATUS_READONLY) != 0) || ((status & BLOCK_STATUS_RESERVED) != 0) ){
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
VOID FMD_PowerUp( )
{
    if (s_hNand == NULL) return;
    NAND_MutexEnter(s_hNand);
    NAND_Enable(s_hNand, TRUE);
    WaitForReadyStatus(s_hNand);   
    NAND_SendCommand(s_hNand, NAND_CMD_RESET);
    WaitForReadyStatus(s_hNand);
    NAND_Enable(s_hNand, FALSE);
    NAND_MutexExit(s_hNand);
}

//------------------------------------------------------------------------------
VOID FMD_PowerDown( )
{
    if (s_hNand == NULL) return;   // exit if FMD wasn't opened 
    NAND_MutexEnter(s_hNand);
    NAND_Enable(s_hNand, TRUE);    //  Only enable during NAND read/write/erase operations
    WaitForReadyStatus(s_hNand);   // Wait for NAND   
    NAND_SendCommand(s_hNand, NAND_CMD_RESET); // Write the reset command
    WaitForReadyStatus(s_hNand);
    NAND_Enable(s_hNand, FALSE);
    NAND_MutexExit(s_hNand);
}

//------------------------------------------------------------------------------
BOOL FMD_OEMIoControl( DWORD code, UCHAR *pInBuffer, DWORD inSize, UCHAR *pOutBuffer,
    DWORD outSize, DWORD *pOutSize )
{
    UNREFERENCED_PARAMETER(code);
    UNREFERENCED_PARAMETER(pInBuffer);
    UNREFERENCED_PARAMETER(inSize);
    UNREFERENCED_PARAMETER(pOutBuffer);
    UNREFERENCED_PARAMETER(outSize);
    UNREFERENCED_PARAMETER(pOutSize);
    
    return FALSE;
}

#if 0
//------------------------------------------------------------------------------
BOOL FMD_WriteSectorOOB( 
    SECTOR_ADDR sector, 
    UCHAR       *pBuffer,
    SectorInfo  *pSectorInfo, 
    DWORD       sectors )
{
    BOOL rc = FALSE;
    NAND_SPARE_AREA sa;
    UINT32 oldIdleMode;
    UINT32 sectorSize;
//    NandDevice_t *pDevice = (NandDevice_t*)s_hNand;
    UCHAR *psa;
	
    if (s_hNand == NULL) goto cleanUp;
    NAND_MutexEnter(s_hNand);

    //  Change idle mode to no-idle to ensure access to GPMC registers
    sectorSize = NAND_GetGeometry(s_hNand)->sectorSize;
    oldIdleMode = INREG32(&(NAND_GetGpmcRegs(s_hNand)->GPMC_SYSCONFIG));
    OUTREG32(&(NAND_GetGpmcRegs(s_hNand)->GPMC_SYSCONFIG), SYSCONFIG_NOIDLE);

    NAND_Enable(s_hNand, TRUE);
    NAND_LockBlocks(s_hNand, FALSE);
    
    while (sectors > 0)
    {
        memset(&sa, 0xFF, sizeof(NAND_SPARE_AREA)); // Clear out spare area struct

        if (pBuffer != NULL) // When there is buffer write oob data
        {
            // avoid 2 bad block bytes
            psa = (UCHAR *)&sa;
            memcpy((psa+2), (pBuffer+2), sizeof(NAND_SPARE_AREA)-2);
        }

        NAND_SendCommand(s_hNand, NAND_CMD_WRITE1);
        NAND_Seek(s_hNand, sector, sectorSize, READ_FULL_PAGE);//send the address to write to

        if (pSectorInfo != NULL)
        {
            // Fill in rest of spare area info (we already have ECC from above)
//            sa.swBadBlock     = pSectorInfo->bBadBlock;
            memcpy(sa.reserved1, &pSectorInfo->dwReserved1, sizeof(sa.reserved1));
            memcpy(sa.reserved2, &pSectorInfo->wReserved2, sizeof(sa.reserved2));
            sa.oemReserved  = pSectorInfo->bOEMReserved;
        }
		
        NAND_Write(s_hNand, (BYTE*)&sa, sizeof(sa), NULL);

//skip_ecc:
        NAND_SendCommand(s_hNand, NAND_CMD_WRITE2);// initiate the data programming process :
        WaitForReadyStatus(s_hNand);

        if ((NAND_GetStatus(s_hNand) & NAND_STATUS_ERROR) != 0)
            break;

        // Move to next sector
        sector++;
        if (pBuffer != NULL) pBuffer += sectorSize;
        pSectorInfo++;
        sectors--;
    }

    NAND_LockBlocks(s_hNand, TRUE);
    NAND_Enable(s_hNand, FALSE);
    OUTREG32(&(NAND_GetGpmcRegs(s_hNand)->GPMC_SYSCONFIG), oldIdleMode);
    
    rc = (sectors == 0); // All is ok, when we read all sectors
cleanUp:
    if (s_hNand != NULL) NAND_MutexExit(s_hNand);
    ASSERT(rc);
    return rc;
}

//------------------------------------------------------------------------------
BOOL FMD_ReadSectorNoECC( SECTOR_ADDR sector, UCHAR *pBuffer,
					SectorInfo *pSectorInfo, DWORD sectors)
{
    BOOL rc = FALSE;
    NAND_SPARE_AREA sa;
    UINT32 oldIdleMode;
    UINT32 sectorSize;
    BYTE rgEcc[ECC_BYTES];
    NandDevice_t *pDevice;  
	
    if (s_hNand == NULL) goto cleanUp;
    pDevice= (NandDevice_t*)s_hNand;    
	
    NAND_MutexEnter(s_hNand);
    sectorSize = NAND_GetGeometry(s_hNand)->sectorSize;
    oldIdleMode = INREG32(&(NAND_GetGpmcRegs(s_hNand)->GPMC_SYSCONFIG));
    OUTREG32(&(NAND_GetGpmcRegs(s_hNand)->GPMC_SYSCONFIG), SYSCONFIG_NOIDLE);
    
    while (sectors > 0){
        NAND_Enable(s_hNand, TRUE);
        
        // Read sector from A
        if (pBuffer != NULL){
            // be sure to do this before sending the READ command and not after ! or the
            // status register would remain at status read mode, which would have to be changed
            // before retreiving the data
            WaitForReadyStatus(s_hNand);

            if (pDevice->prefetch_enable)
            {
                NAND_ConfigurePrefetch(s_hNand, NAND_DATA_READ);
            }

            NAND_SendCommand (s_hNand, NAND_CMD_READ1);
			
            NAND_Seek(s_hNand, sector, 0, READ_FULL_PAGE);
            NAND_SendCommand(s_hNand, NAND_CMD_READ2);
            WaitForReadyStatus(s_hNand);
            NAND_SendCommand (s_hNand, NAND_CMD_READ1);
            // read data
            NAND_Read(s_hNand, pBuffer, sectorSize, rgEcc);

            if(pDevice->ECCtype == BCH8bitElmX)
				{
                NAND_SendCommand (s_hNand, NAND_CMD_RANDOM_READ);

                // Send the address
                NAND_Seek(s_hNand, sector, sectorSize + ECC_OFFSET, READ_RANDOM_COLUMN);
                NAND_SendCommand (s_hNand, NAND_CMD_RANDOM_READ_CONFIRM);
               
                // read spare area
                NAND_Read(s_hNand, (BYTE*)&sa.ecc,  sizeof(sa) - ECC_OFFSET, NULL);  //sizeof(sa) - ECC_OFFSET
                WaitForReadyStatus(s_hNand);

				// read bad block 2 bytes
                NAND_SendCommand (s_hNand, NAND_CMD_RANDOM_READ);

                // Send the address
                NAND_Seek(s_hNand, sector, sectorSize, READ_RANDOM_COLUMN);
                NAND_SendCommand (s_hNand, NAND_CMD_RANDOM_READ_CONFIRM);

                if (IS_NAND_16BIT(pDevice))
                    *(NANDREG16_PTR(&(sa.hwBadBlock))) = READ_NAND_DATA(pDevice);
                else
                    *(NANDREG8_PTR(&(sa.hwBadBlock))) = (UINT8)READ_NAND_DATA(pDevice);

				}
			else
				NAND_Read(s_hNand, (BYTE*)&sa,  sizeof(sa), NULL); 	

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
            NAND_Seek(s_hNand, sector, sectorSize, READ_FULL_PAGE);
    
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
    
        }        

		// Copy sector info
        if (pSectorInfo != NULL){
            pSectorInfo->bBadBlock    = sa.hwBadBlock[0] & sa.hwBadBlock[1];    // HW bad block check
            pSectorInfo->bOEMReserved = sa.oemReserved;
            memset (&pSectorInfo->wReserved2, 0xFF, sizeof(pSectorInfo->wReserved2) );
            memcpy(
                &pSectorInfo->wReserved2, sa.reserved2,
                sizeof(sa.reserved2)
                );
#if 0 // this change is because only 6 bytes left for sector info.			
            pSectorInfo->bBadBlock    = pSectorInfo->bBadBlock & sa.swBadBlock; // SW bad block flag check

            memcpy(
                &pSectorInfo->wReserved2, sa.reserved2,
                sizeof(pSectorInfo->wReserved2)
                );
#endif			
            memcpy(
                &pSectorInfo->dwReserved1, sa.reserved1,
                sizeof(pSectorInfo->dwReserved1)
                );

		}
        
		NAND_Enable(s_hNand, FALSE);

		ECC_Result(pDevice->pGpmcRegs, rgEcc, pDevice->ECCsize);				  

        // perform ecc correction and correct data when possible
        if ((pBuffer != NULL) && 
            (NAND_CorrectEccData(s_hNand, pBuffer, sectorSize, sa.ecc, rgEcc) == FALSE)){
            UINT count;
            UCHAR *pData = pBuffer;
            DEBUGMSG (ZONE_ERROR, (L"NAND_CorrectEccData returns FALSE, sector=%d\r\n", sector));	
            for (count = 0; count < sizeof(sa); count++){
                // Allow OEMReserved byte to be set to reserved/readonly
                if (&(((UINT8*)&sa)[count]) == &sa.oemReserved) continue;
                if (((UINT8*)&sa)[count] != 0xFF) goto cleanUp;
            }

            for (count = 0; count < sectorSize; count++){
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

    OUTREG32(&(NAND_GetGpmcRegs(s_hNand)->GPMC_SYSCONFIG), oldIdleMode);

	
    rc = TRUE;
cleanUp:
    if (s_hNand != NULL) NAND_MutexExit(s_hNand);
    ASSERT(rc);
    return rc;
}
#endif


void FMD_ShowBytes(UINT8 *pBytes, UINT32 size)
{
#if 0
{
    UINT8 *psa = (UINT8*)pBytes;
    UINT32 show_rows = 0;
    UINT32 ii = 0;
    UINT32 last_row_size = 0;

    if (!psa) return;

    show_rows = size / 16;

    last_row_size = size - (show_rows * 16);

    RETAILMSG(1,(L"size=%d\r\n", size));
    for (ii=0; ii < show_rows; ii++, psa+=16)
    {
        RETAILMSG(1, (L"%02x %02x %02x %02x %02x %02x %02x %02x  %02x %02x %02x %02x %02x %02x %02x %02x\r\n", 
         *(psa), *(psa+1), *(psa+2), *(psa+3), *(psa+4), *(psa+5), *(psa+6), *(psa+7),
         *(psa+8), *(psa+9), *(psa+10), *(psa+11), *(psa+12), *(psa+13), *(psa+14), *(psa+15)
        ));
    }

    if (last_row_size == 15)
    {
        RETAILMSG(1, (L"%02x %02x %02x %02x %02x %02x %02x %02x  %02x %02x %02x %02x %02x %02x %02x\r\n", 
         *(psa), *(psa+1), *(psa+2), *(psa+3), *(psa+4), *(psa+5), *(psa+6), *(psa+7),
         *(psa+8), *(psa+9), *(psa+10), *(psa+11), *(psa+12), *(psa+13), *(psa+14)
        ));
    }
    else if (last_row_size == 14)
    {
        RETAILMSG(1, (L"%02x %02x %02x %02x %02x %02x %02x %02x  %02x %02x %02x %02x %02x %02x \r\n",
         *(psa), *(psa+1), *(psa+2), *(psa+3), *(psa+4), *(psa+5), *(psa+6), *(psa+7),
         *(psa+8), *(psa+9), *(psa+10), *(psa+11), *(psa+12), *(psa+13)
        ));
    }
    else if (last_row_size == 13)
    {
        RETAILMSG(1, (L"%02x %02x %02x %02x %02x %02x %02x %02x  %02x %02x %02x %02x %02x \r\n",
         *(psa), *(psa+1), *(psa+2), *(psa+3), *(psa+4), *(psa+5), *(psa+6), *(psa+7),
         *(psa+8), *(psa+9), *(psa+10), *(psa+11), *(psa+12)
        ));
    }
    else if (last_row_size == 12)
    {
        RETAILMSG(1, (L"%02x %02x %02x %02x %02x %02x %02x %02x  %02x %02x %02x %02x \r\n",
         *(psa), *(psa+1), *(psa+2), *(psa+3), *(psa+4), *(psa+5), *(psa+6), *(psa+7),
         *(psa+8), *(psa+9), *(psa+10), *(psa+11)
        ));
    }
    else if (last_row_size == 11)
    {
        RETAILMSG(1, (L"%02x %02x %02x %02x %02x %02x %02x %02x  %02x %02x %02x \r\n",
         *(psa), *(psa+1), *(psa+2), *(psa+3), *(psa+4), *(psa+5), *(psa+6), *(psa+7),
         *(psa+8), *(psa+9), *(psa+10)
        ));
    }
    else if (last_row_size == 10)
    {
        RETAILMSG(1, (L"%02x %02x %02x %02x %02x %02x %02x %02x  %02x %02x \r\n",
         *(psa), *(psa+1), *(psa+2), *(psa+3), *(psa+4), *(psa+5), *(psa+6), *(psa+7),
         *(psa+8), *(psa+9)
        ));
    }
    else if (last_row_size == 9)
    {
        RETAILMSG(1, (L"%02x %02x %02x %02x %02x %02x %02x %02x  %02x \r\n",
         *(psa), *(psa+1), *(psa+2), *(psa+3), *(psa+4), *(psa+5), *(psa+6), *(psa+7),
         *(psa+8)
        ));
    }
    else if (last_row_size == 8)
    {
        RETAILMSG(1, (L"%02x %02x %02x %02x %02x %02x %02x %02x \r\n", 
         *(psa), *(psa+1), *(psa+2), *(psa+3), *(psa+4), *(psa+5), *(psa+6), *(psa+7)
        ));
    }
    else if (last_row_size == 7)
    {
        RETAILMSG(1, (L"%02x %02x %02x %02x %02x %02x %02x \r\n", 
         *(psa), *(psa+1), *(psa+2), *(psa+3), *(psa+4), *(psa+5), *(psa+6)
        ));
    }
    else if (last_row_size == 6)
    {
        RETAILMSG(1, (L"%02x %02x %02x %02x %02x %02x \r\n", 
         *(psa), *(psa+1), *(psa+2), *(psa+3), *(psa+4), *(psa+5)
        ));
    }
    else if (last_row_size == 5)
    {
        RETAILMSG(1, (L"%02x %02x %02x %02x %02x \r\n", 
         *(psa), *(psa+1), *(psa+2), *(psa+3), *(psa+4)
        ));
    }
    else if (last_row_size == 4)
    {
        RETAILMSG(1, (L"%02x %02x %02x %02x \r\n", 
         *(psa), *(psa+1), *(psa+2), *(psa+3)
        ));
    }
    else if (last_row_size == 3)
    {
        RETAILMSG(1, (L"%02x %02x %02x \r\n",
         *(psa), *(psa+1), *(psa+2)
        ));
    }
    else if (last_row_size == 2)
    {
        RETAILMSG(1, (L"%02x %02x \r\n",
         *(psa), *(psa+1)
        ));
    }
    else if (last_row_size == 1)
    {
        RETAILMSG(1, (L"%02x \r\n",
         *(psa)
        ));
    }

}
#endif
}

