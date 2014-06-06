/*
 * edma3cfg.c
 *
 * EDMA3 Resource Manager Adaptation Configuration File (SoC Specific).
 *
 * Copyright (C) 2009 Texas Instruments Incorporated - http://www.ti.com/
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

#include <edma3_rm.h>
#include "am33x.h"

// Global variable to determine how many channel controllers (instances)
// are available. This is chip specific
const unsigned int g_EDMA3_NUM_INSTANCES = 1;

#define NUM_SHADOW_REGIONS					(4u)

/**
 * \brief Mapping of DMA channels 0-31 to Hardware Events from
 * various peripherals, which use EDMA for data transfer.
 * All channels need not be mapped, some can be free also.
 * 1: Mapped
 * 0: Not mapped
 *
 * This mapping will be used to allocate DMA channels when user passes
 * EDMA3_RM_DMA_CHANNEL_ANY as dma channel id (for eg to do memory-to-memory
 * copy). The same mapping is used to allocate the TCC when user passes
 * EDMA3_RM_TCC_ANY as tcc id (for eg to do memory-to-memory copy).
 *
 * To allocate more DMA channels or TCCs, one has to modify the event mapping.
 */
/* EDMA3 0 */
 												/* 31     0 */
#define DMA_CHANNEL_TO_EVENT_MAPPING_0_0		(0xFFFFFFFFu)
/**
 * EDMA channels 22 and 23, which correspond to GPIO
 * bank interrupts will be used for memory-to-memory data transfers.
 */

/* EDMA3 1 does not exist */
/* EDMA3 1 */
 												/* 31     0 */
#define DMA_CHANNEL_TO_EVENT_MAPPING_1_0		(0x0u)
/**
 * EDMA channels 19-23, 30 & 31, which donot correspond to any
 * peripheral will be used for memory-to-memory data transfers.
 */

/**
 * \brief Mapping of DMA channels 32-63 to Hardware Events from
 * various peripherals, which use EDMA for data transfer.
 * All channels need not be mapped, some can be free also.
 * 1: Mapped
 * 0: Not mapped
 *
 * This mapping will be used to allocate DMA channels when user passes
 * EDMA3_RM_DMA_CHANNEL_ANY as dma channel id (for eg to do memory-to-memory
 * copy). The same mapping is used to allocate the TCC when user passes
 * EDMA3_RM_TCC_ANY as tcc id (for eg to do memory-to-memory copy).
 *
 * To allocate more DMA channels or TCCs, one has to modify the event mapping.
 */
/* EDMA3 0 */
#define DMA_CHANNEL_TO_EVENT_MAPPING_0_1		(0x003FFFFFu)

/* EDMA3 1 does not exist */
/* EDMA3 1 */
#define DMA_CHANNEL_TO_EVENT_MAPPING_1_1		(0x0u)


