/*
 * (C) Copyright 2010-2011 Texas Instruments, <www.ti.com>
 * Mansoor Ahamed <mansoor.ahamed@ti.com>
 *
 * BCH Error Location Module (ELM) support.
 *
 * NOTE:
 * 1. Supports only continuous mode. Dont see need for page mode in uboot
 * 2. Supports only syndrome polynomial 0. i.e. poly local variable is
 *    always set to ELM_DEFAULT_POLY. Dont see need for other polynomial
 *    sets in uboot
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include <windows.h>
#include <ceddk.h>
#include <ceddkex.h>
#include <oal.h>
#include <oalex.h>
#include "omap.h"
#include "omap_gpmc_regs.h"
#include <gpmc_ecc.h>
#include <elm.h>
#include <oal_clock.h>

#define ELM_DEFAULT_POLY (0)
/*
 * ELM Module Registers
 */

/* ELM registers bit fields */
#define ELM_SYSCONFIG_SOFTRESET_MASK			(0x2)
#define ELM_SYSCONFIG_SOFTRESET			(0x2)
#define ELM_SYSSTATUS_RESETDONE_MASK			(0x1)
#define ELM_SYSSTATUS_RESETDONE			(0x1)
#define ELM_LOCATION_CONFIG_ECC_BCH_LEVEL_MASK		(0x3)
#define ELM_LOCATION_CONFIG_ECC_SIZE_MASK		(0x7FF0000)
#define ELM_LOCATION_CONFIG_ECC_SIZE_POS		(16)
#define ELM_SYNDROME_FRAGMENT_6_SYNDROME_VALID		(0x00010000)
#define ELM_LOCATION_STATUS_ECC_CORRECTABLE_MASK	(0x100)
#define ELM_LOCATION_STATUS_ECC_NB_ERRORS_MASK		(0x1F)

#define ELM_REGS_PA                     0x48080000

#define u32 UINT32
#define u8   UINT8
#define s8   INT8
#define writel(val, ptr) OUTREG32(ptr, val)
#define readl(ptr) INREG32(ptr)

enum bch_level {
	BCH_4_BIT = 0,
	BCH_8_BIT,
	BCH_16_BIT
};


/* BCH syndrome registers */
struct syndrome {
	u32 syndrome_fragment_x[7]; 	
	u8 res1[36]; 			/* 0x41c */
};

/* BCH error status & location register */
struct location {
	u32 location_status;		/* 0x800 */
	u8 res1[124];			/* 0x804 */
	u32 error_location_x[16];	/* 0x880.... */
	u8 res2[64];			/* 0x8c0 */
};

/* BCH ELM register map - do not try to allocate memmory for this structure.
 * We have used plenty of reserved variables to fill the slots in the ELM
 * register memory map.
 * Directly initialize the struct pointer to ELM base address.
 */
struct elm {
	u32 rev; 				/* 0x000 */
	u8 res1[12];				/* 0x004 */
	u32 sysconfig;  			/* 0x010 */
	u32 sysstatus;				/* 0x014 */
	u32 irqstatus;				/* 0x018 */
	u32 irqenable;				/* 0x01c */
	u32 location_config;			/* 0x020 */
	u8 res2[92]; 				/* 0x024 */
	u32 page_ctrl;				/* 0x080 */
	u8 res3[892]; 				/* 0x084 */
	struct  syndrome syndrome_fragments[8]; /* 0x400 */
	u8 res4[512]; 				/* 0x600 */
	struct location  error_location[8]; 	/* 0x800 */
};

int elm_check_error(u8 *syndrome, u32 nibbles, u32 *error_count,
		u32 *error_locations);
int elm_config(enum bch_level level);
void elm_reset(void);
void elm_init(void);

struct elm *elm_cfg;


/**
 * elm_load_syndromes - Load BCH syndromes based on nibble selection
 * @syndrome: BCH syndrome
 * @nibbles:
 * @poly: Syndrome Polynomial set to use
 *
 * Load BCH syndromes based on nibble selection
 */
