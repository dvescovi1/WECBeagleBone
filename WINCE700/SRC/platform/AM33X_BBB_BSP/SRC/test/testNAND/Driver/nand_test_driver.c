/*
 * Copyright (c) 2011 Texas Instruments Incorporated http://www.ti.tom/
 * ALL RIGHTS RESERVED
 * 
 * File:  nand_test_driver.c
 *
 */

#include <nandtest.h>
#pragma warning(push)
#pragma warning(disable: 4115 4214)
#include <fmd.h>
#include "bsp.h"
#include "bsp_base_regs.h"
#pragma warning(pop)

//------------------------------------------------------------------------------
//  Local Definitions
//
#define OMAP_GPMC_REGS_PA                       0x6E000000

#define SKIP_ECC_WRITE_MAGIC_NUMBER    0x0f
#define SECTOR_SIZE 2048

//------------------------------------------------------------------------------
//  Local Functions
//


//------------------------------------------------------------------------------
//  Debug Zones
//
#ifdef DEBUG

#define ZONE_FUNCTION       DEBUGZONE(2)

DBGPARAM dpCurSettings = {
    L"NANDTEST", {
        L"Errors",      L"Warnings",    L"Function",    L"Test",
        L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined",
        L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined",
        L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined"
    },
    0x000f
};

#endif


typedef struct
{
    HANDLE hFMD;
    DWORD test_sector;
} NANDTEST_CONTEXT;

void GpmcEnable(BOOL on)
{
    RETAILMSG(1,(L" >>>>> nand_test_driver: enable/disable gpmc and nand, on=%d\r\n", on));
    return;
}


