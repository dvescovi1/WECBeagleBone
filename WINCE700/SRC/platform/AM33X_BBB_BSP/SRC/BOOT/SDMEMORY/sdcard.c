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
//  File:  sdcard.c
//
//  This file implements bootloader functions related to sdcard download
//  

#include <SDCardDDK.h>
#include <diskio.h>
#include <blcommon.h>
#include "SDHCD.h"
#include "fileio.h"
#include "oal.h"
#include "oal_alloc.h"

#include "mmcdisk.h"
#include "sdhc.h"

// set to true to enable more OALMSGs
// Caution: leave FALSE for XLOADER builds to keep size down
#define OALMSG_ENABLE   FALSE
//#define OALMSG_ENABLE   TRUE

#ifndef OAL_FUNC
#define OAL_FUNC 1
#endif

#ifndef OAL_INFO
#define OAL_INFO 1
#endif

#ifndef OAL_ERROR
#define OAL_ERROR 1
#endif

#if OALMSG_ENABLE
    #define OALMSGX(z, m)   OALMSG(z, m)
#else
    #define OALMSGX(z, m)   {}
#endif
    
S_FILEIO_OPERATIONS fileio_ops;
FILEHANDLE   File;
PFILEHANDLE   pFile;
DISK Disk;

struct MMC_command MMCcmd;


SD_BUS_REQUEST Request;

// instantiate command to response lookup tables Normal commands
static const BYTE CommandToResponse[] = RESPONSE_TABLE;
static const BYTE CommandToTransferClass[] = TRANSFER_CLASS_TABLE;

// Alternate commands (previous command was CMD55)
static const BYTE AlternateCommandToResponse[] = ALT_RESPONSE_TABLE;
static const BYTE AlternateCommandToTransferClass[] = ALT_TRANSFER_CLASS_TABLE;

static BOOL bAlternateCommandMode = FALSE;

#define MMCREAD_SUCCESS     0
#define MMCREAD_FAILURE     1

// forward declarations
DWORD MMCSelectCard(PDISK pDisk);
unsigned int MMCCommandResponse(struct MMC_command * pMMCcmd, int init);
DWORD MMCSetBlockLen(PDISK pDisk);


//----------------------------------------------------------------------------
//
//  __zeromemory
//
void __zeromemory(void *pdst, size_t size)
{
    UINT8 *pch = (UINT8*)pdst;
    while (size-- > 0) *pch++ = 0;
}


//==============================================================================
// INTF_xxx functions
//==============================================================================

static SD_API_STATUS WaitForCommandResult(SD_BUS_REQUEST * pRequest)
{
    SD_API_STATUS ResultCode = SD_API_STATUS_PENDING;

    while (ResultCode == SD_API_STATUS_PENDING)
    {
        ResultCode = SdhcControllerIstThread(pRequest);
    }

    return ResultCode;
}

//----------------------------------------------------------------------------
//
// INTF_MMCSendCommand()
//
//----------------------------------------------------------------------------

#define CONVERT_TC(tc)                    ( \
    (tc) == MMC_TC_COMMAND ? SD_COMMAND : ( \
    (tc) == MMC_TC_READ ? SD_READ :       ( \
    SD_WRITE)))

#define CONVERT_RESPONSE_TYPE(rt)            ( \
    (rt) == MMC_RESPONSE_NONE ? NoResponse : ( \
    (rt) == MMC_RESPONSE_R1 ? ResponseR1   : ( \
    (rt) == MMC_RESPONSE_R2 ? ResponseR2   : ( \
    (rt) == MMC_RESPONSE_R3 ? ResponseR3   : ( \
    NoResponse)))))

unsigned int INTF_MMCSendCommand(struct MMC_command * pMMC_command, int init)
{
    // ignore init flag, init clocks are requested by card insert...

    Request.CommandCode = pMMC_command->command;
    Request.CommandArgument = pMMC_command->argument;
    Request.BlockSize = pMMC_command->block_len;
    Request.NumBlocks = pMMC_command->num_blocks;
    Request.HCParam = 0;

    if (bAlternateCommandMode)
    {
        Request.CommandResponse.ResponseType = CONVERT_RESPONSE_TYPE(AlternateCommandToResponse[Request.CommandCode]);
        Request.TransferClass = CONVERT_TC(AlternateCommandToTransferClass[Request.CommandCode]);
    }
    else
    {
        Request.CommandResponse.ResponseType = CONVERT_RESPONSE_TYPE(CommandToResponse[Request.CommandCode]);
        Request.TransferClass = CONVERT_TC(CommandToTransferClass[Request.CommandCode]);
    }

    // check for commands with R1b response
    if (Request.CommandResponse.ResponseType == ResponseR1)
        if (Request.CommandCode == 12 || Request.CommandCode == 28 || Request.CommandCode == 29 || Request.CommandCode == 38)
            Request.CommandResponse.ResponseType = ResponseR1b;
    
    if( (Request.CommandCode == MMC_SEND_EXT_CSD) && (pMMC_command->card_type == CARDTYPE_MMC ) )
    {
        // We're trying to send the get ExtCSD request; in MMC that requires a different
        // response type, so change it here.  This overrides the 1 disconnect in the unified
        // response table (mmcdisk.h) as opposed to creating an entirely new table for 1 
        // command

        //OEMWriteDebugString(L"INTF_MMCSendCommand: Changing response type for MMC_SEND_EXT_CSD \r\n ");
        Request.CommandResponse.ResponseType = ResponseR1;
        // Pick a non-zero value to trigger that this is MMC so don't have to tweak the global SD_BUS_REQUEST define
        // for just 1 command
        Request.HCParam = 0x5;
    }

    if ( (pMMC_command->card_type == CARDTYPE_MMC ) && ( (Request.CommandCode == SEND_WRITE_PROT) || (Request.CommandCode == SEND_WRITE_PROT_TYPE) ) )
    {
        // MMC send write protect & send write protect type use data lines to send status
        Request.TransferClass = SD_READ;
    }

    Request.pBlockBuffer = pMMC_command->pBuffer;

    // send command
    if (!SD_API_SUCCESS(SdhcBusRequestHandler(&Request)))
        return 1;

    // wait for command done, check result code
    if (!SD_API_SUCCESS(WaitForCommandResult(&Request)))
        return 1;

    return 0;
}