EDMA3_RM_GblConfigParams platformEdma3GblCfgParams [EDMA3_MAX_EDMA3_INSTANCES] =
{
	/* EDMA3 INSTANCE# 0 */
    {
    64u,			/** Total number of DMA Channels supported by the EDMA3 Controller */
    8u,				/** Total number of QDMA Channels supported by the EDMA3 Controller */   
    64u,			/** Total number of TCCs supported by the EDMA3 Controller */
    256u,			/** Total number of PaRAM Sets supported by the EDMA3 Controller */
    3u,				/** Total number of Event Queues in the EDMA3 Controller */
    3u,				/** Total number of Transfer Controllers (TCs) in the EDMA3 Controller */
    4u,				/** Number of Regions on this EDMA3 controller */
    1u,				/* Channel mapping existence */
    0u,				/** Existence of memory protection feature */
    
    (void *)(AM33X_TPCC_REGS_PA),			/** Global Register Region of CC Registers */
    AM_DEVICE_TPCC,
    {
        (void *)(AM33X_TPTC0_REGS_PA),
        (void *)(AM33X_TPTC1_REGS_PA),
        (void *)(AM33X_TPTC2_REGS_PA),
		(void *) 0
    },
    {AM_DEVICE_TPTC0, AM_DEVICE_TPTC1, AM_DEVICE_TPTC2, 0}, 
    
    IRQ_EDMACOMPINIT,	/** Interrupt no. for Transfer Completion */
    IRQ_EDMAMPERR,		/** Interrupt no. for CC Error */
    {   IRQ_TCERR0, IRQ_TCERR1, IRQ_TCERR2, 0},

    { 4u, 0u, 5u, 1u }, /* EDMA3 TC priority setting */
    { 16u, 16u, 16u, 16u }, /* Threshold level of number of events */
    { 16u, 16u, 16u, 16u }, /* Default Burst Size (DBS) of TCs */

    /**
     * \brief Mapping from each DMA channel to a Parameter RAM set,
     * if it exists, otherwise of no use.
     */
        {
        0u,  1u,   2u,  3u,  4u,  5u,  6u,  7u,
        8u,  9u,  10u, 11u, 12u, 13u, 14u, 15u,
        16u, 17u, 18u, 19u, 20u, 21u, 22u, 23u,
        24u, 25u, 26u, 27u, 28u, 29u, 30u, 31u,

        32u, 33u, 34u, 35u, 36u, 37u, 38u, 39u,
        40u, 41u, 42u, 43u, 44u, 45u, 46u, 47u,
        48u, 49u, 50u, 51u, 52u, 53u, 54u, 55u,
        56u, 57u, 58u, 59u, 60u, 61u, 62u, 63u
        },

     /**
      * \brief Mapping from each DMA channel to a TCC. This specific
      * TCC code will be returned when the transfer is completed
      * on the mapped channel.
      */
        {
        0u, 1u, 2u, 3u,
        4u, 5u, 6u, 7u,
        8u, 9u, 10u, 11u,
        12u, 13u, 14u, 15u,
        16u, 17u, 18u, 19u,
        20u, 21u, 22u, 23u,
        24u, 25u, 26u, 27u,
        28u, 29u, 30u, 31u,
        
        32u, 33u, 34u, 35u,
        36u, 37u, 38u, 39u,
        40u, 41u, 42u, 43u,
        44u, 45u, 46u, 47u,
        48u, 49u, 50u, 51u,
        52u, 53u, EDMA3_RM_CH_NO_TCC_MAP, EDMA3_RM_CH_NO_TCC_MAP,
        EDMA3_RM_CH_NO_TCC_MAP, EDMA3_RM_CH_NO_TCC_MAP, EDMA3_RM_CH_NO_TCC_MAP, EDMA3_RM_CH_NO_TCC_MAP,
        EDMA3_RM_CH_NO_TCC_MAP, EDMA3_RM_CH_NO_TCC_MAP, EDMA3_RM_CH_NO_TCC_MAP, EDMA3_RM_CH_NO_TCC_MAP
        },

    /**
     * \brief Mapping of DMA channels to Hardware Events from
     * various peripherals, which use EDMA for data transfer.
     * All channels need not be mapped, some can be free also.
     */
        {
        DMA_CHANNEL_TO_EVENT_MAPPING_0_0,
        DMA_CHANNEL_TO_EVENT_MAPPING_0_1
        }
    },

};