BOOL nand_ecc_test( DWORD Context,  UCHAR *pInBuffer, UCHAR *p_outbuf)
{
    NANDTEST_CONTEXT *pContext = (NANDTEST_CONTEXT *)Context;
    BOOL rc = FALSE;
    DWORD block;
    FlashInfo flashInfo;
    UCHAR outBuf[SECTOR_SIZE];
    SectorInfo secInfo;
    UCHAR *p_gooddata, *p_baddata;
	
    /* NAND ECC test
         sector: pContext.test_sector
         data: p_gooddata, p_baddata
    */
    p_gooddata = pInBuffer;
    p_baddata = pInBuffer + SECTOR_SIZE;
	
    DEBUGMSG(ZONE_FUNCTION, (L"+nand_ecc_test: p_gooddata=%x, p_baddata=%x\r\n", p_gooddata, p_baddata));

    /* Step 0: Erase block */
    if (!FMD_GetInfo(&flashInfo))
    {
        DEBUGMSG(ZONE_ERROR,  (L"ERROR: FMD_GetInfo call failed!\r\n"));
        goto cleanUp;
    }
	
    block  = pContext->test_sector / flashInfo.wSectorsPerBlock;
	
    rc = FMD_EraseBlock(block);
    if(!rc)
    {
        DEBUGMSG(ZONE_ERROR,  (L"ERROR: nand_ecc_test ERASE block(%d) failed!\r\n", block));
        goto cleanUp;
    }
    /* Step 1: flash the NAND with Good data */
    rc = FMD_WriteSector(pContext->test_sector, p_gooddata, NULL, 1); 
    if( !rc )
    {
        DEBUGMSG(ZONE_ERROR,  (L"ERROR: nand_ecc_test WRITE good data in sector(%d) failed!\r\n", pContext->test_sector));
        goto cleanUp;
    }
	
    /* Step 2: read back to see if there are errors */
    rc = FMD_ReadSector(pContext->test_sector, outBuf, NULL, 1); 

    if( !rc )
    {
        DEBUGMSG(ZONE_ERROR,  (L"ERROR: nand_ecc_test READ good data in sector(%d) failed!\r\n", pContext->test_sector));
        goto cleanUp;
    }
    if(memcmp(outBuf, p_gooddata, SECTOR_SIZE))
    {
        DEBUGMSG(ZONE_ERROR,  (L"ERROR: nand_ecc_test READ data does not match good data in sector(%d) failed!\r\n", pContext->test_sector));
        goto cleanUp;
    }

    /* Step 3: flash the NAND with Bad data WITHOUT UPDATING ecc*/
    memset (&secInfo, 0, sizeof(secInfo));
    secInfo.bOEMReserved = SKIP_ECC_WRITE_MAGIC_NUMBER;
    rc = FMD_WriteSector(pContext->test_sector, p_baddata, &secInfo, 1); 
    if( !rc )
    {
        DEBUGMSG(ZONE_ERROR,  (L"ERROR: nand_ecc_test WRITE bad data in sector(%d) failed!\r\n", pContext->test_sector));
        goto cleanUp;
    }
	
    /* Step 4: read back to see if there are errors */
    rc = FMD_ReadSector(pContext->test_sector, outBuf, NULL, 1); 
    if( !rc )
    {
        DEBUGMSG(ZONE_ERROR,  (L"ERROR: nand_ecc_test READ bad data in sector(%d) failed!\r\n", pContext->test_sector));
        goto cleanUp; 
    }
	
    if(memcmp(outBuf, p_gooddata, SECTOR_SIZE))
    {
        RETAILMSG(TRUE,  (L"ERROR: nand_ecc_test READ data after correction does not match good data in sector(%d) failed!\r\n",
			pContext->test_sector));
        goto cleanUp;
    }
    rc = TRUE;
	
cleanUp:
    memcpy(p_outbuf, outBuf, SECTOR_SIZE );
	
    return rc;
}
//------------------------------------------------------------------------------
//  Function:  DFT_Init
//
DWORD DFT_Init(LPCTSTR szContext, LPCVOID pBusContext)
{
    NANDTEST_CONTEXT *pContext = NULL;
    UNREFERENCED_PARAMETER(szContext); 
    UNREFERENCED_PARAMETER(pBusContext); 

    DEBUGMSG(ZONE_FUNCTION, (L"DFT_Init(0x%08x, 0x%08x)\r\n", szContext, pBusContext));

    pContext = (NANDTEST_CONTEXT *)LocalAlloc(LPTR, sizeof(NANDTEST_CONTEXT));
    if (pContext == NULL)
    {
        DEBUGMSG(ZONE_ERROR,
                 (L"DFT_Init: ERROR - Failed allocate DFT context structure\r\n"));
    }
    else
    {
	 pContext->test_sector = 1280;
	 pContext->hFMD = NULL;	 
    }
	
    return (DWORD)pContext;
}

//------------------------------------------------------------------------------
//  Function:  DFT_Deinit
//
BOOL DFT_Deinit(DWORD context)
{
    NANDTEST_CONTEXT *pContext = (NANDTEST_CONTEXT *)context;

    DEBUGMSG(ZONE_FUNCTION, (L"DFT_Deinit(0x%08x)\r\n", context));

    if (pContext)
    {
	 pContext->hFMD = NULL;	 
        LocalFree(pContext);
    }

    return TRUE;
}

//------------------------------------------------------------------------------
//  Function:  DFT_Open
//
DWORD DFT_Open(DWORD context, DWORD accessCode, DWORD shareMode)
{
    NANDTEST_CONTEXT *pContext = (NANDTEST_CONTEXT *)context;
    UNREFERENCED_PARAMETER(accessCode); 
    UNREFERENCED_PARAMETER(shareMode); 

    DEBUGMSG(ZONE_FUNCTION, (L"DFT_Open(0x%08x, 0x%08x, 0x%08x)\r\n",
                             context, accessCode, shareMode));

    return (DWORD)pContext;
}

//------------------------------------------------------------------------------
//  Function:  DFT_Close
//
BOOL DFT_Close(DWORD context)
{

    UNREFERENCED_PARAMETER(context); 

    return TRUE;
}