static void elm_load_syndromes(u8 *syndrome, u32 nibbles, u8 poly)
{
	u32 *ptr;
	u32 val;

	/* reg 0 */
	ptr = &elm_cfg->syndrome_fragments[poly].syndrome_fragment_x[0];
	val = syndrome[0] | (syndrome[1] << 8) | (syndrome[2] << 16) |
				(syndrome[3] << 24);
	writel(val, ptr);
	/* reg 1 */
	ptr = &elm_cfg->syndrome_fragments[poly].syndrome_fragment_x[1];
	val = syndrome[4] | (syndrome[5] << 8) | (syndrome[6] << 16) |
				(syndrome[7] << 24);
	writel(val, ptr);

	/* BCH 8-bit with 26 nibbles (4*8=32) */
	if (nibbles > 13) {
		/* reg 2 */
		ptr = &elm_cfg->syndrome_fragments[poly].syndrome_fragment_x[2];
		val = syndrome[8] | (syndrome[9] << 8) | (syndrome[10] << 16) |
				(syndrome[11] << 24);
		writel(val, ptr);
		/* reg 3 */
		ptr = &elm_cfg->syndrome_fragments[poly].syndrome_fragment_x[3];
		val = syndrome[12] | (syndrome[13] << 8) | (syndrome[14] << 16) |
				(syndrome[15] << 24);
		writel(val, ptr);
	}

	/* BCH 16-bit with 52 nibbles (7*8=56) */
	if (nibbles > 26) {
		/* reg 4 */
		ptr = &elm_cfg->syndrome_fragments[poly].syndrome_fragment_x[4];
		val = syndrome[16] | (syndrome[17] << 8) | (syndrome[18] << 16) |
				(syndrome[19] << 24);
		writel(val, ptr);

		/* reg 5 */
		ptr = &elm_cfg->syndrome_fragments[poly].syndrome_fragment_x[5];
		val = syndrome[20] | (syndrome[21] << 8) | (syndrome[22] << 16) |
				(syndrome[23] << 24);
		writel(val, ptr);

		/* reg 6 */
		ptr = &elm_cfg->syndrome_fragments[poly].syndrome_fragment_x[6];
		val = syndrome[24] | (syndrome[25] << 8) | (syndrome[26] << 16) |
				(syndrome[27] << 24);
		writel(val, ptr);
	}
}

/**
 * elm_check_errors - Check for BCH errors and return error locations
 * @syndrome: BCH syndrome
 * @nibbles:
 * @error_count: Returns number of errrors in the syndrome
 * @error_locations: Returns error locations (in decimal) in this array
 *
 * Check the provided syndrome for BCH errors and return error count
 * and locations in the array passed. Returns -1 if error is not correctable,
 * else returns 0
 */
int elm_check_error(u8 *syndrome, u32 nibbles, u32 *error_count,
		u32 *error_locations)
{
	u8 poly = ELM_DEFAULT_POLY;
	u8 i;
	u32 location_status;

	elm_load_syndromes(syndrome, nibbles, poly);

	/* start processing */
	writel((readl(&elm_cfg->syndrome_fragments[poly].syndrome_fragment_x[6])
				| ELM_SYNDROME_FRAGMENT_6_SYNDROME_VALID),
			&elm_cfg->syndrome_fragments[poly].syndrome_fragment_x[6]);

	/* wait for processing to complete */
	while((readl(&elm_cfg->irqstatus) & (0x1 << poly)) != 0x1);
	/* clear status */
	writel((readl(&elm_cfg->irqstatus) | (0x1 << poly)), &elm_cfg->irqstatus);

	/* check if correctable */
	location_status = readl(&elm_cfg->error_location[poly].location_status);
	if (!(location_status & ELM_LOCATION_STATUS_ECC_CORRECTABLE_MASK))
		return -1;

	/* get error count */
	*error_count = readl(&elm_cfg->error_location[poly].location_status) &
						ELM_LOCATION_STATUS_ECC_NB_ERRORS_MASK;

	for (i = 0; i < *error_count; i++)
		error_locations[i] =
			readl(&elm_cfg->error_location[poly].error_location_x[i]);

	return 0;
}