unsigned int INTF_MMCReadResponse(struct MMC_command * pMMC_command)
{
    int i;

    // change response byte order from SDHC to match format expected by MMC/bootloader driver
    //  response[0] = Request.CommandResponse.ResponseBuffer[MSB]
    //  response[1] = Request.CommandResponse.ResponseBuffer[MSB-1]
    //  response[2] = Request.CommandResponse.ResponseBuffer[MSB-2]
    //  response[1] = Request.CommandResponse.ResponseBuffer[MSB-3]
    //  etc.

    __zeromemory(pMMC_command->response, 17);

    memset(pMMC_command->response, 0, 17);

    if (NoResponse != Request.CommandResponse.ResponseType) 
    {
        if (ResponseR2 == Request.CommandResponse.ResponseType)
        {
            // 17 byte response
            for (i = 0; i < 17; i++)
            {
                pMMC_command->response[i] = Request.CommandResponse.ResponseBuffer[16 - i];
            }
        }
        else
        {
            // 6 byte response
            for (i = 0; i < 6; i++)
            {
                pMMC_command->response[i] = Request.CommandResponse.ResponseBuffer[5 - i];
            }
        }
    }

    // for some commands parse the response
    if (pMMC_command->command == SEND_CSD)
    {
        if (pMMC_command->card_type == CARDTYPE_SD || pMMC_command->card_type == CARDTYPE_SDHC)
        {
            pMMC_command->csd.sd_csd.sdcsd_struct = (pMMC_command->response[1] >> 6) & 0x3;
            pMMC_command->csd.sd_csd.sdtacc = pMMC_command->response[2];
            pMMC_command->csd.sd_csd.sdnsac = pMMC_command->response[3];
            pMMC_command->csd.sd_csd.sdtr_speed = pMMC_command->response[4];
            pMMC_command->csd.sd_csd.sdccc = (pMMC_command->response[5] << 4) | ((pMMC_command->response[6] >> 4) & 0xF);
            pMMC_command->csd.sd_csd.sdrd_bl_len = pMMC_command->response[6] & 0xF;
            pMMC_command->csd.sd_csd.sdrd_bl_part = (pMMC_command->response[7] >> 7) & 0x1;
            pMMC_command->csd.sd_csd.sdwr_bl_msalign = (pMMC_command->response[7] >> 6) & 0x1;
            pMMC_command->csd.sd_csd.sddsr_imp = (pMMC_command->response[7] >> 5) & 0x1;
            if (pMMC_command->card_type == CARDTYPE_SDHC)
            {
                pMMC_command->csd.sd_csd.sdhcc_size = ((pMMC_command->response[8] & 0x3f) << 16) |
                    (pMMC_command->response[9] << 8) | (pMMC_command->response[10]);

                pMMC_command->csd.sd_csd.sdvdd_r_min = 0;
                pMMC_command->csd.sd_csd.sdvdd_r_max = 0;
                pMMC_command->csd.sd_csd.sdvdd_w_min = 0;
                pMMC_command->csd.sd_csd.sdvdd_w_max = 0;
                pMMC_command->csd.sd_csd.sdc_size_mult = 0;
            }
            else
            {
                pMMC_command->csd.sd_csd.sdc_size = ((pMMC_command->response[7] & 0x3)<<10) |
                    (pMMC_command->response[8]<<2) | ((pMMC_command->response[9]>>6)  & 0x3);
                pMMC_command->csd.sd_csd.sdvdd_r_min = (pMMC_command->response[9] >> 3) & 0x7;
                pMMC_command->csd.sd_csd.sdvdd_r_max = pMMC_command->response[9] & 0x7;
                pMMC_command->csd.sd_csd.sdvdd_w_min = (pMMC_command->response[9] >> 5) & 0x7;
                pMMC_command->csd.sd_csd.sdvdd_w_max = (pMMC_command->response[10] >> 2) & 0x7;
                pMMC_command->csd.sd_csd.sdc_size_mult = (pMMC_command->response[10] & 0x3) << 1 |
                    ((pMMC_command->response[11] >> 7) & 0x1);
            }
            pMMC_command->csd.sd_csd.sderase_bk_en = (pMMC_command->response[11] >> 6) & 0x1;
            pMMC_command->csd.sd_csd.sdsector_size = ((pMMC_command->response[11] << 1) & 0x7E) | ((pMMC_command->response[12] >> 7) & 0x1);
            pMMC_command->csd.sd_csd.sderase_grp_size = pMMC_command->response[12] & 0x7F;
            pMMC_command->csd.sd_csd.sdwp_grp_en = (pMMC_command->response[13] >> 7) & 0x1;
            pMMC_command->csd.sd_csd.sdr2w_factor = (pMMC_command->response[13] >> 2) & 0x7;
            pMMC_command->csd.sd_csd.sdwr_blk_len = ((pMMC_command->response[13] & 0x3) << 2) | ((pMMC_command->response[14] >> 6) & 0x3);
            pMMC_command->csd.sd_csd.sdwr_blk_part = (pMMC_command->response[14] >> 5) & 0x1;
            pMMC_command->csd.sd_csd.sdfile_fmt_grp = (pMMC_command->response[15] >> 7) & 0x1;
            pMMC_command->csd.sd_csd.sdcopy = (pMMC_command->response[15] >> 6) & 0x1;
            pMMC_command->csd.sd_csd.sdperm_wr_prot = (pMMC_command->response[15] >> 5) & 0x1;
            pMMC_command->csd.sd_csd.sdtmp_wr_prot = (pMMC_command->response[15] >> 4) & 0x1;
            pMMC_command->csd.sd_csd.sdfile_fmt = (pMMC_command->response[15] >> 2) & 0x3;
            pMMC_command->csd.sd_csd.sdcrc = 0;  //no computed crc provided
            pMMC_command->crc = 0;
        }
        else
        {
            pMMC_command->csd.mmc_csd.csd_struct = (pMMC_command->response[1] >> 6) & 0x3;
            pMMC_command->csd.mmc_csd.spec_vers = (pMMC_command->response[1] >> 2) & 0xF;
            pMMC_command->csd.mmc_csd.tacc = pMMC_command->response[2];
            pMMC_command->csd.mmc_csd.nsac = pMMC_command->response[3];
            pMMC_command->csd.mmc_csd.tr_speed = pMMC_command->response[4];
            pMMC_command->csd.mmc_csd.ccc = (pMMC_command->response[5] << 4) | ((pMMC_command->response[6] >> 4) & 0xF);
            pMMC_command->csd.mmc_csd.rd_bl_len = pMMC_command->response[6] & 0xF;
            pMMC_command->csd.mmc_csd.c_size = ((pMMC_command->response[7] & 0x3)<<10) | (pMMC_command->response[8]<<2) | ((pMMC_command->response[9]>>6)  & 0x3);
            pMMC_command->csd.mmc_csd.rd_bl_part = (pMMC_command->response[7] >> 7) & 0x1;
            pMMC_command->csd.mmc_csd.wr_bl_msalign = (pMMC_command->response[7] >> 6) & 0x1;
            pMMC_command->csd.mmc_csd.dsr_imp = (pMMC_command->response[7] >> 5) & 0x1;
            pMMC_command->csd.mmc_csd.vdd_r_min = (pMMC_command->response[9] >> 3) & 0x7;
            pMMC_command->csd.mmc_csd.vdd_r_max = pMMC_command->response[9] & 0x7;
            pMMC_command->csd.mmc_csd.vdd_w_min = (pMMC_command->response[10] >> 5) & 0x7;
            pMMC_command->csd.mmc_csd.vdd_w_max = (pMMC_command->response[10] >> 2) & 0x7;
            pMMC_command->csd.mmc_csd.c_size_mult = (pMMC_command->response[10] & 0x3) << 1 | ((pMMC_command->response[11] >> 7) & 0x1);
            pMMC_command->csd.mmc_csd.sector_size = (pMMC_command->response[11] >> 2) & 0x1F;
            pMMC_command->csd.mmc_csd.erase_grp_size = ((pMMC_command->response[11] & 0x3) << 3) | ((pMMC_command->response[12] >> 5) & 0x7);
            pMMC_command->csd.mmc_csd.wp_grp_size = pMMC_command->response[12] & 0x1F;
            pMMC_command->csd.mmc_csd.wp_grp_en = (pMMC_command->response[13] >> 7) & 0x1;
            pMMC_command->csd.mmc_csd.default_ecc = (pMMC_command->response[13] >> 5) & 0x3;
            pMMC_command->csd.mmc_csd.r2w_factor = (pMMC_command->response[13] >> 2) & 0x7;
            pMMC_command->csd.mmc_csd.wr_blk_len = ((pMMC_command->response[13] & 0x3) << 2) | ((pMMC_command->response[14] >> 6) & 0x3);
            pMMC_command->csd.mmc_csd.wr_blk_part = (pMMC_command->response[14] >> 5) & 0x1;
            pMMC_command->csd.mmc_csd.file_fmt_grp = (pMMC_command->response[15] >> 7) & 0x1;
            pMMC_command->csd.mmc_csd.copy = (pMMC_command->response[15] >> 6) & 0x1;
            pMMC_command->csd.mmc_csd.perm_wr_prot = (pMMC_command->response[15] >> 5) & 0x1;
            pMMC_command->csd.mmc_csd.tmp_wr_prot = (pMMC_command->response[15] >> 4) & 0x1;
            pMMC_command->csd.mmc_csd.file_fmt = (pMMC_command->response[15] >> 2) & 0x3;
            pMMC_command->csd.mmc_csd.ecc = pMMC_command->response[15] & 0x3;
            pMMC_command->csd.mmc_csd.crc = 0;
            pMMC_command->crc = 0;
        }
    }
    
    if (pMMC_command->command == SEND_CID)
    {
        if (pMMC_command->card_type == CARDTYPE_SD || pMMC_command->card_type == CARDTYPE_SDHC)
        {
            pMMC_command->cid.sd_cid.sdmid = pMMC_command->response[1];
            pMMC_command->cid.sd_cid.sdoid = (pMMC_command->response[2] << 8) |
                pMMC_command->response[3];
            for (i=0;i<5; i++)
                pMMC_command->cid.sd_cid.sdpnm[i] = pMMC_command->response[4+i];
            pMMC_command->cid.sd_cid.sdprv = pMMC_command->response[9];
            pMMC_command->cid.sd_cid.sdpsn = (pMMC_command->response[10]<<24) |
                (pMMC_command->response[11]<<16) | (pMMC_command->response[12]<<8) |
                 pMMC_command->response[13];
            pMMC_command->cid.sd_cid.sdmdt = ((pMMC_command->response[14] & 0xF) << 8) |
                pMMC_command->response[15];
            // pMMC_command->cid.sd_cid.sdcrc = pMMC_command->response[16];
            pMMC_command->cid.sd_cid.sdcrc = 0;
            pMMC_command->crc = 0;
        }
        else
        {
            pMMC_command->cid.mmc_cid.mid = pMMC_command->response[1];
            pMMC_command->cid.mmc_cid.oid = (pMMC_command->response[2] << 8) |
                pMMC_command->response[3];
            for (i=0; i<6; i++)
                pMMC_command->cid.mmc_cid.pnm[i] = pMMC_command->response[4+i];
            pMMC_command->cid.mmc_cid.prv = pMMC_command->response[10];
            pMMC_command->cid.mmc_cid.psn = (pMMC_command->response[11]<<24) |
                (pMMC_command->response[12]<<16) | (pMMC_command->response[13]<<8) |
                 pMMC_command->response[14];
            pMMC_command->cid.mmc_cid.mdt = pMMC_command->response[15];
            // pMMC_command->cid.mmc_cid.crc = pMMC_command->response[16];
            pMMC_command->cid.mmc_cid.crc = 0;
            pMMC_command->crc = 0;
        }
    }
        
    // format status register if needed
    if ((pMMC_command->card_type == CARDTYPE_SD || pMMC_command->card_type == CARDTYPE_SDHC) && pMMC_command->command == SEND_RELATIVE_ADDRESS)
    {
        pMMC_command->relative_address = (pMMC_command->response[1] << 8) |
                                         (pMMC_command->response[2]);
        pMMC_command->status = 0;
        pMMC_command->ocr = 0;
    }
    else if (Request.CommandResponse.ResponseType == ResponseR1 || Request.CommandResponse.ResponseType == ResponseR1b)
    {
        pMMC_command->status = (pMMC_command->response[4]) |
                               (pMMC_command->response[3] << 8) |
                               (pMMC_command->response[2] << 16) |
                               (pMMC_command->response[1] << 24);
        pMMC_command->ocr = 0;

        /*
        // This test fails for command 18 which reports 16...
        if (pMMC_command->command != (pMMC_command->response[0] & 0x3f))
        {
            OALMSGX(OAL_ERROR, (TEXT("MMC: command in response %d != command %d\r\n"), pMMC_command->response[0], pMMC_command->command));
        }
        */
    }
    else if (Request.CommandResponse.ResponseType == ResponseR3)
    {
        pMMC_command->status = 0;
        pMMC_command->ocr = (pMMC_command->response[4]) |
                            (pMMC_command->response[3] << 8) |
                            (pMMC_command->response[2] << 16) |
                            (pMMC_command->response[1] << 24);
    }
    else if (Request.CommandResponse.ResponseType == ResponseR7)
    {
        pMMC_command->status = 0;
        pMMC_command->ocr = (pMMC_command->response[4]) |
            (pMMC_command->response[3] << 8) |
            (pMMC_command->response[2] << 16) |
            (pMMC_command->response[1] << 24) |
            (pMMC_command->response[0] << 28);
    }
    else
    {
        pMMC_command->status = 0;
        pMMC_command->ocr = 0;
    }

    // check for alternate command mode entry
    if (Request.CommandCode == APP_CMD)
        bAlternateCommandMode = TRUE;
    else
        bAlternateCommandMode = FALSE;

    return 0;
}


