/*
 * pru/hal/uart/src/pru.c
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
 
/************************************************************
* Include Files                                             *
************************************************************/
#include <windows.h>
//#include <psc.h>

// TI redefine E_FAIL as 0x00000001 whereas
// Windows defines it as 0x80004004
// We'll stick with the TI definition within
// the PRU Soft UART driver.
#undef E_FAIL

#if 0
#include "am33x.h"
#include "am33x_config.h"
#include "am33x_oal_prcm.h"
#endif

//#include "am33x_prcm.h"
#include "am33x_clocks.h"

#include "tistdtypes.h"
#include "csl/cslr.h"
#include "csl/cslr_prucore.h"
#include "pru.h"


#define CSL_PRUCORE_0                       (0)
#define CSL_PRUCORE_1                       (1)

extern BOOL PrcmDomainResetRelease(UINT powerDomain, UINT resetMask);
extern BOOL EnableDeviceClocks(AM33X_DEVICE_ID devID, BOOL bEnable);

/************************************************************
* Local Function Declarations                               *
************************************************************/
static void pru_psc_disable(arm_pru_iomap * pru_arm_iomap);
static void pru_psc_enable(arm_pru_iomap * pru_arm_iomap);

// Load the specified PRU with code
Uint32 pru_load(Uint8 pruNum, Uint32 * pruCode, Uint32 codeSizeInWords,
		arm_pru_iomap * pru_arm_iomap)
{
	Uint32 *pruIram;
	Uint32 i;

	if (pruNum == CSL_PRUCORE_0) {
		pruIram = (Uint32 *) ((Uint32 )pru_arm_iomap->pru_io_addr + PRU0_IRAM_BASE_ADDRESS);
	} else if (pruNum == CSL_PRUCORE_1) {
		pruIram = (Uint32 *) ((Uint32 )pru_arm_iomap->pru_io_addr + PRU1_IRAM_BASE_ADDRESS);
	} else {
		return E_FAIL;
	}

	pru_enable(pruNum, pru_arm_iomap);

	// Copy dMAX code to its instruction RAM
	for (i = 0; i < codeSizeInWords; i++) {
		pruIram[i] = pruCode[i];
	}

	return E_PASS;
}

Uint32 pru_run(Uint8 pruNum, arm_pru_iomap * pru_arm_iomap)
{
	CSL_PrucoreRegsOvly hPru;

	if (pruNum == CSL_PRUCORE_0) {
		hPru = (CSL_PrucoreRegsOvly) ((Uint32 )pru_arm_iomap->pru_io_addr + PRU0_CONTROL_BASE_ADDRESS);	//CSL_PRUCORE_0_REGS;
	} else if (pruNum == CSL_PRUCORE_1) {
		hPru = (CSL_PrucoreRegsOvly) ((Uint32 )pru_arm_iomap->pru_io_addr + PRU1_CONTROL_BASE_ADDRESS);	//CSL_PRUCORE_1_REGS;
	} else {
		return E_FAIL;
	}

	// Enable dMAX, let it execute the code we just copied
	CSL_FINST(hPru->CONTROL, PRUCORE_CONTROL_COUNTENABLE, ENABLE);
	CSL_FINST(hPru->CONTROL, PRUCORE_CONTROL_ENABLE, ENABLE);
	return E_PASS;
}

Uint32 pru_waitForHalt(Uint8 pruNum, Int32 timeout,
		       arm_pru_iomap * pru_arm_iomap)
{
	CSL_PrucoreRegsOvly hPru;

	Int32 cnt = timeout;

	if (pruNum == CSL_PRUCORE_0) {
		hPru = (CSL_PrucoreRegsOvly) ((Uint32 )pru_arm_iomap->pru_io_addr + PRU0_CONTROL_BASE_ADDRESS);	//CSL_PRUCORE_0_REGS;
	} else if (pruNum == CSL_PRUCORE_1) {
		hPru = (CSL_PrucoreRegsOvly) ((Uint32 )pru_arm_iomap->pru_io_addr + PRU1_CONTROL_BASE_ADDRESS);	//CSL_PRUCORE_1_REGS;
	} else {
		return E_FAIL;
	}

	while (CSL_FEXT(hPru->CONTROL, PRUCORE_CONTROL_RUNSTATE) ==
	       CSL_PRUCORE_CONTROL_RUNSTATE_RUN) {
		if (cnt > 0) {
			cnt--;
		}
		if (cnt == 0) {
			return E_TIMEOUT;
		}
	}

	return E_PASS;
}