//------------------------------------------------------------------------------
//
//  Function:  DFT_IOControl
//
//  This function sends a command to a device.
//
BOOL DFT_IOControl(
    DWORD context, DWORD dwCode,
    BYTE *pInBuffer, DWORD inSize,
    BYTE *pOutBuffer, DWORD outSize, DWORD *pOutSize)
{
    NANDTEST_CONTEXT *pContext = (NANDTEST_CONTEXT *)context;
    HANDLE hFMD;
    PCI_REG_INFO regInfo;
    FlashInfo flashInfo;
    DWORD sector,block; 
    BOOL rc=FALSE;
    UCHAR p_goodata[SECTOR_SIZE*2];

    UNREFERENCED_PARAMETER(pOutSize); 

    DEBUGMSG(ZONE_FUNCTION,
             (L"+DFT_IOControl(0x%08x, 0x%08x, 0x%08x, %d, 0x%08x, %d, 0x%08x)\r\n",
              context, dwCode, pInBuffer, inSize, pOutBuffer, outSize, pOutSize));

    regInfo.MemBase.Reg[0] = SOCGetGPMCAddress(0);
    regInfo.MemLen.Reg[0] = 0x00001000;
    regInfo.MemBase.Reg[1] = BSP_NAND_REGS_PA;
    regInfo.MemLen.Reg[1] = 0x00001000;
	
    hFMD  = FMD_Init(NULL, &regInfo, NULL);
    if (hFMD  == NULL)
    {
        DEBUGMSG(ZONE_ERROR, (L"ERROR: FMD_Init call failed!\r\n"));
        goto cleanUp;
    }

    switch (dwCode)
    {
        case NAND_SET_SECTOR:
            // Checking parameter
            if (pInBuffer == NULL || inSize < sizeof(DWORD))
            {
                DEBUGMSG(ZONE_ERROR, (L"+DFT_IOControl(%d): ERROR - invalid parameters\r\n", dwCode));
                goto cleanUp;
            }
            // Get flash info
            if (!FMD_GetInfo(&flashInfo))
            {
                DEBUGMSG(ZONE_ERROR,  (L"ERROR: FMD_GetInfo call failed!\r\n"));
                goto cleanUp;
            }

            pContext->test_sector = *(DWORD*)pInBuffer;
            block  =     pContext->test_sector / flashInfo.wSectorsPerBlock;
            sector = 	pContext->test_sector % flashInfo.wSectorsPerBlock;

            DEBUGMSG(ZONE_ERROR,  (L"Test section info: block=%d sector=%d !\r\n", block, sector));

	     if(block >= flashInfo.dwNumBlocks)
            {
                DEBUGMSG(ZONE_ERROR,  (L"ERROR: invalid sector!\r\n"));
                goto cleanUp;
            }
            if(FMD_GetBlockStatus(block) & 
				(BLOCK_STATUS_BAD |BLOCK_STATUS_RESERVED |BLOCK_STATUS_READONLY))		

            {
                DEBUGMSG(ZONE_ERROR,  (L"ERROR: invalid sector status:%x!\r\n", FMD_GetBlockStatus(block) ));
                goto cleanUp;
            }
			
            rc = TRUE;
			
	     break;
			
        case NAND_ECC_CORRECTION:
            /* Expecting 2 * 2K data, first 2K is good data, 
			                             second 2K is bad data needs correction */
            // Checking parameter
            if (pInBuffer == NULL || inSize != SECTOR_SIZE *2)
            {
                DEBUGMSG(ZONE_ERROR, (L"+DFT_IOControl(%d): ERROR - invalid in parameters\r\n", dwCode));
                goto cleanUp;
            }
            memcpy(p_goodata, pInBuffer, SECTOR_SIZE * 2);
			
            if (pOutBuffer == NULL || outSize != SECTOR_SIZE)
            {
                DEBUGMSG(ZONE_ERROR, (L"+DFT_IOControl(%d): ERROR - invalid out parameters\r\n", dwCode));
                goto cleanUp;
            }

            /* Call ECC test routine */
            rc = nand_ecc_test(context, p_goodata, pOutBuffer);
	     break;
			
	 default:
	     break;
    }
	
cleanUp:
    if (hFMD  != NULL) FMD_Deinit(hFMD);
	
    return rc;
}

//------------------------------------------------------------------------------