//----------------------------------------------------------------------------
//
// MMCSetTranSpeed()
//
//----------------------------------------------------------------------------

DWORD MMCSetTranSpeed(DWORD speed)
{
    static DWORD CurrentSpeed = 0;
    DWORD ActualSpeed = speed;

    if (CurrentSpeed != speed)
	{
        SdhcSetClockRate(&ActualSpeed);
	    OALMSG(1, (TEXT("SDCARD:Reqested speed %d, actual speed %d\r\n"), speed, ActualSpeed));
        CurrentSpeed = speed;
    }		
    return ActualSpeed;
}

DWORD MMCSetBlockLen(PDISK pDisk)
{
    MMCcmd.card_type = pDisk->d_CardType;
    MMCcmd.command = SET_BLOCKLEN;
    MMCcmd.num_blocks = 1;
    MMCcmd.block_len = pDisk->d_DiskInfo.di_bytes_per_sect;
    MMCcmd.argument = pDisk->d_DiskInfo.di_bytes_per_sect;

    OALMSGX(OAL_FUNC, (TEXT("SET_BLOCKLEN\r\n")));
    if (MMCCommandResponse(&MMCcmd, 0))
    {
        OALMSGX(OAL_FUNC, (TEXT("MMCCommandResponse SET_BLOCKLEN error!\r\n")));
        return 1;
    }
    return 0;
}

//==============================================================================
// MMC_xxx functions
//==============================================================================

unsigned int UTIL_csd_get_sectorsize(struct MMC_command * pMMCcmd)
{
    if (pMMCcmd->card_type == CARDTYPE_MMC)
        return 1 << pMMCcmd->csd.mmc_csd.rd_bl_len;
    else
        return 1 << pMMCcmd->csd.sd_csd.sdrd_bl_len;
}

// returns number of 512 byte sectors on card
unsigned int UTIL_csd_get_devicesize(struct MMC_command * pMMCcmd)
{
    if (pMMCcmd->card_type == CARDTYPE_MMC)
    {
        OALMSGX(1, (TEXT("UTIL_csd_get_devicesize: MMC c_size = 0x%X\r\n"), pMMCcmd->csd.mmc_csd.c_size));
		if (pMMCcmd->csd.mmc_csd.c_size == 0xFFF)
		{
			return ((pMMCcmd->mmc_extcsd.Sec_Cnt[0]) |
				(pMMCcmd->mmc_extcsd.Sec_Cnt[1] << 8) |
				(pMMCcmd->mmc_extcsd.Sec_Cnt[2] << 16) |
				(pMMCcmd->mmc_extcsd.Sec_Cnt[3] << 24));
		}
		if (UTIL_csd_get_sectorsize(pMMCcmd) == SECTOR_SIZE)
            return (pMMCcmd->csd.mmc_csd.c_size  + 1) * (1 << (pMMCcmd->csd.mmc_csd.c_size_mult  + 2));
        else if (UTIL_csd_get_sectorsize(pMMCcmd) == 1024)
            return 2 * (pMMCcmd->csd.mmc_csd.c_size  + 1) * (1 << (pMMCcmd->csd.mmc_csd.c_size_mult  + 2));
        else
            return 0;
    }
    else if (pMMCcmd->card_type == CARDTYPE_SD)
    {
        if (UTIL_csd_get_sectorsize(pMMCcmd) == SECTOR_SIZE)
            return (pMMCcmd->csd.sd_csd.sdc_size + 1) * (1 << (pMMCcmd->csd.sd_csd.sdc_size_mult + 2));
        else if (UTIL_csd_get_sectorsize(pMMCcmd) == 1024)
            return 2 * (pMMCcmd->csd.sd_csd.sdc_size + 1) * (1 << (pMMCcmd->csd.sd_csd.sdc_size_mult + 2));
        else if (UTIL_csd_get_sectorsize(pMMCcmd) == 2048)
            return 4 * (pMMCcmd->csd.sd_csd.sdc_size + 1) * (1 << (pMMCcmd->csd.sd_csd.sdc_size_mult + 2));
        else
            return 0;
    }
    else if (pMMCcmd->card_type == CARDTYPE_SDHC)
    {
        // for SDHC, c_size is in 512K units, we want it in 512 byte sectors
        return (pMMCcmd->csd.sd_csd.sdhcc_size + 1) * 1024;
    }
    else
        return 0;
}

unsigned int UTIL_csd_get_tran_speed(struct MMC_command * pMMCcmd)
{
    unsigned int temp, mant, exp;

    temp = (unsigned int)(pMMCcmd->card_type == CARDTYPE_MMC) ? pMMCcmd->csd.mmc_csd.tr_speed : pMMCcmd->csd.sd_csd.sdtr_speed;

    /* get exponent factor */
    switch(temp & 0x7)
    {
        case 0:
            exp = 100000;       // 100khz
            break;
        case 1:
            exp = 1000000;      // 1MHz
            break;
        case 2:
            exp = 10000000;     // 10MHz
            break;
        case 3:
            exp = 100000000;    // 100MHz
            break;
        default:
            exp = 0;
    }

    /* get mantissa factor (10x so we can use integer math) */
    switch((temp >> 3) & 0xf)
    {
            case 1:
                mant = 10;      // 1.0
                break;
            case 2:
                mant = 12;      // 1.2
                break;
            case 3:
                mant = 13;      // 1.3
                break;
            case 4:
                mant = 15;      // 1.5
                break;
            case 5:
                mant = 20;      // 2.0
                break;
            case 6:
                mant = 25;      // 2.5
                break;
            case 7:
                mant = 30;      // 3.0
                break;
            case 8:
                mant = 35;      // 3.5
                break;
            case 9:
                mant = 40;      // 4.0
                break;
            case 0xa:
                mant = 45;      // 4.5
                break;
            case 0xb:
                mant = 50;      // 5.0
                break;
            case 0xc:
                mant = 55;      // 5.5
                break;
            case 0xd:
                mant = 60;      // 6.0
                break;
            case 0xe:
                mant = 70;      // 7.0
                break;
            case 0xf:
                mant = 80;      // 8.0
                break;
            default:
                mant = 0;
    }

    return (exp * mant)/10;
}

void MMCSetMMCState(PDISK pDisk, ULONG state)
{
    pDisk->d_MMCState = state;
}


//----------------------------------------------------------------------------
//
//  MMCWaitForReady
//
unsigned int MMCWaitForReady(PDISK pDisk)
{
    UINT32 StartTime;
    int card_state;
    int bTimeout;

    MMCcmd.card_type = pDisk->d_CardType;
    MMCcmd.command = SEND_STATUS;
    MMCcmd.argument = (pDisk->d_RelAddress)<<16;
    MMCcmd.num_blocks = 1;
    MMCcmd.block_len = SECTOR_SIZE;

    if (MMCCommandResponse(&MMCcmd, 0))
        return 1;

    StartTime = OALGetTickCount();
    card_state = MMC_STATUS_STATE(MMCcmd.status);
    bTimeout = FALSE;

    // make sure the card is ready to accept data and we are in the proper
    // state.  If an error occured on the last transaction, the card might still
    // be in the prg state, etc.
        
    while ( (!(MMC_STATUS_READY(MMCcmd.status))) ||
           ( (card_state != MMC_STATUS_STATE_STBY) && (card_state != MMC_STATUS_STATE_TRAN) ) )
    {
        if (bTimeout)
        {
            return 1;
        }

        if ( (card_state != MMC_STATUS_STATE_STBY) && (card_state != MMC_STATUS_STATE_TRAN) )
        {
            OALMSGX(OAL_FUNC, (TEXT("SDMem: wait for card state = %i\r\n"), card_state));
            OALMSGX(OAL_FUNC, (TEXT("SDMem: MMCcmd.status = 0x%X\r\n"), MMCcmd.status));
        }
        
        if (MMCCommandResponse(&MMCcmd, 0)) 
            return 1;

        card_state = MMC_STATUS_STATE(MMCcmd.status);

        if (OALGetTickCount() - StartTime > 1000)
            bTimeout = TRUE;

        OALStall(10 * 1000);
    }
    
    return 0;
}