Uint32 pru_disable(arm_pru_iomap * pru_arm_iomap)
{
	CSL_PrucoreRegsOvly hPru;
	unsigned int delay_cnt;

	// Disable PRU0
	hPru = (CSL_PrucoreRegsOvly) ((Uint32 )pru_arm_iomap->pru_io_addr + PRU0_CONTROL_BASE_ADDRESS);	//CSL_PRUCORE_0_REGS;  
	CSL_FINST(hPru->CONTROL, PRUCORE_CONTROL_COUNTENABLE, DISABLE);
	for (delay_cnt = 0x10000; delay_cnt > 0; delay_cnt--)
	CSL_FINST(hPru->CONTROL, PRUCORE_CONTROL_ENABLE, DISABLE);

	for (delay_cnt = 0x10000; delay_cnt > 0; delay_cnt--)
	// Reset PRU0
	hPru->CONTROL = CSL_PRUCORE_CONTROL_RESETVAL;

	// Disable PRU1
	hPru = (CSL_PrucoreRegsOvly) ((Uint32 )pru_arm_iomap->pru_io_addr + PRU1_CONTROL_BASE_ADDRESS);	//CSL_PRUCORE_1_REGS;  
	CSL_FINST(hPru->CONTROL, PRUCORE_CONTROL_COUNTENABLE, DISABLE);

	for (delay_cnt = 0x10000; delay_cnt > 0; delay_cnt--)
	CSL_FINST(hPru->CONTROL, PRUCORE_CONTROL_ENABLE, DISABLE);

	for (delay_cnt = 0x10000; delay_cnt > 0; delay_cnt--)
	// Reset PRU1
	hPru->CONTROL = CSL_PRUCORE_CONTROL_RESETVAL;

	return E_PASS;
}

Uint32 pru_enable(Uint8 pruNum, arm_pru_iomap * pru_arm_iomap)
{
	CSL_PrucoreRegsOvly hPru;

	// Enable PRU SS
	pru_psc_enable(pru_arm_iomap);

	if (pruNum == CSL_PRUCORE_0) {
		// Reset PRU0
		hPru = (CSL_PrucoreRegsOvly) ((Uint32 )pru_arm_iomap->pru_io_addr + PRU0_CONTROL_BASE_ADDRESS);	//CSL_PRUCORE_0_REGS;
		hPru->CONTROL = CSL_PRUCORE_CONTROL_RESETVAL;
	} else if (pruNum == CSL_PRUCORE_1) {
		// Reset PRU1
		hPru = (CSL_PrucoreRegsOvly) ((Uint32 )pru_arm_iomap->pru_io_addr + PRU1_CONTROL_BASE_ADDRESS);	//CSL_PRUCORE_1_REGS;
		hPru->CONTROL = CSL_PRUCORE_CONTROL_RESETVAL;
	}
	return E_PASS;
}

/************************************************************
* Local Function Definitions                                *
************************************************************/

static void pru_psc_enable(arm_pru_iomap * pru_arm_iomap)
{
    PrcmDomainResetRelease(POWERDOMAIN_PER, (1 << 1) /*ICSS_LRST*/);
    EnableDeviceClocks(32 /*AM_DEVICE_ICSS*/, TRUE);
}

static void pru_psc_disable(arm_pru_iomap * pru_arm_iomap)
{
    EnableDeviceClocks(32 /*AM_DEVICE_ICSS*/, FALSE);
}

/***********************************************************
* End file                                                 *
***********************************************************/