/**
 * elm_config - Configure ELM module
 * @level: 4 / 8 / 16 bit BCH
 * @buffer_size: Buffer size in bytes
 *
 * Configure ELM module based on BCH level and buffer size passed.
 * Set mode as continuous mode.
 * Currently we are using only syndrome 0 and syndromes 1 to 6 are not used.
 * Also, the mode is set only for syndrome 0
 */
/* int elm_config(enum bch_level level, u32 buffer_size) */
int elm_config(enum bch_level level)
{
	u32 val;
	u8 poly = ELM_DEFAULT_POLY;
	u32 buffer_size= 0x7FF;

	/* config size and level */
	val = (u32)(level) & ELM_LOCATION_CONFIG_ECC_BCH_LEVEL_MASK;
	val |= ((buffer_size << ELM_LOCATION_CONFIG_ECC_SIZE_POS) &
				ELM_LOCATION_CONFIG_ECC_SIZE_MASK);
	writel(val, &elm_cfg->location_config);

	/* config continous mode */
	/* enable interrupt generation for syndrome polynomial set */
	writel((readl(&elm_cfg->irqenable) | (0x1 << poly)), &elm_cfg->irqenable);
	/* set continuous mode for the syndrome polynomial set */
	writel((readl(&elm_cfg->page_ctrl) & ~(0x1 << poly)), &elm_cfg->page_ctrl);
	
	return 0;
}

/**
 * elm_reset - Do a soft reset of ELM
 *
 * Perform a soft reset of ELM and return after reset is done.
 */
void elm_reset(void)
{
	/* initiate reset */
	writel((readl(&elm_cfg->sysconfig) | ELM_SYSCONFIG_SOFTRESET),
				&elm_cfg->sysconfig);

	/* wait for reset complete and normal operation */
	while((readl(&elm_cfg->sysstatus) & ELM_SYSSTATUS_RESETDONE) !=
		ELM_SYSSTATUS_RESETDONE);
}

/**
 * elm_init - Initialize ELM module
 *
 * Initialize ELM support. Currently it does only base address init
 * and ELM reset.
 */
void elm_init( void )
{
#ifdef BOOT_MODE    
	elm_cfg = (struct elm *)OALPAtoUA(ELM_REGS_PA);
#else
        PHYSICAL_ADDRESS pa;

        pa.QuadPart = ELM_REGS_PA;
	 //TODO change the size to #define
	 elm_cfg = (struct elm *)MmMapIoSpace(pa, 0x1000, FALSE);		
#endif
	elm_reset();

}

/*
 * ti81xx_fix_errors_bch - Correct bch error in the data
 *
 * @mtd:	MTD device structure
 * @data:	Data read from flash
 * @error_count:Number of errors in data
 * @error_loc:	Locations of errors in the data
 *
 */
static void ti81xx_fix_errors_bch(enum bch_level type, u8 *data,
		u32 error_count, u32 *error_loc)
{
	u8 count = 0;
	u32 error_byte_pos;
	u32 error_bit_mask;
	u32 nibbles, last_bit;

	switch(type) {
		case BCH_4_BIT:
			nibbles = 13;
			break;

		case BCH_16_BIT:
			nibbles = 52;
			break;

		case BCH_8_BIT:
		default:
			nibbles = 26;
			break;
	}
       last_bit = (nibbles * 4) - 1;
	   
	/* Flip all bits as specified by the error location array. */
	/* FOR( each found error location flip the bit ) */
	for (count = 0; count < error_count; count++) {
		if (error_loc[count] > last_bit) {
			/* Remove the ECC spare bits from correction. */
			error_loc[count] -= (last_bit + 1);
			/* Offset bit in data region */
			error_byte_pos = (512 ) - (error_loc[count] /8 ) -1;
			/* Error Bit mask */
			error_bit_mask = 0x1 << (error_loc[count] % 8);
			/* Toggle the error bit to make the correction. */
			data[error_byte_pos] ^= error_bit_mask;
		}
	}
}