DWORD MMCReadMultiSectors(PDISK pDisk, UINT32 LogicalSector, void *pBuffer, UINT16 numSectors)
{
    DWORD Status;

    // Make sure we don't access beyond end of disk
    if ((LogicalSector + numSectors) >= pDisk->d_DiskInfo.di_total_sectors)
    {
        Status = ERROR_INVALID_PARAMETER;
        OALMSGX(OAL_FUNC, (TEXT("MMCReadMultiSectorsExit: ERROR_INVALID_PARAMETER\r\n")));
        goto MMCReadMultiSectorsExit;
    }
    
    if (pDisk->d_DiskCardState != STATE_OPENED && pDisk->d_DiskCardState != STATE_CLOSED)
    {
        Status = MMCREAD_FAILURE;
        OALMSGX(OAL_FUNC, (TEXT("MMCReadMultiSectorsExit: incorrect disk state\r\n")));
        goto MMCReadMultiSectorsExit;
    }

    if (MMCWaitForReady(pDisk))
    {
        Status = MMCREAD_FAILURE;
        OALMSGX(OAL_ERROR, (TEXT("MMCReadMultiSectors: MMCWaitForReady error\r\n")));
        goto MMCReadMultiSectorsExit;
    }

    // build command
    MMCcmd.command = READ_MULTIPLE_BLOCK;
    MMCcmd.num_blocks = numSectors;
    MMCcmd.block_len = pDisk->d_DiskInfo.di_bytes_per_sect;
    MMCcmd.pBuffer = pBuffer;
    
    // starting address
    if (MMCcmd.card_type == CARDTYPE_SDHC)
        MMCcmd.argument = LogicalSector;
    else
        MMCcmd.argument = LogicalSector * pDisk->d_DiskInfo.di_bytes_per_sect;

    OALMSGX(OAL_FUNC, (TEXT("READ_MULTIPLE_BLOCK\r\n")));
    if (MMCCommandResponse(&MMCcmd, 0))
    {
        Status = MMCREAD_FAILURE;
        OALMSGX(OAL_ERROR, (TEXT("MMCReadMultiSectors: MMCCommandResponse error on READ_MULTIPLE_BLOCK!\r\n")));
        goto MMCReadMultiSectorsExit;
    }

    // send STOP_TRANSMISSION
    MMCcmd.command = STOP_TRANSMISSION;
    MMCcmd.argument = 0;
    MMCcmd.num_blocks = 0;
    MMCcmd.block_len = 0;
    MMCcmd.pBuffer = 0;
    if (MMCCommandResponse(&MMCcmd, 0))
    {
        Status = MMCREAD_FAILURE;
        OALMSGX(OAL_ERROR, (TEXT("MMCReadMultiSectors: MMCCommandResponse error on STOP_TRANSMISSION!\r\n")));
        goto MMCReadMultiSectorsExit;
    }

    Status = MMCREAD_SUCCESS;

MMCReadMultiSectorsExit:
    
    if (Status != MMCREAD_SUCCESS)
    {
        OALMSGX(OAL_ERROR, (TEXT("read multi sectors error\r\n")));

        //MMCcmd.command = SELECT_DESELECT_CARD;
        // zero deselects all cards
        //MMCcmd.argument = 0;
        //OALMSGX(OAL_FUNC, (TEXT("SELECT_DESELECT_CARD\r\n")));
        //MMCCommandResponse(&MMCcmd, 0);
    }

    return Status;
}


DWORD MMCRead(PDISK pDisk, UINT32 LogicalSector, void *pSectorBuffer)
{
    DWORD Status;

    // Make sure we don't access beyond end of disk
    if (LogicalSector >= pDisk->d_DiskInfo.di_total_sectors)
    {
        Status = ERROR_INVALID_PARAMETER;
        OALMSGX(OAL_FUNC, (TEXT("DoDiskIO: ERROR_INVALID_PARAMETER\r\n")));
        goto MMCReadExit;
    }
    
    if (pDisk->d_DiskCardState != STATE_OPENED && pDisk->d_DiskCardState != STATE_CLOSED)
    {
        Status = MMCREAD_FAILURE;
        OALMSGX(OAL_FUNC, (TEXT("DoDiskIO: incorrect disk state\r\n")));
        goto MMCReadExit;
    }

    if (MMCWaitForReady(pDisk))
    {
        Status = MMCREAD_FAILURE;
        goto MMCReadExit;
    }

    // build command
    MMCcmd.command = READ_SINGLE_BLOCK;
    MMCcmd.num_blocks = 1;
    MMCcmd.block_len = pDisk->d_DiskInfo.di_bytes_per_sect;
    MMCcmd.pBuffer = pSectorBuffer;
    
    // starting address
    if (MMCcmd.card_type == CARDTYPE_SDHC || (MMCcmd.card_type == CARDTYPE_MMC && pDisk->d_MMC_HC))
        MMCcmd.argument = LogicalSector;
	else
        MMCcmd.argument = LogicalSector * pDisk->d_DiskInfo.di_bytes_per_sect;

    OALMSGX(OAL_FUNC, (TEXT("READ_SINGLE_BLOCK\r\n")));
    if (MMCCommandResponse(&MMCcmd, 0))
    {
        Status = MMCREAD_FAILURE;
        OALMSGX(OAL_ERROR, (TEXT("MMCRead: MMCCommandResponse error on READ_SINGLE_BLOCK!\r\n")));
        goto MMCReadExit;
    }

    Status = MMCREAD_SUCCESS;

MMCReadExit:
    
    if (Status != MMCREAD_SUCCESS)
    {
        OALMSGX(OAL_ERROR, (TEXT("read error\r\n")));

        //MMCcmd.command = SELECT_DESELECT_CARD;
        // zero deselects all cards
        //MMCcmd.argument = 0;
        //OALMSGX(OAL_FUNC, (TEXT("SELECT_DESELECT_CARD\r\n")));
        //MMCCommandResponse(&MMCcmd, 0);
    }

    return Status;
}

