/*
 * pru/hal/uart/include/pru.h
 *
 * Copyright (C) 2010, 2011 Texas Instruments Incorporated - http://www.ti.com/
 *
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef _PRU_H_
#define _PRU_H_

#include "tistdtypes.h"
#include "csl/cslr_prucore.h"

// Prevent C++ name mangling
#ifdef __cplusplus
extern far "c" {
#endif

#define PRU_NUM0		(0)
#define PRU_NUM1		(1)

/***********************************************************
* Global Macro Declarations                                *
***********************************************************/
// PRU Memory Macros

// Note: These local view addresses are device specific
//Data 8KB RAM 0/1
#define DATARAM0_BASE_ADDRESS       (0x00000000)
#define DATARAM1_BASE_ADDRESS       (0x00002000)
#define PRU_DATARAM01_SIZE          (0x2000)

//Data 12KB RAM 2
#define DATARAM2_BASE_ADDRESS       (0x00010000)
#define PRU_DATARAM2_SIZE           (0x3000)

#define PRU0_CONTROL_BASE_ADDRESS   (0x00022000)
#define PRU0_DEBUG_BASE_ADDRESS     (0x00022400)

#define PRU1_CONTROL_BASE_ADDRESS   (0x00024000)
#define PRU1_DEBUG_BASE_ADDRESS     (0x00024400)


#define PRU0_IRAM_BASE_ADDRESS      (0x00034000)
#define PRU1_IRAM_BASE_ADDRESS      (0x00038000)
#define PRU_IRAM_SIZE               (0x2000)

#define PRU_PRU0_BASE_ADDRESS					DATARAM0_BASE_ADDRESS
#define PRU_INTC_BASE_ADDRESS					(PRU_PRU0_BASE_ADDRESS + 0x00020000)

#define PRU_INTC_REVID							(PRU_INTC_BASE_ADDRESS + 0)
#define PRU_INTC_CONTROL						(PRU_INTC_BASE_ADDRESS + 0x4)
#define PRU_INTC_GLBLEN							(PRU_INTC_BASE_ADDRESS + 0x10)
#define PRU_INTC_GLBLNSTLVL						(PRU_INTC_BASE_ADDRESS + 0x1C)
#define PRU_INTC_STATIDXSET						(PRU_INTC_BASE_ADDRESS + 0x20)
#define PRU_INTC_STATIDXCLR						(PRU_INTC_BASE_ADDRESS + 0x24)
#define PRU_INTC_ENIDXSET						(PRU_INTC_BASE_ADDRESS + 0x28)
#define PRU_INTC_ENIDXCLR						(PRU_INTC_BASE_ADDRESS + 0x2C)
#define PRU_INTC_HSTINTENIDXSET					(PRU_INTC_BASE_ADDRESS + 0x34)
#define PRU_INTC_HSTINTENIDXCLR					(PRU_INTC_BASE_ADDRESS + 0x38)
#define PRU_INTC_GLBLPRIIDX						(PRU_INTC_BASE_ADDRESS + 0x80)
#define PRU_INTC_STATSETINT0					(PRU_INTC_BASE_ADDRESS + 0x200)
#define PRU_INTC_STATSETINT1					(PRU_INTC_BASE_ADDRESS + 0x204)
#define PRU_INTC_STATCLRINT0					(PRU_INTC_BASE_ADDRESS + 0x280)
#define PRU_INTC_STATCLRINT1					(PRU_INTC_BASE_ADDRESS + 0x284)
#define PRU_INTC_ENABLESET0						(PRU_INTC_BASE_ADDRESS + 0x300)
#define PRU_INTC_ENABLESET1						(PRU_INTC_BASE_ADDRESS + 0x304)
#define PRU_INTC_ENABLECLR0						(PRU_INTC_BASE_ADDRESS + 0x380)
#define PRU_INTC_ENABLECLR1						(PRU_INTC_BASE_ADDRESS + 0x384)
#define PRU_INTC_CHANMAP0						(PRU_INTC_BASE_ADDRESS + 0x400)
#define PRU_INTC_CHANMAP1						(PRU_INTC_BASE_ADDRESS + 0x404)
#define PRU_INTC_CHANMAP2						(PRU_INTC_BASE_ADDRESS + 0x408)
#define PRU_INTC_CHANMAP3						(PRU_INTC_BASE_ADDRESS + 0x40C)
#define PRU_INTC_CHANMAP4						(PRU_INTC_BASE_ADDRESS + 0x410)
#define PRU_INTC_CHANMAP5						(PRU_INTC_BASE_ADDRESS + 0x414)
#define PRU_INTC_CHANMAP6						(PRU_INTC_BASE_ADDRESS + 0x418)
#define PRU_INTC_CHANMAP7						(PRU_INTC_BASE_ADDRESS + 0x41C)
#define PRU_INTC_CHANMAP8						(PRU_INTC_BASE_ADDRESS + 0x420)
#define PRU_INTC_CHANMAP9						(PRU_INTC_BASE_ADDRESS + 0x424)
#define PRU_INTC_CHANMAP10						(PRU_INTC_BASE_ADDRESS + 0x428)
#define PRU_INTC_CHANMAP11						(PRU_INTC_BASE_ADDRESS + 0x42C)
#define PRU_INTC_CHANMAP12						(PRU_INTC_BASE_ADDRESS + 0x430)
#define PRU_INTC_CHANMAP13						(PRU_INTC_BASE_ADDRESS + 0x434)
#define PRU_INTC_CHANMAP14						(PRU_INTC_BASE_ADDRESS + 0x438)
#define PRU_INTC_CHANMAP15						(PRU_INTC_BASE_ADDRESS + 0x43C)
#define PRU_INTC_HOSTMAP0						(PRU_INTC_BASE_ADDRESS + 0x800)
#define PRU_INTC_HOSTMAP1						(PRU_INTC_BASE_ADDRESS + 0x804)
#define PRU_INTC_HOSTMAP2						(PRU_INTC_BASE_ADDRESS + 0x808)
#define PRU_INTC_HOSTINTPRIIDX0					(PRU_INTC_BASE_ADDRESS + 0x900)
#define PRU_INTC_HOSTINTPRIIDX1					(PRU_INTC_BASE_ADDRESS + 0x904)
#define PRU_INTC_HOSTINTPRIIDX2					(PRU_INTC_BASE_ADDRESS + 0x908)
#define PRU_INTC_HOSTINTPRIIDX3					(PRU_INTC_BASE_ADDRESS + 0x90C)
#define PRU_INTC_HOSTINTPRIIDX4					(PRU_INTC_BASE_ADDRESS + 0x910)
#define PRU_INTC_HOSTINTPRIIDX5					(PRU_INTC_BASE_ADDRESS + 0x914)
#define PRU_INTC_HOSTINTPRIIDX6					(PRU_INTC_BASE_ADDRESS + 0x918)
#define PRU_INTC_HOSTINTPRIIDX7					(PRU_INTC_BASE_ADDRESS + 0x91C)
#define PRU_INTC_HOSTINTPRIIDX8					(PRU_INTC_BASE_ADDRESS + 0x920)
#define PRU_INTC_HOSTINTPRIIDX9					(PRU_INTC_BASE_ADDRESS + 0x924)
#define PRU_INTC_POLARITY0						(PRU_INTC_BASE_ADDRESS + 0xD00)
#define PRU_INTC_POLARITY1						(PRU_INTC_BASE_ADDRESS + 0xD04)
#define PRU_INTC_TYPE0							(PRU_INTC_BASE_ADDRESS + 0xD80)
#define PRU_INTC_TYPE1							(PRU_INTC_BASE_ADDRESS + 0xD84)
#define PRU_INTC_HOSTINTNSTLVL0					(PRU_INTC_BASE_ADDRESS + 0x1100)
#define PRU_INTC_HOSTINTNSTLVL1					(PRU_INTC_BASE_ADDRESS + 0x1104)
#define PRU_INTC_HOSTINTNSTLVL2					(PRU_INTC_BASE_ADDRESS + 0x1108)
#define PRU_INTC_HOSTINTNSTLVL3					(PRU_INTC_BASE_ADDRESS + 0x110C)
#define PRU_INTC_HOSTINTNSTLVL4					(PRU_INTC_BASE_ADDRESS + 0x1110)
#define PRU_INTC_HOSTINTNSTLVL5					(PRU_INTC_BASE_ADDRESS + 0x1114)
#define PRU_INTC_HOSTINTNSTLVL6					(PRU_INTC_BASE_ADDRESS + 0x1118)
#define PRU_INTC_HOSTINTNSTLVL7					(PRU_INTC_BASE_ADDRESS + 0x111C)
#define PRU_INTC_HOSTINTNSTLVL8					(PRU_INTC_BASE_ADDRESS + 0x1120)
#define PRU_INTC_HOSTINTNSTLVL9					(PRU_INTC_BASE_ADDRESS + 0x1124)
#define PRU_INTC_HOSTINTEN						(PRU_INTC_BASE_ADDRESS + 0x1500)