/*
 * ti81xx_rotate_ecc_bch - Rotate the syndrome bytes
 *
 * @mtd:	MTD device structure
 * @calc_ecc:	ECC read from ECC registers
 * @syndrome:	Rotated syndrome will be retuned in this array
 *
 */
static void ti81xx_rotate_ecc_bch(enum bch_level type, u8 *calc_ecc,
		u8 *syndrome)
{
	u8 n_bytes = 0;
	u8 i,j;

	switch(type) {
		case BCH_4_BIT:
			n_bytes = 8;
			break;

		case BCH_16_BIT:
			n_bytes = 28;
			break;

		case BCH_8_BIT:
		default:
			n_bytes = 13;
			break;
	}

	for (i = 0, j = (n_bytes-1); i < n_bytes; i++, j--)
		syndrome[i] =  calc_ecc[j];

}


//-----------------------------------------------------------------------------
VOID
BCH8_ELM_ECC_Init(
    OMAP_GPMC_REGS *pGpmcRegs,
    UINT configMask,
    UINT xfer_mode
    )
{
    UINT32 ecc_conf , ecc_size_conf=0;

    ecc_conf = configMask | GPMC_ECC_CONFIG_BCH | GPMC_ECC_CONFIG_BCH8; 
	
    switch (xfer_mode) 
    {
        case NAND_ECC_READ:
          /* configration is for ECC at 1 bytes padding */
          ecc_size_conf = (0x2 << 22) | (0x1A << 12);  
          ecc_conf |= ((0x01 << 8)  | (0x1));
          break;
		  
        case NAND_ECC_WRITE:
          /* configration is for ECC at 2 bytes offset  */
          ecc_size_conf = (0x1C << 22) ;
          ecc_conf |= ((0x01 << 8)  |(0x1));
          break;
		  
        default:
          RETAILMSG(1, (L"Error: Unrecognized Mode[%d]!\r\n", xfer_mode));
          break;
    }


    OUTREG32(&pGpmcRegs->GPMC_ECC_CONTROL, GPMC_ECC_CONTROL_POINTER1);
    //  Set ECC field sizes
    OUTREG32(&pGpmcRegs->GPMC_ECC_SIZE_CONFIG, ecc_size_conf);

    //  Configure ECC calculator engine for NAND part
    OUTREG32(&pGpmcRegs->GPMC_ECC_CONFIG, ecc_conf );

    //  Select result reg 1 and clear results
    OUTREG32(&pGpmcRegs->GPMC_ECC_CONTROL, GPMC_ECC_CONTROL_CLEAR | GPMC_ECC_CONTROL_POINTER1);
}