DWORD MMCSelectCard(PDISK pDisk)
{
    UINT32 StartTime;
    int bTimeout;

    StartTime = OALGetTickCount();
    MMCcmd.card_type = pDisk->d_CardType;
    
    bTimeout = FALSE;
    for(;;)
    {
        if (OALGetTickCount() - StartTime > 1000)
            bTimeout = TRUE;

        MMCcmd.command = SEND_STATUS;
        MMCcmd.argument = (pDisk->d_RelAddress)<<16;
        MMCcmd.num_blocks = 1;
        MMCcmd.block_len = SECTOR_SIZE;
        OALMSGX(OAL_FUNC, (TEXT("SEND_STATUS\r\n")));
        if (MMCCommandResponse(&MMCcmd, 0)) 
            goto select_card_error;

        if (MMC_STATUS_POLL_ERROR(MMCcmd.status))
        {
            OALMSGX(OAL_ERROR, (TEXT("MMCSelectCard: poll status error, status = 0x%X\r\n"), MMCcmd.status));
            goto select_card_error;
        }

        if ( MMC_STATUS_STATE(MMCcmd.status) == MMC_STATUS_STATE_TRAN )
            break;

        OALMSGX(OAL_FUNC, (TEXT("MMCSelectCard: not in tran state, status = 0x%X\r\n"), MMCcmd.status));

        // try to get card into transfer state  
        // this should only have to happen once per card insertion
        MMCcmd.command = SELECT_DESELECT_CARD;
        MMCcmd.argument = (pDisk->d_RelAddress) << 16;

        OALMSGX(OAL_FUNC, (TEXT("SELECT_DESELECT_CARD\r\n")));
        if ( MMCCommandResponse(&MMCcmd, 0) )
        {
            OALMSGX(OAL_ERROR, (TEXT("SELECT_DESELECT_CARD failed\r\n")));
            goto select_card_error;
        }

        // check for bTimeout
        if (bTimeout)
        {
            OALMSGX(OAL_ERROR, (TEXT("MMCSelectCard: timeout waiting for card to get into tran state, status = 0x%X\r\n"), MMCcmd.status));
            goto select_card_error;
        }
    }

    if (MMCSetBlockLen(pDisk))
	{
        OALMSGX(OAL_ERROR, (L"SDCardInit: MMCSetBlockLen failed!\r\n"));
        goto select_card_error;
	}


    if (pDisk->d_CardType == CARDTYPE_SD || pDisk->d_CardType == CARDTYPE_SDHC)
    {
        BYTE scr[8];

        MMCcmd.command = APP_CMD;
        MMCcmd.argument = (pDisk->d_RelAddress) << 16;
        OALMSGX(OAL_FUNC, (TEXT("APP_CMD\r\n")));
        if (MMCCommandResponse(&MMCcmd, 0) )
        {
            //OALMSGX(OAL_ERROR, (TEXT("APP_CMD failed\r\n")));
            goto Detect4BitCardDone;
        }

        // SEND_SCR - send configuration register
        MMCcmd.command = SEND_SCR;
        MMCcmd.argument = 0;
        MMCcmd.num_blocks = 1;
        MMCcmd.block_len = 8;
        MMCcmd.pBuffer = scr;
		OALMSGX(OAL_FUNC, (TEXT("SEND_SCR\r\n")));
        if (MMCCommandResponse(&MMCcmd, 0) )
        {
            //OALMSGX(OAL_ERROR, (TEXT("SEND_SCR failed\r\n")));
            goto Detect4BitCardDone;
        }

        // most significant byte arrives first at scr[0]
		// scr[1] has bits 55:48, bit 50 is set for 4 bit capable card
        pDisk->d_Supports4Bit = scr[1] & 0x04 ? TRUE : FALSE;

Detect4BitCardDone:

		if (pDisk->d_Supports4Bit)
		{
			OALMSGX(OAL_FUNC, (TEXT("SDCARD: 4 bit mode\r\n")));

			MMCcmd.command = APP_CMD;
			MMCcmd.argument = (pDisk->d_RelAddress) << 16;
			OALMSGX(OAL_FUNC, (TEXT("APP_CMD\r\n")));
			if (MMCCommandResponse(&MMCcmd, 0) )
			{
				OALMSGX(OAL_WARN, (TEXT("APP_CMD failed\r\n")));
				goto select_card_error;
			}

			MMCcmd.command = SET_BUS_WIDTH;
			MMCcmd.argument = 0x2;
			OALMSGX(OAL_FUNC, (TEXT("SET_BUS_WIDTH\r\n")));
			if (MMCCommandResponse(&MMCcmd, 0))
			{
				OALMSGX(OAL_WARN, (TEXT("MMC: unable to set wide bus mode for SD/SDHC card.\r\n")));
				goto select_card_error;
			}
			else
			{
				OALMSGX(1, (TEXT("SDCARD:Using 4 bit mode\r\n")));
				SdhcSetInterface(SD_INTERFACE_SD_4BIT);
			}
		}
    }


	if (pDisk->d_CardType == CARDTYPE_MMC)
	{
		// assume its our eMMC!
		MMCcmd.command = MMC_SWITCH;
		MMCcmd.argument = 0x03B90100;			// Highspeed timing
		//MMCcmd.argument = (3 << 24) | (0xB9 << 16) | (0x1 << 8);
		OALMSGX(1, (TEXT("SDCARD:SET HS eMMC\r\n")));
        if (MMCCommandResponse(&MMCcmd, 0))
        {
            OALMSGX(OAL_FUNC, (TEXT("SDCARD: unable to set high speed mode for MMC card.\r\n")));
            goto select_card_error;
        }
        else
        {
			OALMSG(1, (TEXT("SDCARD:Using High Speed mode\r\n")));
			pDisk->MaxClkFreq = 52000000;
		}

		// needed for Micron part...slow controller!
		OALStall(2000);

		MMCcmd.command = MMC_SWITCH;
		MMCcmd.argument = 0x03B70200;			// 8bit bus
		// Access = 0x3, Index = 0xB7, Value = 0x2
		//MMCcmd.argument = (0x3 << 24) | (0xB7 << 16) | (0x2 << 8);
		OALMSGX(1, (TEXT("SDCARD:SET 8bit eMMC\r\n")));
		// Note: DDR not supported on 335x
		//// Access = 0x3 (write value), Index = 0xB7 (183), Value = 0x6 (8-bit DDR)
		//MMCcmd.argument = (0x3 << 24) | (0xB7 << 16) | (0x6 << 8);
		//MMCcmd.argument = 0x03B70600;			// 8bit DDR bus
		//OALMSGX(OAL_FUNC, (TEXT("SET 8bit DDR eMMC\r\n")));
        if (MMCCommandResponse(&MMCcmd, 0))
        {
            OALMSGX(OAL_FUNC, (TEXT("SDCARD: unable to set 8 bitbus for MMC card.\r\n")));
            goto select_card_error;
        }
        else
        {
			OALMSG(OAL_INFO, (TEXT("SDCARD:Using 8bit mode\r\n")));
            SdhcSetInterface(SD_INTERFACE_MMC_8BIT);
        }

		// needed for Micron part...slow controller!
		OALStall(2000);
	}
	else
	{
        OALMSGX(OAL_FUNC, (TEXT("SDCARD: no support for 4/8 bit mode\r\n")));
	}

    return 0;
        
select_card_error:
    return 1;
}