/* Macros defining some PRU specific constants. */
#define PRU_INTC_HOSTINTLVL_MAX					9

/*
 *====================
 * Typedef structures
 *====================
 */

typedef struct arm_pru_iomap {
	void *pru_io_addr;
    void *pru0_dataram_base;
    void *pru1_dataram_base;
    void *intc_base;
    void *pru0_control_base;
    void *pru0_debug_base;
    void *pru1_control_base;
    void *pru1_debug_base;
    void *pru0_iram_base;
    void *pru1_iram_base;
} arm_pru_iomap;

/***********************************************************
* Global Function Declarations                             *
***********************************************************/

extern __FAR__ Uint32 pru_enable(Uint8 pruNum,
				 arm_pru_iomap * pru_arm_iomap);
extern __FAR__ Uint32 pru_load(Uint8 pruNum, Uint32 * pruCode,
				   Uint32 codeSizeInWords,
				   arm_pru_iomap * pru_arm_iomap);
extern __FAR__ Uint32 pru_run(Uint8 pruNum,
				  arm_pru_iomap * pru_arm_iomap);
extern __FAR__ Uint32 pru_waitForHalt(Uint8 pruNum, Int32 timeout,
					  arm_pru_iomap * pru_arm_iomap);
extern __FAR__ Uint32 pru_disable(arm_pru_iomap * pru_arm_iomap);

short pru_ram_write_data(Uint32 u32offset, Uint8 * pu8datatowrite,
			 Uint16 u16wordstowrite,
			 arm_pru_iomap * pru_arm_iomap);
short pru_ram_read_data(Uint32 u32offset, Uint8 * pu8datatoread,
			Uint16 u16wordstoread,
			arm_pru_iomap * pru_arm_iomap);
short pru_ram_read_data_4byte(unsigned int u32offset,
				  unsigned int *pu32datatoread,
				  short u16wordstoread);
short pru_ram_write_data_4byte(unsigned int u32offset,
				   unsigned int *pu32datatoread,
				   short u16wordstoread);

/***********************************************************
* End file                                                 *
***********************************************************/

#ifdef __cplusplus
}
#endif
#endif				// End _PRU_H_