//-----------------------------------------------------------------------------
VOID
BCH8_ELM_ECC_Calculate(
    OMAP_GPMC_REGS *pGpmcRegs,
    BYTE *pEcc,
    int size
    )
{
    UINT8   eccIndex=0;
    UINT32  regVal1, regVal2, regVal3, regVal4, i;
	
    UNREFERENCED_PARAMETER(size); 

    // the ecc engine is setup encode 512 bytes at a time
    // so reading a sectore of 2048 bytes will require 4 sets of encoded
    // groups
    
    for (i=0;i<4;i++)
    {
        /* Reading HW ECC_BCH_Results
         * 0x240-0x24C, 0x250-0x25C, 0x260-0x26C, 0x270-0x27C
         */
        regVal1 = INREG32((UINT32*)&pGpmcRegs->GPMC_BCH_RESULT[i].GPMC_BCH_RESULT0);
        regVal2 = INREG32((UINT32*)&pGpmcRegs->GPMC_BCH_RESULT[i].GPMC_BCH_RESULT1);
        regVal3 = INREG32((UINT32*)&pGpmcRegs->GPMC_BCH_RESULT[i].GPMC_BCH_RESULT2);
        regVal4 = INREG32((UINT32*)&pGpmcRegs->GPMC_BCH_RESULT[i].GPMC_BCH_RESULT3);

        pEcc[eccIndex++]  = (BYTE)(regVal4 & 0xFF);
        pEcc[eccIndex++]  = (BYTE)((regVal3 >> 24) & 0xFF);
        pEcc[eccIndex++]  = (BYTE)((regVal3 >> 16) & 0xFF);
        pEcc[eccIndex++]  = (BYTE)((regVal3 >> 8) & 0xFF);
        pEcc[eccIndex++]  = (BYTE)(regVal3 & 0xFF);
        pEcc[eccIndex++]  = (BYTE)((regVal2 >> 24) & 0xFF);
   
        pEcc[eccIndex++]  = (BYTE)((regVal2 >> 16) & 0xFF);
        pEcc[eccIndex++]  = (BYTE)((regVal2 >> 8) & 0xFF);
        pEcc[eccIndex++]  = (BYTE)(regVal2 & 0xFF);
        pEcc[eccIndex++]  = (BYTE)((regVal1 >> 24) & 0xFF);
        pEcc[eccIndex++]  = (BYTE)((regVal1 >> 16) & 0xFF);
        pEcc[eccIndex++]  = (BYTE)((regVal1 >> 8) & 0xFF);
        pEcc[eccIndex++]  = (BYTE)(regVal1 & 0xFF);
        pEcc[eccIndex++]  = 0xFF;   //redundent byte required by Centaurus BootROM

    }
    return;
}

//-----------------------------------------------------------------------------
// Function:    BCH8_ELM_ECC_CorrectData()
//
// Description: Call to correct errors (if possible) in the specified data.
//
// Notes:       This implemention uses 3 bytes of ECC info for every 512 bytes
//              of data.  Furthermore, only single bit errors can be corrected
//              for every 512 bytes of data.
// 
//              Based off of algorithm described at www.samsung.com
//              http://www.samsung.com/global/business/semiconductor/products/flash/FlashApplicationNote.html
//
// Returns:     Boolean indicating if the data was corrected.
//
BOOL BCH8_ELM_ECC_CorrectData(
    OMAP_GPMC_REGS *pGpmcRegs,  // GPMC register
    BYTE *pData,                // Data buffer
    int sizeData,               // Count of bytes in data buffer
    BYTE const *pEccOld,        // Pointer to the ECC on flash
    BYTE const *pEccNew         // Pointer to the ECC the caller calculated
    )
{
    BYTE *read_ecc, *cal_ecc;
    u8 syndrome[28];
    u32 error_count = 0;
    u32 error_loc[8];
    u32 i;
   
    UNREFERENCED_PARAMETER(sizeData); 


    read_ecc = (BYTE *)pEccOld;
    cal_ecc = (BYTE *)pEccNew; 

    /* check if the page is a clean page with all 0xFF */
    for (i=0;i<ECC_BYTES_BCH8_ELM;i++)
    {
        if(read_ecc[i] != 0xFF) break;
	 if (i == (ECC_BYTES_BCH8_ELM -1))
	 	return TRUE;
    }
	
    // 2048/512 = 4, handles 512 bytes each loop
    for (i=0;i<4;i++)
    {
        elm_reset();
        elm_config((enum bch_level)BCH_8_BIT);
    
        /* while reading ECC result we read it in big endian.
         * Hence while loading to ELM we have rotate to get the right endian.
         */
         
        ti81xx_rotate_ecc_bch(BCH_8_BIT, cal_ecc, syndrome);
        /* use elm module to check for errors */
        if (elm_check_error(syndrome, 26, &error_count, error_loc) != 0) {
        	RETAILMSG(1, (L"ECC%d: uncorrectable.\r\n", i));
        	return FALSE;
        }
        
        /* correct bch error */
        if (error_count > 0) {
        	ti81xx_fix_errors_bch(BCH_8_BIT, pData, error_count, error_loc);
        }
			
	 cal_ecc += ECC_BYTES_BCH8_ELM/4;
	 pData += 512;
    }
    return TRUE;
}