/* Default RM Instance Initialization Configuration */
EDMA3_RM_InstanceInitConfig platformInstInitConfig [EDMA3_MAX_EDMA3_INSTANCES][NUM_SHADOW_REGIONS] =
{
		/* EDMA3 INSTANCE# 0 */
        {
          {
            /* Resources owned by Region 0 */
             /* ownPaRAMSets */
            /* 31     0     63    32     95    64     127   96 */
            {0xFFFFFFFFu, 0xFFFFFFFFu, 0x0000FFFFu, 0x00000000u,
            /* 159  128     191  160     223  192     255  224 */
             0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u,
            /* 287  256     319  288     351  320     383  352 */
             0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u,
            /* 415  384     447  416     479  448     511  480 */
             0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u,},

            /* ownDmaChannels */
            /* 31     0     63    32 */
            {0xFFFFFFFFu, 0xFFFFFFFFu},

            /* ownQdmaChannels */
            /* 31     0 */
            {0x000000FFu},

            /* ownTccs */
            /* 31     0     63    32 */
            {0xFFFFFFFFu, 0xFFFFFFFFu},

            /* Resources reserved by Region 0 */
            /* resvdPaRAMSets */
            /* 31     0     63    32     95    64     127   96 */
            {0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u,
            /* 159  128     191  160     223  192     255  224 */
             0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u,
            /* 287  256     319  288     351  320     383  352 */
             0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u,
            /* 415  384     447  416     479  448     511  480 */
             0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u,},

            /* resvdDmaChannels */
            /* 31     0     63    32 */
            {0x0003003fu, 0x00000000u},

            /* resvdQdmaChannels */
            /* 31     0 */
            {0x00000000u},

            /* resvdTccs */
            /* 31     0     63    32 */
            {0x00000000u, 0x00000000u},
          },

          {
            /* Resources owned by Region 1 */
            /* ownPaRAMSets */
            /* 31     0     63    32     95    64     127   96 */
            {0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u,
            /* 159  128     191  160     223  192     255  224 */
             0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u,
            /* 287  256     319  288     351  320     383  352 */
             0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u,
            /* 415  384     447  416     479  448     511  480 */
             0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u,},

            /* ownDmaChannels */
            /* 31     0     63    32 */
           	{0x00000000u, 0x00000000u},

            /* ownQdmaChannels */
            /* 31     0 */
            {0x00000000u},

            /* ownTccs */
            /* 31     0     63    32 */
            {0x00000000u, 0x00000000u},

            /* Resources reserved by Region 1 */
            /* resvdPaRAMSets */
            /* 31     0     63    32     95    64     127   96 */
            {0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u,
            /* 159  128     191  160     223  192     255  224 */
             0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u,
            /* 287  256     319  288     351  320     383  352 */
             0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u,
            /* 415  384     447  416     479  448     511  480 */
             0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u,},

            /* resvdDmaChannels */
            /* 31      0  63..32 */
            {DMA_CHANNEL_TO_EVENT_MAPPING_0_0, DMA_CHANNEL_TO_EVENT_MAPPING_0_1},

            /* resvdQdmaChannels */
            /* 31     0 */
            {0x00000000u},

            /* resvdTccs */
            /* 31      0  63..32 */
            {0x00000000u, 0x00000000u},
          },

          {
            /* Resources owned by Region 2 */
             /* ownPaRAMSets */
            /* 31     0     63    32     95    64     127   96 */
            {0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u,
            /* 159  128     191  160     223  192     255  224 */
             0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u,
            /* 287  256     319  288     351  320     383  352 */
             0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u,
            /* 415  384     447  416     479  448     511  480 */
             0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u,},

            /* ownDmaChannels */
            /* 31     0     63    32 */
            {0x00000000u, 0x00000000u},

            /* ownQdmaChannels */
            /* 31     0 */
            {0x00000000u},

            /* ownTccs */
            /* 31     0     63    32 */
            {0x00000000u, 0x00000000u},

            /* Resources reserved by Region 2 */
            /* resvdPaRAMSets */
            /* 31     0     63    32     95    64     127   96 */
            {0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u,
            /* 159  128     191  160     223  192     255  224 */
             0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u,
            /* 287  256     319  288     351  320     383  352 */
             0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u,
            /* 415  384     447  416     479  448     511  480 */
             0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u,},

            /* resvdDmaChannels */
            /* 31     0     63    32 */
            {0x00000000u, 0x00000000u},

            /* resvdQdmaChannels */
            /* 31     0 */
            {0x00000000u},

            /* resvdTccs */
            /* 31     0     63    32 */
            {0x00000000u, 0x00000000u},
          },

          {
            /* Resources owned by Region 3 */
             /* ownPaRAMSets */
            /* 31     0     63    32     95    64     127   96 */
            {0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u,
            /* 159  128     191  160     223  192     255  224 */
             0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u,
            /* 287  256     319  288     351  320     383  352 */
             0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u,
            /* 415  384     447  416     479  448     511  480 */
             0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u,},

            /* ownDmaChannels */
            /* 31     0     63    32 */
            {0x00000000u, 0x00000000u},

            /* ownQdmaChannels */
            /* 31     0 */
            {0x00000000u},

            /* ownTccs */
            /* 31     0     63    32 */
            {0x00000000u, 0x00000000u},

            /* Resources reserved by Region 3 */
            /* resvdPaRAMSets */
            /* 31     0     63    32     95    64     127   96 */
            {0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u,
            /* 159  128     191  160     223  192     255  224 */
             0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u,
            /* 287  256     319  288     351  320     383  352 */
             0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u,
            /* 415  384     447  416     479  448     511  480 */
             0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u,},

            /* resvdDmaChannels */
            /* 31     0     63    32 */
            {0x00000000u, 0x00000000u},

            /* resvdQdmaChannels */
            /* 31     0 */
            {0x00000000u},

            /* resvdTccs */
            /* 31     0     63    32 */
            {0x00000000u, 0x00000000u},
          }
		}
};

/* End of File */