//----------------------------------------------------------------------------
//
//  MMCIssueIdentify
//
BOOL MMCIssueIdentify(PDISK pDisk)
{
    BOOL bCommandFailed=TRUE;
    UINT32 StartTime=0;
    int bTimeout=FALSE;
    BOOL bCardSupportsSD2 = FALSE;
    
    OALMSGX(1, (TEXT("MMCIssueIdentify: MMCIssueIdentify \r\n")));
    OALMSGX(1, (TEXT("MMCIssueIdentify: Test of SD... \r\n")));

    // send init sequence to card at beginning of card ID process as specified in TRM 
    SendInitSequence();

	MMCSetTranSpeed(1000000);  // set clock to reasonable rate

    // CMD1: SEND_OP_COND - send operating conditions
    // result is wire or of all cards
    MMCcmd.command = GO_IDLE_STATE;
    MMCcmd.argument = 0;
    MMCcmd.num_blocks = 1;
    MMCcmd.block_len = SECTOR_SIZE;
    MMCcmd.card_type = CARDTYPE_MMC;
    
    if ( MMCCommandResponse(&MMCcmd, 1) )
    {
        OALMSGX(1, (TEXT("MMCIssueIdentify: MMCCommandResponse Error\r\n")));
        goto command_error;
    }

    OALStall(1 * 1000);

	pDisk->d_MMC_HC = FALSE;

	MMCCommandResponse(&MMCcmd, 0);

    MMCcmd.ocr = 0;
    MMCcmd.card_type = CARDTYPE_SD;     // must be set during detection of SD card

    OALStall(1 * 1000);

    StartTime = OALGetTickCount();
    bTimeout = FALSE;

    // SD 2.0 spec requires CMD8 before ACMD41
    MMCcmd.command = SD_SEND_IF_COND;

    // specify VSH = 2.7-3.6V (bit 8), check pattern = 0xaa (bits 7:0)
    MMCcmd.argument = 0x000001aa;

    if ( MMCCommandResponse(&MMCcmd, 0) )
    {
        // CMD8 failed, not SD 2.00 card
        MMCcmd.command = GO_IDLE_STATE;
        MMCcmd.argument = 0;
        MMCCommandResponse(&MMCcmd, 0);
    }
    else
    {
        // Need to verify voltage support and check pattern
        bCardSupportsSD2 = TRUE;
    }
    
    // SD spec says timeout for ACMD41 should be 1 second
    for(;;)
    {
        if (OALGetTickCount() - StartTime > 1000)
            bTimeout = TRUE;

        MMCcmd.command = APP_CMD;
        MMCcmd.argument = (pDisk->d_RelAddress) << 16;

        if ( MMCCommandResponse(&MMCcmd, 0) )
            goto sd_det_error;

        OALStall(1 * 1000);

        MMCcmd.command = SD_SEND_OP_CODE;
        if (bCardSupportsSD2)
        {
            // specify 3.3V, HC
            MMCcmd.argument = 0x40200000;
        }
        else
        {
            // specify 3.3V
            MMCcmd.argument = 0x00200000;
        }

        if ( MMCCommandResponse(&MMCcmd, 0) )
            goto sd_det_error;

        if ( (!(MMC_OCR_BUSY(MMCcmd.ocr))) || bTimeout)
            break;

        OALStall(100 * 1000);
    }

    // No card or not an sd one...
    if ( MMC_OCR_BUSY(MMCcmd.ocr) )
    {
        OALMSGX(1, (TEXT("MMCIssueIdentify: busy bit never deactivated -- no sd card detected, ocr = 0x%X\r\n"), MMCcmd.ocr));
        goto sd_det_error;
    }
    
    // check for high capacity SD memory card, OCR bit 30 == 1: high capacity card
    if (bCardSupportsSD2 && (MMCcmd.ocr & (1 << 30)))
    {
        MMCcmd.card_type = CARDTYPE_SDHC;
        pDisk->d_CardType = CARDTYPE_SDHC;
    }
    else
    {
        pDisk->d_CardType = CARDTYPE_SD;
    }
    
    OALMSG(1, (TEXT("MMCIssueIdentify: SD%s card detected\r\n"), MMCcmd.card_type == CARDTYPE_SDHC ? TEXT(" HC") : TEXT("")));
    goto found_card;
    
sd_det_error:
    OALMSGX(1, (TEXT("MMCIssueIdentify: No SD, Test of MMC... \r\n")));
    MMCcmd.command = GO_IDLE_STATE;
    MMCcmd.argument = 0;
    MMCcmd.num_blocks = 1;
    MMCcmd.block_len = SECTOR_SIZE;
    MMCcmd.card_type = CARDTYPE_MMC;

    if ( MMCCommandResponse(&MMCcmd, 1) )
        goto command_error;

    OALStall(1 * 1000);

    MMCCommandResponse(&MMCcmd, 0);
    
    StartTime = OALGetTickCount();
    bTimeout = FALSE;

    for(;;)
    {
        if (OALGetTickCount() - StartTime > 1000)
            bTimeout = TRUE;

		MMCcmd.ocr = 0;
        MMCcmd.command = SEND_OP_COND;
        MMCcmd.argument = 0x40FF8000;
        bCommandFailed = MMCCommandResponse(&MMCcmd, 0);

        if (bTimeout || !(MMC_OCR_BUSY(MMCcmd.ocr)) || bCommandFailed)
            break;
    }

    if ( !bCommandFailed && !(MMC_OCR_BUSY(MMCcmd.ocr)) )
    {
        pDisk->d_CardType = CARDTYPE_MMC;
        pDisk->d_RelAddress = 1;
		OALMSG(1, (TEXT("MMCIssueIdentify: eMMC%s detected\r\n"), MMCcmd.ocr == 0xC0FF8080 ? TEXT(" HC") : TEXT("")));
		if (MMCcmd.ocr == 0xC0FF8080)
			pDisk->d_MMC_HC = TRUE;
	}

    OALStall(1 * 1000);

    if ( MMC_OCR_BUSY(MMCcmd.ocr) )
    {
        OALMSGX(1, (TEXT("MMCIssueIdentify: busy bit never deactivated -- probably no card, ocr = 0x%X\r\n"), MMCcmd.ocr));
        return FALSE;
    }
    
found_card:
    
    MMCSetMMCState(pDisk, MMC_STATE_READY);

    // CMD2: ALL_SEND_CID - all cards send CID data
    MMCcmd.command = ALL_SEND_CID;
    MMCcmd.argument = 0;

    OALMSGX(1, (TEXT("ALL_SEND_CID\r\n")));
    if ( MMCCommandResponse(&MMCcmd, 0) )
        goto command_error;

    MMCSetMMCState(pDisk, MMC_STATE_IDENT);

    /*
     * CMD3: SET_RELATIVE_ADDR - set relative address
     * relative address should probably be interface number
     *
     * note: this command is SEND_RELATIVE_ADDRESS for SD cards
     */

    MMCcmd.card_type = pDisk->d_CardType;

    if (pDisk->d_CardType == CARDTYPE_MMC)
    {
        MMCcmd.command = SET_RELATIVE_ADDR;
        MMCcmd.argument = (pDisk->d_RelAddress) << 16;

        OALMSGX(1, (TEXT("SET_RELATIVE_ADDR MMC\r\n")));
        if ( MMCCommandResponse(&MMCcmd, 0) )
            goto command_error;

        MMCSetMMCState(pDisk, MMC_STATE_STBY);
    }
    else if (pDisk->d_CardType == CARDTYPE_SD || pDisk->d_CardType == CARDTYPE_SDHC)
    {
        MMCcmd.command = SEND_RELATIVE_ADDRESS;
        MMCcmd.argument = 0;

        OALMSGX(1, (TEXT("SET_RELATIVE_ADDR SD\r\n")));
        if ( MMCCommandResponse(&MMCcmd, 0) )
            goto command_error;

        pDisk->d_RelAddress = MMCcmd.relative_address;
        OALMSGX(1, (TEXT("SD card relative address is %d\r\n"), pDisk->d_RelAddress));

        MMCSetMMCState(pDisk, MMC_STATE_STBY);
    }

    /*
     * CMD10: SEND_CID - send card identification
     */
    MMCcmd.command = SEND_CID;
    MMCcmd.argument = (pDisk->d_RelAddress) << 16;

    OALMSGX(1, (TEXT("SEND_CID\r\n")));
    if ( MMCCommandResponse(&MMCcmd, 0) )
        goto command_error;

//#if 0
    OALMSGX(1, (TEXT("MMCIssueIdentify: DUMP CID\r\n")));
    if (pDisk->d_CardType == CARDTYPE_SD || pDisk->d_CardType == CARDTYPE_SDHC)
    {
        //char temp[10];
        //dump sd cid
        OALMSGX(1, (TEXT("SD: Mfr id: %d\r\n"), MMCcmd.cid.sd_cid.sdmid));
        //UTIL_sd_cid_get_name(&MMCcmd, temp);
        //OALMSG(OAL_INFO, (TEXT("SD: Product name: %s\r\n"), temp));
        OALMSGX(OAL_INFO, (TEXT("SD: Serial number: 0x%X\r\n"), MMCcmd.cid.sd_cid.sdpsn));
    }
    else
    {
        //char temp[10];
        // default to mmc
        OALMSGX(1, (TEXT("MMC: Mfr id: %d\r\n"), MMCcmd.cid.mmc_cid.mid));
        //UTIL_mmc_cid_get_name(&MMCcmd, temp);
        //OALMSG(OAL_INFO, (TEXT("MMC: Product name: %s\r\n"), temp));
        OALMSGX(1, (TEXT("MMC: Serial number: 0x%X\r\n"), MMCcmd.cid.mmc_cid.psn));
    }
//#endif
        
	/*
	 * CMD9: SEND_CSD - send card specific data
	 */
	MMCcmd.command = SEND_CSD;
	MMCcmd.argument = (pDisk->d_RelAddress) << 16;

	OALMSGX(1, (TEXT("SEND_CSD\r\n")));
	if ( MMCCommandResponse(&MMCcmd, 0) )
		goto command_error;
	
    OALStall(1 * 1000);

	if (pDisk->d_MMC_HC)
	{

		// try to get card into transfer state  
		MMCcmd.command = SELECT_DESELECT_CARD;
		MMCcmd.argument = (pDisk->d_RelAddress) << 16;

		OALMSGX(1, (TEXT("SELECT_DESELECT_CARD\r\n")));
		if ( MMCCommandResponse(&MMCcmd, 0) )
		{
			OALMSGX(OAL_ERROR, (TEXT("SELECT_DESELECT_CARD failed\r\n")));
			goto command_error;
		}

		// Now get the extended CSD
		OALMSGX(1, (TEXT("Get extended CSD\r\n")));

		// build command
		MMCcmd.command = MMC_SEND_EXT_CSD;
		MMCcmd.num_blocks = 1;
		MMCcmd.block_len = SECTOR_SIZE;     
		MMCcmd.pBuffer = (void *)&MMCcmd.mmc_extcsd;
		MMCcmd.card_type = CARDTYPE_MMC;
		MMCcmd.argument = (pDisk->d_RelAddress)<<16;

		if (MMCCommandResponse(&MMCcmd, 0))
		{
			OALMSGX(OAL_ERROR, (TEXT("MMC_SEND_EXT_CSD failed\r\n")));
			goto command_error;
		}

		MMCcmd.command = SEND_STATUS;
		MMCcmd.argument = (pDisk->d_RelAddress)<<16;
		MMCcmd.pBuffer = NULL;

		OALMSGX(1, (TEXT("SEND_STATUS\r\n")));
		if ( MMCCommandResponse(&MMCcmd, 0) )
		{
			OALMSGX(OAL_ERROR, (TEXT("SEND_STATUS failed\r\n")));
			goto command_error;
		}
	}

	
	OALMSGX(1, (TEXT("reported block size = %d\r\n"), UTIL_csd_get_sectorsize(&MMCcmd)));
    OALMSGX(1, (TEXT("Card size is = %u 512 byte sectors\r\n"), UTIL_csd_get_devicesize(&MMCcmd)));
    OALMSGX(1, (TEXT("max clock freq = %d\r\n"), UTIL_csd_get_tran_speed(&MMCcmd)));

    // set up data in DiskInfo data structure
    //pDisk->d_DiskInfo.di_bytes_per_sect = UTIL_csd_get_sectorsize(&MMCcmd);
    // some SD (not SDHC) cards will report 1024 or 2048 byte sectors, this is only to 
    // allow >1GB capacity to be reported, bootloader FAT file system only supports 512 byte sectors.
    pDisk->d_DiskInfo.di_bytes_per_sect = SECTOR_SIZE;
    pDisk->d_DiskInfo.di_total_sectors = UTIL_csd_get_devicesize(&MMCcmd);

	if (pDisk->d_CardType == CARDTYPE_MMC)
	{
		pDisk->MaxClkFreq = UTIL_csd_get_tran_speed(&MMCcmd) * 2;
	}
	else
	{
		pDisk->MaxClkFreq = UTIL_csd_get_tran_speed(&MMCcmd);
	}

	OALMSGX(1, (TEXT("SDCARD:Card identification complete\r\n")));
    return TRUE;

command_error:
    OALMSGX(1, (TEXT("MMC: identification error\r\n")));
    return FALSE;
}