//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

//-----------------------------------------------------------------------------
VOID
BCH8_ELMX_ECC_Init(
    OMAP_GPMC_REGS *pGpmcRegs,
    UINT configMask,
    UINT xfer_mode
    )
{
    UINT32 ecc_conf , ecc_size_conf=0;

    ecc_conf = configMask | GPMC_ECC_CONFIG_BCH | GPMC_ECC_CONFIG_BCH8; 
	
    switch (xfer_mode) 
    {
        case NAND_ECC_READ:
          /* configration is for ECC at 1 bytes padding */
          ecc_size_conf = (0x2 << 22) | (0x1A << 12);  
          ecc_conf |= ((0x01 << 8)  | (0x1));
          break;
		  
        case NAND_ECC_WRITE:
          /* configration is for ECC at 2 bytes offset  */
          ecc_size_conf = (0x1C << 22) ;
          ecc_conf |= ((0x01 << 8)  |(0x1));
          break;
		  
        default:
          RETAILMSG(1, (L"Error: Unrecognized Mode[%d]!\r\n", xfer_mode));
          break;
    }


    OUTREG32(&pGpmcRegs->GPMC_ECC_CONTROL, GPMC_ECC_CONTROL_POINTER1);
    //  Set ECC field sizes
    OUTREG32(&pGpmcRegs->GPMC_ECC_SIZE_CONFIG, ecc_size_conf);

    //  Configure ECC calculator engine for NAND part
    OUTREG32(&pGpmcRegs->GPMC_ECC_CONFIG, ecc_conf );

    //  Select result reg 1 and clear results
    OUTREG32(&pGpmcRegs->GPMC_ECC_CONTROL, GPMC_ECC_CONTROL_CLEAR | GPMC_ECC_CONTROL_POINTER1);
}

//-----------------------------------------------------------------------------
VOID
BCH8_ELMX_ECC_Calculate(
    OMAP_GPMC_REGS *pGpmcRegs,
    BYTE *pEcc,
    int size
    )
{
    UINT8   eccIndex=0;
    UINT32  regVal1, regVal2, regVal3, regVal4, i;
	
    UNREFERENCED_PARAMETER(size); 

    // the ecc engine is setup encode 512 bytes at a time
    // so reading a sectore of 2048 bytes will require 4 sets of encoded
    // groups
    
    for (i=0;i<4;i++)
    {
        /* Reading HW ECC_BCH_Results
         * 0x240-0x24C, 0x250-0x25C, 0x260-0x26C, 0x270-0x27C
         */
        regVal1 = INREG32((UINT32*)&pGpmcRegs->GPMC_BCH_RESULT[i].GPMC_BCH_RESULT0);
        regVal2 = INREG32((UINT32*)&pGpmcRegs->GPMC_BCH_RESULT[i].GPMC_BCH_RESULT1);
        regVal3 = INREG32((UINT32*)&pGpmcRegs->GPMC_BCH_RESULT[i].GPMC_BCH_RESULT2);
        regVal4 = INREG32((UINT32*)&pGpmcRegs->GPMC_BCH_RESULT[i].GPMC_BCH_RESULT3);

        pEcc[eccIndex++]  = (BYTE)((regVal4 >> 0) & 0xFF);

        pEcc[eccIndex++]  = (BYTE)((regVal3 >> 24) & 0xFF);
        pEcc[eccIndex++]  = (BYTE)((regVal3 >> 16) & 0xFF);
        pEcc[eccIndex++]  = (BYTE)((regVal3 >> 8)  & 0xFF);
        pEcc[eccIndex++]  = (BYTE)((regVal3 >> 0)  & 0xFF);

        pEcc[eccIndex++]  = (BYTE)((regVal2 >> 24) & 0xFF);
        pEcc[eccIndex++]  = (BYTE)((regVal2 >> 16) & 0xFF);
        pEcc[eccIndex++]  = (BYTE)((regVal2 >> 8)  & 0xFF);
        pEcc[eccIndex++]  = (BYTE)((regVal2 >> 0)  & 0xFF);

        pEcc[eccIndex++]  = (BYTE)((regVal1 >> 24) & 0xFF);
        pEcc[eccIndex++]  = (BYTE)((regVal1 >> 16) & 0xFF);
        pEcc[eccIndex++]  = (BYTE)((regVal1 >> 8)  & 0xFF);
        pEcc[eccIndex++]  = (BYTE)((regVal1 >> 0)  & 0xFF);

        pEcc[eccIndex++]  = 0xFF;   //redundent byte required by Centaurus BootROM

    }
    return;
}