/** ********************************************************************************************************
 * \brief    pru_ram_write_data()       Download the data into data RAM of PRU0 or PRU1 of OMAP L138.
 *
 * This API will be called by the Application to download the data into data RAM of PRU0 or PRU1
 *
 * \param   u32offset           Offset of the data RAM where the data has to be written
 * \param   pu32datatowrite     Pointer to a buffer that holds the data to be written into RAM
* \param    u16wordstowrite     Number of bytes to be written into that RAM
 *
 * \return   SUCCESS or FAILURE
 *
 ***********************************************************************************************************/
short pru_ram_write_data
    (Uint32 u32offset,
     Uint8 * pu8datatowrite,
     Uint16 u16bytestowrite, arm_pru_iomap * pru_arm_iomap) {
	Uint8 *pu8addresstowrite;
	Uint16 u16loop;
	u32offset = (unsigned int)pru_arm_iomap->pru_io_addr + u32offset;
	pu8addresstowrite = (Uint8 *) (u32offset);

	for (u16loop = 0; u16loop < u16bytestowrite; u16loop++)
		*pu8addresstowrite++ = *pu8datatowrite++;
	return 0;
}

/** ********************************************************************************************************
 * \brief    pru_ram_read_data()        Download the data into data RAM of PRU0 or PRU1 of OMAP L138.
 *
 * This API will be called by the Application to read the data from data RAM of PRU0 or PRU1
 *
 * \param   u32offset           Offset of the data RAM where the data has to be read
 * \param   pu8datatoread      Pointer to a buffer that would hold the data to be read from the RAM
* \param    u16bytestoread      Number of bytes to be read from RAM
 *
 * \return   SUCCESS or FAILURE
 *
 ***********************************************************************************************************/
short pru_ram_read_data
    (Uint32 u32offset,
     Uint8 * pu8datatoread,
     Uint16 u16bytestoread, arm_pru_iomap * pru_arm_iomap) {

	Uint8 *pu8addresstoread;
	Uint16 u16loop;
	u32offset = (unsigned int)pru_arm_iomap->pru_io_addr + u32offset;
	pu8addresstoread = (Uint8 *) (u32offset);

	for (u16loop = 0; u16loop < u16bytestoread; u16loop++)
		*pu8datatoread++ = *pu8addresstoread++;

	return 0;
}

/** ********************************************************************************************************
 * \brief    pru_ram_write_data_4byte()		Download the data into data RAM of PRU0 or PRU1 of OMAP L138.
 *
 * This API will be called by the Application to download the data into data RAM of PRU0 or PRU1
 *
 * \param 	u32offset			Offset of the data RAM where the data has to be written
 * \param	pu32datatowrite		Pointer to a buffer that holds the data to be written into RAM
* \param	u16wordstowrite		Number of words to be written into that RAM
 *
 * \r//eturn   SUCCESS or FAILURE
 *
 ***********************************************************************************************************/
short pru_ram_write_data_4byte(unsigned int u32offset,
			       unsigned int *pu32datatowrite,
			       short u16wordstowrite)
{

	unsigned int *pu32addresstowrite;
	short u16loop;

	pu32addresstowrite = (unsigned int *)(u32offset);

	for (u16loop = 0; u16loop < u16wordstowrite; u16loop++)
		*pu32addresstowrite++ = *pu32datatowrite++;

	return 0;
}

/** ********************************************************************************************************
 * \brief    pru_ram_read_data_4byte()		Download the data into data RAM of PRU0 or PRU1 of OMAP L138.
 *
 * This API will be called by the Application to read the data from data RAM of PRU0 or PRU1
 *
 * \param 	u32offset			Offset of the data RAM where the data has to be read
 * \param	pu32datatoread		Pointer to a buffer that would hold the data to be read from the RAM
* \param	u16wordstoread		Number of words to be read from RAM
 *
 * \return   SUCCESS or FAILURE
 *
 ***********************************************************************************************************/
short pru_ram_read_data_4byte(unsigned int u32offset,
			      unsigned int *pu32datatoread,
			      short u16wordstoread)
{
	unsigned int *pu32addresstoread;
	short u16loop;

	pu32addresstoread = (unsigned int *)(u32offset);

	for (u16loop = 0; u16loop < u16wordstoread; u16loop++)
		*pu32datatoread++ = *pu32addresstoread++;

	return 0;
}