unsigned int MMCCommandResponse(struct MMC_command * pMMCcmd, int init)
{
    if ( INTF_MMCSendCommand(pMMCcmd, init) )
    {
        OALMSGX(OAL_ERROR, (TEXT("MMC::MMCCommandResponse: MMCSendCommand error, command = %d\r\n"), pMMCcmd->command));
        bAlternateCommandMode = FALSE;
        goto CommandResponseError;
    }

    INTF_MMCReadResponse(pMMCcmd);

    // check response for errors
    if ( MMC_STATUS_CMD_ERROR(pMMCcmd->status) )
    {
        OALMSGX(OAL_ERROR,(TEXT("MMC::MMCCommandResponse: Command = %d: Response Status Error = 0x%x\r\n"), pMMCcmd->command, pMMCcmd->status));
        goto CommandResponseError;
    }

    return 0;

CommandResponseError:

    OALMSGX(OAL_ERROR, (TEXT("MMC::MMCCommandResponse: Command Response Error\r\n")));
    return 1;
}

//==============================================================================
// IO_xxx functions
//==============================================================================
#define DL_SUCCESS      0
#define DL_MMC_ERROR    -1

/*
 * Used to check if media is available
 */
BOOL MMCCardDetect(PDISK pDisk)
{
#if 1
        // check if card is preset in connector
    if (SdhcCardDetect())
        {
            if (pDisk->d_MMCState == MMC_STATE_IDLE)
            {
                // card present so handle card entry actions
                SdhcHandleInsertion();
                return MMCIssueIdentify(pDisk);
            }
            return TRUE;
        }
#else
    // send status command to detect card
        int result;

        if (pDisk->d_MMCState == MMC_STATE_IDLE)
        {
            result = MMCIssueIdentify(pDisk);
            if (result)
            {
                SdhcHandleInsertion();
                return result;
            }
        }
        else
        {
            MMCcmd.card_type = pDisk->d_CardType;
            MMCcmd.command = SEND_STATUS;
            MMCcmd.argument = (pDisk->d_RelAddress)<<16;
            MMCcmd.num_blocks = 1;
            MMCcmd.block_len = SECTOR_SIZE;
            result = (MMCCommandResponse(&MMCcmd, 0));
            if (result)
            {
                MMCSetMMCState(pDisk, MMC_STATE_IDLE);
                return FALSE;
            }
            else return TRUE;
        }
#endif
    return FALSE;
}

/*
 * InitDisk
 */

BOOL MMCInitDisk(PDISK pDisk)
{

    OALMSGX(OAL_INFO, (TEXT("MMCInitDisk\r\n")));

    // default is for all IDE devices to support 16 bit data transfers (older drives don't support 8 bit transfers)

    if (SdhcInitialize() == FALSE)
        return FALSE;

    pDisk->d_DiskInfo.di_total_sectors = 0;
    pDisk->d_DiskInfo.di_bytes_per_sect = SECTOR_SIZE;
    pDisk->d_DiskInfo.di_cylinders = 0;
    pDisk->d_DiskInfo.di_heads = 0;
    pDisk->d_DiskInfo.di_sectors = 0;
    pDisk->d_Supports4Bit = FALSE;

    return TRUE;

}   // Initdisk


//==============================================================================
// sdcardxxx functions
//==============================================================================

/*----------------------------------------------------------------------------
 * SDCardInit
 *
 */
int SDCardInit(DISK *pDisk)
{
    OALMSGX(OAL_INFO, (L"SDCardInit: Init device ...\r\n"));

    pDisk->d_MMCState = MMC_STATE_IDLE;

    pDisk->d_DiskCardState = STATE_INITING;

    if (MMCInitDisk(pDisk))
    {
        OALMSGX(OAL_ERROR, (L"SDCardInit: MMCInitDisk failed!\r\n"));
        return DL_MMC_ERROR;
    }

    if (MMCCardDetect(pDisk) == FALSE) {
        OALMSGX(OAL_ERROR, (L"SDCardInit: No media found!\r\n"));
        return DL_MMC_ERROR;
    }

    pDisk->d_DiskCardState = STATE_OPENED;

    if (MMCWaitForReady(pDisk))
    {
        OALMSGX(OAL_ERROR, (L"SDCardInit: MMCWaitForReady failed!\r\n"));
        return DL_MMC_ERROR;
    }
    
    if (MMCSelectCard(pDisk))
    {
        OALMSGX(OAL_ERROR, (L"SDCardInit: MMCSelectCard failed!\r\n"));
        return DL_MMC_ERROR;
    }       

    MMCSetTranSpeed(pDisk->MaxClkFreq);

    return DL_SUCCESS;  
}

/*
 * SDCardIdentify
 *
 */
int SDCardIdentify(DISK *pDisk, void *pSector)
{
    UNREFERENCED_PARAMETER(pSector);
    UNREFERENCED_PARAMETER(pDisk);
    OALMSGX(OAL_INFO, (L"SDCardIdentify: Identify device ...\r\n"));
    
    return DL_SUCCESS;
}


/*----------------------------------------------------------------------------
 * SDCardReadSector
 *
 */
int SDCardReadSector(DISK *pDisk, UINT32 LogicalSector, void *pSector)
{
    int retry = 2;      //allow 3 attempts to read sector correctly
    DWORD Status;
    
    OALMSGX(OAL_FUNC, (L"SDCardReadSector %d\r\n", LogicalSector));

    do
    {
        Status = MMCRead(pDisk, LogicalSector, pSector);
        if (Status != MMCREAD_SUCCESS)
        {
            if (retry > 0)
                OALMSGX(OAL_ERROR, (L"SDCardReadSector: Error reading file, retry (sector %d)\r\n", LogicalSector));
        }
    } 
    while (Status != MMCREAD_SUCCESS && retry--);
    
    if (Status != MMCREAD_SUCCESS)
    {
        OALMSGX(OAL_ERROR, (L"SDCardReadSector: Error reading file! (sector %d)\r\n", LogicalSector));
        return DL_MMC_ERROR;
    }

    return DL_SUCCESS;
}

/*
 * SDCardReadMultiSectors
 *
 */
int SDCardReadMultiSectors(void *pDisk, UINT32 LogicalSector, void *pBuffer, UINT16 numSectors)
{
    int retry = 0;      //allow 3 attempts to read sector correctly
    DWORD Status;
    
    OALMSGX(OAL_FUNC, (L"SDCardReadMultiSectors %d, num sec %d\r\n", LogicalSector, numSectors));

    do
    {
        Status = MMCReadMultiSectors((DISK *)pDisk, LogicalSector, pBuffer, numSectors);
        if (Status != MMCREAD_SUCCESS)
        {
            if (retry > 0)
					OALMSGX(OAL_ERROR, (L"SDCardReadMultiSectors: Error reading file, retry (sector %d)\r\n", LogicalSector));
        }
    } 
    while (Status != MMCREAD_SUCCESS && retry--);
    
    if (Status != MMCREAD_SUCCESS)
    {
			OALMSGX(OAL_ERROR, (L"SDCardReadMultiSectors: Error reading file! (sector %d, num sec %d)\r\n", LogicalSector, numSectors));
        return DL_MMC_ERROR;
    }

    return DL_SUCCESS;
}


//==============================================================================
// Public Functions
//==============================================================================

//------------------------------------------------------------------------------
//
//  Function:  BLSDCardDownload
//
//  This function initialize SDCard controller and call download function from
//  bootloader common library.
//
UINT32
BLSDCardDownload(WCHAR *szFilename)
{
	memset(&fileio_ops,0,sizeof(S_FILEIO_OPERATIONS));
	memset(&Disk,0,sizeof(DISK));

    pFile = &File;

    OALMSGX(OAL_INFO, (L"BLSDCardDownload: Filename %s\r\n", szFilename));

	if (fileio_ops.init != &SDCardInit)
	{
		OALMSGX(OAL_INFO, (L"BLSDCardDownload: fileio init\r\n"));
		// set up data structure used by file system driver
		fileio_ops.init = &SDCardInit;
		fileio_ops.identify = &SDCardIdentify;
		fileio_ops.read_sector = &SDCardReadSector;
		fileio_ops.read_multi_sectors = &SDCardReadMultiSectors;
		fileio_ops.drive_info = (PVOID)&Disk;

		// initialize file system driver
		if (FileIoInit(&fileio_ops) != FILEIO_STATUS_OK)
		{
			OALMSGX(OAL_ERROR, (L"BLSDCardDownload:  fileio init failed\r\n"));
			return (UINT32) BL_ERROR;
		}
	}

    // fill in file name (8.3 format)
    FileNameToDirEntry(szFilename, pFile->name, pFile->extension);

    // try to open file specified by pConfig->filename, return BL_ERROR on failure
    if (FileIoOpen(&fileio_ops, pFile) != FILEIO_STATUS_OK)
    {
        OALMSGX(OAL_ERROR, (L"BLSDCardDownload:  cannot open file\r\n"));
        return (UINT32) BL_ERROR;
    }

    // return BL_DOWNLOAD, BootloaderMain will then call OEMReadData
    // (which calls BLSDCardReadData) to get image data.
    return BL_DOWNLOAD;
}