//-----------------------------------------------------------------------------
// Function:    BCH8_ELMX_ECC_CorrectData()
//
// Description: Call to correct errors (if possible) in the specified data.
//
// Notes:       This implemention uses 3 bytes of ECC info for every 512 bytes
//              of data.  Furthermore, only single bit errors can be corrected
//              for every 512 bytes of data.
// 
//              Based off of algorithm described at www.samsung.com
//              http://www.samsung.com/global/business/semiconductor/products/flash/FlashApplicationNote.html
//
// Returns:     Boolean indicating if the data was corrected.
//
BOOL BCH8_ELMX_ECC_CorrectData(
    OMAP_GPMC_REGS *pGpmcRegs,  // GPMC register
    BYTE *pData,                // Data buffer
    int sizeData,               // Count of bytes in data buffer
    BYTE const *pEccOld,        // Pointer to the ECC on flash
    BYTE const *pEccNew         // Pointer to the ECC the caller calculated
    )
{
    BYTE *read_ecc, *cal_ecc;
    u8 syndrome[28];
    u32 error_count = 0;
    u32 error_loc[8];
    u32 i;
   
    UNREFERENCED_PARAMETER(sizeData); 


    read_ecc = (BYTE *)pEccOld;
    cal_ecc = (BYTE *)pEccNew; 

    /* check if the page is a clean page with all 0xFF */
    for (i=0;i<ECC_BYTES_BCH8_ELMX;i++)
    {
        if(read_ecc[i] != 0xFF) break;
	 if (i == (ECC_BYTES_BCH8_ELMX -1))
	 	return TRUE;
    }
	
    // 2048/512 = 4, handles 512 bytes each loop
    for (i=0;i<4;i++)
    {
        elm_reset();
        elm_config((enum bch_level)BCH_8_BIT);
    
        /* while reading ECC result we read it in big endian.
         * Hence while loading to ELM we have rotate to get the right endian.
         */
         
        ti81xx_rotate_ecc_bch(BCH_8_BIT, cal_ecc, syndrome);
        /* use elm module to check for errors */
        if (elm_check_error(syndrome, 26, &error_count, error_loc) != 0) {
        	RETAILMSG(1, (L"ECC%d: uncorrectable.\r\n", i));
        	return FALSE;
        }
        
        /* correct bch error */
        if (error_count > 0) {
        	ti81xx_fix_errors_bch(BCH_8_BIT, pData, error_count, error_loc);
        }
			
	 cal_ecc += ECC_BYTES_BCH8_ELMX/4;
	 pData += 512;
    }
    return TRUE;
}