//------------------------------------------------------------------------------
//
//  Function:   BLSDCardReadData
//
//  This function is called to read data from the transport during
//  the download process.
//
BOOL
BLSDCardReadData(
                 ULONG size,
                 UCHAR *pData
                 )
{
    // called to read data from MMC/SD card as stream data, not as block data

    OALMSGX(OAL_FUNC, (L"BLSDCardReadData: address 0x%x, %d bytes\r\n", pData, size));

    //g_eboot.readSize = size;
    //g_eboot.pReadBuffer = pData;

    if (FileIoRead(&fileio_ops, pFile, (PVOID)pData, size) != FILEIO_STATUS_OK)
        return FALSE;
    else
        return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:   BLSDCardReadLogo
//
//  This function is called to read the splash screen bitmap from SDCard
//  If called pData NULL it will attempt to get the file size
//  and return it in the size parameter field.
//
//
BOOL
BLSDCardReadLogo(
    WCHAR *filename,
    UCHAR *pData,
	DWORD *size
	)
{
	FILEHANDLE logoFile;
	WORD	   wSignature = 0;
	DWORD	   dwOffset = 0;
	BYTE*	   pTmpBuf = NULL;
	DWORD	   dwCursor = 0;
	DWORD datasize;

	memset(&fileio_ops,0,sizeof(S_FILEIO_OPERATIONS));
	memset(&Disk,0,sizeof(DISK));

	// set up data structure used by file system driver
	fileio_ops.init = &SDCardInit;
	fileio_ops.identify = &SDCardIdentify;
	fileio_ops.read_sector = &SDCardReadSector;
	fileio_ops.read_multi_sectors = &SDCardReadMultiSectors;
	fileio_ops.drive_info = (PVOID)&Disk;

	// initialize file system driver
	if (FileIoInit(&fileio_ops) != FILEIO_STATUS_OK)
	{
	OALMSGX(OAL_ERROR, (L"BLSDCardReadLogo:  fileio init failed\r\n"));
		return FALSE;
	}

    // fill in file name (8.3 format)
    FileNameToDirEntry(filename, logoFile.name, logoFile.extension);
	
    // try to open file specified by pConfig->filename, return BL_ERROR on failure
    if (FileIoOpen(&fileio_ops, &logoFile) != FILEIO_STATUS_OK)
    {
        OALMSGX(OAL_ERROR, (L"BLSDCardReadLogo:  cannot open %s\r\n", filename));
        return FALSE;
    }

	// Read signature
    if (FileIoRead(&fileio_ops, &logoFile, (PVOID)&wSignature, sizeof(wSignature)) != FILEIO_STATUS_OK)
	{
        OALMSGX(OAL_ERROR, (L"BLSDCardReadLogo:  cannot read file signature\r\n"));
        return FALSE;
	}

	dwCursor += sizeof(wSignature);

    if( wSignature != 0x4D42 )  
    {
        OALMSGX(OAL_ERROR, (L"BLSDCardReadLogo:  Invalid file signature\r\n"));
        return FALSE;
	}

	// Read dummy data
	pTmpBuf = (BYTE*)OALLocalAlloc(0, 2*sizeof(DWORD));
    if (FileIoRead(&fileio_ops, &logoFile, (PVOID)pTmpBuf, 2*sizeof(DWORD)) != FILEIO_STATUS_OK)
	{
        OALMSGX(OAL_ERROR, (L"BLSDCardReadLogo:  cannot read dummy file header\r\n"));
		OALLocalFree((HLOCAL)pTmpBuf);
        return FALSE;
	}

	OALLocalFree((HLOCAL)pTmpBuf);
	dwCursor += 2*sizeof(DWORD);

	// Read pixel data offset
    if (FileIoRead(&fileio_ops, &logoFile, (PVOID)&dwOffset, sizeof(dwOffset)) != FILEIO_STATUS_OK)
	{
        OALMSGX(OAL_ERROR, (L"BLSDCardReadLogo:  cannot read pixel data offset\r\n"));
        return FALSE;
	}

	dwCursor += sizeof(dwOffset);

	// Read dummy data before pixel data offset
	pTmpBuf = (BYTE*)OALLocalAlloc(0, dwOffset - dwCursor);
    if (FileIoRead(&fileio_ops, &logoFile, (PVOID)pTmpBuf, dwOffset - dwCursor) != FILEIO_STATUS_OK)
	{
        OALMSGX(OAL_ERROR, (L"BLSDCardReadLogo:  cannot read file\r\n"));
		OALLocalFree((HLOCAL)pTmpBuf);
        return FALSE;
	}

	OALLocalFree((HLOCAL)pTmpBuf);

	datasize = logoFile.file_size - dwOffset;
	if (datasize != *size)
	{
        OALMSGX(OAL_ERROR, (L"BLSDCardReadLogo: file too big\r\n"));
        return FALSE;
	}

	// Read pixel data
    if (FileIoRead(&fileio_ops, &logoFile, (PVOID)pData, datasize) != FILEIO_STATUS_OK)
	{
        OALMSGX(OAL_ERROR, (L"BLSDCardReadLogo:  cannot read pixel data\r\n"));
        return FALSE;
	}

	return TRUE;
}
    
//------------------------------------------------------------------------------

WCHAR *BLSDCardCfgLineGetField(WCHAR *Line)
{
    WCHAR *pwc = Line;

    if (*pwc == ';')
    {
        // a comment line
        return NULL;
    }

    while((*pwc != ':') && (*pwc != 0))
        ++pwc;

    if(*pwc == ':')
    {
        return (Line);
    }
    else
    {
        return NULL;
    }
}


WCHAR *BLSDCardCfgLineGetValue(WCHAR *Line)
{
    WCHAR *pwc = Line;
    WCHAR *pVal;

    // skip to ':'
    while((*pwc != ':') && (*pwc != 0))
        ++pwc;

    // no ':' exists
    if(*pwc == 0)
        return NULL;

    // found ':', nullify it and skip to next wchar
    *pwc = 0;
    ++pwc;

    // skip white space after ':'
    while (*pwc == ' ')
        ++pwc;

    if (*pwc == 0)
        // No value found
        return NULL;

    pVal = pwc;

    // remove white spce after value
    while (*pwc != ' ' && *pwc != 0)
        ++pwc;
    
    *pwc = 0;

    return (pVal);
}


BOOL BLSDCardCfgGetLine(
    WCHAR    *CfgBuf,
    DWORD    *pos,
    WCHAR    *Line,
	DWORD    size
)
{
    WCHAR  *pwc;
    DWORD i;
    BOOL  bCompleteLine = FALSE;
    DWORD skipped = 0;

	pwc = CfgBuf + *pos;

    i = 0;
    while (*pwc != 0 && (i < size-1))
    {
        Line[i] = *pwc;

        if (Line[i] == '\n' && Line[i-1] == '\r')
        {
            Line[i-1] = 0;
            *pos = *pos+(i+1); // update pos for next getline call
            bCompleteLine = TRUE;
            break;
        }
        ++i;
        ++pwc;
    }

    if (bCompleteLine == TRUE)
    {
        // a complete line (with \r\n and less than (size-1) chars is found
        if (i < size -1)
            Line[i+1] = 0;
        else
            Line[size-1] = 0;
        return TRUE;
    }

    if (*pwc == 0)
    {
        // line ends without a "\r\n"
        *pos = *pos+i;
        Line[i] = 0;
        return TRUE;
    }

    // Line is longer than size chars
    // skip the remaining of the line and update with the correct pos
    while (*pwc != 0)
    {
        ++pwc;
        ++skipped;

        if(skipped > 1)
        {
            if ((*(pwc-2) == '\r') && (*(pwc-1) == '\n'))
            {
                //enough
                break;
            }
        }
    }

    *pos = *pos + (size -1) + skipped;
    Line[size-1] = 0;

    return (TRUE);
}


//------------------------------------------------------------------------------
//
//  Function:   BLSDCardReadCfg
//
//  This function is called to read the bootloader configuration file from the SDCard
//
//
BOOL
BLSDCardReadCfg(
    WCHAR *filename,
    UCHAR *pData,
	DWORD size
)
{
	FILEHANDLE cfgFile;

	memset(&fileio_ops,0,sizeof(S_FILEIO_OPERATIONS));
	memset(&Disk,0,sizeof(DISK));

		// set up data structure used by file system driver
		fileio_ops.init = &SDCardInit;
		fileio_ops.identify = &SDCardIdentify;
		fileio_ops.read_sector = &SDCardReadSector;
		fileio_ops.read_multi_sectors = &SDCardReadMultiSectors;
		fileio_ops.drive_info = (PVOID)&Disk;

		// initialize file system driver
		if (FileIoInit(&fileio_ops) != FILEIO_STATUS_OK)
		{
		OALMSGX(OAL_ERROR, (L"BLSDCardReadCfg:  fileio init failed\r\n"));
			return FALSE;
		}

    // fill in file name (8.3 format)
    FileNameToDirEntry(filename, cfgFile.name, cfgFile.extension);
	
    // try to open file specified by pConfig->filename, return BL_ERROR on failure
    if (FileIoOpen(&fileio_ops, &cfgFile) != FILEIO_STATUS_OK)
    {
        OALMSGX(OAL_ERROR, (L"BLSDCardReadCfg:  cannot open %s\r\n", filename));
        return FALSE;
    }

    if (FileIoRead(&fileio_ops, &cfgFile, (PVOID)pData, size) != FILEIO_STATUS_OK)
	{
        OALMSGX(OAL_ERROR, (L"BLSDCardReadCfg:  cannot read file\r\n"));
        return FALSE;
	}

    OALMSGX(OAL_ERROR, (L"read %s\r\n", filename));

	return TRUE;
}
    


