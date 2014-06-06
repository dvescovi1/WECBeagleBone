/*
 * edma3resmgr.c
 *
 * EDMA3 Controller Resource Manager Interface Implementation
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

/* Resource Manager Internal Header Files */
#include "edma3resmgr.h"

/* Instrumentation Header File */
#ifdef EDMA3_INSTRUMENTATION_ENABLED
#include "edma3_log.h"
#endif

/* For assert() */
/**
 * Define NDEBUG to ignore assert().
 * NDEBUG should be defined before including assert.h header file.
 */
#include <assert.h>


/* Global Defines, need to re-compile if values are changed */
/*---------------------------------------------------------------------------*/
/**
 * \brief EDMA3 Resource Manager behaviour of clearing CC ERROR interrupts.
 *         This macro controls the driver to enable/disable clearing of error
 *         status of all channels.
 *
 *         On disabling this (with value 0x0), the channels owned by the region
 *         is cleared and its expected that some other entity is responsible for
 *         clearing the error status for channels not owned.
 *
 *         Its recomended that this flag is a positive value, to ensure that
 *         error flags are cleared for all the channels.
 */
#define EDMA3_RM_RES_CLEAR_ERROR_STATUS_FOR_ALL_CHANNELS (TRUE)

/**
 * \brief EDMA3 Resource Manager retry count to check the pending interrupts inside ISR.
 *         This macro controls the driver to check the pending interrupt for
 *         'n' number of times.
 *         Minumum value is 1.
 */
#define EDMA3_RM_COMPL_HANDLER_RETRY_COUNT (10u)

/**
 * \brief EDMA3 Resource Manager retry count to check the pending CC Error Interrupt inside ISR
 *         This macro controls the driver to check the pending CC Error
 *         interrupt for 'n' number of times.
 *         Minumum value is 1.
 */
#define EDMA3_RM_CCERR_HANDLER_RETRY_COUNT (10u)


/* Externel Variables */
/*---------------------------------------------------------------------------*/
/**
 * Maximum Resource Manager Instances supported by the EDMA3 Package.
 */
extern const unsigned int EDMA3_MAX_RM_INSTANCES;

/**
 * \brief Static Configuration structure for EDMA3
 * controller, to provide Global SoC specific Information.
 *
 * This configuration info can also be provided by the user at run-time,
 * while calling EDMA3_RM_create (). If not provided at run-time,
 * this info will be taken from the config file "edma3_<PLATFORM_NAME>_cfg.c",
 * for the specified platform.
 */
extern EDMA3_RM_GblConfigParams edma3GblCfgParams [EDMA3_MAX_EDMA3_INSTANCES];

/**
 * \brief Default Static Region Specific Configuration structure for
 * EDMA3 controller, to provide region specific Information.
 */
extern EDMA3_RM_InstanceInitConfig defInstInitConfig [EDMA3_MAX_EDMA3_INSTANCES][EDMA3_MAX_REGIONS];

/**
 * \brief Region Specific Configuration structure for
 * EDMA3 controller, to provide region specific Information.
 *
 * This configuration info can also be provided by the user at run-time,
 * while calling EDMA3_RM_open (). If not provided at run-time,
 * this info will be taken from the config file "edma3_<PLATFORM_NAME>_cfg.c",
 * for the specified platform.
 */
extern EDMA3_RM_InstanceInitConfig *userInitConfig;
extern EDMA3_RM_InstanceInitConfig *ptrInitCfgArray;

/**
 * Handles of EDMA3 Resource Manager Instances.
 *
 * Used to maintain information of the EDMA3 RM Instances
 * for each HW controller.
 * There could be a maximum of EDMA3_MAX_RM_INSTANCES instances per
 * EDMA3 HW.
 */
extern EDMA3_RM_Instance *resMgrInstance;
extern EDMA3_RM_Instance *ptrRMIArray;


/* Globals */
/*---------------------------------------------------------------------------*/
/**
 * \brief EDMA3 Resource Manager Objects, tied to each EDMA3 HW Controller.
 *
 * Typically one RM object will cater to one EDMA3 HW controller
 * and will have all the global config information.
 */
EDMA3_RM_Obj resMgrObj[EDMA3_MAX_EDMA3_INSTANCES];

/**
 * Global Array to store the mapping between DMA channels and Interrupt
 * channels i.e. TCCs.
 * DMA channel X can use any TCC Y. Transfer completion
 * interrupt will occur on the TCC Y (IPR/IPRH Register, bit Y), but error
 * interrupt will occur on DMA channel X (EMR/EMRH register, bit X). In that
 * scenario, this DMA channel <-> TCC mapping will be used to point to
 * the correct callback function.
 */
static unsigned int edma3DmaChTccMapping [EDMA3_MAX_EDMA3_INSTANCES][EDMA3_MAX_DMA_CH];

/**
 * Global Array to store the mapping between QDMA channels and Interrupt
 * channels i.e. TCCs.
 * QDMA channel X can use any TCC Y. Transfer completion
 * interrupt will occur on the TCC Y (IPR/IPRH Register, bit Y), but error
 * interrupt will occur on QDMA channel X (QEMR register, bit X). In that
 * scenario, this QDMA channel <-> TCC mapping will be used to point to
 * the correct callback function.
 */
static unsigned int edma3QdmaChTccMapping [EDMA3_MAX_EDMA3_INSTANCES][EDMA3_MAX_QDMA_CH];

/**
 * Global Array to maintain the Callback details registered
 * against a particular TCC. Used to call the callback
 * functions linked to the particular channel.
 */
static EDMA3_RM_TccCallbackParams edma3IntrParams [EDMA3_MAX_EDMA3_INSTANCES][EDMA3_MAX_TCC];

/** edma3RegionId will be updated ONCE using the parameter regionId passed to
 * the EDMA3_RM_open() function, for the Master RM instance (one who
 * configures the Global Registers).
 * This global variable will be used within the Interrupt handlers to know
 * which shadow region registers to access. All other interrupts coming
 * from other shadow regions will not be handled.
 */
static EDMA3_RM_RegionId edma3RegionId = EDMA3_MAX_REGIONS;

/** masterExists[] will be updated when the Master RM Instance modifies the
 * Global EDMA3 configuration registers. It is used to prevent any other
 * Master RM Instance creation.
 * masterExists[] is per EDMA3 hardware, hence it is created
 * as an array.
 */
static unsigned short masterExists [EDMA3_MAX_EDMA3_INSTANCES] = {FALSE};

/**
 * Number of PaRAM Sets actually present on the SoC. This will be updated
 * while creating the Resource Manager Object.
 */
unsigned int edma3NumPaRAMSets = EDMA3_MAX_PARAM_SETS;


/**
 * The list of Interrupt Channels which get allocated while requesting the
 * TCC. It will be used while checking the IPR/IPRH bits in the RM ISR.
 */
static unsigned int allocatedTCCs[EDMA3_MAX_EDMA3_INSTANCES][2u] = {0};


/**
 * Arrays ownDmaChannels[], resvdDmaChannels and avlblDmaChannels will be ANDed
 * and stored in this array. It will be referenced in
 * EDMA3_RM_allocContiguousResource () to look for contiguous resources.
 */
static unsigned int contiguousDmaRes[EDMA3_MAX_EDMA3_INSTANCES][EDMA3_MAX_DMA_CHAN_DWRDS] = {0};

/**
 * Arrays ownDmaChannels[], resvdDmaChannels and avlblDmaChannels will be ANDed
 * and stored in this array. It will be referenced in
 * EDMA3_RM_allocContiguousResource () to look for contiguous resources.
 */
static unsigned int contiguousQdmaRes[EDMA3_MAX_EDMA3_INSTANCES][EDMA3_MAX_QDMA_CHAN_DWRDS] = {0};

/**
 * Arrays ownDmaChannels[], resvdDmaChannels and avlblDmaChannels will be ANDed
 * and stored in this array. It will be referenced in
 * EDMA3_RM_allocContiguousResource () to look for contiguous resources.
 */
static unsigned int contiguousTccRes[EDMA3_MAX_EDMA3_INSTANCES][EDMA3_MAX_TCC_DWRDS] = {0};

/**
 * Arrays ownDmaChannels[], resvdDmaChannels and avlblDmaChannels will be ANDed
 * and stored in this array. It will be referenced in
 * EDMA3_RM_allocContiguousResource () to look for contiguous resources.
 */
static unsigned int contiguousParamRes[EDMA3_MAX_EDMA3_INSTANCES][EDMA3_MAX_PARAM_DWRDS];


/**
 * \brief Resources bound to a Channel
 *
 * When a request for a channel is made, the resources PaRAM Set and TCC
 * get bound to that channel. This information is needed internally by the
 * resource manager, when a request is made to free the channel,
 * to free up the channel-associated resources.
 */
static EDMA3_RM_ChBoundResources edma3RmChBoundRes [EDMA3_MAX_EDMA3_INSTANCES][EDMA3_MAX_LOGICAL_CH];



/*---------------------------------------------------------------------------*/

/* Local functions prototypes */
/*---------------------------------------------------------------------------*/
/** EDMA3 Instance 0 Completion Handler Interrupt Service Routine */
void lisrEdma3ComplHandler0(unsigned int arg);
/** EDMA3 Instance 0 CC Error Interrupt Service Routine */
void lisrEdma3CCErrHandler0(unsigned int arg);
/**
 * EDMA3 Instance 0 TC[0-7] Error Interrupt Service Routines
 * for a maximum of 8 TCs (Transfer Controllers).
 */
void lisrEdma3TC0ErrHandler0(unsigned int arg);
void lisrEdma3TC1ErrHandler0(unsigned int arg);
void lisrEdma3TC2ErrHandler0(unsigned int arg);
void lisrEdma3TC3ErrHandler0(unsigned int arg);
void lisrEdma3TC4ErrHandler0(unsigned int arg);
void lisrEdma3TC5ErrHandler0(unsigned int arg);
void lisrEdma3TC6ErrHandler0(unsigned int arg);
void lisrEdma3TC7ErrHandler0(unsigned int arg);


/** Interrupt Handler for the Transfer Completion interrupt */
static void edma3ComplHandler (const EDMA3_RM_Obj *rmObj);
/** Interrupt Handler for the Channel Controller Error interrupt */
static void edma3CCErrHandler (const EDMA3_RM_Obj *rmObj);
/** Interrupt Handler for the Transfer Controller Error interrupt */
static void edma3TCErrHandler (const EDMA3_RM_Obj *rmObj, unsigned int tcNum);


/** Local MemSet function */
void edma3MemSet(void *dst, unsigned char data, unsigned int len);
/** Local MemCpy function */
void edma3MemCpy(void *dst, const void *src, unsigned int len);

/** Initialization of the Global region registers of the EDMA3 Controller */
static void edma3GlobalRegionInit (unsigned int phyCtrllerInstId);
/** Initialization of the Shadow region registers of the EDMA3 Controller */
static void edma3ShadowRegionInit (const EDMA3_RM_Instance *pRMInstance);



/* Internal functions for contiguous resource allocation */
/**
 * Finds a particular bit ('0' or '1') in the particular word from 'start'.
 * If found, returns the position, else return -1.
 */
static int findBitInWord (int source, unsigned int start, unsigned short bit);

/**
 * Finds a particular bit ('0' or '1') in the specified resources' array
 * from 'start' to 'end'. If found, returns the position, else return -1.
 */
static int findBit (unsigned int phyCtrllerInstId,
                            EDMA3_RM_ResType resType,
                            unsigned int start,
                            unsigned int end,
                            unsigned short bit);

/**
 * If successful, this function returns EDMA3_RM_SOK and the position
 * of first available resource in 'positionRes'. Else returns error.
 */
static EDMA3_RM_Result allocAnyContigRes(unsigned int phyCtrllerInstId,
                                    EDMA3_RM_ResType resType,
                                    unsigned int numResources,
                                    unsigned int *positionRes);

/**
 * Starting from 'firstResIdObj', this function makes the next 'numResources'
 * Resources non-available for future. Also, it does some global resisters'
 * setting also.
 */
static EDMA3_RM_Result gblChngAllocContigRes(EDMA3_RM_Instance *rmInstance,
                        const EDMA3_RM_ResDesc *firstResIdObj,
                                        unsigned int numResources);

/*---------------------------------------------------------------------------*/

/**\fn      EDMA3_RM_Result EDMA3_RM_create (unsigned int phyCtrllerInstId,
 *          const EDMA3_RM_GblConfigParams *gblCfgParams,
 *          const void *miscParam)
 * \brief   Create EDMA3 Resource Manager Object
 *
 * This API is used to create the EDMA3 Resource Manager Object. It should be
 * called only ONCE for each EDMA3 hardware instance.
 *
 * Init-time Configuration structure for EDMA3 hardware is provided to pass the
 * SoC specific information. This configuration information could be provided
 * by the user at init-time. In case user doesn't provide it, this information
 * could be taken from the SoC specific configuration file
 * edma3_<SOC_NAME>_cfg.c, in case it is available.
 *
 * This API clears the error specific registers (EMCR/EMCRh, QEMCR, CCERRCLR)
 * and sets the TCs priorities and Event Queues' watermark levels, if the 'miscParam'
 * argument is NULL. User can avoid these registers' programming (in some specific
 * use cases) by SETTING the 'isSlave' field of 'EDMA3_RM_MiscParam' configuration
 * structure and passing this structure as the third argument (miscParam).
 *
 * After successful completion of this API, Resource Manager Object's state
 * changes to EDMA3_RM_CREATED from EDMA3_RM_DELETED.
 *
 * \param phyCtrllerInstId  [IN]    EDMA3 Controller Instance Id
 *                                 (Hardware instance id, starting from 0).
 * \param gblCfgParams      [IN]    SoC specific configuration structure for the
 *                                  EDMA3 Hardware.
 * \param miscParam         [IN]    Misc configuration options provided in the
 *                                  structure 'EDMA3_RM_MiscParam'.
 *                                  For default options, user can pass NULL
 *                                  in this argument.
 *
 * \return  EDMA3_RM_SOK or EDMA3_RM Error Code
 */
EDMA3_RM_Result EDMA3_RM_create (unsigned int phyCtrllerInstId,
                                const EDMA3_RM_GblConfigParams *gblCfgParams,
                                const void *miscParam)
    {
    unsigned int count = 0u;
    EDMA3_RM_Result result = EDMA3_RM_SOK;
    /**
     * Used to reset the Internal EDMA3 Resource Manager Data Structures for the first time.
     */
    static unsigned short rmInitDone = FALSE;
    const EDMA3_RM_MiscParam *miscOpt = (const EDMA3_RM_MiscParam *)miscParam;

    /**
     * We are NOT checking 'gblCfgParams' for NULL.
     * If user has passed NULL, default config info will be
     * taken from config file.
     * 'param' is also not being checked because it could be
     * NULL also.
     */

    /* If parameter checking is enabled... */
#ifndef EDMA3_RM_PARAM_CHECK_DISABLE
    if (phyCtrllerInstId >= EDMA3_MAX_EDMA3_INSTANCES)
        {
        result = EDMA3_RM_E_INVALID_PARAM;
        }
#endif

	/* Check if the parameters are OK. */
    if (EDMA3_RM_SOK == result)
        {
        /* Initialize the global variables for the first time */
        if (FALSE == rmInitDone)
            {
            edma3MemSet((void *)&(resMgrObj[count]) , 0x00u,
                        sizeof(resMgrObj));
            rmInitDone = TRUE;
            }

        /* Initialization has been done */
        if (resMgrObj[phyCtrllerInstId].state != EDMA3_RM_DELETED)
            {
            result = EDMA3_RM_E_OBJ_NOT_DELETED;
            }
        else
            {
            /**
              * Check whether user has passed the Global Config Info.
              * If yes, copy it to the driver data structures. Else, use the
              * info from the config file edma3Cfg.c
              */
            if (NULL == gblCfgParams)
                {
                /* Take info from the specific config file */
                edma3MemCpy((void *)(&resMgrObj[phyCtrllerInstId].gblCfgParams),
                                            (const void *)(&edma3GblCfgParams[phyCtrllerInstId]),
                                            sizeof (EDMA3_RM_GblConfigParams));
                }
            else
                {
                /* User passed the info, save it in the RM object first */
                edma3MemCpy((void *)(&resMgrObj[phyCtrllerInstId].gblCfgParams),
                                            (const void *)(gblCfgParams),
                                            sizeof (EDMA3_RM_GblConfigParams));
                }


            /**
              * Check whether DMA channel to PaRAM Set mapping exists or not.
              * If it does not exist, set the mapping array as 1-to-1 mapped.
              */
            if (FALSE == resMgrObj[phyCtrllerInstId].gblCfgParams.dmaChPaRAMMapExists)
                {
                for (count = 0u; count < resMgrObj[phyCtrllerInstId].gblCfgParams.numDmaChannels; count++)
                    {
                    resMgrObj[phyCtrllerInstId].gblCfgParams.dmaChannelPaRAMMap[count] = count;
                    }
                }


            /**
             * Update the actual number of PaRAM sets.
             */
            edma3NumPaRAMSets = resMgrObj[phyCtrllerInstId].gblCfgParams.numPaRAMSets;

            resMgrObj[phyCtrllerInstId].phyCtrllerInstId = phyCtrllerInstId;
            resMgrObj[phyCtrllerInstId].state = EDMA3_RM_CREATED;
            resMgrObj[phyCtrllerInstId].numOpens = 0u;

            /* Make all the RM instances for this EDMA3 HW NULL */
            for (count = 0u; count < EDMA3_MAX_RM_INSTANCES; count++)
                {
                edma3MemSet((void *)((EDMA3_RM_Instance *)(ptrRMIArray) + (phyCtrllerInstId*EDMA3_MAX_RM_INSTANCES) + count),
                            0x00u,
                            sizeof(EDMA3_RM_Instance));

                /* Also make this data structure NULL */
                edma3MemSet((void *)((EDMA3_RM_InstanceInitConfig *)(ptrInitCfgArray) + (phyCtrllerInstId*EDMA3_MAX_RM_INSTANCES) + count),
                            0x00u,
                            sizeof(EDMA3_RM_InstanceInitConfig));
                }

            /* Initialize the global edma3DmaChTccMapping array with EDMA3_MAX_TCC */
            for (  count = 0u;
                    count < resMgrObj[phyCtrllerInstId].gblCfgParams.numDmaChannels;
                    count++
                )
                {
                edma3DmaChTccMapping[phyCtrllerInstId][count] = EDMA3_MAX_TCC;
                }

            /* Initialize the global edma3QdmaChTccMapping array with EDMA3_MAX_TCC */
            for (   count = 0u;
                    count < resMgrObj[phyCtrllerInstId].gblCfgParams.numQdmaChannels;
                    count++
                )
                {
                edma3QdmaChTccMapping[phyCtrllerInstId][count] = EDMA3_MAX_TCC;
                }

            /* Make the global edma3IntrParams array for interrupts NULL */
            edma3MemSet((void *)(&(edma3IntrParams[phyCtrllerInstId][0u])), 0x00u,
                sizeof(edma3IntrParams[0]));


            /* Reset edma3RmChBoundRes Array*/
            for (count = 0u; count < EDMA3_MAX_LOGICAL_CH; count++)
                {
                edma3RmChBoundRes[phyCtrllerInstId][count].paRAMId = -1;
                edma3RmChBoundRes[phyCtrllerInstId][count].tcc = EDMA3_MAX_TCC;
                }

            /* Make the contiguousParamRes array NULL */
            edma3MemSet((void *)(&(contiguousParamRes[phyCtrllerInstId][0u])), 0x00u,
                sizeof(contiguousParamRes[0]));


            /**
             * Check the misc configuration options structure.
             * Check whether the global registers' initialization
             * is required or not.
             * It is required ONLY if RM is running on the Master Processor.
             */
            if (NULL != miscOpt)
                {
                if (miscOpt->isSlave == FALSE)
                    {
                    /* It is a master. */
                    edma3GlobalRegionInit(phyCtrllerInstId);
                    }
                }
            else
                {
                /* By default, global registers will be initialized. */
                edma3GlobalRegionInit(phyCtrllerInstId);
                }
            }
        }

    return result;
    }



/**\fn      EDMA3_RM_Result EDMA3_RM_delete (unsigned int phyCtrllerInstId,
 *                               const void *param)
 * \brief   Delete EDMA3 Resource Manager Object
 *
 * This API is used to delete the EDMA3 RM Object. It should be called
 * once for each EDMA3 hardware instance, ONLY after closing all the
 * previously opened EDMA3 RM Instances.
 *
 * After successful completion of this API, Resource Manager Object's state
 * changes to EDMA3_RM_DELETED.
 *
 * \param phyCtrllerInstId  [IN]    EDMA3 Phy Controller Instance Id (Hardware
 *                                  instance id, starting from 0).
 * \param   param           [IN]    For possible future use.
 *
 * \return  EDMA3_RM_SOK or EDMA3_RM Error Code
 */
EDMA3_RM_Result EDMA3_RM_delete (unsigned int phyCtrllerInstId,
                                                const void *param)
    {
    EDMA3_RM_Result result = EDMA3_RM_SOK;

    /* If parameter checking is enabled... */
#ifndef EDMA3_RM_PARAM_CHECK_DISABLE
    if (phyCtrllerInstId >= EDMA3_MAX_EDMA3_INSTANCES)
        {
        result = EDMA3_RM_E_INVALID_PARAM;
        }
#endif

	/* Check if the parameters are OK. */
	if (EDMA3_RM_SOK == result)
        {
        /*to remove CCS remark: parameter "param" was never referenced */
        (void)param;

        /**
         * If number of RM Instances is 0, then state should be
         * EDMA3_RM_CLOSED OR EDMA3_RM_CREATED.
         */
        if ((NULL == resMgrObj[phyCtrllerInstId].numOpens)
            && ((resMgrObj[phyCtrllerInstId].state != EDMA3_RM_CLOSED)
            && (resMgrObj[phyCtrllerInstId].state != EDMA3_RM_CREATED)))
            {
            result = EDMA3_RM_E_OBJ_NOT_CLOSED;
            }
        else
            {
            /**
             * If number of RM Instances is NOT 0, then this function
             * SHOULD NOT be called by anybody.
             */
            if (NULL != resMgrObj[phyCtrllerInstId].numOpens)
                {
                result = EDMA3_RM_E_INVALID_STATE;
                }
            else
                {
                /** Change state to EDMA3_RM_DELETED */
                resMgrObj[phyCtrllerInstId].state = EDMA3_RM_DELETED;

                /* Reset the Allocated TCCs Array also. */
                allocatedTCCs[phyCtrllerInstId][0u] = 0x0u;
                allocatedTCCs[phyCtrllerInstId][1u] = 0x0u;

                /* Also, reset the RM Object Global Config Info */
                edma3MemSet((void *)&(resMgrObj[phyCtrllerInstId].gblCfgParams),
                         0x00u,
                         sizeof(EDMA3_RM_GblConfigParams));
                }
            }
        }

    return result;
    }


/**\fn      EDMA3_RM_Handle EDMA3_RM_open (unsigned int phyCtrllerInstId,
 *                                const EDMA3_RM_Param *initParam,
 *                               EDMA3_RM_Result *errorCode)
 * \brief   Open EDMA3 Resource Manager Instance
 *
 * This API is used to open an EDMA3 Resource Manager Instance. It could be
 * called multiple times, for each possible EDMA3 shadow region. Maximum
 * EDMA3_MAX_RM_INSTANCES instances are allowed for each EDMA3 hardware
 * instance.
 *
 * Also, only ONE Master Resource Manager Instance is permitted. This master
 * instance (and hence the region to which it belongs) will only receive the
 * EDMA3 interrupts, if enabled.
 *
 * User could pass the instance specific configuration structure
 * (initParam->rmInstInitConfig) as a part of the 'initParam' structure,
 * during init-time. In case user doesn't provide it, this information could
 * be taken from the SoC specific configuration file edma3_<SOC_NAME>_cfg.c,
 * in case it is available.
 *
 * \param   phyCtrllerInstId    [IN]    EDMA3 Controller Instance Id (Hardware
 *                                      instance id, starting from 0).
 * \param   initParam           [IN]    Used to Initialize the Resource Manager
 *                                      Instance (Master or Slave).
 * \param   errorCode           [OUT]   Error code while opening RM instance.
 *
 * \return  Handle to the opened Resource Manager instance Or NULL in case of
 *          error.
 *
 * \note    This function disables the global interrupts (by calling API
 *          edma3OsProtectEntry with protection level
 *          EDMA3_OS_PROTECT_INTERRUPT) while modifying the global RM data
 *          structures, to make it re-entrant.
 */
EDMA3_RM_Handle EDMA3_RM_open (unsigned int phyCtrllerInstId,
                                const EDMA3_RM_Param *initParam,
                                EDMA3_RM_Result *errorCode)
    {
    unsigned int intState           = 0u;
    unsigned int resMgrIdx          = 0u;
    EDMA3_RM_Result result          = EDMA3_RM_SOK;
    EDMA3_RM_Obj *rmObj             = NULL;
    EDMA3_RM_Instance *rmInstance   = NULL;
    EDMA3_RM_Instance *temp_ptr_rm_inst   = NULL;
    EDMA3_RM_Handle retVal          = NULL;
    unsigned int dmaChDwrds = 0u;
    unsigned int paramSetDwrds = 0u;
    unsigned int tccDwrds = 0u;
    volatile EDMA3_CCRL_Regs *globalRegs = NULL;
    unsigned int mappedPaRAMId;

	/* If parameter checking is enabled... */
#ifndef EDMA3_RM_PARAM_CHECK_DISABLE
	if (((initParam == NULL)
		|| (phyCtrllerInstId >= EDMA3_MAX_EDMA3_INSTANCES))
        || (errorCode == NULL))
        {
        result = EDMA3_RM_E_INVALID_PARAM;
        }
#endif

	/* Check if the parameters are OK. */
	if (EDMA3_RM_SOK == result)
        {
        /* Check whether the semaphore handle is null or not */
        if (NULL == initParam->rmSemHandle)
            {
            result = EDMA3_RM_E_INVALID_PARAM;
            }
        else
            {
            rmObj = &resMgrObj[phyCtrllerInstId];
            if  (
                (NULL == rmObj)
                || (initParam->regionId >=
                        resMgrObj[phyCtrllerInstId].gblCfgParams.numRegions)
                )
                {
                result = EDMA3_RM_E_INVALID_PARAM;
                }
            else
                {
                edma3OsProtectEntry (phyCtrllerInstId, 
									EDMA3_OS_PROTECT_INTERRUPT, 
									&intState);

                /** Check state of RM Object.
                  * If no RM instance is opened and this is the first one,
                  * then state should be created/closed.
                  */
                if ((rmObj->numOpens == NULL) &&
                    ((rmObj->state != EDMA3_RM_CREATED) &&
                    (rmObj->state != EDMA3_RM_CLOSED)))
                    {
                    result = EDMA3_RM_E_INVALID_STATE;
                    edma3OsProtectExit (phyCtrllerInstId, 
										EDMA3_OS_PROTECT_INTERRUPT, 
										intState);
                    }
                else
                    {
                    /**
                     * If num of instances opened is more than 0 and less than
                     *  max allowed, then state should be opened.
                     */
                    if (((rmObj->numOpens > 0) &&
                            (rmObj->numOpens < EDMA3_MAX_RM_INSTANCES))
                        && (rmObj->state != EDMA3_RM_OPENED))
                        {
                        result = EDMA3_RM_E_INVALID_STATE;
	                    edma3OsProtectExit (phyCtrllerInstId, 
											EDMA3_OS_PROTECT_INTERRUPT, 
											intState);
                        }
                    else
                        {
                        /* Check if max opens have passed */
                        if (rmObj->numOpens >= EDMA3_MAX_RM_INSTANCES)
                            {
                            result = EDMA3_RM_E_MAX_RM_INST_OPENED;
		                    edma3OsProtectExit (phyCtrllerInstId, 
												EDMA3_OS_PROTECT_INTERRUPT, 
												intState);
                            }
                        }
                    }
                }
            }
        }

    if (EDMA3_RM_SOK == result)
        {
        /*
        * Check whether the RM instance is Master or not.
        * If it is master, check whether a master already exists
        * or not. There should NOT be more than 1 master.
        * Return error code if master already exists
        */
        if ((TRUE == masterExists[phyCtrllerInstId]) && (TRUE == initParam->isMaster))
            {
            /* No two masters should exist, return error */
            result = EDMA3_RM_E_RM_MASTER_ALREADY_EXISTS;
            edma3OsProtectExit (phyCtrllerInstId, 
								EDMA3_OS_PROTECT_INTERRUPT, 
								intState);
            }
        else
            {
            /* Create Res Mgr Instance */
            for (resMgrIdx = 0u; resMgrIdx < EDMA3_MAX_RM_INSTANCES; resMgrIdx++)
                {
                temp_ptr_rm_inst = ((EDMA3_RM_Instance *)(ptrRMIArray) + (phyCtrllerInstId*EDMA3_MAX_RM_INSTANCES) + resMgrIdx);

                if (NULL != temp_ptr_rm_inst)
                    {
                    if (NULL == temp_ptr_rm_inst->pResMgrObjHandle)
                        {
                        /* Handle to the EDMA3 HW Object */
                        temp_ptr_rm_inst->pResMgrObjHandle = rmObj;
                        /* Handle of the Res Mgr Instance */
                        rmInstance = temp_ptr_rm_inst;

                        /* Also make this data structure NULL, just for safety. */
                        edma3MemSet((void *)((EDMA3_RM_InstanceInitConfig *)(ptrInitCfgArray) + (phyCtrllerInstId*EDMA3_MAX_RM_INSTANCES) + resMgrIdx),
                                    0x00u,
                                    sizeof(EDMA3_RM_InstanceInitConfig));

                        break;
                        }
                    }
                }

            /* Check whether a RM instance has been created or not */
            if (NULL == rmInstance)
                {
                result = EDMA3_RM_E_MAX_RM_INST_OPENED;
                edma3OsProtectExit (phyCtrllerInstId, 
									EDMA3_OS_PROTECT_INTERRUPT, 
									intState);
                }
            else
                {
                /* Copy the InitPaRAM first */
                edma3MemCpy((void *)(&rmInstance->initParam),
                                            (const void *)(initParam),
                                            sizeof (EDMA3_RM_Param));

                if (rmObj->gblCfgParams.globalRegs != NULL)
                    {
                    globalRegs = (volatile EDMA3_CCRL_Regs *)
                                            (rmObj->gblCfgParams.globalRegs);
                    rmInstance->shadowRegs = (EDMA3_CCRL_ShadowRegs *)
                        &(globalRegs->SHADOW[rmInstance->initParam.regionId]);

                    /* copy the instance specific semaphore handle */
                    rmInstance->initParam.rmSemHandle = initParam->rmSemHandle;

                    /**
                    * Check whether user has passed information about resources
                    * owned and reserved by this instance. This is region specific
                    * information. If he has not passed, dafault static config info will be taken
                    * from the config file edma3Cfg.c, according to the regionId specified.
                    *
                    * resMgrIdx specifies the RM instance number created just now.
                    * Use it to populate the userInitConfig [].
                    */
                    if (NULL == initParam->rmInstInitConfig)
                        {
                        /* Take the info from the specific config file */
                        edma3MemCpy((void *)((EDMA3_RM_InstanceInitConfig *)(ptrInitCfgArray) + (phyCtrllerInstId*EDMA3_MAX_RM_INSTANCES) + resMgrIdx),
                                (const void *)(&defInstInitConfig[phyCtrllerInstId][initParam->regionId]),
                                sizeof (EDMA3_RM_InstanceInitConfig));
                        }
                    else
                        {
                        /* User has passed the region specific info. */
                        edma3MemCpy((void *)((EDMA3_RM_InstanceInitConfig *)(ptrInitCfgArray) + (phyCtrllerInstId*EDMA3_MAX_RM_INSTANCES) + resMgrIdx),
                                (const void *)(initParam->rmInstInitConfig),
                                sizeof (EDMA3_RM_InstanceInitConfig));
                        }

                    rmInstance->initParam.rmInstInitConfig =
                                ((EDMA3_RM_InstanceInitConfig *)(ptrInitCfgArray) + (phyCtrllerInstId*EDMA3_MAX_RM_INSTANCES) + resMgrIdx);

                    dmaChDwrds = rmObj->gblCfgParams.numDmaChannels / 32u;
                    paramSetDwrds = rmObj->gblCfgParams.numPaRAMSets / 32u;
                    tccDwrds = rmObj->gblCfgParams.numTccs / 32u;

                    for (resMgrIdx = 0u; resMgrIdx < dmaChDwrds; ++resMgrIdx)
                        {
                        rmInstance->avlblDmaChannels[resMgrIdx]
                            = rmInstance->initParam.rmInstInitConfig->ownDmaChannels[resMgrIdx];
                        }

                    rmInstance->avlblQdmaChannels[0u]
                        = rmInstance->initParam.rmInstInitConfig->ownQdmaChannels[0u];

                    for (resMgrIdx = 0u; resMgrIdx < paramSetDwrds; ++resMgrIdx)
                        {
                        rmInstance->avlblPaRAMSets[resMgrIdx]
                            = rmInstance->initParam.rmInstInitConfig->ownPaRAMSets[resMgrIdx];
                        }

                    for (resMgrIdx = 0u; resMgrIdx < tccDwrds; ++resMgrIdx)
                        {
                        rmInstance->avlblTccs [resMgrIdx]
                            = rmInstance->initParam.rmInstInitConfig->ownTccs[resMgrIdx];
                        }

                    /*
                    * If mapping exists b/w DMA channel and PaRAM set (i.e. programmable),
                    * then mark those PaRAM sets which are mapped to some specific
                    * DMA channels as RESERVED. If NO mapping (ie ch 0 is tied to PaRAM 0,
                    * ch 1 is tied to PaRAM 1), mark all as RESERVED.
                    */
                    if (rmObj->gblCfgParams.dmaChPaRAMMapExists == TRUE)
                        {
                        /* Mapping Exists */
                        for (resMgrIdx = 0u; resMgrIdx < rmObj->gblCfgParams.numDmaChannels; ++resMgrIdx)
                            {
                            mappedPaRAMId = rmObj->gblCfgParams.dmaChannelPaRAMMap[resMgrIdx];
                            if (mappedPaRAMId != EDMA3_RM_CH_NO_PARAM_MAP)
                                {
                                /* Channel is mapped to a particular PaRAM Set, mark it as Reserved. */
                                rmInstance->initParam.rmInstInitConfig->resvdPaRAMSets[mappedPaRAMId/32u] |= (1u<<(mappedPaRAMId%32u));
                                }
                            }
                        }
                    else
                        {
                        /* Mapping Doesnot Exist, PaRAM Sets are 1-to-1 mapped, mark all as Reserved */
                        for (resMgrIdx = 0u; resMgrIdx < dmaChDwrds; ++resMgrIdx)
                            {
                            rmInstance->initParam.rmInstInitConfig->resvdPaRAMSets[resMgrIdx] = 0xFFFFFFFFu;
                            }
                        }

                    /*
                    * If the EDMA RM instance is MASTER (ie. initParam->isMaster
                    * is TRUE), save the region ID.
                    * Only this shadow region will receive the
                    * EDMA3 interrupts, if enabled.
                    */
                    if (TRUE == initParam->isMaster)
                        {
                        /* Store the region id to use it in the ISRs */
                        edma3RegionId = rmInstance->initParam.regionId;
                        masterExists[phyCtrllerInstId] = TRUE;
                        }

                    if (TRUE == initParam->regionInitEnable)
                        {
                        edma3ShadowRegionInit (rmInstance);
                        }

                    /**
                     * By default, PaRAM Sets allocated using this RM Instance
                     * will get cleared during their allocation.
                     * User can stop their clearing by calling specific IOCTL
                     * command.
                     */
                    rmInstance->paramInitRequired = TRUE;


                    /**
                     * By default, during the EDMA3_RM_allocLogicalChannel (),
                     * global EDMA3 registers (DCHMAP/QCHMAP) and the allocated
                     * PaRAM Set will be programmed accordingly, for users using this
                     * RM Instance.
                     * User can stop their pre-programming by calling
                     * EDMA3_RM_IOCTL_SET_GBL_REG_MODIFY_OPTION
                     * IOCTL command.
                     */
                    rmInstance->regModificationRequired = TRUE;


                    if (EDMA3_RM_SOK == result)
                        {
                        rmObj->state = EDMA3_RM_OPENED;
                        /* Increase the Instance count */
                        resMgrObj[phyCtrllerInstId].numOpens++;
                        retVal = rmInstance;
                        }
                    }
                else
                    {
                    result = EDMA3_RM_E_INVALID_PARAM;
                    }

                edma3OsProtectExit (phyCtrllerInstId, 
									EDMA3_OS_PROTECT_INTERRUPT, 
									intState);
                }
            }
        }

    *errorCode = result;
    return (EDMA3_RM_Handle)retVal;
    }


/**\fn      EDMA3_RM_Result EDMA3_RM_close (EDMA3_RM_Handle hEdmaResMgr,
 *                                const void *param)
 * \brief  Close EDMA3 Resource Manager Instance
 *
 * This API is used to close a previously opened EDMA3 RM Instance.
 *
 * \param  hEdmaResMgr         [IN]    Handle to the previously opened Resource
 *                                     Manager Instance.
 * \param  param               [IN]    For possible future use.
 *
 * \return EDMA3_RM_SOK or EDMA3_RM Error Code
 *
 * \note    This function disables the global interrupts (by calling API
 *          edma3OsProtectEntry with protection level
 *          EDMA3_OS_PROTECT_INTERRUPT) while modifying the global RM data
 *          structures, to make it re-entrant.
 */
EDMA3_RM_Result EDMA3_RM_close (EDMA3_RM_Handle hEdmaResMgr,
                                    const void *param)
    {
    EDMA3_RM_Result result = EDMA3_RM_SOK;
    unsigned int intState = 0u;
    unsigned int resMgrIdx = 0u;
    EDMA3_RM_Obj *rmObj             = NULL;
    EDMA3_RM_Instance *rmInstance   = NULL;
    unsigned int dmaChDwrds;
    unsigned int paramSetDwrds;
    unsigned int tccDwrds;

    /*to remove CCS remark: parameter "param" was never referenced */
    (void)param;

	/* If parameter checking is enabled... */
#ifndef EDMA3_RM_PARAM_CHECK_DISABLE
    if (NULL == hEdmaResMgr)
        {
        result = EDMA3_RM_E_INVALID_PARAM;
        }
#endif

	/* Check if the parameters are OK. */
	if (EDMA3_RM_SOK == result)
        {
        rmInstance = (EDMA3_RM_Instance *)hEdmaResMgr;
        rmObj = rmInstance->pResMgrObjHandle;

        if (rmObj == NULL)
            {
            result = (EDMA3_RM_E_INVALID_PARAM);
            }
        else
            {
            /* Check state of driver, state should be opened */
            if (rmObj->state != EDMA3_RM_OPENED)
                {
                result = (EDMA3_RM_E_OBJ_NOT_OPENED);
                }
            else
                {
                dmaChDwrds = rmObj->gblCfgParams.numDmaChannels / 32u;
                paramSetDwrds = rmObj->gblCfgParams.numPaRAMSets / 32u;
                tccDwrds = rmObj->gblCfgParams.numTccs / 32u;

                /* Set the instance config as NULL*/
                for (resMgrIdx = 0u; resMgrIdx < dmaChDwrds; ++resMgrIdx)
                    {
                    rmInstance->avlblDmaChannels[resMgrIdx] = 0x0u;
                    }
                for (resMgrIdx = 0u; resMgrIdx < paramSetDwrds; ++resMgrIdx)
                    {
                    rmInstance->avlblPaRAMSets[resMgrIdx] = 0x0u;
                    }
                rmInstance->avlblQdmaChannels[0u] = 0x0u;
                for (resMgrIdx = 0u; resMgrIdx < tccDwrds; ++resMgrIdx)
                    {
                    rmInstance->avlblTccs[resMgrIdx] = 0x0u;
                    }

                /**
                 * If this is the Master Instance, reset the static variable
                 * 'masterExists[]'.
                 */
                if (TRUE == rmInstance->initParam.isMaster)
                    {
                    masterExists[rmObj->phyCtrllerInstId] = FALSE;
                    edma3RegionId = EDMA3_MAX_REGIONS;
                    }

                /* Reset the Initparam for this RM Instance */
                edma3MemSet((void *)&(rmInstance->initParam) , 0x00u,
                                            sizeof(EDMA3_RM_Param));

                /* Critical section starts */
                edma3OsProtectEntry (rmObj->phyCtrllerInstId,
                					EDMA3_OS_PROTECT_INTERRUPT, 
                					&intState);

                /* Decrease the Number of Opens */
                --rmObj->numOpens;
                if (NULL == rmObj->numOpens)
                    {
                    edma3MemSet((void *)&(edma3RmChBoundRes[rmObj->phyCtrllerInstId]), 0x00u,
                                            sizeof(edma3RmChBoundRes[0]));

                    rmObj->state = EDMA3_RM_CLOSED;
                    }

                /* Critical section ends */
                edma3OsProtectExit (rmObj->phyCtrllerInstId,
                					EDMA3_OS_PROTECT_INTERRUPT, 
                					intState);

                rmInstance->pResMgrObjHandle = NULL;
                rmInstance->shadowRegs = NULL;
                rmInstance = NULL;
                }
            }
        }

    return result;
    }


/**\fn      EDMA3_RM_Result EDMA3_RM_allocResource (EDMA3_RM_Handle hEdmaResMgr,
 *                                        EDMA3_RM_ResDesc *resObj)
 * \brief   This API is used to allocate specified EDMA3 Resources like
 * DMA/QDMA channel, PaRAM Set or TCC.
 *
 * Note: To free the resources allocated by this API, user should call
 * EDMA3_RM_freeResource () ONLY to de-allocate all the allocated resources.
 *
 * User can either request a specific resource by passing the resource id
 * in 'resObj->resId' OR request ANY available resource of the type
 * 'resObj->type'.
 *
 * ANY types of resources are those resources when user doesn't care about the
 * actual resource allocated; user just wants a resource of the type specified.
 * One use-case is to perform memory-to-memory data transfer operation. This
 * operation can be performed using any available DMA or QDMA channel.
 * User doesn't need any specific channel for the same.
 *
 * To allocate a specific resource, first this API checks whether that resource
 * is OWNED by the Resource Manager instance. Then it checks the current
 * availability of that resource.
 *
 * To allocate ANY available resource, this API tries to allocate a resource
 * from the pool of (owned && non_reserved && available_right_now) resources.
 *
 * After allocating a DMA/QDMA channel or TCC, the same resource is enabled in
 * the shadow region specific register (DRAE/DRAEH/QRAE).
 *
 * Allocated PaRAM Set is initialized to NULL before this API returns if user
 * has requested for one.
 *
 * \param  hEdmaResMgr      [IN]        Handle to the previously opened Resource
 *                                      Manager Instance.
 * \param   resObj          [IN/OUT]    Handle to the resource descriptor
 *                                      object, which needs to be allocated.
 *                                      In case user passes a specific resource
 *                                      Id, resObj value is left unchanged.
 *                                      In case user requests ANY available
 *                                      resource, the allocated resource id is
 *                                      returned in resObj.
 *
 * \return  EDMA3_RM_SOK or EDMA3_RM Error Code
 *
 * \note    This function acquires a RM Instance specific semaphore
 *          to prevent simultaneous access to the global pool of resources.
 *          It is re-entrant, but should not be called from the user callback
 *          function (ISR context).
 */
EDMA3_RM_Result EDMA3_RM_allocResource(EDMA3_RM_Handle hEdmaResMgr,
                                        EDMA3_RM_ResDesc *resObj)
    {
    EDMA3_RM_Instance *rmInstance = NULL;
    EDMA3_RM_Obj *rmObj = NULL;
    EDMA3_RM_Result result = EDMA3_RM_SOK;
    EDMA3_RM_Result semResult = EDMA3_RM_SOK;
    unsigned int avlblIdx = 0u;
    unsigned int resIdClr = 0x0;
    unsigned int resIdSet = 0x0;
    unsigned int resId;
    volatile EDMA3_CCRL_Regs *gblRegs = NULL;

#ifdef EDMA3_INSTRUMENTATION_ENABLED
    EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3",
                EDMA3_DVT_DESC(EDMA3_DVT_eFUNC_START,
                EDMA3_DVT_dCOUNTER,
                EDMA3_DVT_dNONE,
                EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */

	/* If parameter checking is enabled... */
#ifndef EDMA3_RM_PARAM_CHECK_DISABLE
    if ((hEdmaResMgr == NULL) || (resObj == NULL))
        {
        result = (EDMA3_RM_E_INVALID_PARAM);
        }
#endif

	/* Check if the parameters are OK. */
	if (EDMA3_RM_SOK == result)
        {
        rmInstance = (EDMA3_RM_Instance *)hEdmaResMgr;
        rmObj = rmInstance->pResMgrObjHandle;

        if ((rmObj == NULL) ||
            (rmObj->gblCfgParams.globalRegs == NULL))
            {
            result = (EDMA3_RM_E_INVALID_PARAM);
            }
        else
            {
            gblRegs = (volatile EDMA3_CCRL_Regs *)(rmObj->gblCfgParams.globalRegs);

            resId = resObj->resId;

            resIdClr = (unsigned int)(~(1u << (resId%32u)));
            resIdSet = (1u << (resId%32u));

            /**
              * Take the instance specific semaphore, to prevent simultaneous
              * access to the shared resources.
              */
            semResult = edma3OsSemTake(rmInstance->initParam.rmSemHandle,
                                    EDMA3_OSSEM_NO_TIMEOUT);
            if (EDMA3_RM_SOK == semResult)
                {
                switch (resObj->type)
                    {
                    case EDMA3_RM_RES_DMA_CHANNEL :
                            {
                            if (resId == EDMA3_RM_RES_ANY)
                                {
                                for (avlblIdx=0u;
                                     avlblIdx <
                                            rmObj->gblCfgParams.numDmaChannels;
                                     ++avlblIdx)
                                    {
                                    if (((rmInstance->initParam.rmInstInitConfig->ownDmaChannels[avlblIdx/32u])
                                          &
                                          (rmInstance->avlblDmaChannels[avlblIdx/32u])
                                          &
                                          ~(rmInstance->initParam.rmInstInitConfig->resvdDmaChannels[avlblIdx/32u])
                                          &
                                          (1u << (avlblIdx%32u))) != FALSE)
                                        {
                                        /*
                                         * Match found.
                                         * A resource which is owned by this instance of the
                                         * Resource Manager and which is presently available
                                         * and which has not been reserved - is found.
                                         */
                                        resObj->resId = avlblIdx;
                                        /*
                                         * Mark the 'match found' resource as "Not Available"
                                         * for future requests
                                         */
                                        rmInstance->avlblDmaChannels[avlblIdx/32u] &= (unsigned int)(~(1u << (avlblIdx%32u)));

                                        /**
                                         * Check if the register modification flag is
                                         * set or not.
                                         */
                                        if (TRUE == rmInstance->regModificationRequired)
                                            {
                                            /**
                                             * Enable the DMA channel in the
                                             * DRAE/DRAEH registers also.
                                             */
                                            if (avlblIdx < 32u)
                                                {
                                                gblRegs->DRA[rmInstance->initParam.regionId].DRAE
                                                    |= (0x1u << avlblIdx);
                                                }
                                            else
                                                {
                                                gblRegs->DRA[rmInstance->initParam.regionId].DRAEH
                                                    |= (0x1u << (avlblIdx - 32u));
                                                }
                                            }

                                        result = EDMA3_RM_SOK;
                                        break;
                                        }
                                    }
                                /*
                                 * If none of the owned resources of this type is available
                                 * then report "All Resources of this type not available" error
                                 */
                                if (avlblIdx == rmObj->gblCfgParams.numDmaChannels)
                                    {
                                    result = EDMA3_RM_E_ALL_RES_NOT_AVAILABLE;
                                    }
                                }
                            else
                                {
                                if (resId < rmObj->gblCfgParams.numDmaChannels)
                                    {
                                    /*
                                     * Check if specified resource is owned
                                     * by this instance of the resource manager
                                     */
                                    if (((rmInstance->initParam.rmInstInitConfig->ownDmaChannels[resId/32u])&(resIdSet))!=FALSE)
                                       {
                                        /* Now check if specified resource is available presently*/
                                        if (((rmInstance->avlblDmaChannels[resId/32u])&(resIdSet))!=FALSE)
                                            {
                                            /*
                                             * Mark the specified channel as "Not Available"
                                             * for future requests
                                             */
                                            rmInstance->avlblDmaChannels[resId/32u] &= resIdClr;

                                            /**
                                             * Check if the register modification flag is
                                             * set or not.
                                             */
                                            if (TRUE == rmInstance->regModificationRequired)
                                                {
                                                if (resId < 32u)
                                                    {
                                                    rmInstance->shadowRegs->EECR = (1UL << resId);

                                                    /**
                                                     * Enable the DMA channel in the
                                                     * DRAE registers also.
                                                     */
                                                    gblRegs->DRA[rmInstance->initParam.regionId].DRAE
                                                        |= (0x1u << resId);
                                                    }
                                                else
                                                    {
                                                    rmInstance->shadowRegs->EECRH = (1UL << resId);

                                                    /**
                                                     * Enable the DMA channel in the
                                                     * DRAEH registers also.
                                                     */
                                                    gblRegs->DRA[rmInstance->initParam.regionId].DRAEH
                                                        |= (0x1u << (resId - 32u));
                                                    }
                                                }

                                            result = EDMA3_RM_SOK;
                                            }
                                        else
                                            {
                                            /* Specified resource is owned but is already booked */
                                            result = EDMA3_RM_E_SPECIFIED_RES_NOT_AVAILABLE;
                                            }
                                        }
                                    else
                                        {
                                        /*
                                         * Specified resource is not owned by this instance
                                         * of the Resource Manager
                                         */
                                        result = EDMA3_RM_E_RES_NOT_OWNED;
                                        }
                                    }
                                else
                                    {
                                    result = EDMA3_RM_E_INVALID_PARAM;
                                    }
                                }
                        }
                        break;

                    case EDMA3_RM_RES_QDMA_CHANNEL :
                        {
                        if (resId == EDMA3_RM_RES_ANY)
                            {
                            for (avlblIdx=0u; avlblIdx<rmObj->gblCfgParams.numQdmaChannels; ++avlblIdx)
                                {
                                if (((rmInstance->initParam.rmInstInitConfig->ownQdmaChannels[avlblIdx/32u])
                                          &
                                          (rmInstance->avlblQdmaChannels[avlblIdx/32u])
                                          &
                                          ~(rmInstance->initParam.rmInstInitConfig->resvdQdmaChannels[avlblIdx/32u])
                                          &
                                          (1u << (avlblIdx%32u))) != FALSE)
                                    {
                                    resObj->resId = avlblIdx;
                                    rmInstance->avlblQdmaChannels[avlblIdx/32u] &= (unsigned int)(~(1u << (avlblIdx%32u)));

                                    /**
                                     * Check if the register modification flag is
                                     * set or not.
                                     */
                                    if (TRUE == rmInstance->regModificationRequired)
                                        {
                                        /**
                                         * Enable the QDMA channel in the
                                         * QRAE register also.
                                         */
                                        gblRegs->QRAE[rmInstance->initParam.regionId]
                                            |= (0x1u << avlblIdx);
                                        }

                                    result = EDMA3_RM_SOK;
                                    break;
                                    }
                                }
                            /*
                             * If none of the owned resources of this type is available
                             * then report "All Resources of this type not available" error
                             */
                            if (avlblIdx == rmObj->gblCfgParams.numQdmaChannels)
                                {
                                result = EDMA3_RM_E_ALL_RES_NOT_AVAILABLE;
                                }
                            }
                        else
                            {
                            if (resId < rmObj->gblCfgParams.numQdmaChannels)
                                {
                                if (((rmInstance->initParam.rmInstInitConfig->ownQdmaChannels [resId/32u])&(resIdSet))!=FALSE)
                                    {
                                    if (((rmInstance->avlblQdmaChannels [resId/32u])&(resIdSet))!=FALSE)
                                        {
                                        rmInstance->avlblQdmaChannels [resId/32u] &= resIdClr;

                                        /**
                                         * Check if the register modification flag is
                                         * set or not.
                                         */
                                        if (TRUE == rmInstance->regModificationRequired)
                                            {
                                            /**
                                             * Enable the QDMA channel in the
                                             * QRAE register also.
                                             */
                                            gblRegs->QRAE[rmInstance->initParam.regionId]
                                                |= (0x1u << resId);
                                            }

                                        result = EDMA3_RM_SOK;
                                        }
                                    else
                                        {
                                        /* Specified resource is owned but is already booked */
                                        result = EDMA3_RM_E_SPECIFIED_RES_NOT_AVAILABLE;
                                       }
                                    }
                                else
                                    {
                                    /*
                                     * Specified resource is not owned by this instance
                                     * of the Resource Manager
                                     */
                                    result = EDMA3_RM_E_RES_NOT_OWNED;
                                    }
                                }
                            else
                                {
                                result = EDMA3_RM_E_INVALID_PARAM;
                                }
                            }
                        }
                        break;

                    case EDMA3_RM_RES_TCC :
                        {
                        if (resId == EDMA3_RM_RES_ANY)
                            {
                            for (avlblIdx=0u; avlblIdx<rmObj->gblCfgParams.numTccs; ++avlblIdx)
                                {
                                if (((rmInstance->initParam.rmInstInitConfig->ownTccs [avlblIdx/32u])
                                    & (rmInstance->avlblTccs [avlblIdx/32u])
                                    & ~(rmInstance->initParam.rmInstInitConfig->resvdTccs [avlblIdx/32u])
                                    & (1u << (avlblIdx%32u)))!=FALSE)
                                    {
                                    resObj->resId = avlblIdx;
                                    rmInstance->avlblTccs [avlblIdx/32u] &= (unsigned int)(~(1u << (avlblIdx%32u)));

                                    /**
                                     * Check if the register modification flag is
                                     * set or not.
                                     */
                                    if (TRUE == rmInstance->regModificationRequired)
                                        {
                                        /**
                                         * Enable the Interrupt channel in the
                                         * DRAE/DRAEH registers also.
                                         * Also, If the region id coming from this
                                         * RM instance is same as the Master RM
                                         * Instance's region id, only then we will be
                                         * getting the interrupts on the same side.
                                         * So save the TCC in the allocatedTCCs[] array.
                                         */
                                        if (avlblIdx < 32u)
                                            {
                                            gblRegs->DRA[rmInstance->initParam.regionId].DRAE
                                                |= (0x1u << avlblIdx);

                                            /**
                                             * Do not modify this global array if the register
                                             * modificatio flag is not set.
                                             * Reason being is based on this flag, the IPR/ICR
                                             * or error bit is cleared in the completion or
                                             * error handler ISR.
                                             */
                                            if (edma3RegionId == rmInstance->initParam.regionId)
                                                {
                                                allocatedTCCs[rmObj->phyCtrllerInstId][0u] |= (0x1u << avlblIdx);
                                                }
                                            }
                                        else
                                            {
                                            gblRegs->DRA[rmInstance->initParam.regionId].DRAEH
                                                |= (0x1u << (avlblIdx - 32u));

                                            /**
                                             * Do not modify this global array if the register
                                             * modificatio flag is not set.
                                             * Reason being is based on this flag, the IPR/ICR
                                             * or error bit is cleared in the completion or
                                             * error handler ISR.
                                             */
                                            if (edma3RegionId == rmInstance->initParam.regionId)
                                                {
                                                allocatedTCCs[rmObj->phyCtrllerInstId][1u] |= (0x1u << (avlblIdx - 32u));
                                                }
                                            }
                                        }


                                    result = EDMA3_RM_SOK;
                                    break;
                                    }
                                }
                            /*
                             * If none of the owned resources of this type is available
                             * then report "All Resources of this type not available" error
                             */
                            if ( avlblIdx == rmObj->gblCfgParams.numTccs)
                                {
                                result = EDMA3_RM_E_ALL_RES_NOT_AVAILABLE;
                                }
                            }
                        else
                            {
                            if (resId < rmObj->gblCfgParams.numTccs)
                                {
                                if (((rmInstance->initParam.rmInstInitConfig->ownTccs [resId/32u])&(resIdSet))!=FALSE)
                                    {
                                    if (((rmInstance->avlblTccs [resId/32u])&(resIdSet))!=FALSE)
                                        {
                                        rmInstance->avlblTccs [resId/32u] &= resIdClr;

                                        /**
                                         * Check if the register modification flag is
                                         * set or not.
                                         */
                                        if (TRUE == rmInstance->regModificationRequired)
                                            {
                                            /**
                                             * Enable the Interrupt channel in the
                                             * DRAE/DRAEH registers also.
                                             * Also, If the region id coming from this
                                             * RM instance is same as the Master RM
                                             * Instance's region id, only then we will be
                                             * getting the interrupts on the same side.
                                             * So save the TCC in the allocatedTCCs[] array.
                                             */
                                            if (resId < 32u)
                                                {
                                                gblRegs->DRA[rmInstance->initParam.regionId].DRAE
                                                    |= (0x1u << resId);

                                                /**
                                                 * Do not modify this global array if the register
                                                 * modification flag is not set.
                                                 * Reason being is based on this flag, the IPR/ICR
                                                 * or error bit is cleared in the completion or
                                                 * error handler ISR.
                                                 */
                                                if (edma3RegionId == rmInstance->initParam.regionId)
                                                    {
                                                    allocatedTCCs[rmObj->phyCtrllerInstId][0u] |= (0x1u << resId);
                                                    }
                                                }
                                            else
                                                {
                                                gblRegs->DRA[rmInstance->initParam.regionId].DRAEH
                                                    |= (0x1u << (resId - 32u));

                                                /**
                                                 * Do not modify this global array if the register
                                                 * modificatio flag is not set.
                                                 * Reason being is based on this flag, the IPR/ICR
                                                 * or error bit is cleared in the completion or
                                                 * error handler ISR.
                                                 */
                                                if (edma3RegionId == rmInstance->initParam.regionId)
                                                    {
                                                    allocatedTCCs[rmObj->phyCtrllerInstId][1u] |= (0x1u << (resId - 32u));
                                                    }
                                                }
                                            }

                                        result = EDMA3_RM_SOK;
                                        }
                                    else
                                        {
                                        /* Specified resource is owned but is already booked */
                                        result = EDMA3_RM_E_SPECIFIED_RES_NOT_AVAILABLE;
                                        }
                                    }
                                else
                                    {
                                    /*
                                     * Specified resource is not owned by this instance
                                     * of the Resource Manager
                                     */
                                    result = EDMA3_RM_E_RES_NOT_OWNED;
                                    }
                                }
                            else
                                {
                                result = EDMA3_RM_E_INVALID_PARAM;
                                }
                            }
                        }
                        break;

                    case EDMA3_RM_RES_PARAM_SET :
                        {
                        if (resId == EDMA3_RM_RES_ANY)
                            {
                            for (avlblIdx=0u; avlblIdx<rmObj->gblCfgParams.numPaRAMSets; ++avlblIdx)
                                {
                                if (((rmInstance->initParam.rmInstInitConfig->ownPaRAMSets [avlblIdx/32u])
                                      &
                                      (rmInstance->avlblPaRAMSets [avlblIdx/32u])
                                      &
                                      ~(rmInstance->initParam.rmInstInitConfig->resvdPaRAMSets [avlblIdx/32u])
                                      &
                                      (1u << (avlblIdx%32u)))!=FALSE)
                                    {
                                    resObj->resId = avlblIdx;
                                    rmInstance->avlblPaRAMSets [avlblIdx/32u] &= (unsigned int)(~(1u << (avlblIdx%32u)));

                                    /**
                                     * Also, make the actual PARAM Set NULL, checking the flag
                                     * whether it is required or not.
                                     */
                                    if ((TRUE == rmInstance->regModificationRequired)
                                        && (TRUE == rmInstance->paramInitRequired))
                                        {
                                        edma3MemSet((void *)(&gblRegs->PARAMENTRY[avlblIdx]),
                                                    0x00u,
                                                    sizeof(gblRegs->PARAMENTRY[avlblIdx]));
                                        }

                                    result = EDMA3_RM_SOK;
                                    break;
                                    }
                                }
                            /*
                             * If none of the owned resources of this type is available
                             * then report "All Resources of this type not available" error
                             */
                            if ( avlblIdx == rmObj->gblCfgParams.numPaRAMSets)
                                {
                                result = EDMA3_RM_E_ALL_RES_NOT_AVAILABLE;
                                }
                            }
                        else
                            {
                            if (resId < rmObj->gblCfgParams.numPaRAMSets)
                                {
                                if (((rmInstance->initParam.rmInstInitConfig->ownPaRAMSets [resId/32u])&(resIdSet))!=FALSE)
                                    {
                                    if (((rmInstance->avlblPaRAMSets [resId/32u])&(resIdSet)) !=FALSE)
                                        {
                                        rmInstance->avlblPaRAMSets [resId/32u] &= resIdClr;

                                        /**
                                         * Also, make the actual PARAM Set NULL, checking the flag
                                         * whether it is required or not.
                                         */
                                        if ((TRUE == rmInstance->regModificationRequired)
                                            && (TRUE == rmInstance->paramInitRequired))
                                            {
                                            edma3MemSet((void *)(&gblRegs->PARAMENTRY[resId]),
                                                        0x00u,
                                                        sizeof(gblRegs->PARAMENTRY[resId]));
                                            }

                                        result = EDMA3_RM_SOK;
                                        }
                                    else
                                        {
                                        /* Specified resource is owned but is already booked */
                                        result = EDMA3_RM_E_SPECIFIED_RES_NOT_AVAILABLE;
                                        }
                                    }
                                else
                                    {
                                    /*
                                     * Specified resource is not owned by this instance
                                     * of the Resource Manager
                                     */
                                    result = EDMA3_RM_E_RES_NOT_OWNED;
                                    }
                                }
                            else
                                {
                                result = EDMA3_RM_E_INVALID_PARAM;
                                }
                            }
                        }
                        break;

                    default:
                            result = EDMA3_RM_E_INVALID_PARAM;
                        break;
                    }

                /* Return the semaphore back */
                semResult = edma3OsSemGive(rmInstance->initParam.rmSemHandle);
                }
            }
        }

    /**
     * Check the Resource Allocation Result 'result' first. If Resource
     * Allocation has resulted in an error, return it (having more priority than
     * semResult.
     * Else, return semResult.
     */
     if (EDMA3_RM_SOK == result)
         {
         /**
          * Resource Allocation successful, return semResult for returning
          * semaphore.
          */
         result = semResult;
         }

#ifdef EDMA3_INSTRUMENTATION_ENABLED
    EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3",
                EDMA3_DVT_DESC(EDMA3_DVT_eFUNC_END,
                EDMA3_DVT_dCOUNTER,
                EDMA3_DVT_dNONE,
                EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */

    return result;
    }



/**\fn      EDMA3_RM_Result EDMA3_RM_freeResource(EDMA3_RM_Handle hEdmaResMgr,
 *                            const EDMA3_RM_ResDesc *resObj)
 * \brief   This API is used to free previously allocated EDMA3 Resources like
 * DMA/QDMA channel, PaRAM Set or TCC.
 *
 * To free a specific resource, first this API checks whether that resource is
 * OWNED by the Resource Manager Instance. Then it checks whether that resource
 * has been allocated by the Resource Manager instance or not.
 *
 * After freeing a DMA/QDMA channel or TCC, the same resource is disabled in
 * the shadow region specific register (DRAE/DRAEH/QRAE).
 *
 * \param  hEdmaResMgr         [IN]    Handle to the previously opened Resource
 *                                     Manager Instance.
 * \param   resObj             [IN]    Handle to the resource descriptor
 *                                     object, which needs to be freed.
 *
 * \return  EDMA3_RM_SOK or EDMA3_RM Error Code
 *
 * \note    This function disables the global interrupts to prevent
 *          simultaneous access to the global pool of resources.
 *          It is re-entrant.
 */
EDMA3_RM_Result EDMA3_RM_freeResource(EDMA3_RM_Handle hEdmaResMgr,
                            const EDMA3_RM_ResDesc *resObj)
    {
    unsigned int intState;
    EDMA3_RM_Instance *rmInstance = NULL;
    EDMA3_RM_Obj *rmObj = NULL;
    EDMA3_RM_Result result = EDMA3_RM_SOK;
    unsigned int resId;
    unsigned int resIdSet = 0x0;
    volatile EDMA3_CCRL_Regs *gblRegs = NULL;

#ifdef EDMA3_INSTRUMENTATION_ENABLED
    EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3",
                EDMA3_DVT_DESC(EDMA3_DVT_eFUNC_START,
                EDMA3_DVT_dCOUNTER,
                EDMA3_DVT_dNONE,
                EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */

	/* If parameter checking is enabled... */
#ifndef EDMA3_RM_PARAM_CHECK_DISABLE
    if ((hEdmaResMgr == NULL) || (resObj == NULL))
        {
        result = EDMA3_RM_E_INVALID_PARAM;
        }
#endif

	/* Check if the parameters are OK. */
	if (EDMA3_RM_SOK == result)
        {
        rmInstance = (EDMA3_RM_Instance *)hEdmaResMgr;
        rmObj = rmInstance->pResMgrObjHandle;

        if ((rmObj == NULL) ||
            (rmObj->gblCfgParams.globalRegs == NULL))
            {
            result = EDMA3_RM_E_INVALID_PARAM;
            }
        else
            {
            gblRegs = (volatile EDMA3_CCRL_Regs *)(rmObj->gblCfgParams.globalRegs);

            resId = resObj->resId;

            resIdSet = 1u << (resId%32u);

            edma3OsProtectEntry (rmObj->phyCtrllerInstId,
								EDMA3_OS_PROTECT_INTERRUPT, 
								&intState);

            if (EDMA3_RM_SOK == result)
                {
                switch (resObj->type)
                    {
                    case EDMA3_RM_RES_DMA_CHANNEL :
                        {
                        if (resId < rmObj->gblCfgParams.numDmaChannels)
                            {
                            if (((rmInstance->initParam.rmInstInitConfig->ownDmaChannels [resId/32u]) & (resIdSet))!=FALSE)
                                {
                                if (((~(rmInstance->avlblDmaChannels[resId/32u]))&(resIdSet))!=FALSE)
                                    {
                                    /*
                                     * Mark the specified channel as "Available"
                                     * for future requests
                                     */
                                    rmInstance->avlblDmaChannels[resId/32u] |= resIdSet;

                                    /**
                                     * Check if the register modification flag is
                                     * set or not.
                                     */
                                    if (TRUE == rmInstance->regModificationRequired)
                                        {
                                        /**
                                         * DMA Channel is freed.
                                         * Reset the bit specific to the DMA channel
                                         * in the DRAE/DRAEH register also.
                                         */
                                        if (resId < 32u)
                                            {
                                            gblRegs->DRA[rmInstance->initParam.regionId].DRAE
                                                            &= (~(0x1u << resId));
                                            }
                                        else
                                            {
                                            gblRegs->DRA[rmInstance->initParam.regionId].DRAEH
                                                            &= (~(0x1u << (resId-32u)));
                                            }
                                        }

                                    result = EDMA3_RM_SOK;
                                    }
                                else
                                    {
                                    result = EDMA3_RM_E_RES_ALREADY_FREE;
                                    }
                                }
                            else
                                {
                                /*
                                 * Specified resource is not owned by this instance
                                 * of the Resource Manager
                                 */
                                result = EDMA3_RM_E_RES_NOT_OWNED;
                                }
                            }
                        else
                            {
                            result = EDMA3_RM_E_INVALID_PARAM;
                            }
                        }
                        break;

                    case EDMA3_RM_RES_QDMA_CHANNEL :
                        {
                        if (resId < rmObj->gblCfgParams.numQdmaChannels)
                            {
                            if (((rmInstance->initParam.rmInstInitConfig->ownQdmaChannels [resId/32u]) & (resIdSet))!=FALSE)
                                {
                                if (((~(rmInstance->avlblQdmaChannels [resId/32u])) & (resIdSet))!=FALSE)
                                    {
                                    rmInstance->avlblQdmaChannels [resId/32u] |= resIdSet;

                                    /**
                                     * Check if the register modification flag is
                                     * set or not.
                                     */
                                    if (TRUE == rmInstance->regModificationRequired)
                                        {
                                        /**
                                         * QDMA Channel is freed.
                                         * Reset the bit specific to the QDMA channel
                                         * in the QRAE register also.
                                         */
                                        gblRegs->QRAE[rmInstance->initParam.regionId]
                                                        &= (~(0x1u << resId));
                                        }

                                    result = EDMA3_RM_SOK;
                                    }
                                else
                                    {
                                    result = EDMA3_RM_E_RES_ALREADY_FREE;
                                    }
                                }
                            else
                                {
                                /*
                                 * Specified resource is not owned by this instance
                                 * of the Resource Manager
                                 */
                                result = EDMA3_RM_E_RES_NOT_OWNED;
                                }
                            }
                        else
                            {
                            result = EDMA3_RM_E_INVALID_PARAM;
                            }
                        }
                        break;

                    case EDMA3_RM_RES_TCC :
                        {
                        if (resId < rmObj->gblCfgParams.numTccs)
                            {
                            if (((rmInstance->initParam.rmInstInitConfig->ownTccs [resId/32u]) & (resIdSet))!=FALSE)
                                {
                                if (((~(rmInstance->avlblTccs [resId/32u])) & (resIdSet))!=FALSE)
                                    {
                                    rmInstance->avlblTccs [resId/32u] |= resIdSet;

                                    /**
                                     * Check if the register modification flag is
                                     * set or not.
                                     */
                                    if (TRUE == rmInstance->regModificationRequired)
                                        {
                                        /**
                                         * Interrupt Channel is freed.
                                         * Reset the bit specific to the Interrupt
                                         * channel in the DRAE/DRAEH register also.
                                         * Also, if we have earlier saved this
                                         * TCC in allocatedTCCs[] array,
                                         * remove it from there too.
                                         */
                                        if (resId < 32u)
                                            {
                                            gblRegs->DRA[rmInstance->initParam.regionId].DRAE
                                                            &= (~(0x1u << resId));

                                            if (edma3RegionId == rmInstance->initParam.regionId)
                                                {
                                                allocatedTCCs[rmObj->phyCtrllerInstId][0u] &= (~(0x1u << resId));
                                                }
                                            }
                                        else
                                            {
                                            gblRegs->DRA[rmInstance->initParam.regionId].DRAEH
                                                            &= (~(0x1u << (resId-32u)));

                                            if (edma3RegionId == rmInstance->initParam.regionId)
                                                {
                                                allocatedTCCs[rmObj->phyCtrllerInstId][1u] &= (~(0x1u << (resId -32u)));
                                                }
                                            }
                                        }

                                    result = EDMA3_RM_SOK;
                                    }
                                else
                                    {
                                    result = EDMA3_RM_E_RES_ALREADY_FREE;
                                    }
                                }
                            else
                                {
                                /*
                                 * Specified resource is not owned by this instance
                                 * of the Resource Manager
                                 */
                                result = EDMA3_RM_E_RES_NOT_OWNED;
                                }
                            }
                        else
                            {
                            result = EDMA3_RM_E_INVALID_PARAM;
                            }
                        }
                        break;

                    case EDMA3_RM_RES_PARAM_SET :
                        {
                        if (resId < rmObj->gblCfgParams.numPaRAMSets)
                            {
                            if (((rmInstance->initParam.rmInstInitConfig->ownPaRAMSets [resId/32u])&(resIdSet))!=FALSE)
                                {
                                if (((~(rmInstance->avlblPaRAMSets [resId/32u]))&(resIdSet))!=FALSE)
                                    {
                                    rmInstance->avlblPaRAMSets [resId/32u] |= resIdSet;

                                    result = EDMA3_RM_SOK;
                                    }
                                else
                                    {
                                    result = EDMA3_RM_E_RES_ALREADY_FREE;
                                    }
                                }
                            else
                                {
                                /*
                                 * Specified resource is not owned by this instance
                                 * of the Resource Manager
                                 */
                                result = EDMA3_RM_E_RES_NOT_OWNED;
                                }
                            }
                        else
                            {
                            result = EDMA3_RM_E_INVALID_PARAM;
                            }
                        }
                        break;

                    default:
                        result = EDMA3_RM_E_INVALID_PARAM;
                        break;
                    }

                }

            edma3OsProtectExit (rmObj->phyCtrllerInstId,
								EDMA3_OS_PROTECT_INTERRUPT, 
								intState);
            }
        }

#ifdef EDMA3_INSTRUMENTATION_ENABLED
    EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3",
                EDMA3_DVT_DESC(EDMA3_DVT_eFUNC_END,
                EDMA3_DVT_dCOUNTER,
                EDMA3_DVT_dNONE,
                EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */

    return result;
    }



/**
 * \fn      EDMA3_RM_Result EDMA3_RM_allocLogicalChannel(EDMA3_RM_Handle
 *                   hEdmaResMgr, EDMA3_RM_ResDesc *lChObj,
 *                   unsigned int *pParam, unsigned int *pTcc)
 * \brief   Request a DMA/QDMA/Link channel.
 *
 * This API is used to allocate a logical channel (DMA/QDMA/Link) along with
 * the associated resources. For DMA and QDMA channels, TCC and PaRAM Set are
 * also allocated along with the requested channel. For Link channel, ONLY a
 * PaRAM Set is allocated.
 *
 * Note: To free the logical channel allocated by this API, user should call
 * EDMA3_RM_freeLogicalChannel () ONLY to de-allocate all the allocated resources
 * and remove certain mappings.
 *
 * User can request a specific logical channel by passing the channel id in
 * 'lChObj->resId' and channel type in 'lChObj->type'. Note that the channel
 * id is the same as the actual resource id. For e.g. in the case of QDMA
 * channels, valid channel ids are from 0 to 7 only.
 *
 * User can also request ANY available logical channel of the type
 * 'lChObj->type' by specifying 'lChObj->resId' as:
 *  a)  EDMA3_RM_DMA_CHANNEL_ANY: For DMA channels
 *  b)  EDMA3_RM_QDMA_CHANNEL_ANY: For QDMA channels, and
 *  c)  EDMA3_RM_PARAM_ANY: For Link channels. Normally user should use this
 *      value to request link channels (PaRAM Sets used for linking purpose
 *      only), unless he wants to use some specific link channels (PaRAM Sets)
 *      which is also allowed.
 *
 * This API internally uses EDMA3_RM_allocResource () to allocate the desired
 * resources (DMA/QDMA channel, PaRAM Set and TCC).
 *
 * For DMA/QDMA channels, after allocating all the EDMA3 resources, this API
 * sets the TCC field of the OPT PaRAM Word with the allocated TCC.
 *
 * For DMA channel, it also sets the DCHMAP register, if required.
 *
 * For QDMA channel, it sets the QCHMAP register and CCNT as trigger word and
 * enables the QDMA channel by writing to the QEESR register.
 *
 * \param  hEdmaResMgr      [IN]        Handle to the previously opened Resource
 *                                      Manager Instance.
 * \param  lChObj           [IN/OUT]    Handle to the requested logical channel
 *                                      object, which needs to be allocated.
 *                                      It could be a specific logical channel
 *                                      or ANY available logical channel of the
 *                                      requested type.
 *                                      In case user passes a specific resource
 *                                      Id, lChObj value is left unchanged. In
 *                                      case user requests ANY available
 *                                      resource, the allocated resource id is
 *                                      returned in lChObj->resId.
 *
 *  \param  pParam          [IN/OUT]    PaRAM Set for a particular logical
 *                                      (DMA/QDMA) channel. Not used if user
 *                                      requested for a Link channel.
 *                                      In case user passes a specific PaRAM
 *                                      Set value, pParam value is left
 *                                      unchanged. In case user requests ANY
 *                                      available PaRAM Set, the allocated one
 *                                      is returned in pParam.
 *
 *  \param  pTcc            [IN/OUT]    TCC for a particular logical (DMA/QDMA)
 *                                      channel. Not used if user requested for
 *                                      a Link channel.
 *                                      In case user passes a specific TCC
 *                                      value, pTcc value is left unchanged.
 *                                      In case user requests ANY available TCC,
 *                                      the allocated one is returned in pTcc
 *
 * \return  EDMA3_RM_SOK or EDMA_RM Error Code
 *
 * \note    This function internally calls EDMA3_RM_allocResource (), which
 *          acquires a RM Instance specific semaphore to prevent simultaneous
 *          access to the global pool of resources. It is re-entrant for unique
 *          logical channel values, but SHOULD NOT be called from the user
 *          callback function (ISR context).
 */
EDMA3_RM_Result EDMA3_RM_allocLogicalChannel(EDMA3_RM_Handle hEdmaResMgr,
                            EDMA3_RM_ResDesc *lChObj,
                            unsigned int *pParam,
                            unsigned int *pTcc)
    {
    EDMA3_RM_ResDesc *chObj = NULL;
    EDMA3_RM_ResDesc resObj;
    EDMA3_RM_Result result = EDMA3_RM_SOK;
    EDMA3_RM_Instance *rmInstance = NULL;
    EDMA3_RM_Obj *rmObj = NULL;
    unsigned int mappedPaRAMId=0u;
    unsigned int mappedTcc = EDMA3_RM_CH_NO_TCC_MAP;
    int paRAMId = (int)EDMA3_RM_RES_ANY;
    volatile EDMA3_CCRL_Regs *gblRegs = NULL;
    unsigned int qdmaChId = EDMA3_MAX_PARAM_SETS;


#ifdef EDMA3_INSTRUMENTATION_ENABLED
    EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3",
                EDMA3_DVT_DESC(EDMA3_DVT_eFUNC_START,
                EDMA3_DVT_dCOUNTER,
                EDMA3_DVT_dNONE,
                EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */

	/* If parameter checking is enabled... */
#ifndef EDMA3_RM_PARAM_CHECK_DISABLE
    if ((lChObj == NULL) || (hEdmaResMgr == NULL))
        {
        result = EDMA3_RM_E_INVALID_PARAM;
        }
#endif

	/* Check if the parameters are OK. */
	if (EDMA3_RM_SOK == result)
        {
        chObj = lChObj;

        if ((chObj->type == EDMA3_RM_RES_DMA_CHANNEL)
            || (chObj->type == EDMA3_RM_RES_QDMA_CHANNEL))
            {
            /**
             * If the request is for a DMA or QDMA channel, check the
             * pParam and pTcc objects also.
             * For the Link channel request, they could be NULL.
             */
			/* If parameter checking is enabled... */
#ifndef EDMA3_RM_PARAM_CHECK_DISABLE
            if ((pParam == NULL) || (pTcc == NULL))
                {
                result = EDMA3_RM_E_INVALID_PARAM;
                }
#endif
            }
        }

    if (result == EDMA3_RM_SOK)
        {
        rmInstance = (EDMA3_RM_Instance *)hEdmaResMgr;

        if (rmInstance == NULL)
            {
            result = EDMA3_RM_E_INVALID_PARAM;
            }
        }

    if (result == EDMA3_RM_SOK)
        {
        rmObj = rmInstance->pResMgrObjHandle;

        if (rmObj == NULL)
            {
            result = EDMA3_RM_E_INVALID_PARAM;
            }
        else
            {
            if (rmObj->gblCfgParams.globalRegs == NULL)
                {
                result = EDMA3_RM_E_INVALID_PARAM;
                }
            }
        }

    if (result == EDMA3_RM_SOK)
        {
        gblRegs = (volatile EDMA3_CCRL_Regs *)(rmObj->gblCfgParams.globalRegs);

        switch (chObj->type)
            {
            case EDMA3_RM_RES_DMA_CHANNEL:
                {
                if ((chObj->resId == EDMA3_RM_DMA_CHANNEL_ANY)
                    || (chObj->resId == EDMA3_RM_RES_ANY))
                    {
                    /* Request for ANY DMA channel. */
                    resObj.type = EDMA3_RM_RES_DMA_CHANNEL;
                    resObj.resId = EDMA3_RM_RES_ANY;
                    result = EDMA3_RM_allocResource(hEdmaResMgr, (EDMA3_RM_ResDesc *)&resObj);

                    if (result == EDMA3_RM_SOK)
                        {
                        /* DMA channel allocated successfully. */
                        chObj->resId = resObj.resId;

                        /**
                         * Check the PaRAM Set user has specified for this DMA channel.
                         * Two cases exist:
                         * a) DCHMAP exists: Any PaRAM Set can be used
                         * b) DCHMAP does not exist: Should not be possible
                         * only if the channel allocated (ANY) and PaRAM requested
                         * are same.
                         */
                        if ((*pParam) == EDMA3_RM_PARAM_ANY)
                            {
                            /* User specified ANY PaRAM Set; Check the mapping. */
                            mappedPaRAMId = rmObj->gblCfgParams.dmaChannelPaRAMMap[resObj.resId];
                            if (mappedPaRAMId != EDMA3_RM_CH_NO_PARAM_MAP)
                                {
                                /** If some PaRAM set is statically mapped to the returned
                                * channel number, use that.
                                */
                                paRAMId = (int)mappedPaRAMId;
                                }
                            }
                        else
                            {
                            /* User specified some PaRAM Set; check that can be used or not. */
                            if (TRUE == rmObj->gblCfgParams.dmaChPaRAMMapExists)
                                {
                                paRAMId = (int)(*pParam);
                                }
                            else
                                {
                                /**
                                 * Channel mapping does not exist. If the PaRAM Set requested
                                 * is the same as dma channel allocated (coincidentally), it is fine.
                                 * Else return error.
                                 */
                                if ((*pParam) != (resObj.resId))
                                    {
                                    result = EDMA3_RM_E_INVALID_PARAM;

                                    /**
                                     * Free the previously allocated DMA channel also.
                                     */
                                    EDMA3_RM_freeResource(hEdmaResMgr, (EDMA3_RM_ResDesc *)&resObj);
                                    }
                                else
                                    {
                                    paRAMId = (int)(*pParam);
                                    }
                                }
                            }

                        mappedTcc = rmObj->gblCfgParams.dmaChannelTccMap[resObj.resId];
                        }
                    }
                else
                    {
                    if (chObj->resId <= EDMA3_RM_DMA_CH_MAX_VAL)
                        {
                        /* Request for a specific DMA channel */
                        resObj.type = EDMA3_RM_RES_DMA_CHANNEL;
                        resObj.resId = chObj->resId;
                        result = EDMA3_RM_allocResource(hEdmaResMgr,
                                        (EDMA3_RM_ResDesc *)&resObj);

                        if (result == EDMA3_RM_SOK)
                            {
                            /**
                             * Check the PaRAM Set user has specified for this DMA channel.
                             * Two cases exist:
                             * a) DCHMAP exists: Any PaRAM Set can be used
                             * b) DCHMAP does not exist: Should not be possible
                             * only if the channel allocated (ANY) and PaRAM requested
                             * are same.
                             */
                            if ((*pParam) == EDMA3_RM_PARAM_ANY)
                                {
                                /* User specified ANY PaRAM Set; Check the mapping. */
                                mappedPaRAMId = rmObj->gblCfgParams.dmaChannelPaRAMMap[resObj.resId];
                                if (mappedPaRAMId != EDMA3_RM_CH_NO_PARAM_MAP)
                                    {
                                    /** If some PaRAM set is statically mapped to the returned
                                    * channel number, use that.
                                    */
                                    paRAMId = (int)mappedPaRAMId;
                                    }
                                }
                            else
                                {
                                /* User specified some PaRAM Set; check that can be used or not. */
                                if (TRUE == rmObj->gblCfgParams.dmaChPaRAMMapExists)
                                    {
                                    paRAMId = (int)(*pParam);
                                    }
                                else
                                    {
                                    /**
                                     * Channel mapping does not exist. If the PaRAM Set requested
                                     * is the same as dma channel allocated (coincidentally), it is fine.
                                     * Else return error.
                                     */
                                    if ((*pParam) != (resObj.resId))
                                        {
                                        result = EDMA3_RM_E_INVALID_PARAM;

                                        /**
                                         * Free the previously allocated DMA channel also.
                                         */
                                        EDMA3_RM_freeResource(hEdmaResMgr, (EDMA3_RM_ResDesc *)&resObj);
                                        }
                                    else
                                        {
                                        paRAMId = (int)(*pParam);
                                        }
                                    }
                                }

                            mappedTcc = rmObj->gblCfgParams.dmaChannelTccMap[chObj->resId];
                            }
                        }
                    else
                        {
                        result = EDMA3_RM_E_INVALID_PARAM;
                        }
                    }
                }
                break;


            case EDMA3_RM_RES_QDMA_CHANNEL:
                {
                if ((chObj->resId == EDMA3_RM_QDMA_CHANNEL_ANY)
                    || (chObj->resId == EDMA3_RM_RES_ANY))
                    {
                    /* First request for any available QDMA channel */
                    resObj.type = EDMA3_RM_RES_QDMA_CHANNEL;
                    resObj.resId = EDMA3_RM_RES_ANY;
                    result = EDMA3_RM_allocResource(hEdmaResMgr,
                                    (EDMA3_RM_ResDesc *)&resObj);

                    if (result == EDMA3_RM_SOK)
                        {
                        /* Return the actual QDMA channel id. */
                        chObj->resId = resObj.resId;

                        /* Save the Logical-QDMA channel id for future use. */
                        qdmaChId = resObj.resId + EDMA3_RM_QDMA_CH_MIN_VAL;

                        /**
                         * Check the PaRAM Set user has specified for this QDMA channel.
                         * If he has specified any particular PaRAM Set, use that.
                         */
                        if ((*pParam) != EDMA3_RM_PARAM_ANY)
                            {
                            /* User specified ANY PaRAM Set; Check the mapping. */
                            paRAMId = (int)(*pParam);
                            }
                        }
                    }
                else
                    {
                    if (chObj->resId < EDMA3_MAX_QDMA_CH)
                        {
                        /* Request for a specific QDMA channel */
                        resObj.type = EDMA3_RM_RES_QDMA_CHANNEL;
                        resObj.resId = chObj->resId;
                        result = EDMA3_RM_allocResource(hEdmaResMgr, (EDMA3_RM_ResDesc *)&resObj);

                        if (result == EDMA3_RM_SOK)
                            {
                            /* Save the Logical-QDMA channel id for future use. */
                            qdmaChId = chObj->resId + EDMA3_RM_QDMA_CH_MIN_VAL;

                            /**
                             * Check the PaRAM Set user has specified for this QDMA channel.
                             * If he has specified any particular PaRAM Set, use that.
                             */
                            if ((*pParam) != EDMA3_RM_PARAM_ANY)
                                {
                                /* User specified ANY PaRAM Set; Check the mapping. */
                                paRAMId = (int)(*pParam);
                                }
                            }
                        }
                    else
                        {
                        result = EDMA3_RM_E_INVALID_PARAM;
                        }
                    }
                }
                break;

            case EDMA3_RM_RES_PARAM_SET:
                    {
                    /* Request for a LINK channel. */
                    if ((chObj->resId == EDMA3_RM_PARAM_ANY)
                        || (chObj->resId == EDMA3_RM_RES_ANY))
                        {
                        /* Request for ANY LINK channel. */
                        paRAMId = (int)EDMA3_RM_RES_ANY;
                        }
                    else
                        {
                        if (chObj->resId < edma3NumPaRAMSets)
                            {
                            /* Request for a Specific LINK channel. */
                            paRAMId = (int)(chObj->resId);
                            }
                        else
                            {
                            result = EDMA3_RM_E_INVALID_PARAM;
                            }
                        }

                    if (result == EDMA3_RM_SOK)
                        {
                        /* Try to allocate the link channel */
                        resObj.type = EDMA3_RM_RES_PARAM_SET;
                        resObj.resId = (unsigned int)paRAMId;
                        result = EDMA3_RM_allocResource(hEdmaResMgr, (EDMA3_RM_ResDesc *)&resObj);

                        if (result == EDMA3_RM_SOK)
                            {
                            unsigned int linkCh = EDMA3_RM_LINK_CH_MIN_VAL;

                            /* Return the actual PaRAM Id. */
                            chObj->resId = resObj.resId;

                            /*
                            * Search for the next Link channel place-holder available,
                            * starting from EDMA3_RM_LINK_CH_MIN_VAL.
                            * It will be used for future operations on the Link channel.
                            */
                            while ((edma3RmChBoundRes[rmObj->phyCtrllerInstId][linkCh].paRAMId != -1)
                                        && (linkCh <= EDMA3_RM_LINK_CH_MAX_VAL))
                                {
                                /* Move to the next place-holder. */
                                linkCh++;
                                }

                            /* Verify the returned handle, it should lie in the correct range */
                            if (linkCh > EDMA3_RM_LINK_CH_MAX_VAL)
                                {
                                result = EDMA3_RM_E_INVALID_PARAM;

                                /* Free the PaRAM Set now. */
                                resObj.type = EDMA3_RM_RES_PARAM_SET;
                                resObj.resId = chObj->resId;
                                result = EDMA3_RM_freeResource(hEdmaResMgr, (EDMA3_RM_ResDesc *)&resObj);
                                }
                            else
                                {
                                /* Save the PaRAM Id for the Link Channel. */
                                edma3RmChBoundRes[rmObj->phyCtrllerInstId][linkCh].paRAMId = (int)(chObj->resId);

                                /**
                                 * Remove any linking. Before doing that, check
                                 * whether it is permitted or not.
                                 */
                                if (TRUE == rmInstance->regModificationRequired)
                                    {
                                    *((&gblRegs->PARAMENTRY[chObj->resId].OPT)
                                            + (unsigned int)EDMA3_RM_PARAM_ENTRY_LINK_BCNTRLD) = 0xFFFFu;
                                    }
                                }
                            }
                        }
                    }
                    break;

            default:
                    result = EDMA3_RM_E_INVALID_PARAM;
            }
        }


    if (result == EDMA3_RM_SOK)
        {
        /**
         * For DMA/QDMA channels, we still have to allocate more resources like
         * TCC, PaRAM Set etc.
         * For Link channel, only the PaRAMSet is required and that has been
         * allocated so no further operations required.
         */

        /* Further resources' allocation for DMA channel. */
        if (chObj->type == EDMA3_RM_RES_DMA_CHANNEL)
            {
            /* First allocate a PaRAM Set */
            resObj.type = EDMA3_RM_RES_PARAM_SET;
            /* Use the saved param id now. */
            resObj.resId = (unsigned int)paRAMId;
            result = EDMA3_RM_allocResource(hEdmaResMgr, (EDMA3_RM_ResDesc *)&resObj);
            if (result == EDMA3_RM_SOK)
                {
                /**
                 * PaRAM Set allocation succeeded.
                 * Save the PaRAM Set first.
                 */
                *pParam = resObj.resId;
                edma3RmChBoundRes[rmObj->phyCtrllerInstId][chObj->resId].paRAMId = (int)(resObj.resId);

                /* Allocate the TCC now. */
                resObj.type = EDMA3_RM_RES_TCC;
                if ((*pTcc) == EDMA3_RM_TCC_ANY)
                    {
                    if (mappedTcc == EDMA3_RM_CH_NO_TCC_MAP)
                        {
                        resObj.resId = EDMA3_RM_RES_ANY;
                        }
                    else
                        {
                        resObj.resId = mappedTcc;
                        }
                    }
                else
                    {
                    resObj.resId = *pTcc;
                    }

                result = EDMA3_RM_allocResource(hEdmaResMgr, (EDMA3_RM_ResDesc *)&resObj);
                if (result == EDMA3_RM_SOK)
                    {
                     /* TCC allocation succeeded. Save it first. */
                    *pTcc = resObj.resId;
                     edma3RmChBoundRes[rmObj->phyCtrllerInstId][chObj->resId].tcc = resObj.resId;

                    /**
                     * Check first whether the global registers and the allocated
                     * PaRAM Set can be modified or not. If yes, do the needful.
                     * Else leave this for the user.
                     */
                    if (TRUE == rmInstance->regModificationRequired)
                        {
                        /* Set TCC of the allocated Param Set. */
                        gblRegs->PARAMENTRY[*pParam].OPT  &= EDMA3_RM_OPT_TCC_CLR_MASK;
                        gblRegs->PARAMENTRY[*pParam].OPT |= EDMA3_RM_OPT_TCC_SET_MASK(*pTcc);

                        /**
                         * Do the mapping between DMA channel and PaRAM Set.
                         * Do this for the EDMA3 Controllers which have a register for mapping
                         * DMA Channel to a particular PaRAM Set.
                         */
                        if (TRUE == rmObj->gblCfgParams.dmaChPaRAMMapExists)
                            {
                            gblRegs = (volatile EDMA3_CCRL_Regs *)(rmObj->gblCfgParams.globalRegs);

                            /* Map Parameter RAM Set Number for specified channelId */
                            gblRegs->DCHMAP[chObj->resId] &= EDMA3_RM_DCH_PARAM_CLR_MASK;
                            gblRegs->DCHMAP[chObj->resId] |= EDMA3_RM_DCH_PARAM_SET_MASK(*pParam);
                            }

                        /* Remove any linking */
                        *((&gblRegs->PARAMENTRY[*pParam].OPT)
                                + (unsigned int)EDMA3_RM_PARAM_ENTRY_LINK_BCNTRLD) = 0xFFFFu;
                        }
                    }
                else
                    {
                    /**
                     * TCC allocation failed, free the previously allocated
                     * PaRAM Set and DMA channel.
                     */
                    resObj.type = EDMA3_RM_RES_PARAM_SET;
                    resObj.resId = *pParam;
                    EDMA3_RM_freeResource(hEdmaResMgr, (EDMA3_RM_ResDesc *)&resObj);

                    /* Reset the book-keeping data structure also. */
                    edma3RmChBoundRes[rmObj->phyCtrllerInstId][chObj->resId].paRAMId = -1;

                    resObj.type = chObj->type;
                    resObj.resId = chObj->resId;
                    EDMA3_RM_freeResource(hEdmaResMgr, (EDMA3_RM_ResDesc *)&resObj);
                    }
                }
            else
                {
                /**
                 * PaRAM Set allocation failed, free the previously allocated
                 * DMA channel also.
                 */
                resObj.type = chObj->type;
                resObj.resId = chObj->resId;
                EDMA3_RM_freeResource(hEdmaResMgr, (EDMA3_RM_ResDesc *)&resObj);
                }
            }


        /* Further resources' allocation for QDMA channel. */
        if (chObj->type == EDMA3_RM_RES_QDMA_CHANNEL)
            {
            /* First allocate a PaRAM Set */
            resObj.type = EDMA3_RM_RES_PARAM_SET;
            /* Use the saved param id now. */
            resObj.resId = (unsigned int)paRAMId;
            result = EDMA3_RM_allocResource(hEdmaResMgr, (EDMA3_RM_ResDesc *)&resObj);
            if (result == EDMA3_RM_SOK)
                {
                /**
                 * PaRAM Set allocation succeeded.
                 * Save the PaRAM Set first.
                 */
                *pParam = resObj.resId;
                edma3RmChBoundRes[rmObj->phyCtrllerInstId][qdmaChId].paRAMId = (int)(resObj.resId);

                /* Allocate the TCC now. */
                resObj.type = EDMA3_RM_RES_TCC;
                if ((*pTcc) == EDMA3_RM_TCC_ANY)
                    {
                    if (mappedTcc == EDMA3_RM_CH_NO_TCC_MAP)
                        {
                        resObj.resId = EDMA3_RM_RES_ANY;
                        }
                    else
                        {
                        resObj.resId = mappedTcc;
                        }
                    }
                else
                    {
                    resObj.resId = *pTcc;
                    }

                result = EDMA3_RM_allocResource(hEdmaResMgr, (EDMA3_RM_ResDesc *)&resObj);
                if (result == EDMA3_RM_SOK)
                    {
                     /* TCC allocation succeeded. Save it first. */
                    *pTcc = resObj.resId;
                     edma3RmChBoundRes[rmObj->phyCtrllerInstId][qdmaChId].tcc = resObj.resId;

                    /**
                     * Check first whether the global registers and the allocated
                     * PaRAM Set can be modified or not. If yes, do the needful.
                     * Else leave this for the user.
                     */
                    if (TRUE == rmInstance->regModificationRequired)
                        {
                        /* Set TCC of the allocated Param Set. */
                        gblRegs->PARAMENTRY[*pParam].OPT  &= EDMA3_RM_OPT_TCC_CLR_MASK;
                        gblRegs->PARAMENTRY[*pParam].OPT |= EDMA3_RM_OPT_TCC_SET_MASK(*pTcc);

                        /* Do the mapping between QDMA channel and PaRAM Set. */
                        /* Map Parameter RAM Set Number for specified channelId */
                        gblRegs->QCHMAP[chObj->resId]
                                        &= EDMA3_RM_QCH_PARAM_CLR_MASK;
                        gblRegs->QCHMAP[chObj->resId]
                                        |= EDMA3_RM_QCH_PARAM_SET_MASK(*pParam);

                        /* Set the Trigger Word */
                        gblRegs->QCHMAP[chObj->resId]
                                        &= EDMA3_RM_QCH_TRWORD_CLR_MASK;
                        gblRegs->QCHMAP[chObj->resId]
                                        |= EDMA3_RM_QCH_TRWORD_SET_MASK(EDMA3_RM_QDMA_TRIG_DEFAULT);

                        /* Remove any linking */
                        *((&gblRegs->PARAMENTRY[*pParam].OPT)
                                + (unsigned int)EDMA3_RM_PARAM_ENTRY_LINK_BCNTRLD) = 0xFFFFu;

                        /* Enable the transfer also. */
                        rmInstance->shadowRegs->QEESR = (1u << chObj->resId);
                        }
                    }
                else
                    {
                    /**
                     * TCC allocation failed, free the previously allocated
                     * PaRAM Set and QDMA channel.
                     */
                    resObj.type = EDMA3_RM_RES_PARAM_SET;
                    resObj.resId = *pParam;
                    EDMA3_RM_freeResource(hEdmaResMgr, (EDMA3_RM_ResDesc *)&resObj);

                    /* Reset the book-keeping data structure also. */
                    edma3RmChBoundRes[rmObj->phyCtrllerInstId][qdmaChId].paRAMId = -1;

                    resObj.type = chObj->type;
                    resObj.resId = chObj->resId;
                    EDMA3_RM_freeResource(hEdmaResMgr, (EDMA3_RM_ResDesc *)&resObj);
                    }
                }
            else
                {
                /**
                 * PaRAM Set allocation failed, free the previously allocated
                 * QDMA channel also.
                 */
                resObj.type = chObj->type;
                resObj.resId = chObj->resId;
                EDMA3_RM_freeResource(hEdmaResMgr, (EDMA3_RM_ResDesc *)&resObj);
                }
            }
        }


#ifdef EDMA3_INSTRUMENTATION_ENABLED
    EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3",
                EDMA3_DVT_DESC(EDMA3_DVT_eFUNC_END,
                EDMA3_DVT_dCOUNTER,
                EDMA3_DVT_dNONE,
                EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */


    return result;
    }


/** \fn     EDMA3_RM_Result EDMA3_RM_freeLogicalChannel (EDMA3_RM_Handle
 *                       hEdmaResMgr, EDMA3_RM_ResDesc *lChObj)
 *  \brief  This API is used to free the specified channel (DMA/QDMA/Link) and
 *          its associated resources (PaRAM Set, TCC etc).
 *
 * This API internally uses EDMA3_RM_freeResource () to free the desired
 * resources.
 *
 * For DMA/QDMA channels, it also clears the DCHMAP/QCHMAP registers
 *
 * \param  hEdmaResMgr      [IN]    Handle to the previously opened Resource
 *                                  Manager Instance.
 * \param  lChObj           [IN]    Handle to the logical channel object,
 *                                  which needs to be freed
 *
 * \return  EDMA3_RM_SOK or EDMA_RM Error Code
 *
 * \note    This is a re-entrant function which internally calls
 *          EDMA3_RM_freeResource () for resource de-allocation.
 */
EDMA3_RM_Result EDMA3_RM_freeLogicalChannel (EDMA3_RM_Handle hEdmaResMgr,
                                                EDMA3_RM_ResDesc *lChObj)
    {
    EDMA3_RM_ResDesc *chObj = NULL;
    EDMA3_RM_ResDesc resObj;
    EDMA3_RM_Result result = EDMA3_RM_SOK;
    EDMA3_RM_Instance *rmInstance = NULL;
    EDMA3_RM_Obj *rmObj = NULL;
    int paRAMId;
    unsigned int tcc;
    volatile EDMA3_CCRL_Regs *globalRegs = NULL;
    unsigned int qdmaChId;
    unsigned int dmaChId;
    EDMA3_RM_InstanceInitConfig *rmConfig = NULL;

#ifdef EDMA3_INSTRUMENTATION_ENABLED
    EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3",
    EDMA3_DVT_DESC(EDMA3_DVT_eFUNC_START,
    EDMA3_DVT_dCOUNTER,
    EDMA3_DVT_dNONE,
    EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */


	/* If parameter checking is enabled... */
#ifndef EDMA3_RM_PARAM_CHECK_DISABLE
    if ((lChObj == NULL) || (hEdmaResMgr == NULL))
        {
        result = (EDMA3_RM_E_INVALID_PARAM);
        }
#endif

	/* Check if the parameters are OK. */
    if (result == EDMA3_RM_SOK)
        {
        chObj = lChObj;

        rmInstance = (EDMA3_RM_Instance *)hEdmaResMgr;

        if (rmInstance == NULL)
            {
            result = EDMA3_RM_E_INVALID_PARAM;
            }
        }

    if (result == EDMA3_RM_SOK)
        {
        rmConfig = rmInstance->initParam.rmInstInitConfig;
        rmObj = rmInstance->pResMgrObjHandle;

        if (rmObj == NULL)
            {
            result = EDMA3_RM_E_INVALID_PARAM;
            }
        else
            {
            if (rmObj->gblCfgParams.globalRegs == NULL)
                {
                result = EDMA3_RM_E_INVALID_PARAM;
                }
            else
                {
                globalRegs = (volatile EDMA3_CCRL_Regs *)(rmObj->gblCfgParams.globalRegs);
                }
            }
        }


    if (result == EDMA3_RM_SOK)
        {
        switch (chObj->type)
            {
            case EDMA3_RM_RES_DMA_CHANNEL:
                {
                /* Save the DMA channel first. */
                dmaChId = chObj->resId;

                /**
                 * Validate DMA channel id first.
                 * It should be a valid channel id.
                 */
                if (dmaChId >=  EDMA3_MAX_DMA_CH)
                    {
                    result = EDMA3_RM_E_INVALID_PARAM;
                    }

                /* It should be owned and allocated by this RM only. */
                if (result == EDMA3_RM_SOK)
                    {
                    if (((rmConfig->ownDmaChannels[dmaChId/32u])
                          &
                          (~(rmInstance->avlblDmaChannels[dmaChId/32u]))
                          &
                          (1u << (dmaChId%32u))) != FALSE)
                        {
                        /** Perfectly valid channel id.
                         * Clear some channel specific registers, if it is permitted.
                         */
                        if (TRUE == rmInstance->regModificationRequired)
                            {
                            if (dmaChId < 32u)
                                {
                                if((rmInstance->shadowRegs->SER & (1u<<dmaChId))!=FALSE)
                                    {
                                    rmInstance->shadowRegs->SECR = (1u<<dmaChId);
                                    }
                                if((globalRegs->EMR & (1u<<dmaChId))!=FALSE)
                                    {
                                    globalRegs->EMCR = (1u<<dmaChId);
                                    }
                                }
                            else
                                {
                                if((rmInstance->shadowRegs->SERH & (1u<<(dmaChId-32u)))!=FALSE)
                                    {
                                    rmInstance->shadowRegs->SECRH = (1u<<(dmaChId-32u));
                                    }
                                if((globalRegs->EMRH & (1u<<(dmaChId-32u)))!=FALSE)
                                    {
                                    globalRegs->EMCRH = (1u<<(dmaChId-32u));
                                    }
                                }

                            /* Clear DCHMAP register also. */
                            if (TRUE == rmObj->gblCfgParams.dmaChPaRAMMapExists)
                                {
                                globalRegs->DCHMAP[dmaChId] &=
                                                        EDMA3_RM_DCH_PARAM_CLR_MASK;
                                }
                            }

                        /* Free the PaRAM Set Now. */
                        paRAMId = edma3RmChBoundRes[rmObj->phyCtrllerInstId][dmaChId].paRAMId;
                        resObj.type = EDMA3_RM_RES_PARAM_SET;
                        resObj.resId = (unsigned int)paRAMId;
                        result = EDMA3_RM_freeResource(hEdmaResMgr, (EDMA3_RM_ResDesc *)&resObj);
                        }
                    else
                        {
                        /* Channel id has some problem. */
                        result = EDMA3_RM_E_INVALID_PARAM;
                        }
                    }


                if (result == EDMA3_RM_SOK)
                    {
                    /* PaRAM Set Freed */
                    edma3RmChBoundRes[rmObj->phyCtrllerInstId][dmaChId].paRAMId = -1;

                    /* Free the TCC */
                    tcc = edma3RmChBoundRes[rmObj->phyCtrllerInstId][dmaChId].tcc;
                    resObj.type = EDMA3_RM_RES_TCC;
                    resObj.resId = tcc;
                    result = EDMA3_RM_freeResource(hEdmaResMgr, (EDMA3_RM_ResDesc *)&resObj);
                    }

                if (result == EDMA3_RM_SOK)
                    {
                    /* TCC Freed */
                    edma3RmChBoundRes[rmObj->phyCtrllerInstId][dmaChId].tcc = EDMA3_MAX_TCC;

                    /**
                     * Try to free the DMA Channel now. DMA Channel should
                     * be freed only in the end because while freeing, DRAE
                     * registers will be RESET.
                     * After that, no shadow region specific DMA channel
                     * register can be modified. So reset that DRAE register
                     * ONLY in the end.
                     */
                    resObj.type = EDMA3_RM_RES_DMA_CHANNEL;
                    resObj.resId = dmaChId;
                    result = EDMA3_RM_freeResource(hEdmaResMgr,
                                            (EDMA3_RM_ResDesc *)&resObj);
                    }
                }
                break;


            case EDMA3_RM_RES_QDMA_CHANNEL:
                {
                /**
                 * Calculate QDMA Logical Channel Id first.
                 * User has given the actual QDMA channel id.
                 * So we have to convert it to make the logical
                 * QDMA channel id first.
                 */
                qdmaChId = chObj->resId + EDMA3_RM_QDMA_CH_MIN_VAL;

                /**
                 * Validate QDMA channel id first.
                 * It should be a valid channel id.
                 */
                if (chObj->resId >=  EDMA3_MAX_QDMA_CH)
                    {
                    result = EDMA3_RM_E_INVALID_PARAM;
                    }

                /* It should be owned and allocated by this RM only. */
                if (result == EDMA3_RM_SOK)
                    {
                    if (((rmConfig->ownQdmaChannels[0u])
                          &
                          (~(rmInstance->avlblQdmaChannels[0u]))
                          &
                          (1u << chObj->resId)) != FALSE)
                        {
                        /** Perfectly valid channel id.
                         * Clear some channel specific registers, if
                         * it is permitted.
                         */
                        if (TRUE == rmInstance->regModificationRequired)
                            {
                            rmInstance->shadowRegs->QEECR = (1u<<chObj->resId);

                            if((globalRegs->QEMR & (1u<<chObj->resId))!=FALSE)
                                {
                                globalRegs->QEMCR = (1u<<chObj->resId);
                                }

                            /* Unmap PARAM Set Number for specified channelId */
                            globalRegs->QCHMAP[chObj->resId] &=
                                                        EDMA3_RM_QCH_PARAM_CLR_MASK;

                            /* Reset the Trigger Word */
                            globalRegs->QCHMAP[chObj->resId] &=
                                                        EDMA3_RM_QCH_TRWORD_CLR_MASK;
                            }

                        /* Free the PaRAM Set now */
                        paRAMId = edma3RmChBoundRes[rmObj->phyCtrllerInstId][qdmaChId].paRAMId;
                        resObj.type = EDMA3_RM_RES_PARAM_SET;
                        resObj.resId = (int)paRAMId;
                        result = EDMA3_RM_freeResource(hEdmaResMgr, (EDMA3_RM_ResDesc *)&resObj);
                        }
                    else
                        {
                        /* Channel id has some problem. */
                        result = EDMA3_RM_E_INVALID_PARAM;
                        }
                    }


                if (result == EDMA3_RM_SOK)
                    {
                    /* PaRAM Set Freed */
                    edma3RmChBoundRes[rmObj->phyCtrllerInstId][qdmaChId].paRAMId = -1;

                    /* Free the TCC */
                    tcc = edma3RmChBoundRes[rmObj->phyCtrllerInstId][qdmaChId].tcc;
                    resObj.type = EDMA3_RM_RES_TCC;
                    resObj.resId = tcc;
                    result = EDMA3_RM_freeResource(hEdmaResMgr, (EDMA3_RM_ResDesc *)&resObj);
                    }

                if (result == EDMA3_RM_SOK)
                    {
                    /* TCC Freed */
                    edma3RmChBoundRes[rmObj->phyCtrllerInstId][qdmaChId].tcc = EDMA3_MAX_TCC;

                    /**
                     * Try to free the QDMA Channel now. QDMA Channel should
                     * be freed only in the end because while freeing, QRAE
                     * registers will be RESET.
                     * After that, no shadow region specific QDMA channel
                     * register can be modified. So reset that QDRAE register
                     * ONLY in the end.
                     */
                    resObj.type = EDMA3_RM_RES_QDMA_CHANNEL;
                    resObj.resId = chObj->resId;
                    result = EDMA3_RM_freeResource(hEdmaResMgr,
                                            (EDMA3_RM_ResDesc *)&resObj);
                    }
                }
                break;


            case EDMA3_RM_RES_PARAM_SET:
                {
                /* Link Channel */
                if (chObj->resId < edma3NumPaRAMSets)
                    {
                    resObj.type = EDMA3_RM_RES_PARAM_SET;
                    resObj.resId = chObj->resId;

                    result = EDMA3_RM_freeResource(hEdmaResMgr, (EDMA3_RM_ResDesc *)&resObj);
                    if (result == EDMA3_RM_SOK)
                        {
                        /* PaRAM Set freed successfully. */
                        unsigned int linkCh = EDMA3_RM_LINK_CH_MIN_VAL;

                        /* Reset the Logical-Link channel */
                        /* Search for the Logical-Link channel first */
                        for (linkCh = EDMA3_RM_LINK_CH_MIN_VAL;
                                linkCh < EDMA3_RM_LINK_CH_MAX_VAL;
                                linkCh++)
                            {
                            if (edma3RmChBoundRes[rmObj->phyCtrllerInstId][linkCh].paRAMId == (int)(chObj->resId))
                                {
                                edma3RmChBoundRes[rmObj->phyCtrllerInstId][linkCh].paRAMId = -1;
                                break;
                                }
                            }
                        }
                    }
                else
                    {
                    result = EDMA3_RM_E_INVALID_PARAM;
                    }
                }
                break;

            default:
                result = EDMA3_RM_E_INVALID_PARAM;
            }
        }


#ifdef EDMA3_INSTRUMENTATION_ENABLED
    EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3",
    EDMA3_DVT_DESC(EDMA3_DVT_eFUNC_END,
    EDMA3_DVT_dCOUNTER,
    EDMA3_DVT_dNONE,
    EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */

    return result;
    }



/**\fn      EDMA3_RM_Result EDMA3_RM_mapEdmaChannel (EDMA3_RM_Handle
 *                   hEdmaResMgr, unsigned int channelId, unsigned int paRAMId)
 * \brief   Bind the resources DMA Channel and PaRAM Set. Both the DMA channel
 * and the PaRAM set should be previously allocated. If they are not,
 * this API will result in error.
 *
 * This API sets the DCHMAP register for a specific DMA channel. This register
 * is used to specify the PaRAM Set associated with that particular DMA Channel.
 *
 * \param   hEdmaResMgr         [IN]    Handle to the previously opened Resource
 *                                      Manager Instance.
 * \param   channelId           [IN]    Previously allocated DMA Channel on which
 *                                      Transfer will occur.
 * \param   paRAMId             [IN]    Previously allocated PaRAM Set which
 *                                      needs to be associated with the dma channel.
 *
 * \return  EDMA3_RM_SOK or EDMA_RM Error Code
 *
 * \note    This API is useful only for the EDMA3 Controllers which have a
 *          register for mapping a DMA Channel to a particular PaRAM Set
 *          (DCHMAP register).
 *          On platforms where this feature is not supported, this API
 *          returns error code: EDMA3_RM_E_FEATURE_UNSUPPORTED.
 *          This function is re-entrant for unique channelId. It is
 *          non-re-entrant for same channelId values.
 */
EDMA3_RM_Result EDMA3_RM_mapEdmaChannel (EDMA3_RM_Handle hEdmaResMgr,
                                 unsigned int channelId,
                                 unsigned int paRAMId)
    {
    EDMA3_RM_Instance *rmInstance = NULL;
    EDMA3_RM_Obj *rmObj = NULL;
    EDMA3_RM_Result result = EDMA3_RM_SOK;
    volatile EDMA3_CCRL_Regs *gblRegs = NULL;

#ifdef EDMA3_INSTRUMENTATION_ENABLED
    EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3",
                EDMA3_DVT_DESC(EDMA3_DVT_eFUNC_START,
                EDMA3_DVT_dCOUNTER,
                EDMA3_DVT_dNONE,
                EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */

	/* If parameter checking is enabled... */
#ifndef EDMA3_RM_PARAM_CHECK_DISABLE
    if (hEdmaResMgr == NULL)
        {
        result = EDMA3_RM_E_INVALID_PARAM;
        }
#endif

    if (result == EDMA3_RM_SOK)
        {
        rmInstance = (EDMA3_RM_Instance *)hEdmaResMgr;

        if (rmInstance == NULL)
            {
            result = EDMA3_RM_E_INVALID_PARAM;
            }
        }

    if (result == EDMA3_RM_SOK)
        {
        rmObj = rmInstance->pResMgrObjHandle;

        if (rmObj == NULL)
            {
            result = EDMA3_RM_E_INVALID_PARAM;
            }
        else
            {
            if (rmObj->gblCfgParams.globalRegs == NULL)
                {
                result = EDMA3_RM_E_INVALID_PARAM;
                }
            }
        }

    if (result == EDMA3_RM_SOK)
        {
        gblRegs = (volatile EDMA3_CCRL_Regs *)(rmObj->gblCfgParams.globalRegs);

		/* If parameter checking is enabled... */
#ifndef EDMA3_RM_PARAM_CHECK_DISABLE
        if ((paRAMId >= rmObj->gblCfgParams.numPaRAMSets)
            || (channelId >= rmObj->gblCfgParams.numDmaChannels))
            {
            result = EDMA3_RM_E_INVALID_PARAM;
            }
#endif
        }

    /* DMA channel and PaRAM Set should be previously allocated. */
    if (result == EDMA3_RM_SOK)
        {
        if (((rmInstance->initParam.rmInstInitConfig->ownDmaChannels[channelId/32u])
                                                  &
                                                  (~(rmInstance->avlblDmaChannels[channelId/32u]))
                                                  &
                                                  (1u << (channelId%32u))) != FALSE)
            {
            /* DMA channel allocated, check for the PaRAM Set */
            if (((rmInstance->initParam.rmInstInitConfig->ownPaRAMSets[paRAMId/32u])
                                          &
                                          (~(rmInstance->avlblPaRAMSets[paRAMId/32u]))
                                          &
                                          (1u << (paRAMId%32u))) == FALSE)
                {
                /* PaRAM Set NOT allocated, return error */
                result = EDMA3_RM_E_RES_NOT_ALLOCATED;
                }
            }
        else
            {
            /* DMA channel NOT allocated, return error */
            result = EDMA3_RM_E_RES_NOT_ALLOCATED;
            }
        }


    if (result == EDMA3_RM_SOK)
        {
        /* Map the Dma Channel to the PaRAM Set corresponding to paramId */
        /**
          * Do this for the EDMA3 Controllers which have a register for mapping
          * DMA Channel to a particular PaRAM Set. So check
          * dmaChPaRAMMapExists first.
          */
        if (TRUE == rmObj->gblCfgParams.dmaChPaRAMMapExists)
            {
            /* Map Parameter RAM Set Number for specified channelId */
            gblRegs->DCHMAP[channelId] &= EDMA3_RM_DCH_PARAM_CLR_MASK;
            gblRegs->DCHMAP[channelId] |= EDMA3_RM_DCH_PARAM_SET_MASK(paRAMId);
            }
        else
            {
            /* Feature NOT supported on the current platform, return error. */
            result = EDMA3_RM_E_FEATURE_UNSUPPORTED;
            }
        }

#ifdef EDMA3_INSTRUMENTATION_ENABLED
    EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3",
                EDMA3_DVT_DESC(EDMA3_DVT_eFUNC_END,
                EDMA3_DVT_dCOUNTER,
                EDMA3_DVT_dNONE,
                EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */

    return result;
    }




/**\fn      EDMA3_RM_Result EDMA3_RM_mapQdmaChannel (EDMA3_RM_Handle
 *                       hEdmaResMgr, unsigned int channelId,
 *                       unsigned int paRAMId,
 *                       EDMA3_RM_QdmaTrigWord trigWord)
 * \brief   Bind the resources QDMA Channel and PaRAM Set. Also, Set the
 * trigger word for the QDMA channel. Both the QDMA channel and the PaRAM set
 * should be previously allocated. If they are not, this API will result in error.
 *
 * This API sets the QCHMAP register for a specific QDMA channel. This register
 * is used to specify the PaRAM Set associated with that particular QDMA
 * Channel along with the trigger word.
 *
 * \param   hEdmaResMgr     [IN]    Handle to the previously opened Resource
 *                                  Manager Instance.
 * \param   channelId       [IN]    Previously allocated  QDMA Channel on which
 *                                  Transfer will occur.
 * \param   paRAMId         [IN]    Previously allocated PaRAM Set, which needs to
 *                                      be associated with channelId
 * \param   trigWord        [IN]    The Trigger Word for the channel.
 *                                  Trigger Word is the word in the PaRAM
 *                                  Register Set which - when written to by CPU
 *                                  -will start the QDMA transfer automatically
 *
 * \return  EDMA3_RM_SOK or EDMA3_RM Error Code
 *
 * \note    This function is re-entrant for unique channelId. It is non-re-entrant
 *          for same channelId values.
 */
EDMA3_RM_Result EDMA3_RM_mapQdmaChannel (EDMA3_RM_Handle hEdmaResMgr,
                                 unsigned int channelId,
                                 unsigned int paRAMId,
                                 EDMA3_RM_QdmaTrigWord trigWord)
    {
    EDMA3_RM_Instance *rmInstance = NULL;
    EDMA3_RM_Obj *rmObj = NULL;
    EDMA3_RM_Result result = EDMA3_RM_SOK;
    volatile EDMA3_CCRL_Regs *gblRegs = NULL;

#ifdef EDMA3_INSTRUMENTATION_ENABLED
    EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3",
                EDMA3_DVT_DESC(EDMA3_DVT_eFUNC_START,
                EDMA3_DVT_dCOUNTER,
                EDMA3_DVT_dNONE,
                EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */

	/* If parameter checking is enabled... */
#ifndef EDMA3_RM_PARAM_CHECK_DISABLE
    if ((hEdmaResMgr == NULL)
        || ((trigWord < EDMA3_RM_QDMA_TRIG_OPT)
        || (trigWord > EDMA3_RM_QDMA_TRIG_CCNT)))
        {
        result = EDMA3_RM_E_INVALID_PARAM;
        }
#endif

    if (result == EDMA3_RM_SOK)
        {
        rmInstance = (EDMA3_RM_Instance *)hEdmaResMgr;

        if (rmInstance == NULL)
            {
            result = EDMA3_RM_E_INVALID_PARAM;
            }
        }

    if (result == EDMA3_RM_SOK)
        {
        rmObj = rmInstance->pResMgrObjHandle;

        if (rmObj == NULL)
            {
            result = EDMA3_RM_E_INVALID_PARAM;
            }
        else
            {
            if (rmObj->gblCfgParams.globalRegs == NULL)
                {
                result = EDMA3_RM_E_INVALID_PARAM;
                }
            }
        }

    if (result == EDMA3_RM_SOK)
        {
        gblRegs = (volatile EDMA3_CCRL_Regs *)(rmObj->gblCfgParams.globalRegs);

		/* If parameter checking is enabled... */
#ifndef EDMA3_RM_PARAM_CHECK_DISABLE
        if ((paRAMId >= rmObj->gblCfgParams.numPaRAMSets)
            || (channelId >= rmObj->gblCfgParams.numQdmaChannels))
            {
            result = EDMA3_RM_E_INVALID_PARAM;
            }
#endif
        }

    /* QDMA channel and PaRAM Set should be previously allocated. */
    if (result == EDMA3_RM_SOK)
        {
        if (((rmInstance->initParam.rmInstInitConfig->ownQdmaChannels[channelId/32u])
                                                  &
                                                  (~(rmInstance->avlblQdmaChannels[channelId/32u]))
                                                  &
                                                  (1u << (channelId%32u))) != FALSE)
            {
            /* QDMA channel allocated, check for the PaRAM Set */
            if (((rmInstance->initParam.rmInstInitConfig->ownPaRAMSets[paRAMId/32u])
                                          &
                                          (~(rmInstance->avlblPaRAMSets[paRAMId/32u]))
                                          &
                                          (1u << (paRAMId%32u))) == FALSE)
                {
                /* PaRAM Set NOT allocated, return error */
                result = EDMA3_RM_E_RES_NOT_ALLOCATED;
                }
            }
        else
            {
            /* QDMA channel NOT allocated, return error */
            result = EDMA3_RM_E_RES_NOT_ALLOCATED;
            }
        }

    if (result == EDMA3_RM_SOK)
        {
        /* Map Parameter RAM Set Number for specified channelId */
        gblRegs->QCHMAP[channelId] &= EDMA3_RM_QCH_PARAM_CLR_MASK;
        gblRegs->QCHMAP[channelId] |= EDMA3_RM_QCH_PARAM_SET_MASK(paRAMId);

        /* Set the Trigger Word */
        gblRegs->QCHMAP[channelId] &= EDMA3_RM_QCH_TRWORD_CLR_MASK;
        gblRegs->QCHMAP[channelId] |= EDMA3_RM_QCH_TRWORD_SET_MASK(trigWord);
        }


#ifdef EDMA3_INSTRUMENTATION_ENABLED
    EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3",
                EDMA3_DVT_DESC(EDMA3_DVT_eFUNC_END,
                EDMA3_DVT_dCOUNTER,
                EDMA3_DVT_dNONE,
                EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */

    return result;
    }



/**
 * \fn  EDMA3_RM_Result EDMA3_RM_registerTccCb(EDMA3_RM_Handle hEdmaResMgr,
 *      const EDMA3_RM_ResDesc *channelObj, unsigned int tcc,
 *      EDMA3_RM_TccCallback tccCb, void *cbData);
 * \brief   Register Interrupt / Completion Handler for a given TCC.
 *
 * This function enables the interrupts in IESR/IESRH, only if the callback
 * function provided by the user is NON-NULL. Moreover, if a call-back function
 * is already registered against that TCC, the API fails with the error code
 * EDMA3_RM_E_CALLBACK_ALREADY_REGISTERED. For a NULL callback function,
 * this API returns error.
 *
 * \param   hEdmaResMgr         [IN]    Handle to the previously opened
 *                                      EDMA3 Resource Manager Instance
 * \param   channelObj          [IN]    Channel ID and type (DMA or QDMA
 *                                      Channel), allocated earlier, and
 *                                      corresponding to which a callback
 *                                      function needs to be registered
 *                                      against the associated TCC.
 * \param   tcc                 [IN]    TCC against which the handler needs to
 *                                      be registered.
 * \param   tccCb               [IN]    The Callback function to be registered
 *                                      against the TCC.
 * \param   cbData              [IN]    Callback data to be passed while calling
 *                                      the callback function.
 *
 * \return  EDMA3_RM_SOK or EDMA3_RM Error Code
 *
 * \note    This function is re-entrant for unique tcc values. It is non-
 *          re-entrant for same tcc value.
 */
EDMA3_RM_Result EDMA3_RM_registerTccCb(EDMA3_RM_Handle hEdmaResMgr,
                    const EDMA3_RM_ResDesc *channelObj,
                    unsigned int tcc,
                    EDMA3_RM_TccCallback tccCb,
                    void *cbData)
    {
    EDMA3_RM_Instance *rmInstance = NULL;
    EDMA3_RM_Obj *rmObj = NULL;
    EDMA3_RM_Result result = EDMA3_RM_SOK;

#ifdef EDMA3_INSTRUMENTATION_ENABLED
    EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3",
                EDMA3_DVT_DESC(EDMA3_DVT_eFUNC_START,
                EDMA3_DVT_dCOUNTER,
                EDMA3_DVT_dNONE,
                EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */

	/* If parameter checking is enabled... */
#ifndef EDMA3_RM_PARAM_CHECK_DISABLE
    if ((NULL == hEdmaResMgr) || (NULL == channelObj))
        {
        result = EDMA3_RM_E_INVALID_PARAM;
        }

    /* Callback function should NOT be NULL */
    if (NULL == tccCb)
        {
        result = EDMA3_RM_E_INVALID_PARAM;
        }
#endif

    if (result == EDMA3_RM_SOK)
        {
        rmInstance = (EDMA3_RM_Instance *)hEdmaResMgr;
        rmObj = rmInstance->pResMgrObjHandle;

        if (rmObj == NULL)
            {
            result = EDMA3_RM_E_INVALID_PARAM;
            }

		/* If parameter checking is enabled... */
#ifndef EDMA3_RM_PARAM_CHECK_DISABLE
        if ((EDMA3_RM_SOK == result) && (tcc >= rmObj->gblCfgParams.numTccs))
            {
            result = EDMA3_RM_E_INVALID_PARAM;
            }
#endif

		/* Check if the parameters are OK. */
		if (EDMA3_RM_SOK == result)
            {
            /* Check whether the callback has already registered. */
            if (NULL != edma3IntrParams[rmObj->phyCtrllerInstId][tcc].tccCb)
                {
                result = EDMA3_RM_E_CALLBACK_ALREADY_REGISTERED;
                }
            else
                {
                /* Store the mapping b/w DMA/QDMA channel and TCC first. */
                if (channelObj->type == EDMA3_RM_RES_DMA_CHANNEL)
                    {
                    /* DMA channel */
                    if (channelObj->resId < rmObj->gblCfgParams.numDmaChannels)
                        {
                        /* Save the TCC */
                        edma3DmaChTccMapping[rmObj->phyCtrllerInstId][channelObj->resId] = tcc;
                        }
                    else
                        {
                        /* Error!!! */
                        result = EDMA3_RM_E_INVALID_PARAM;
                        }
                    }
                else
                    {
                    if (channelObj->type == EDMA3_RM_RES_QDMA_CHANNEL)
                        {
                        /* QDMA channel */
                        if (channelObj->resId < rmObj->gblCfgParams.numQdmaChannels)
                            {
                            /* Save the TCC */
                            edma3QdmaChTccMapping[rmObj->phyCtrllerInstId][channelObj->resId] = tcc;
                            }
                        else
                            {
                            /* Error!!! */
                            result = EDMA3_RM_E_INVALID_PARAM;
                            }
                        }
                    else
                        {
                        /* Error!!! */
                        result = EDMA3_RM_E_INVALID_PARAM;
                        }
                    }
                }

            if (EDMA3_RM_SOK == result)
                {
                /**
                 * Enable the interrupts in IESR/IESRH, only if the Callback
                 * function is NOT NULL.
                 */
                if (tcc < 32u)
                    {
                    rmInstance->shadowRegs->IESR = (1UL << tcc);
                    }
                else
                    {
                    rmInstance->shadowRegs->IESRH = (1UL << (tcc-32u));
                    }

                /* Save the callback functions also */
                edma3IntrParams[rmObj->phyCtrllerInstId][tcc].cbData = cbData;
                edma3IntrParams[rmObj->phyCtrllerInstId][tcc].tccCb = tccCb;
                }
            }
        }

#ifdef EDMA3_INSTRUMENTATION_ENABLED
    EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3",
                EDMA3_DVT_DESC(EDMA3_DVT_eFUNC_END,
                EDMA3_DVT_dCOUNTER,
                EDMA3_DVT_dNONE,
                EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */

    return result;
    }




/**
 * \fn      EDMA3_RM_Result EDMA3_RM_unregisterTccCb(EDMA3_RM_Handle
 *          hEdmaResMgr, const EDMA3_RM_ResDesc *channelObj);
 * \brief   Unregister the previously registered callback function against a
 *          DMA/QDMA channel.
 *
 * This function unregisters the previously registered callback function against
 * a DMA/QDMA channel by removing any stored callback function. Moreover, it
 * clears the interrupt enable register (IESR/IESRH) by writing to the IECR/
 * IECRH register, for the TCC associated with that particular channel.
 *
 * \param   hEdmaResMgr         [IN]    Handle to the previously opened
 *                                      EDMA3 Resource Manager Instance
 * \param   channelObj          [IN]    Channel ID and type, allocated earlier
 *                                      (DMA or QDMA Channel ONLY), and
 *                                      corresponding to which a TCC is there.
 *                                      Against that TCC, the callback needs
 *                                      to be un-registered.
 *
 * \return  EDMA3_RM_SOK or EDMA3_RM Error Code.
 *
 * \note    This function is re-entrant for unique (channelObj->type +
 *          channelObj->resId) combination. It is non-re-entrant for same
 *          channelObj Resource.
 */
EDMA3_RM_Result EDMA3_RM_unregisterTccCb(EDMA3_RM_Handle hEdmaResMgr,
                    const EDMA3_RM_ResDesc *channelObj)
    {
    EDMA3_RM_Instance *rmInstance = NULL;
    EDMA3_RM_Obj *rmObj = NULL;
    EDMA3_RM_Result result = EDMA3_RM_SOK;
    unsigned int mappedTcc = EDMA3_MAX_TCC;

#ifdef EDMA3_INSTRUMENTATION_ENABLED
    EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3",
                EDMA3_DVT_DESC(EDMA3_DVT_eFUNC_START,
                EDMA3_DVT_dCOUNTER,
                EDMA3_DVT_dNONE,
                EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */

	/* If parameter checking is enabled... */
#ifndef EDMA3_RM_PARAM_CHECK_DISABLE
    if ((NULL == hEdmaResMgr) || (NULL == channelObj))
        {
        result = EDMA3_RM_E_INVALID_PARAM;
        }
#endif

	/* Check if the parameters are OK. */
	if (EDMA3_RM_SOK == result)
        {
        rmInstance = (EDMA3_RM_Instance *)hEdmaResMgr;
        rmObj = rmInstance->pResMgrObjHandle;

        if (rmObj == NULL)
            {
            result = EDMA3_RM_E_INVALID_PARAM;
            }
        else
            {
            if (channelObj->type == EDMA3_RM_RES_DMA_CHANNEL)
                {
                /* DMA channel */
                if (channelObj->resId < rmObj->gblCfgParams.numDmaChannels)
                    {
                    /* Save the mapped TCC */
                    mappedTcc = edma3DmaChTccMapping[rmObj->phyCtrllerInstId][channelObj->resId];

                    /* Remove the mapping now. */
                    edma3DmaChTccMapping[rmObj->phyCtrllerInstId][channelObj->resId] = EDMA3_MAX_TCC;
                    }
                else
                    {
                    /* Error!!! */
                    result = EDMA3_RM_E_INVALID_PARAM;
                    }
                }
            else
                {
                if (channelObj->type == EDMA3_RM_RES_QDMA_CHANNEL)
                    {
                    /* QDMA channel */
                    if (channelObj->resId < rmObj->gblCfgParams.numQdmaChannels)
                        {
                        /* Save the mapped TCC */
                        mappedTcc = edma3QdmaChTccMapping[rmObj->phyCtrllerInstId][channelObj->resId];

                        /* Remove the mapping now. */
                        edma3QdmaChTccMapping[rmObj->phyCtrllerInstId][channelObj->resId] = EDMA3_MAX_TCC;
                        }
                    else
                        {
                        /* Error!!! */
                        result = EDMA3_RM_E_INVALID_PARAM;
                        }
                    }
                else
                    {
                    /* Error!!! */
                    result = EDMA3_RM_E_INVALID_PARAM;
                    }
                }

            if (EDMA3_RM_SOK == result)
                {
                /* Remove the callback function too */
                if (mappedTcc < 32u)
                    {
                    rmInstance->shadowRegs->IECR = (1UL << mappedTcc);
                    }
                else
                    {
                    rmInstance->shadowRegs->IECRH = (1UL << (mappedTcc-32u));
                    }

                edma3IntrParams[rmObj->phyCtrllerInstId][mappedTcc].cbData = NULL;
                edma3IntrParams[rmObj->phyCtrllerInstId][mappedTcc].tccCb = NULL;
                }
            }
        }

#ifdef EDMA3_INSTRUMENTATION_ENABLED
    EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3",
                EDMA3_DVT_DESC(EDMA3_DVT_eFUNC_END,
                EDMA3_DVT_dCOUNTER,
                EDMA3_DVT_dNONE,
                EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */

    return result;
    }


/**\fn      EDMA3_RM_Result EDMA3_RM_allocContiguousResource(EDMA3_RM_Handle
 *                        hEdmaResMgr, EDMA3_RM_ResDesc *firstResIdObj,
 *                       unsigned int numResources)
 * \brief   Allocate a contiguous region of specified EDMA3 Resource
 * like DMA channel, QDMA channel, PaRAM Set or TCC.
 *
 * This API is used to allocate a contiguous region of specified EDMA3
 * Resources like DMA channel, QDMA channel, PaRAM Set or TCC.
 *
 * User can specify a particular resource Id to start with and go up to the
 * number of resources requested. The specific resource id to start from could
 * be passed in 'firstResIdObject->resId' and the number of resources requested
 * in 'numResources'.
 *
 * User can also request ANY available resource(s) of the type
 * 'firstResIdObject->type' by specifying 'firstResIdObject->resId' as
 * EDMA3_RM_RES_ANY.
 *
 * ANY types of resources are those resources when user doesn't care about the
 * actual resource allocated; user just wants a resource of the type specified.
 * One use-case is to perform memory-to-memory data transfer operation. This
 * operation can be performed using any available DMA or QDMA channel. User
 * doesn't need any specific channel for the same.
 *
 * To allocate specific contiguous resources, first this API checks whether
 * those requested resources are OWNED by the Resource Manager instance. Then
 * it checks the current availability of those resources.
 *
 * To allocate ANY available contiguous resources, this API tries to allocate
 * resources from the pool of (owned && non_reserved && available_right_now)
 * resources.
 *
 * After allocating DMA/QDMA channels or TCCs, the same resources are enabled in
 * the shadow region specific register (DRAE/DRAEH/QRAE). Allocated PaRAM Sets
 * are initialized to NULL before this API returns.
 *
 * \param   hEdmaResMgr         [IN]    Handle to the previously opened Resource
 *                                      Manager Instance.
 * \param   firstResIdObj       [IN]    Handle to the first resource descriptor
 *                                      object, which needs to be allocated.
 *                                      firstResIdObject->resId could be a valid
 *                                      resource id in case user wants to
 *                                      allocate specific resources OR it could
 *                                      be EDMA3_RM_RES_ANY in case user wants
 *                                      only the required number of resources
 *                                      and doesn't care about which resources
 *                                      were allocated.
 * \param   numResources        [IN]    Number of contiguous resources user
 *                                      wants to allocate.
 *
 * \return  EDMA3_RM_SOK or EDMA3_RM Error Code
 *
 * \note    This function acquires a RM Instance specific semaphore
 *          to prevent simultaneous access to the global pool of resources.
 *          It is re-entrant, but should not be called from the user callback
 *          function (ISR context).
 */
EDMA3_RM_Result EDMA3_RM_allocContiguousResource(EDMA3_RM_Handle hEdmaResMgr,
                                            EDMA3_RM_ResDesc *firstResIdObj,
                                            unsigned int numResources)
    {
    EDMA3_RM_Instance *rmInstance = NULL;
    EDMA3_RM_Obj *rmObj = NULL;
    EDMA3_RM_Result result = EDMA3_RM_SOK;
    EDMA3_RM_ResDesc *resObj = NULL;
    unsigned int resAllocIdx = 0u;
    unsigned int firstResId = 0u;
    unsigned int lastResId = 0u;
    unsigned int maxNumResources = 0u;
    EDMA3_RM_Result semResult = EDMA3_RM_SOK;
    unsigned int resIdClr = 0x0;
    unsigned int resIdSet = 0x0;
    volatile EDMA3_CCRL_Regs *gblRegs = NULL;
    unsigned int i = 0u;
    unsigned int position = 0u;


#ifdef EDMA3_INSTRUMENTATION_ENABLED
    EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3",
    EDMA3_DVT_DESC(EDMA3_DVT_eFUNC_START,
    EDMA3_DVT_dCOUNTER,
    EDMA3_DVT_dNONE,
    EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */


	/* If parameter checking is enabled... */
#ifndef EDMA3_RM_PARAM_CHECK_DISABLE
    if ((hEdmaResMgr == NULL) || (firstResIdObj == NULL))
        {
        result = EDMA3_RM_E_INVALID_PARAM;
        }
#endif

    if (EDMA3_RM_SOK == result)
        {
        rmInstance = (EDMA3_RM_Instance *)hEdmaResMgr;
        if (rmInstance == NULL)
            {
            result = EDMA3_RM_E_INVALID_PARAM;
            }
        }

    if (EDMA3_RM_SOK == result)
        {
        rmObj = rmInstance->pResMgrObjHandle;

        if (rmObj == NULL)
            {
            result = EDMA3_RM_E_INVALID_PARAM;
            }
        }

    if (EDMA3_RM_SOK == result)
        {
     gblRegs = (volatile EDMA3_CCRL_Regs *)(rmObj->gblCfgParams.globalRegs);

        if (rmInstance->initParam.rmSemHandle == NULL)
            {
            result = EDMA3_RM_E_INVALID_PARAM;
            }
        }

    if (EDMA3_RM_SOK == result)
        {
        resObj = firstResIdObj;
        if (resObj != NULL)
            {
            firstResId = resObj->resId;
            }

        switch (resObj->type)
            {
            case EDMA3_RM_RES_DMA_CHANNEL :
                maxNumResources = rmObj->gblCfgParams.numDmaChannels;
                break;
            case EDMA3_RM_RES_QDMA_CHANNEL :
                maxNumResources = rmObj->gblCfgParams.numQdmaChannels;
                break;
            case EDMA3_RM_RES_TCC :
                maxNumResources = rmObj->gblCfgParams.numTccs;
                break;
            case EDMA3_RM_RES_PARAM_SET :
                maxNumResources = rmObj->gblCfgParams.numPaRAMSets;
                break;
            default:
                result = EDMA3_RM_E_INVALID_PARAM;
                break;
            }
        }


    if (EDMA3_RM_SOK == result)
        {
        /* First resource id (firstResId) can be a valid Resource ID as well as
         * 'EDMA3_RM_RES_ANY', in case user does not want to
         * start from a specific resource. For eg, user allocating link channels.
         */
        if (firstResId != EDMA3_RM_RES_ANY)
            {
            /* User want specific resources. */
            lastResId = firstResId + numResources;

            if (((firstResId >= maxNumResources) || (firstResId > lastResId))
                || (lastResId > maxNumResources))
                {
                result = EDMA3_RM_E_INVALID_PARAM;
                }
            }
        else
            {
            /* (firstResId == EDMA3_RM_RES_ANY)
             * So just check whether the number of resources
             * requested does not cross the limit.
             */
            if (numResources > maxNumResources)
                {
                result = EDMA3_RM_E_INVALID_PARAM;
                }
            }
        }


    if (result == EDMA3_RM_SOK)
        {
        /* Now try to allocate resources for the first case */
        if (firstResId != EDMA3_RM_RES_ANY)
            {
            /* Request for specific resources */

            /**
              * Take the instance specific semaphore, to prevent simultaneous
              * access to the shared resources.
              */
            semResult = edma3OsSemTake(rmInstance->initParam.rmSemHandle,
                                    EDMA3_OSSEM_NO_TIMEOUT);

            if (EDMA3_RM_SOK == semResult)
                {
                switch (resObj->type)
                    {
                    case EDMA3_RM_RES_DMA_CHANNEL :
                            {
                            for (resAllocIdx = firstResId; resAllocIdx < lastResId; ++resAllocIdx)
                                {
                                resIdClr = (unsigned int)(~(1u << (resAllocIdx%32u)));
                                resIdSet = (1u << (resAllocIdx%32u));

                                /* Check whether it is owned or not */
                                if (((rmInstance->initParam.rmInstInitConfig->ownDmaChannels[resAllocIdx/32u])&(resIdSet)) != FALSE)
                                   {
                                    /* Now check if specified resource is available presently*/
                                    if (((rmInstance->avlblDmaChannels[resAllocIdx/32u])&(resIdSet)) != FALSE)
                                        {
                                        /*
                                         * Mark the specified resource as "Not Available"
                                         * for future requests
                                         */
                                        rmInstance->avlblDmaChannels[resAllocIdx/32u] &= resIdClr;

                                        if (resAllocIdx < 32u)
                                            {
                                            rmInstance->shadowRegs->EECR = (1UL << resAllocIdx);

                                            /**
                                             * Enable the DMA channel in the
                                             * DRAE registers also.
                                             */
                                            gblRegs->DRA[rmInstance->initParam.regionId].DRAE
                                                |= (0x1u << resAllocIdx);
                                            }
                                        else
                                            {
                                            rmInstance->shadowRegs->EECRH = (1UL << (resAllocIdx - 32u));

                                            /**
                                             * Enable the DMA channel in the
                                             * DRAEH registers also.
                                             */
                                            gblRegs->DRA[rmInstance->initParam.regionId].DRAEH
                                                |= (0x1u << (resAllocIdx - 32u));
                                            }

                                        result = EDMA3_RM_SOK;
                                        }
                                    else
                                        {
                                        /* Specified resource is owned but is already booked */
                                        result = EDMA3_RM_E_SPECIFIED_RES_NOT_AVAILABLE;
                                        break;
                                        }
                                    }
                                else
                                    {
                                    /*
                                     * Specified resource is not owned by this instance
                                     * of the Resource Manager
                                     */
                                    result = EDMA3_RM_E_RES_NOT_OWNED;
                                    break;
                                    }
                                }

                            break;
                            }

                    case EDMA3_RM_RES_QDMA_CHANNEL:
                            {
                            for (resAllocIdx = firstResId; resAllocIdx < lastResId; ++resAllocIdx)
                                {
                                resIdClr = (unsigned int)(~(1u << resAllocIdx));
                                resIdSet = (1u << resAllocIdx);

                                /* Check whether it is owned or not */
                                if (((rmInstance->initParam.rmInstInitConfig->ownQdmaChannels[0u])&(resIdSet))!=FALSE)
                                   {
                                    /* Now check if specified resource is available presently*/
                                    if (((rmInstance->avlblQdmaChannels[0u])&(resIdSet))!=FALSE)
                                        {
                                        /*
                                         * Mark the specified resource as "Not Available"
                                         * for future requests
                                         */
                                        rmInstance->avlblQdmaChannels[0u] &= resIdClr;

                                        /**
                                         * Enable the QDMA channel in the
                                         * QRAE register also.
                                         */
                                        gblRegs->QRAE[rmInstance->initParam.regionId]
                                            |= (0x1u << resAllocIdx);

                                        result = EDMA3_RM_SOK;
                                        }
                                    else
                                        {
                                        /* Specified resource is owned but is already booked */
                                        result = EDMA3_RM_E_SPECIFIED_RES_NOT_AVAILABLE;
                                        break;
                                        }
                                    }
                                else
                                    {
                                    /*
                                     * Specified resource is not owned by this instance
                                     * of the Resource Manager
                                     */
                                    result = EDMA3_RM_E_RES_NOT_OWNED;
                                    break;
                                    }
                                }

                            break;
                            }

                    case EDMA3_RM_RES_TCC:
                            {
                            for (resAllocIdx = firstResId; resAllocIdx < lastResId; ++resAllocIdx)
                                {
                                resIdClr = (unsigned int)(~(1u << (resAllocIdx%32u)));
                                resIdSet = (1u << (resAllocIdx%32u));

                                /* Check whether it is owned or not */
                                if (((rmInstance->initParam.rmInstInitConfig->ownTccs[resAllocIdx/32u])&(resIdSet))!=FALSE)
                                   {
                                    /* Now check if specified resource is available presently*/
                                    if (((rmInstance->avlblTccs[resAllocIdx/32u])&(resIdSet))!=FALSE)
                                        {
                                        /*
                                         * Mark the specified resource as "Not Available"
                                         * for future requests
                                         */
                                        rmInstance->avlblTccs[resAllocIdx/32u] &= resIdClr;

                                        /**
                                         * Enable the Interrupt channel in the
                                         * DRAE/DRAEH registers also.
                                         * Also, If the region id coming from this
                                         * RM instance is same as the Master RM
                                         * Instance's region id, only then we will be
                                         * getting the interrupts on the same side.
                                         * So save the TCC in the allocatedTCCs[] array.
                                         */
                                        if (resAllocIdx < 32u)
                                            {
                                            gblRegs->DRA[rmInstance->initParam.regionId].DRAE
                                                |= (0x1u << resAllocIdx);

                                            if (edma3RegionId == rmInstance->initParam.regionId)
                                                {
                                                allocatedTCCs[rmObj->phyCtrllerInstId][0u] |= (0x1u << resAllocIdx);
                                                }
                                            }
                                        else
                                            {
                                            gblRegs->DRA[rmInstance->initParam.regionId].DRAEH
                                                |= (0x1u << (resAllocIdx - 32u));

                                            if (edma3RegionId == rmInstance->initParam.regionId)
                                                {
                                                allocatedTCCs[rmObj->phyCtrllerInstId][1u] |= (0x1u << (resAllocIdx - 32u));
                                                }
                                            }

                                        result = EDMA3_RM_SOK;
                                        }
                                    else
                                        {
                                        /* Specified resource is owned but is already booked */
                                        result = EDMA3_RM_E_SPECIFIED_RES_NOT_AVAILABLE;
                                        break;
                                        }
                                    }
                                else
                                    {
                                    /*
                                     * Specified resource is not owned by this instance
                                     * of the Resource Manager
                                     */
                                    result = EDMA3_RM_E_RES_NOT_OWNED;
                                    break;
                                    }
                                }

                            break;
                            }

                    case EDMA3_RM_RES_PARAM_SET:
                            {
                            for (resAllocIdx = firstResId; resAllocIdx < lastResId; ++resAllocIdx)
                                {
                                resIdClr = (unsigned int)(~(1u << (resAllocIdx%32u)));
                                resIdSet = (1u << (resAllocIdx%32u));

                                /* Check whether it is owned or not */
                                if (((rmInstance->initParam.rmInstInitConfig->ownPaRAMSets[resAllocIdx/32u])&(resIdSet))!=FALSE)
                                   {
                                    /* Now check if specified resource is available presently*/
                                    if (((rmInstance->avlblPaRAMSets[resAllocIdx/32u])&(resIdSet))!=FALSE)
                                        {
                                        /*
                                         * Mark the specified resource as "Not Available"
                                         * for future requests
                                         */
                                        rmInstance->avlblPaRAMSets[resAllocIdx/32u] &= resIdClr;

                                        /**
                                         * Also, make the actual PARAM Set NULL, checking the flag
                                         * whether it is required or not.
                                         */
                                        if (TRUE == rmInstance->paramInitRequired)
                                            {
                                            edma3MemSet((void *)(&gblRegs->PARAMENTRY[resAllocIdx]),
                                                        0x00u,
                                                        sizeof(gblRegs->PARAMENTRY[resAllocIdx]));
                                            }

                                        result = EDMA3_RM_SOK;
                                        }
                                    else
                                        {
                                        /* Specified resource is owned but is already booked */
                                        result = EDMA3_RM_E_SPECIFIED_RES_NOT_AVAILABLE;
                                        break;
                                        }
                                    }
                                else
                                    {
                                    /*
                                     * Specified resource is not owned by this instance
                                     * of the Resource Manager
                                     */
                                    result = EDMA3_RM_E_RES_NOT_OWNED;
                                    break;
                                    }
                                }

                            break;
                            }

                    default:
                            {
                            result = EDMA3_RM_E_INVALID_PARAM;
                            break;
                            }
                    }

                /* resource allocation completed, release the semaphore first */
                semResult = edma3OsSemGive(rmInstance->initParam.rmSemHandle);
                }

            }
        else
            {
            /* (firstResId == EDMA3_RM_RES_ANY) */
            /**
            * Take the instance specific semaphore, to prevent simultaneous
            * access to the shared resources.
            */
            semResult = edma3OsSemTake(rmInstance->initParam.rmSemHandle,
            EDMA3_OSSEM_NO_TIMEOUT);

            if (EDMA3_RM_SOK == semResult)
                {
                /**
                * We have to search three different arrays, namely ownedResoures,
                * avlblResources and resvdResources, to find the 'common' contiguous
                * resources. For this, take an 'AND' of all three arrays in one single
                * array and use your algorithm on that array.
                */
                switch (resObj->type)
                    {
                    case EDMA3_RM_RES_DMA_CHANNEL:
                        {
                        /* AND all the arrays to look into */
                        contiguousDmaRes[rmObj->phyCtrllerInstId][0u] = ((rmInstance->initParam.rmInstInitConfig->ownDmaChannels[0u]
                                                & rmInstance->avlblDmaChannels[0u])
                                                & (~(rmInstance->initParam.rmInstInitConfig->resvdDmaChannels[0u]))
                                                );
                        contiguousDmaRes[rmObj->phyCtrllerInstId][1u] = ((rmInstance->initParam.rmInstInitConfig->ownDmaChannels[1u]
                                                & rmInstance->avlblDmaChannels[1u])
                                                & (~(rmInstance->initParam.rmInstInitConfig->resvdDmaChannels[1u]))
                                                );
                        }
                        break;

                    case EDMA3_RM_RES_QDMA_CHANNEL:
                        {
                        /* AND all the arrays to look into */
                        contiguousQdmaRes[rmObj->phyCtrllerInstId][0u] = ((rmInstance->initParam.rmInstInitConfig->ownQdmaChannels[0u]
                                                & rmInstance->avlblQdmaChannels[0u])
                                                & (~(rmInstance->initParam.rmInstInitConfig->resvdQdmaChannels[0u]))
                                                );
                        }
                        break;

                    case EDMA3_RM_RES_TCC:
                        {
                        /* AND all the arrays to look into */
                        contiguousTccRes[rmObj->phyCtrllerInstId][0u] = ((rmInstance->initParam.rmInstInitConfig->ownTccs[0u]
                                                & rmInstance->avlblTccs[0u])
                                                & (~(rmInstance->initParam.rmInstInitConfig->resvdTccs[0u]))
                                                );
                        contiguousTccRes[rmObj->phyCtrllerInstId][1u] = ((rmInstance->initParam.rmInstInitConfig->ownTccs[1u]
                                                & rmInstance->avlblTccs[1u])
                                                & (~(rmInstance->initParam.rmInstInitConfig->resvdTccs[1u]))
                                                );
                        }
                        break;

                    case EDMA3_RM_RES_PARAM_SET:
                        {
                        /* AND all the arrays to look into */
                        for (i = 0u; i < (maxNumResources/32u); ++i)
                            {
                            contiguousParamRes[rmObj->phyCtrllerInstId][i] = ((rmInstance->initParam.rmInstInitConfig->ownPaRAMSets[i]
                                                    & rmInstance->avlblPaRAMSets[i])
                                                    & (~(rmInstance->initParam.rmInstInitConfig->resvdPaRAMSets[i]))
                                                    );
                            }
                        }
                        break;

                    default:
                        {
                        result = EDMA3_RM_E_INVALID_PARAM;
                        }
                        break;
                    }

                if (EDMA3_RM_SOK == result)
                    {
                    /**
                     * Try to allocate 'numResources' contiguous resources
                     * of type RES_ANY.
                     */
                    result = allocAnyContigRes (rmObj->phyCtrllerInstId, resObj->type, numResources, &position);

                    /**
                    * If result != EDMA3_RM_SOK, resource allocation failed.
                    * Else resources successfully allocated.
                    */
                    if (result == EDMA3_RM_SOK)
                        {
                        /* Update the first resource id with the position returned. */
                        resObj->resId = position;

                        /*
                         * Do some further changes in the book-keeping
                         * data structures and global registers accordingly.
                         */
                        result = gblChngAllocContigRes(rmInstance, resObj, numResources);
                        }
                    }

                /* resource allocation completed, release the semaphore first */
                semResult = edma3OsSemGive(rmInstance->initParam.rmSemHandle);
                }
            }
        }


    /**
     * Check the Resource Allocation Result 'result' first. If Resource
     * Allocation has resulted in an error, return it (having more priority than
     * semResult. Else, return semResult.
     */
     if (EDMA3_RM_SOK == result)
         {
         /**
          * Resource Allocation successful, return semResult for returning
          * semaphore.
          */
         result = semResult;
         }


#ifdef EDMA3_INSTRUMENTATION_ENABLED
    EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3",
                EDMA3_DVT_DESC(EDMA3_DVT_eFUNC_END,
                EDMA3_DVT_dCOUNTER,
                EDMA3_DVT_dNONE,
                EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */

    return result;
    }



/**
 * \fn      EDMA3_RM_Result EDMA3_RM_freeContiguousResource(EDMA3_RM_Handle
 *                       hEdmaResMgr, EDMA3_RM_ResDesc *firstResIdObj,
 *                       unsigned int numResources)
 * \brief   Free a contiguous region of specified EDMA3 Resource
 * like DMA channel, QDMA channel, PaRAM Set or TCC, previously allocated.
 *
 * This API frees a contiguous region of specified EDMA3 Resources
 * like DMA channel, QDMA channel, PaRAM Set or TCC, which have been previously
 * allocated. In case of an error during the freeing of any specific resource,
 * user can check the 'firstResIdObj' object to know the last resource id
 * whose freeing has failed. In case of success, there is no need to check this
 * object.
 *
 * \param  hEdmaResMgr         [IN]         Handle to the previously opened
 *                                          Resource Manager Instance.
 * \param   firstResIdObj      [IN/OUT]     Handle to the first resource
 *                                          descriptor object, which needs to be
 *                                          freed. In case of an error while
 *                                          freeing any particular resource,
 *                                          the last resource id whose freeing has
 *                                          failed is returned in this resource
 *                                          descriptor object.
 * \param   numResources      [IN]          Number of contiguous resources allocated
 *                                          previously which user wants to release
 *
 * \note        This is a re-entrant function which internally calls
 *              EDMA3_RM_freeResource() for resource de-allocation.
 */
EDMA3_RM_Result EDMA3_RM_freeContiguousResource(EDMA3_RM_Handle hEdmaResMgr,
                                           EDMA3_RM_ResDesc *firstResIdObj,
                                           unsigned int numResources)
    {
    EDMA3_RM_Instance *rmInstance = NULL;
    EDMA3_RM_Obj *rmObj = NULL;
    EDMA3_RM_Result result = EDMA3_RM_SOK;
    EDMA3_RM_ResDesc *resObj;
    unsigned int resFreeIdx = 0u;
    unsigned int firstResId = 0u;
    unsigned int lastResId = 0u;
    unsigned int maxNumResources = 0u;

#ifdef EDMA3_INSTRUMENTATION_ENABLED
    EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3",
                EDMA3_DVT_DESC(EDMA3_DVT_eFUNC_START,
                EDMA3_DVT_dCOUNTER,
                EDMA3_DVT_dNONE,
                EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */

	/* If parameter checking is enabled... */
#ifndef EDMA3_RM_PARAM_CHECK_DISABLE
    if ((hEdmaResMgr == NULL) || (firstResIdObj == NULL))
        {
        result = EDMA3_RM_E_INVALID_PARAM;
        }
#endif

	/* Check if the parameters are OK. */
	if (EDMA3_RM_SOK == result)
        {
        rmInstance = (EDMA3_RM_Instance *)hEdmaResMgr;
        rmObj = rmInstance->pResMgrObjHandle;

        if (rmObj == NULL)
            {
            result = EDMA3_RM_E_INVALID_PARAM;
            }
        else
            {
            resObj = firstResIdObj;
            if (resObj != NULL)
                {
                firstResId = resObj->resId;
                lastResId = firstResId + (numResources - 1u);
                }

            switch (resObj->type)
                {
                case EDMA3_RM_RES_DMA_CHANNEL :
                    maxNumResources = rmObj->gblCfgParams.numDmaChannels;
                    break;
                case EDMA3_RM_RES_QDMA_CHANNEL :
                    maxNumResources = rmObj->gblCfgParams.numQdmaChannels;
                    break;
                case EDMA3_RM_RES_TCC :
                    maxNumResources = rmObj->gblCfgParams.numTccs;
                    break;
                case EDMA3_RM_RES_PARAM_SET :
                    maxNumResources = rmObj->gblCfgParams.numPaRAMSets;
                    break;
                default:
                    result = EDMA3_RM_E_INVALID_PARAM;
                    break;
                }

            if (result == EDMA3_RM_SOK)
                {
                if ((firstResId > lastResId) || (lastResId >= maxNumResources))
                    {
                    result = EDMA3_RM_E_INVALID_PARAM;
                    }
                else
                    {
                    for (resFreeIdx = firstResId; resFreeIdx <= lastResId; ++resFreeIdx)
                        {
                        resObj->resId = resFreeIdx;
                        result = EDMA3_RM_freeResource(rmInstance, resObj);

                        if (result != EDMA3_RM_SOK)
                            {
                            break;
                            }
                        }
                    }
                }
            }
        }

#ifdef EDMA3_INSTRUMENTATION_ENABLED
    EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3",
                EDMA3_DVT_DESC(EDMA3_DVT_eFUNC_END,
                EDMA3_DVT_dCOUNTER,
                EDMA3_DVT_dNONE,
                EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */

    return result;
    }




/**\fn      EDMA3_RM_Result EDMA3_RM_setCCRegister (EDMA3_RM_Handle hEdmaResMgr,
 *                   unsigned int regOffset,
 *                   unsigned int newRegValue)
 * \brief   Set the Channel Controller (CC) Register value
 *
 * \param   hEdmaResMgr     [IN]    Handle to the previously opened Resource
 *                                  Manager Instance.
 * \param   regOffset       [IN]    CC Register offset whose value needs to be
 *                                  set. It should be word-aligned.
 * \param   newRegValue     [IN]    New CC Register Value
 *
 * \return  EDMA3_RM_SOK or EDMA3_RM Error Code
 *
 * \note    This function is non re-entrant for users using the same
 *          Resource Manager handle.
 *          Before modifying a register, it tries to acquire a semaphore
 *          (RM instance specific), to protect simultaneous
 *          modification of the same register by two different users.
 *          After the successful change, it releases the semaphore.
 *          For users using different RM handles, this function is re-entrant.
 */
EDMA3_RM_Result EDMA3_RM_setCCRegister (EDMA3_RM_Handle hEdmaResMgr,
                    unsigned int regOffset,
                    unsigned int newRegValue)
    {
    EDMA3_RM_Result result = EDMA3_RM_SOK;
    EDMA3_RM_Instance *rmInstance = NULL;
    EDMA3_RM_Obj *rmObj = NULL;
    volatile unsigned int regPhyAddr = 0x0u;


#ifdef EDMA3_INSTRUMENTATION_ENABLED
    EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3",
                EDMA3_DVT_DESC(EDMA3_DVT_eFUNC_START,
                EDMA3_DVT_dCOUNTER,
                EDMA3_DVT_dNONE,
                EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */


	/* If parameter checking is enabled... */
#ifndef EDMA3_RM_PARAM_CHECK_DISABLE
    if ((hEdmaResMgr == NULL) || ((regOffset % 4u) != 0))
        {
        result = (EDMA3_RM_E_INVALID_PARAM);
        }
#endif

	/* Check if the parameters are OK. */
	if (EDMA3_RM_SOK == result)
        {
        rmInstance = (EDMA3_RM_Instance *)hEdmaResMgr;
        rmObj = rmInstance->pResMgrObjHandle;

        if (rmObj == NULL)
            {
            result = (EDMA3_RM_E_INVALID_PARAM);
            }
        else
            {
            if (rmObj->gblCfgParams.globalRegs != NULL)
                {
                /**
                  * Take the instance specific semaphore, to prevent simultaneous
                  * access to the shared resources.
                  */
                result = edma3OsSemTake(rmInstance->initParam.rmSemHandle,
                                        EDMA3_OSSEM_NO_TIMEOUT);

                if (EDMA3_RM_SOK == result)
                    {
                    /* Semaphore taken successfully, modify the registers. */
                    regPhyAddr = (unsigned int)(rmObj->gblCfgParams.globalRegs) + regOffset;

                    *(unsigned int *)regPhyAddr = newRegValue;

                    /* Return the semaphore back */
                    result = edma3OsSemGive(rmInstance->initParam.rmSemHandle);
                    }
                }
            }
        }


#ifdef EDMA3_INSTRUMENTATION_ENABLED
    EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3",
                EDMA3_DVT_DESC(EDMA3_DVT_eFUNC_END,
                EDMA3_DVT_dCOUNTER,
                EDMA3_DVT_dNONE,
                EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */


    return result;
    }




/**\fn      EDMA3_RM_Result EDMA3_RM_getCCRegister (EDMA3_RM_Handle hEdmaResMgr,
 *                    unsigned int regOffset,
 *                    unsigned int *regValue)
 * \brief   Get the Channel Controller (CC) Register value
 *
 * \param   hEdmaResMgr     [IN]        Handle to the previously opened Resource
 *                                      Manager Instance.
 * \param   regOffset       [IN]        CC Register offset whose value is
 *                                      needed. It should be word-aligned.
 * \param   regValue        [IN/OUT]    Fetched CC Register Value
 *
 * \return  EDMA3_RM_SOK or EDMA3_RM Error Code
 *
 * \note    This function is re-entrant.
 */
EDMA3_RM_Result EDMA3_RM_getCCRegister (EDMA3_RM_Handle hEdmaResMgr,
                    unsigned int regOffset,
                    unsigned int *regValue)
    {
    EDMA3_RM_Result result = EDMA3_RM_SOK;
    EDMA3_RM_Instance *rmInstance = NULL;
    EDMA3_RM_Obj *rmObj = NULL;
    volatile unsigned int regPhyAddr = 0x0u;


#ifdef EDMA3_INSTRUMENTATION_ENABLED
    EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3",
                EDMA3_DVT_DESC(EDMA3_DVT_eFUNC_START,
                EDMA3_DVT_dCOUNTER,
                EDMA3_DVT_dNONE,
                EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */


	/* If parameter checking is enabled... */
#ifndef EDMA3_RM_PARAM_CHECK_DISABLE
    if (((hEdmaResMgr == NULL) || (regValue == NULL))
        || ((regOffset % 4u) != 0))
        {
        result = (EDMA3_RM_E_INVALID_PARAM);
        }
#endif

	/* Check if the parameters are OK. */
	if (EDMA3_RM_SOK == result)
        {
        rmInstance = (EDMA3_RM_Instance *)hEdmaResMgr;
        rmObj = rmInstance->pResMgrObjHandle;

        if (rmObj == NULL)
            {
            result = (EDMA3_RM_E_INVALID_PARAM);
            }
        else
            {
            if (rmObj->gblCfgParams.globalRegs != NULL)
                {
                regPhyAddr = (unsigned int)(rmObj->gblCfgParams.globalRegs) + regOffset;

                *regValue = *(unsigned int *)regPhyAddr;
                }
            }
        }


#ifdef EDMA3_INSTRUMENTATION_ENABLED
    EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3",
                EDMA3_DVT_DESC(EDMA3_DVT_eFUNC_END,
                EDMA3_DVT_dCOUNTER,
                EDMA3_DVT_dNONE,
                EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */


    return result;
    }



/**\fn      EDMA3_RM_Result EDMA3_RM_waitAndClearTcc (EDMA3_RM_Handle
 *                        hEdmaResMgr, unsigned int tccNo)
 * \brief   Wait for a transfer completion interrupt to occur and clear it.
 *
 * This is a blocking function that returns when the IPR/IPRH bit corresponding
 * to the tccNo specified, is SET. It clears the corresponding bit while
 * returning also.
 *
 * This function waits for the specific bit indefinitely in a tight loop, with
 * out any delay in between. USE IT CAUTIOUSLY.
 *
 * \param   hEdmaResMgr     [IN]        Handle to the previously opened Resource
 *                                      Manager Instance.
 * \param   tccNo           [IN]        TCC, specific to which the function
 *                                      waits on a IPR/IPRH bit.
 *
 * \return  EDMA3_RM_SOK or EDMA3_RM Error Code
 *
 * \note    This function is re-entrant for different tccNo.
 *
 */
EDMA3_RM_Result EDMA3_RM_waitAndClearTcc (EDMA3_RM_Handle hEdmaResMgr,
                    unsigned int tccNo)
    {
    EDMA3_RM_Result result = EDMA3_RM_SOK;
    EDMA3_RM_Instance *rmInstance = NULL;
    EDMA3_RM_Obj *rmObj = NULL;
    volatile EDMA3_CCRL_Regs *globalRegs = NULL;
    volatile EDMA3_CCRL_ShadowRegs *shadowRegs = NULL;
    unsigned int tccBitMask = 0x0u;


#ifdef EDMA3_INSTRUMENTATION_ENABLED
    EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3",
                EDMA3_DVT_DESC(EDMA3_DVT_eFUNC_START,
                EDMA3_DVT_dCOUNTER,
                EDMA3_DVT_dNONE,
                EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */


	/* If parameter checking is enabled... */
#ifndef EDMA3_RM_PARAM_CHECK_DISABLE
    if (hEdmaResMgr == NULL)
        {
        result = (EDMA3_RM_E_INVALID_PARAM);
        }
#endif

	/* Check if the parameters are OK. */
	if (EDMA3_RM_SOK == result)
        {
        rmInstance = (EDMA3_RM_Instance *)hEdmaResMgr;
        rmObj = rmInstance->pResMgrObjHandle;

        if ((rmObj == NULL) || (rmObj->gblCfgParams.globalRegs == NULL))
            {
            result = (EDMA3_RM_E_INVALID_PARAM);
            }
        else
            {
			/* If parameter checking is enabled... */
#ifndef EDMA3_RM_PARAM_CHECK_DISABLE
            if (tccNo >= rmObj->gblCfgParams.numTccs)
                {
                result = (EDMA3_RM_E_INVALID_PARAM);
                }
#endif

			/* Check if the parameters are OK. */
			if (EDMA3_RM_SOK == result)
		        {
                globalRegs = (volatile EDMA3_CCRL_Regs *)(rmObj->gblCfgParams.globalRegs);
                shadowRegs = (volatile EDMA3_CCRL_ShadowRegs *)
                                        (&globalRegs->SHADOW[rmInstance->initParam.regionId]);


                if (shadowRegs != NULL)
                    {
                    if(tccNo < 32u)
                        {
                        tccBitMask = (1u << tccNo);

                        /* Check the status of the IPR[tccNo] bit. */
                        while (FALSE == (shadowRegs->IPR & tccBitMask))
                            {
                            /* Transfer not yet completed, bit not SET */
                            }

                        /**
                         * Bit found SET, transfer is completed,
                         * clear the pending interrupt and return.
                         */
                        shadowRegs->ICR = tccBitMask;
                        }
                    else
                        {
                        tccBitMask = (1u << (tccNo - 32u));

                        /* Check the status of the IPRH[tccNo-32] bit. */
                        while (FALSE == (shadowRegs->IPRH & tccBitMask))
                            {
                            /* Transfer not yet completed, bit not SET */
                            }

                        /**
                         * Bit found SET, transfer is completed,
                         * clear the pending interrupt and return.
                         */
                        shadowRegs->ICRH = tccBitMask;
                        }
                    }
                }
            }
        }


#ifdef EDMA3_INSTRUMENTATION_ENABLED
    EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3",
                EDMA3_DVT_DESC(EDMA3_DVT_eFUNC_END,
                EDMA3_DVT_dCOUNTER,
                EDMA3_DVT_dNONE,
                EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */


    return result;
    }




/**\fn      EDMA3_RM_Result EDMA3_RM_checkAndClearTcc (EDMA3_RM_Handle
 *                        hEdmaResMgr, unsigned int tccNo,
 *                       unsigned short *tccStatus)
 * \brief   Returns the status of a previously initiated transfer.
 *
 * This is a non-blocking function that returns the status of a previously
 * initiated transfer, based on the IPR/IPRH bit. This bit corresponds to
 * the tccNo specified by the user. It clears the corresponding bit, if SET,
 * while returning also.
 *
 * \param   hEdmaResMgr     [IN]        Handle to the previously opened Resource
 *                                      Manager Instance.
 * \param   tccNo           [IN]        TCC, specific to which the function
 *                                      checks the status of the IPR/IPRH bit.
 * \param   tccStatus       [IN/OUT]    Status of the transfer is returned here.
 *                                      Returns "TRUE" if the transfer has
 *                                      completed (IPR/IPRH bit SET),
 *                                      "FALSE" if the transfer has not
 *                                      completed successfully (IPR/IPRH bit
 *                                      NOT SET).
 *
 * \return  EDMA3_RM_SOK or EDMA3_RM Error Code
 *
 * \note    This function is re-entrant for different tccNo.
 */
EDMA3_RM_Result EDMA3_RM_checkAndClearTcc (EDMA3_RM_Handle hEdmaResMgr,
                    unsigned int tccNo,
                    unsigned short *tccStatus)
    {
    EDMA3_RM_Result result = EDMA3_RM_SOK;
    EDMA3_RM_Instance *rmInstance = NULL;
    EDMA3_RM_Obj *rmObj = NULL;
    volatile EDMA3_CCRL_Regs *globalRegs = NULL;
    volatile EDMA3_CCRL_ShadowRegs *shadowRegs = NULL;
    unsigned int tccBitMask = 0x0u;


#ifdef EDMA3_INSTRUMENTATION_ENABLED
    EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3",
                EDMA3_DVT_DESC(EDMA3_DVT_eFUNC_START,
                EDMA3_DVT_dCOUNTER,
                EDMA3_DVT_dNONE,
                EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */


	/* If parameter checking is enabled... */
#ifndef EDMA3_RM_PARAM_CHECK_DISABLE
    if ((hEdmaResMgr == NULL) || (tccStatus == NULL))
        {
        result = (EDMA3_RM_E_INVALID_PARAM);
        }
#endif

	/* Check if the parameters are OK. */
	if (EDMA3_RM_SOK == result)
        {
        rmInstance = (EDMA3_RM_Instance *)hEdmaResMgr;
        rmObj = rmInstance->pResMgrObjHandle;

        if ((rmObj == NULL) || (rmObj->gblCfgParams.globalRegs == NULL))
            {
            result = (EDMA3_RM_E_INVALID_PARAM);
            }
        else
            {
			/* If parameter checking is enabled... */
#ifndef EDMA3_RM_PARAM_CHECK_DISABLE
            if (tccNo >= rmObj->gblCfgParams.numTccs)
                {
                result = (EDMA3_RM_E_INVALID_PARAM);
                }
#endif

			/* Check if the parameters are OK. */
			if (EDMA3_RM_SOK == result)
                {
                globalRegs = (volatile EDMA3_CCRL_Regs *)(rmObj->gblCfgParams.globalRegs);
                shadowRegs = (volatile EDMA3_CCRL_ShadowRegs *)
                                        (&globalRegs->SHADOW[rmInstance->initParam.regionId]);

                /* Reset the tccStatus */
                *tccStatus = FALSE;

                if (shadowRegs != NULL)
                    {
                    if(tccNo < 32u)
                        {
                        tccBitMask = (1u << tccNo);

                        /* Check the status of the IPR[tccNo] bit. */
                        if ((shadowRegs->IPR & tccBitMask) != FALSE)
                            {
                            /* Transfer completed, bit found SET */
                            *tccStatus = TRUE;

                            /* Clear the pending interrupt also. */
                            shadowRegs->ICR = tccBitMask;
                            }
                        }
                    else
                        {
                        tccBitMask = (1u << (tccNo - 32u));

                        /* Check the status of the IPRH[tccNo-32] bit. */
                        if ((shadowRegs->IPRH & tccBitMask) != FALSE)
                            {
                            /* Transfer completed, bit found SET */
                            *tccStatus = TRUE;

                            /* Clear the pending interrupt also. */
                            shadowRegs->ICRH = tccBitMask;
                            }
                        }
                    }
                }
            }
        }


#ifdef EDMA3_INSTRUMENTATION_ENABLED
    EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3",
                EDMA3_DVT_DESC(EDMA3_DVT_eFUNC_END,
                EDMA3_DVT_dCOUNTER,
                EDMA3_DVT_dNONE,
                EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */


    return result;
    }



/**\fn      EDMA3_RM_Result EDMA3_RM_setPaRAM (EDMA3_RM_Handle hEdmaResMgr,
 *                        EDMA3_RM_ResDesc *lChObj,
 *                       const EDMA3_RM_PaRAMRegs *newPaRAM)
 * \brief   Copy the user specified PaRAM Set onto the PaRAM Set
 *          associated with the logical channel (DMA/QDMA/Link).
 *
 * This API takes a PaRAM Set as input and copies it onto the actual PaRAM Set
 * associated with the logical channel. OPT field of the PaRAM Set is written
 * first and the CCNT field is written last.
 *
 * Caution: It should be used carefully when programming the QDMA channels whose
 *          trigger words are not CCNT field.
 *
 * \param   hEdmaResMgr     [IN]        Handle to the previously opened Resource
 *                                      Manager Instance.
 * \param   lChObj          [IN]        Logical Channel object for which new
 *                                      PaRAM set is specified. User should pass
 *                                      the resource type and id in this object.
 * \param   newPaRAM        [IN]        PaRAM set to be copied onto existing one
 *
 * \return  EDMA3_RM_SOK or EDMA3_RM Error Code
 *
 * \note    This function is re-entrant for unique lChObj values. It is non-
 *          re-entrant for same lChObj value.
 */
EDMA3_RM_Result EDMA3_RM_setPaRAM (EDMA3_RM_Handle hEdmaResMgr,
                        EDMA3_RM_ResDesc *lChObj,
                        const EDMA3_RM_PaRAMRegs *newPaRAM)
    {
    EDMA3_RM_Result result = EDMA3_RM_SOK;
    EDMA3_RM_Instance *rmInstance = NULL;
    EDMA3_RM_Obj *rmObj = NULL;
    int paRAMId = 0u;
    unsigned int qdmaChId = 0u;
    volatile EDMA3_CCRL_Regs *globalRegs = NULL;

#ifdef EDMA3_INSTRUMENTATION_ENABLED
    EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3",
    EDMA3_DVT_DESC(EDMA3_DVT_eFUNC_START,
    EDMA3_DVT_dCOUNTER,
    EDMA3_DVT_dNONE,
    EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */

	/* If parameter checking is enabled... */
#ifndef EDMA3_RM_PARAM_CHECK_DISABLE
    if (hEdmaResMgr == NULL)
        {
        result = EDMA3_RM_E_INVALID_PARAM;
        }

    if ((lChObj == NULL) || (newPaRAM == NULL))
        {
        result = EDMA3_RM_E_INVALID_PARAM;
        }
#endif

	/* Check if the parameters are OK. */
    if (result == EDMA3_RM_SOK)
        {
        rmInstance = (EDMA3_RM_Instance *)hEdmaResMgr;

        if (rmInstance == NULL)
            {
            result = EDMA3_RM_E_INVALID_PARAM;
            }
        }

    if (result == EDMA3_RM_SOK)
        {
        rmObj = rmInstance->pResMgrObjHandle;

        if (rmObj == NULL)
            {
            result = EDMA3_RM_E_INVALID_PARAM;
            }
        else
            {
            if (rmObj->gblCfgParams.globalRegs == NULL)
                {
                result = EDMA3_RM_E_INVALID_PARAM;
                }
            }
        }


    if (result == EDMA3_RM_SOK)
        {
        globalRegs = (volatile EDMA3_CCRL_Regs *)(rmObj->gblCfgParams.globalRegs);

        switch (lChObj->type)
            {
            case EDMA3_RM_RES_DMA_CHANNEL:
                {
                if (lChObj->resId <= EDMA3_RM_DMA_CH_MAX_VAL)
                    {
                    paRAMId = edma3RmChBoundRes[rmObj->phyCtrllerInstId][lChObj->resId].paRAMId;
                    }
                else
                    {
                    result = EDMA3_RM_E_INVALID_PARAM;
                    }
                }
                break;

            case EDMA3_RM_RES_QDMA_CHANNEL:
                {
                if (lChObj->resId < EDMA3_MAX_QDMA_CH)
                    {
                    qdmaChId = lChObj->resId + EDMA3_RM_QDMA_CH_MIN_VAL;
                    paRAMId = edma3RmChBoundRes[rmObj->phyCtrllerInstId][qdmaChId].paRAMId;
                    }
                else
                    {
                    result = EDMA3_RM_E_INVALID_PARAM;
                    }
                }
                break;

            case EDMA3_RM_RES_PARAM_SET:
                {
                if (lChObj->resId < edma3NumPaRAMSets)
                    {
                    /**
                     * User has passed the actual param set value here.
                     * Use this value only
                     */
                    paRAMId = (int)(lChObj->resId);
                    }
                else
                    {
                    result = EDMA3_RM_E_INVALID_PARAM;
                    }
                }
                break;

            default:
                result = EDMA3_RM_E_INVALID_PARAM;
            }
        }


    if (result == EDMA3_RM_SOK)
        {
        /* Check the param id first. */
        if ((paRAMId != -1) && ((unsigned int)paRAMId < edma3NumPaRAMSets))
            {
            /* Set the PaRAM Set now. */
            edma3MemCpy ((void *)(&(globalRegs->PARAMENTRY[paRAMId].OPT)),
                                        (const void *)newPaRAM,
                                        sizeof(EDMA3_CCRL_ParamentryRegs));
            }
        else
            {
            result = EDMA3_RM_E_INVALID_PARAM;
            }
        }


#ifdef EDMA3_INSTRUMENTATION_ENABLED
    EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3",
                EDMA3_DVT_DESC(EDMA3_DVT_eFUNC_END,
                EDMA3_DVT_dCOUNTER,
                EDMA3_DVT_dNONE,
                EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */

    return result;
    }



/**\fn      EDMA3_RM_Result EDMA3_RM_getPaRAM (EDMA3_RM_Handle hEdmaResMgr,
 *                   EDMA3_RM_ResDesc *lChObj,
 *                   EDMA3_RM_PaRAMRegs *currPaRAM)
 * \brief   Retrieve existing PaRAM set associated with specified logical
 *          channel (DMA/QDMA/Link).
 *
 * \param   hEdmaResMgr     [IN]        Handle to the previously opened Resource
 *                                      Manager Instance.
 * \param   lChObj          [IN]        Logical Channel object for which the
 *                                      PaRAM set is requested. User should pass
 *                                      the resource type and id in this object.
 * \param   currPaRAM       [IN/OUT]    User gets the existing PaRAM here.
 *
 * \return  EDMA3_RM_SOK or EDMA3_RM Error Code
 *
 * \note    This function is re-entrant.
 */
EDMA3_RM_Result EDMA3_RM_getPaRAM (EDMA3_RM_Handle hEdmaResMgr,
                    EDMA3_RM_ResDesc *lChObj,
                    EDMA3_RM_PaRAMRegs *currPaRAM)
    {
    EDMA3_RM_Result result = EDMA3_RM_SOK;
    EDMA3_RM_Instance *rmInstance = NULL;
    EDMA3_RM_Obj *rmObj = NULL;
    int paRAMId = 0u;
    unsigned int qdmaChId = 0u;
    volatile EDMA3_CCRL_Regs *globalRegs = NULL;

#ifdef EDMA3_INSTRUMENTATION_ENABLED
    EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3",
    EDMA3_DVT_DESC(EDMA3_DVT_eFUNC_START,
    EDMA3_DVT_dCOUNTER,
    EDMA3_DVT_dNONE,
    EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */

	/* If parameter checking is enabled... */
#ifndef EDMA3_RM_PARAM_CHECK_DISABLE
    if (hEdmaResMgr == NULL)
        {
        result = EDMA3_RM_E_INVALID_PARAM;
        }

    if ((lChObj == NULL) || (currPaRAM == NULL))
        {
        result = EDMA3_RM_E_INVALID_PARAM;
        }
#endif

	/* Check if the parameters are OK. */
    if (result == EDMA3_RM_SOK)
        {
        rmInstance = (EDMA3_RM_Instance *)hEdmaResMgr;

        if (rmInstance == NULL)
            {
            result = EDMA3_RM_E_INVALID_PARAM;
            }
        }

    if (result == EDMA3_RM_SOK)
        {
        rmObj = rmInstance->pResMgrObjHandle;

        if (rmObj == NULL)
            {
            result = EDMA3_RM_E_INVALID_PARAM;
            }
        else
            {
            if (rmObj->gblCfgParams.globalRegs == NULL)
                {
                result = EDMA3_RM_E_INVALID_PARAM;
                }
            }
        }

    if (result == EDMA3_RM_SOK)
        {
        globalRegs = (volatile EDMA3_CCRL_Regs *)(rmObj->gblCfgParams.globalRegs);

        switch (lChObj->type)
            {
            case EDMA3_RM_RES_DMA_CHANNEL:
                {
                if (lChObj->resId <= EDMA3_RM_DMA_CH_MAX_VAL)
                    {
                    paRAMId = edma3RmChBoundRes[rmObj->phyCtrllerInstId][lChObj->resId].paRAMId;
                    }
                else
                    {
                    result = EDMA3_RM_E_INVALID_PARAM;
                    }
                }
                break;

            case EDMA3_RM_RES_QDMA_CHANNEL:
                {
                if (lChObj->resId < EDMA3_MAX_QDMA_CH)
                    {
                    qdmaChId = lChObj->resId + EDMA3_RM_QDMA_CH_MIN_VAL;
                    paRAMId = edma3RmChBoundRes[rmObj->phyCtrllerInstId][qdmaChId].paRAMId;
                    }
                else
                    {
                    result = EDMA3_RM_E_INVALID_PARAM;
                    }
                }
                break;

            case EDMA3_RM_RES_PARAM_SET:
                {
                if (lChObj->resId < edma3NumPaRAMSets)
                    {
                    /**
                     * User has passed the actual param set value here.
                     * Use this value only
                     */
                    paRAMId = (int)(lChObj->resId);
                    }
                else
                    {
                    result = EDMA3_RM_E_INVALID_PARAM;
                    }
                }
                break;

            default:
                result = EDMA3_RM_E_INVALID_PARAM;
            }
        }


    if (result == EDMA3_RM_SOK)
        {
        /* Check the param id first. */
        if ((paRAMId != -1) && ((unsigned int)paRAMId < edma3NumPaRAMSets))
            {
            /* Get the PaRAM Set now. */
            edma3MemCpy ((void *)currPaRAM ,
                                    (const void *)(&(globalRegs->PARAMENTRY [paRAMId].OPT)),
                                    sizeof(EDMA3_CCRL_ParamentryRegs));
            }
        else
            {
            result = EDMA3_RM_E_INVALID_PARAM;
            }
        }


#ifdef EDMA3_INSTRUMENTATION_ENABLED
    EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3",
                EDMA3_DVT_DESC(EDMA3_DVT_eFUNC_END,
                EDMA3_DVT_dCOUNTER,
                EDMA3_DVT_dNONE,
                EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */


    return result;
    }


/**\fn      EDMA3_RM_Result EDMA3_RM_getPaRAMPhyAddr(EDMA3_RM_Handle
 *                           hEdmaResMgr, EDMA3_RM_ResDesc *lChObj,
 *                           unsigned int *paramPhyAddr)
 * \brief   Get the PaRAM Set Physical Address associated with a logical channel
 *
 * This function returns the PaRAM Set Phy Address (unsigned 32 bits).
 * The returned address could be used by the advanced users to program the
 * PaRAM Set directly without using any APIs.
 *
 * Least significant 16 bits of this address could be used to program
 * the LINK field in the PaRAM Set.
 * Users which program the LINK field directly SHOULD use this API
 * to get the associated PaRAM Set address with the LINK channel.
 *
 * \param   hEdmaResMgr     [IN]        Handle to the previously opened Resource
 *                                      Manager Instance.
 * \param   lChObj          [IN]        Logical Channel object for which the
 *                                      PaRAM set physical address is required.
 *                                      User should pass the resource type and
 *                                      id in this object.
 * \param   paramPhyAddr [IN/OUT]       PaRAM Set physical address is returned
 *                                      here.
 *
 * \return  EDMA3_RM_SOK or EDMA3_RM Error Code
 *
 * \note    This function is re-entrant.
 */
EDMA3_RM_Result EDMA3_RM_getPaRAMPhyAddr(EDMA3_RM_Handle hEdmaResMgr,
                    EDMA3_RM_ResDesc *lChObj,
                    unsigned int *paramPhyAddr)
    {
    EDMA3_RM_Result result = EDMA3_RM_SOK;
    EDMA3_RM_Instance *rmInstance = NULL;
    EDMA3_RM_Obj *rmObj = NULL;
    int paRAMId = 0u;
    unsigned int qdmaChId = 0u;
    volatile EDMA3_CCRL_Regs *globalRegs = NULL;

#ifdef EDMA3_INSTRUMENTATION_ENABLED
    EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3",
                EDMA3_DVT_DESC(EDMA3_DVT_eFUNC_START,
                EDMA3_DVT_dCOUNTER,
                EDMA3_DVT_dNONE,
                EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */


	/* If parameter checking is enabled... */
#ifndef EDMA3_RM_PARAM_CHECK_DISABLE
    if (hEdmaResMgr == NULL)
        {
        result = EDMA3_RM_E_INVALID_PARAM;
        }

    if ((lChObj == NULL) || (paramPhyAddr == NULL))
        {
        result = EDMA3_RM_E_INVALID_PARAM;
        }
#endif

    if (result == EDMA3_RM_SOK)
        {
        rmInstance = (EDMA3_RM_Instance *)hEdmaResMgr;

        if (rmInstance == NULL)
            {
            result = EDMA3_RM_E_INVALID_PARAM;
            }
        }

    if (result == EDMA3_RM_SOK)
        {
        rmObj = rmInstance->pResMgrObjHandle;

        if (rmObj == NULL)
            {
            result = EDMA3_RM_E_INVALID_PARAM;
            }
        else
            {
            if (rmObj->gblCfgParams.globalRegs == NULL)
                {
                result = EDMA3_RM_E_INVALID_PARAM;
                }
            }
        }

    if (result == EDMA3_RM_SOK)
        {
        globalRegs = (volatile EDMA3_CCRL_Regs *)(rmObj->gblCfgParams.globalRegs);

        switch (lChObj->type)
            {
            case EDMA3_RM_RES_DMA_CHANNEL:
                {
                if (lChObj->resId <= EDMA3_RM_DMA_CH_MAX_VAL)
                    {
                    paRAMId = edma3RmChBoundRes[rmObj->phyCtrllerInstId][lChObj->resId].paRAMId;
                    }
                else
                    {
                    result = EDMA3_RM_E_INVALID_PARAM;
                    }
                }
                break;

            case EDMA3_RM_RES_QDMA_CHANNEL:
                {
                if (lChObj->resId < EDMA3_MAX_QDMA_CH)
                    {
                    qdmaChId = lChObj->resId + EDMA3_RM_QDMA_CH_MIN_VAL;
                    paRAMId = edma3RmChBoundRes[rmObj->phyCtrllerInstId][qdmaChId].paRAMId;
                    }
                else
                    {
                    result = EDMA3_RM_E_INVALID_PARAM;
                    }
                }
                break;

            case EDMA3_RM_RES_PARAM_SET:
                {
                if (lChObj->resId < edma3NumPaRAMSets)
                    {
                    /**
                     * User has passed the actual param set value here.
                     * Use this value only
                     */
                    paRAMId = (int)(lChObj->resId);
                    }
                else
                    {
                    result = EDMA3_RM_E_INVALID_PARAM;
                    }
                }
                break;

            default:
                result = EDMA3_RM_E_INVALID_PARAM;
            }
        }



    if (result == EDMA3_RM_SOK)
        {
        /* Check the param id first. */
        if ((paRAMId != -1) && ((unsigned int)paRAMId < edma3NumPaRAMSets))
            {
            /* Get the PaRAM Set Address now. */
            *paramPhyAddr = (unsigned int)(&(globalRegs->PARAMENTRY [paRAMId].OPT));
            }
        else
            {
            result = EDMA3_RM_E_INVALID_PARAM;
            }
        }

#ifdef EDMA3_INSTRUMENTATION_ENABLED
    EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3",
                EDMA3_DVT_DESC(EDMA3_DVT_eFUNC_END,
                EDMA3_DVT_dCOUNTER,
                EDMA3_DVT_dNONE,
                EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */

    return result;
    }


/**\fn      EDMA3_RM_Result EDMA3_RM_getBaseAddress (EDMA3_RM_Handle
 *                   hEdmaResMgr, EDMA3_RM_Cntrlr_PhyAddr controllerId,
 *                   unsigned int *phyAddress)
 * \brief   Get the Channel Controller or Transfer Controller (n) Physical
 *          Address.
 *
 * \param   hEdmaResMgr     [IN]        Handle to the previously opened Resource
 *                                      Manager Instance.
 * \param   controllerId    [IN]        Channel Controller or Transfer
 *                                      Controller (n) for which the physical
 *                                      address is required.
 * \param   phyAddress      [IN/OUT]    Physical address is returned here.
 *
 * \return  EDMA3_RM_SOK or EDMA3_RM Error Code
 *
 * \note    This function is re-entrant.
 */
EDMA3_RM_Result EDMA3_RM_getBaseAddress (EDMA3_RM_Handle hEdmaResMgr,
                    EDMA3_RM_Cntrlr_PhyAddr controllerId,
                    unsigned int *phyAddress)
    {
    EDMA3_RM_Result result = EDMA3_RM_SOK;
    EDMA3_RM_Instance *rmInstance = NULL;
    EDMA3_RM_Obj *rmObj = NULL;

#ifdef EDMA3_INSTRUMENTATION_ENABLED
    EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3",
                EDMA3_DVT_DESC(EDMA3_DVT_eFUNC_START,
                EDMA3_DVT_dCOUNTER,
                EDMA3_DVT_dNONE,
                EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */


	/* If parameter checking is enabled... */
#ifndef EDMA3_RM_PARAM_CHECK_DISABLE
    if ((hEdmaResMgr == NULL) || (phyAddress == NULL))
        {
        result = EDMA3_RM_E_INVALID_PARAM;
        }
#endif

    if (result == EDMA3_RM_SOK)
        {
        rmInstance = (EDMA3_RM_Instance *)hEdmaResMgr;

        if (rmInstance == NULL)
            {
            result = EDMA3_RM_E_INVALID_PARAM;
            }
        }

    if (result == EDMA3_RM_SOK)
        {
        rmObj = rmInstance->pResMgrObjHandle;

        if (rmObj == NULL)
            {
            result = EDMA3_RM_E_INVALID_PARAM;
            }
        }

    if (result == EDMA3_RM_SOK)
        {
		/* If parameter checking is enabled... */
#ifndef EDMA3_RM_PARAM_CHECK_DISABLE
        /* Verify the 'controllerId' */
        if ((controllerId < ((EDMA3_RM_Cntrlr_PhyAddr)(EDMA3_RM_CC_PHY_ADDR)))
            || (controllerId > (EDMA3_RM_Cntrlr_PhyAddr)(rmObj->gblCfgParams.numTcs)))
            {
            /* Invalid controllerId */
            result = EDMA3_RM_E_INVALID_PARAM;
            }
#endif

		/* Check if the parameters are OK. */
		if (EDMA3_RM_SOK == result)
            {
            if (controllerId == EDMA3_RM_CC_PHY_ADDR)
                {
                /* EDMA3 Channel Controller Address */
                *phyAddress = (unsigned int)(rmObj->gblCfgParams.globalRegs);
                }
            else
                {
                /**
                 * Since the TCs enum start from 1, and TCs start from 0,
                 * subtract 1 from the enum to get the actual address.
                 */
                *phyAddress = (unsigned int)(rmObj->gblCfgParams.tcRegs[controllerId-1u]);
                }
            }
        }

#ifdef EDMA3_INSTRUMENTATION_ENABLED
    EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3",
                EDMA3_DVT_DESC(EDMA3_DVT_eFUNC_END,
                EDMA3_DVT_dCOUNTER,
                EDMA3_DVT_dNONE,
                EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */

    return result;
    }




/**\fn      EDMA3_RM_Result EDMA3_RM_getGblConfigParams (unsigned int phyCtrllerInstId,
 *                                        EDMA3_RM_GblConfigParams *gblCfgParams)
 * \brief   Get the SoC specific configuration structure for the EDMA3 Hardware.
 *
 * This API is used to fetch the global SoC specific configuration structure
 * for the EDMA3 Hardware. It is useful for the user who has not passed
 * this information during EDMA3_RM_create() and taken the default configuration
 * coming along with the package.
 *
 * \param phyCtrllerInstId  [IN]    EDMA3 Controller Instance Id
 *                                 (Hardware instance id, starting from 0).
 * \param gblCfgParams      [IN/OUT]    SoC specific configuration structure for the
 *                                  EDMA3 Hardware will be returned here.
 *
 * \return  EDMA3_RM_SOK or EDMA3_RM Error Code
 *
 * \note    This function is re-entrant.
 */
EDMA3_RM_Result EDMA3_RM_getGblConfigParams (
                                        unsigned int phyCtrllerInstId,
                                        EDMA3_RM_GblConfigParams *gblCfgParams)
    {
    EDMA3_RM_Result result = EDMA3_RM_SOK;

#ifdef EDMA3_INSTRUMENTATION_ENABLED
        EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3",
        EDMA3_DVT_DESC(EDMA3_DVT_eFUNC_START,
        EDMA3_DVT_dCOUNTER,
        EDMA3_DVT_dNONE,
        EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */

	/* If parameter checking is enabled... */
#ifndef EDMA3_RM_PARAM_CHECK_DISABLE
    if ((phyCtrllerInstId >= EDMA3_MAX_EDMA3_INSTANCES)
        || (NULL == gblCfgParams))
        {
        result = EDMA3_RM_E_INVALID_PARAM;
        }
#endif

    if (EDMA3_RM_SOK == result)
        {
        /* Return the previously saved global config information for the EDMA3 HW */
        edma3MemCpy((void *)(gblCfgParams),
                                    (const void *)(&resMgrObj[phyCtrllerInstId].gblCfgParams),
                                    sizeof (EDMA3_RM_GblConfigParams));
        }

#ifdef EDMA3_INSTRUMENTATION_ENABLED
        EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3",
                    EDMA3_DVT_DESC(EDMA3_DVT_eFUNC_END,
                    EDMA3_DVT_dCOUNTER,
                    EDMA3_DVT_dNONE,
                    EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */

    return result;
    }



/**\fn      EDMA3_RM_Result EDMA3_RM_getInstanceInitCfg (EDMA3_RM_Handle hEdmaResMgr,
 *                                      EDMA3_RM_InstanceInitConfig *instanceInitConfig)
 * \brief   Get the RM Instance specific configuration structure for different
 *           EDMA3 resources' usage (owned resources, reserved resources etc).
 *
 * This API is used to fetch the Resource Manager Instance specific configuration
 * structure, for a specific shadow region. It is useful for the user who has not passed
 * this information during EDMA3_RM_opn() and taken the default configuration
 * coming along with the package. EDMA3 resources, owned and reserved by this RM
 * instance, will be returned from this API.
 *
 * \param   hEdmaResMgr         [IN]    Handle to the previously opened Resource
 *                                      Manager Instance.
 * \param   instanceInitConfig      [IN/OUT]    RM Instance specific configuration
 *                                      structure will be returned here.
 *
 * \return  EDMA3_RM_SOK or EDMA3_RM Error Code
 *
 * \note    This function is re-entrant.
 */
EDMA3_RM_Result EDMA3_RM_getInstanceInitCfg (
                                        EDMA3_RM_Handle hEdmaResMgr,
                                        EDMA3_RM_InstanceInitConfig *instanceInitConfig)
    {
    EDMA3_RM_Result result = EDMA3_RM_SOK;
    EDMA3_RM_Instance *rmInstance = NULL;
    EDMA3_RM_Obj *rmObj = NULL;
    unsigned int resMgrIdx = 0u;
    unsigned int hwId;

#ifdef EDMA3_INSTRUMENTATION_ENABLED
    EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3",
                EDMA3_DVT_DESC(EDMA3_DVT_eFUNC_START,
                EDMA3_DVT_dCOUNTER,
                EDMA3_DVT_dNONE,
                EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */

	/* If parameter checking is enabled... */
#ifndef EDMA3_RM_PARAM_CHECK_DISABLE
    if ((hEdmaResMgr == NULL) || (instanceInitConfig == NULL))
        {
        result = EDMA3_RM_E_INVALID_PARAM;
        }
#endif

    if (result == EDMA3_RM_SOK)
        {
        rmInstance = (EDMA3_RM_Instance *)hEdmaResMgr;

        if (rmInstance == NULL)
            {
            result = EDMA3_RM_E_INVALID_PARAM;
            }
        }

    if (result == EDMA3_RM_SOK)
        {
        rmObj = rmInstance->pResMgrObjHandle;

        if (rmObj == NULL)
            {
            result = EDMA3_RM_E_INVALID_PARAM;
            }
        }

    if (result == EDMA3_RM_SOK)
        {
        hwId = rmObj->phyCtrllerInstId;

        for (resMgrIdx = 0u; resMgrIdx < EDMA3_MAX_RM_INSTANCES; resMgrIdx++)
            {
                if (rmInstance == ((EDMA3_RM_Instance *)(ptrRMIArray) +
                                            (hwId*EDMA3_MAX_RM_INSTANCES) +
                                            resMgrIdx))
                {
                 /* RM Id found. Return the specific config info to the user. */
                edma3MemCpy((void *)(instanceInitConfig),
                                            (const void *)((EDMA3_RM_InstanceInitConfig *)(ptrInitCfgArray) +
                                                                    (hwId*EDMA3_MAX_RM_INSTANCES) +
                                                                    resMgrIdx),
                                            sizeof (EDMA3_RM_InstanceInitConfig));
                break;
                }
            }

        if (EDMA3_MAX_RM_INSTANCES == resMgrIdx)
            {
            /* RM Id not found, report error... */
            result = EDMA3_RM_E_INVALID_PARAM;
            }
        }

#ifdef EDMA3_INSTRUMENTATION_ENABLED
    EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3",
                EDMA3_DVT_DESC(EDMA3_DVT_eFUNC_END,
                EDMA3_DVT_dCOUNTER,
                EDMA3_DVT_dNONE,
                EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */

    return result;
    }



/**
 *  \brief EDMA3 Resource Manager IOCTL
 *
 *  This function provides IOCTL functionality for EDMA3 Resource Manager
 *
 *  \param  hEdmaResMgr      [IN]       Handle to the previously opened Resource
 *                                      Manager Instance.
 *  \param  cmd             [IN]        IOCTL command to be performed
 *  \param  cmdArg          [IN/OUT]    IOCTL command argument (if any)
 *  \param  param           [IN/OUT]    Device/Cmd specific argument
 *
 *  \return EDMA3_RM_SOK or EDMA3_RM Error Code
 */
EDMA3_RM_Result EDMA3_RM_Ioctl(
                      EDMA3_RM_Handle       hEdmaResMgr,
                      EDMA3_RM_IoctlCmd     cmd,
                      void                  *cmdArg,
                      void                  *param
                     )
    {
    EDMA3_RM_Result result = EDMA3_RM_SOK;
    EDMA3_RM_Instance *rmInstance = NULL;
    unsigned int paramInitRequired = 0xFFu;
    unsigned int regModificationRequired = 0xFFu;
    unsigned int *ret_val = NULL;

#ifdef EDMA3_INSTRUMENTATION_ENABLED
    EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3",
                EDMA3_DVT_DESC(EDMA3_DVT_eFUNC_START,
                EDMA3_DVT_dCOUNTER,
                EDMA3_DVT_dNONE,
                EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */

	/* If parameter checking is enabled... */
#ifndef EDMA3_RM_PARAM_CHECK_DISABLE
    if (hEdmaResMgr == NULL)
        {
        result = EDMA3_RM_E_INVALID_PARAM;
        }

    if ((cmd <= EDMA3_RM_IOCTL_MIN_IOCTL)
        || (cmd >= EDMA3_RM_IOCTL_MAX_IOCTL))
        {
        result = EDMA3_RM_E_INVALID_PARAM;
        }
#endif

    if (result == EDMA3_RM_SOK)
        {
        rmInstance = (EDMA3_RM_Instance *)hEdmaResMgr;

        if (rmInstance == NULL)
            {
            result = EDMA3_RM_E_INVALID_PARAM;
            }
        }

    /* To remove CCS warnings */
    (void)param;

    if (result == EDMA3_RM_SOK)
        {
        switch (cmd)
            {
            case EDMA3_RM_IOCTL_SET_PARAM_CLEAR_OPTION:
                {
                paramInitRequired = (unsigned int)cmdArg;

				/* If parameter checking is enabled... */
#ifndef EDMA3_RM_PARAM_CHECK_DISABLE
                if ((paramInitRequired != 0u)
                    && (paramInitRequired != 1u))
                    {
	                result = EDMA3_RM_E_INVALID_PARAM;
                    }
#endif

				/* Check if the parameters are OK. */
				if (EDMA3_RM_SOK == result)
					{
                    /* Set/Reset the flag which is being used to do the PaRAM clearing. */
                    rmInstance->paramInitRequired = paramInitRequired;
                	}

                break;
                }

            case EDMA3_RM_IOCTL_GET_PARAM_CLEAR_OPTION:
                {
				/* If parameter checking is enabled... */
#ifndef EDMA3_RM_PARAM_CHECK_DISABLE
                if (NULL == cmdArg)
                    {
                    result = EDMA3_RM_E_INVALID_PARAM;
                    }
#endif

				/* Check if the parameters are OK. */
				if (EDMA3_RM_SOK == result)
                    {
                    ret_val = (unsigned int *)cmdArg;

                    /* Get the flag which is being used to do the PaRAM clearing. */
                    *ret_val = rmInstance->paramInitRequired;
                    }

                break;
                }

            case EDMA3_RM_IOCTL_SET_GBL_REG_MODIFY_OPTION:
                {
                regModificationRequired = (unsigned int)cmdArg;

				/* If parameter checking is enabled... */
#ifndef EDMA3_RM_PARAM_CHECK_DISABLE
                if ((regModificationRequired != 0u)
                    && (regModificationRequired != 1u))
                    {
                    /* All other values are invalid. */
                    result = EDMA3_RM_E_INVALID_PARAM;
                    }
#endif

				/* Check if the parameters are OK. */
				if (EDMA3_RM_SOK == result)
					{
                    /**
                     * Set/Reset the flag which is being used to do the global
                     * registers and PaRAM modification.
                     */
                    rmInstance->regModificationRequired = regModificationRequired;
                	}

                break;
                }

            case EDMA3_RM_IOCTL_GET_GBL_REG_MODIFY_OPTION:
                {
				/* If parameter checking is enabled... */
#ifndef EDMA3_RM_PARAM_CHECK_DISABLE
                if (NULL == cmdArg)
                    {
                    result = EDMA3_RM_E_INVALID_PARAM;
                    }
#endif

				/* Check if the parameters are OK. */
				if (EDMA3_RM_SOK == result)
                    {
                    ret_val = (unsigned int *)cmdArg;

                    /**
                     * Get the flag which is being used to do the global
                     * registers and PaRAM modification.
                     */
                    *ret_val = rmInstance->regModificationRequired;
                    }

                break;
                }

            default:
                /* Hey dude! you passed invalid IOCTL cmd */
                result = EDMA3_RM_E_INVALID_PARAM;

            }
        }


#ifdef EDMA3_INSTRUMENTATION_ENABLED
        EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3",
                    EDMA3_DVT_DESC(EDMA3_DVT_eFUNC_END,
                    EDMA3_DVT_dCOUNTER,
                    EDMA3_DVT_dNONE,
                    EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */

        return result;

    }



/**
 * edma3ComplHandler
 * \brief   Interrupt handler for successful transfer completion.
 *
 * \note    This function first disables its own interrupt to make it non-
 *          entrant. Later, after calling all the callback functions, it
 *          re-enables its own interrupt.
 *
 * \return  None.
 */
static void edma3ComplHandler (const EDMA3_RM_Obj *rmObj)
    {
    unsigned int Cnt;
    volatile EDMA3_CCRL_Regs *ptrEdmaccRegs = NULL;
    volatile EDMA3_CCRL_ShadowRegs *shadowRegs = NULL;
    volatile unsigned int pendingIrqs;
    unsigned int indexl;
    unsigned int indexh;

#ifdef EDMA3_INSTRUMENTATION_ENABLED
    EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3",
                EDMA3_DVT_DESC(EDMA3_DVT_eINT_START,
                EDMA3_DVT_dNONE,
                EDMA3_DVT_dNONE,
                EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */

    assert (NULL != rmObj);

    ptrEdmaccRegs =
            (volatile EDMA3_CCRL_Regs *)rmObj->gblCfgParams.globalRegs;
    if (ptrEdmaccRegs != NULL)
        {
        shadowRegs = (volatile EDMA3_CCRL_ShadowRegs *)
                                    (&ptrEdmaccRegs->SHADOW[edma3RegionId]);
        }

    Cnt = 0u;
    pendingIrqs = 0u;
    indexl = 1u;
    indexh = 1u;

    if((shadowRegs->IPR !=0 ) || (shadowRegs->IPRH !=0 ))
        {
        /**
         * Since an interrupt has found, we have to make sure that this
         * interrupt (TCC) belongs to the TCCs allocated by us only.
         * It might happen that someone else, who is using EDMA3 also,
         * is the owner of this interrupt channel i.e. the TCC.
         * For this, use the allocatedTCCs[], to check which all interrupt
         * channels are owned by the EDMA3 RM Instances.
         */

        edma3OsProtectEntry (rmObj->phyCtrllerInstId,
        					EDMA3_OS_PROTECT_INTERRUPT_XFER_COMPLETION, 
        					NULL);

        /* Loop for EDMA3_RM_COMPL_HANDLER_RETRY_COUNT number of time,
		   breaks when no pending interrupt is found */
        while ((Cnt < EDMA3_RM_COMPL_HANDLER_RETRY_COUNT)
                    && ((indexl != 0u) || (indexh != 0u)))
            {
            indexl = 0u;
            pendingIrqs = shadowRegs->IPR;

            /**
             * Choose interrupts coming from our allocated TCCs
             * and MASK remaining ones.
             */
            pendingIrqs = (pendingIrqs & allocatedTCCs[rmObj->phyCtrllerInstId][0u]);

            while (pendingIrqs)
                {
                /*Process all the pending interrupts*/
                if((pendingIrqs & 1u) == TRUE)
                    {
                    /**
                     * If the user has not given any callback function
                     * while requesting the TCC, its TCC specific bit
                     * in the IPR register will NOT be cleared.
                     */
                    if(edma3IntrParams[rmObj->phyCtrllerInstId][indexl].tccCb != NULL)
                        {
                         /* here write to ICR to clear the corresponding IPR bits*/
                        shadowRegs->ICR = (1u << indexl);

                        edma3IntrParams[rmObj->phyCtrllerInstId][indexl].tccCb (indexl,
                                    EDMA3_RM_XFER_COMPLETE,
                                    edma3IntrParams[rmObj->phyCtrllerInstId][indexl].cbData);
                        }
                    }
                ++indexl;
                pendingIrqs >>= 1u;
                }

            indexh = 0u;
            pendingIrqs = shadowRegs->IPRH;

            /**
             * Choose interrupts coming from our allocated TCCs
             * and MASK remaining ones.
             */
            pendingIrqs = (pendingIrqs & allocatedTCCs[rmObj->phyCtrllerInstId][1u]);

            while (pendingIrqs)
                {
                /*Process all the pending interrupts*/
                if((pendingIrqs & 1u)==TRUE)
                    {
                    /**
                     * If the user has not given any callback function
                     * while requesting the TCC, its TCC specific bit
                     * in the IPRH register will NOT be cleared.
                     */
                    if(edma3IntrParams[rmObj->phyCtrllerInstId][32u+indexh].tccCb!=NULL)
                        {
                         /* here write to ICR to clear the corresponding IPR bits*/
                        shadowRegs->ICRH = (1u << indexh);

                        edma3IntrParams[rmObj->phyCtrllerInstId][32u+indexh].tccCb(32u+indexh,
                                    EDMA3_RM_XFER_COMPLETE,
                                    edma3IntrParams[rmObj->phyCtrllerInstId][32u+indexh].cbData);
                        }
                    }
                ++indexh;
                pendingIrqs >>= 1u;
                }

            Cnt++;
            }

        indexl = (shadowRegs->IPR & allocatedTCCs[rmObj->phyCtrllerInstId][0u]);
        indexh = (shadowRegs->IPRH & allocatedTCCs[rmObj->phyCtrllerInstId][1u]);

        if((indexl !=0 ) || (indexh !=0 ))
            {
            shadowRegs->IEVAL=0x1u;
            }

        edma3OsProtectExit (rmObj->phyCtrllerInstId,
							EDMA3_OS_PROTECT_INTERRUPT_XFER_COMPLETION, 
							NULL);
        }

#ifdef EDMA3_INSTRUMENTATION_ENABLED
    EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3",
                EDMA3_DVT_DESC(EDMA3_DVT_eINT_END,
                EDMA3_DVT_dNONE,
                EDMA3_DVT_dNONE,
                EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */
    }


void lisrEdma3ComplHandler0(unsigned int edma3InstanceId)
    {
    /* Invoke Completion Handler ISR */
    edma3ComplHandler(&resMgrObj[edma3InstanceId]);
    }


/**
 * \brief   Interrupt handler for Channel Controller Error.
 *
 * \note    This function first disables its own interrupt to make it non-
 *          entrant. Later, after calling all the callback functions, it
 *          re-enables its own interrupt.
 *
 * \return  None.
 */
static void edma3CCErrHandler(const EDMA3_RM_Obj *rmObj)
    {
    unsigned int Cnt = 0u;
    unsigned int resMgrInstIdx = 0u;
    volatile EDMA3_CCRL_Regs *ptrEdmaccRegs = NULL;
    volatile EDMA3_CCRL_ShadowRegs *shadowRegs = NULL;
    volatile unsigned int pendingIrqs;
    unsigned int index;
    unsigned int evtqueNum;
    EDMA3_RM_Instance *rm_instance = NULL;
    unsigned int hwId;
    unsigned int num_rm_instances_opened;
    EDMA3_RM_Instance *rmInstance   = NULL;
    unsigned int ownedDmaError = 0;
    unsigned int ownedDmaHError = 0;
    unsigned int ownedQdmaError = 0;

#ifdef EDMA3_INSTRUMENTATION_ENABLED
    EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3_CCERR",
                EDMA3_DVT_DESC(EDMA3_DVT_eINT_START,
                EDMA3_DVT_dNONE,
                EDMA3_DVT_dNONE,
                EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */

    assert (rmObj != NULL);

    ptrEdmaccRegs = (volatile EDMA3_CCRL_Regs *)rmObj->gblCfgParams.globalRegs;
    if (ptrEdmaccRegs != NULL)
        {
        shadowRegs = (volatile EDMA3_CCRL_ShadowRegs *)&ptrEdmaccRegs->SHADOW[edma3RegionId];
        hwId = rmObj->phyCtrllerInstId;
        rmInstance = ((EDMA3_RM_Instance *)(ptrRMIArray)
                            + ((rmObj->phyCtrllerInstId)*EDMA3_MAX_RM_INSTANCES)
                            + edma3RegionId);

        pendingIrqs = 0u;
        index = 1u;

        if(((ptrEdmaccRegs->EMR != 0 )
            || (ptrEdmaccRegs->EMRH != 0 ))
            || ((ptrEdmaccRegs->QEMR != 0)
            || (ptrEdmaccRegs->CCERR != 0)))
            {
            edma3OsProtectEntry (rmObj->phyCtrllerInstId,
								EDMA3_OS_PROTECT_INTERRUPT_CC_ERROR, 
								NULL);

            /* Loop for EDMA3_RM_CCERR_HANDLER_RETRY_COUNT number of time,
			   breaks when no pending interrupt is found */
            while ((Cnt < EDMA3_RM_CCERR_HANDLER_RETRY_COUNT)
                        && (index != 0u))
                {
                index = 0u;
                pendingIrqs = ptrEdmaccRegs->EMR;

                while (pendingIrqs)
                    {
                    /*Process all the pending interrupts*/
                    if((pendingIrqs & 1u)==TRUE)
                        {
                        unsigned int mappedTcc = 0u;

                        /**
                         * Using the 'index' value (basically the DMA
                         * channel), fetch the corresponding TCC
                         * value, mapped to this DMA channel.
                         */
                        mappedTcc = edma3DmaChTccMapping[rmObj->phyCtrllerInstId][index];

                        /**
                         * Ensure that the mapped tcc is valid and the call
                         * back is not NULL
                         */
                        if (mappedTcc < EDMA3_MAX_TCC)
                            {
                            /**
                             * TCC owned and allocated by RM.
                             * Write to EMCR to clear the corresponding EMR bits.
                             */
                            ptrEdmaccRegs->EMCR = (1u<<index);
                            /*Clear any SER*/
                            shadowRegs->SECR = (1u<<index);

                            /* Call the callback function if registered earlier. */
                            if((edma3IntrParams[rmObj->phyCtrllerInstId][mappedTcc].tccCb) != NULL)
                                {
                                edma3IntrParams[rmObj->phyCtrllerInstId][mappedTcc].tccCb (
                                        mappedTcc,
                                        EDMA3_RM_E_CC_DMA_EVT_MISS,
                                        edma3IntrParams[rmObj->phyCtrllerInstId][mappedTcc].cbData
                                        );
                                }
                            }
                        else
                            {
                            /**
                             * DMA channel not owned by the RM instance.
                             * Check the global error interrupt clearing option.
                             * If it is TRUE, clear the error interupt else leave
                             * it like that.
                             */
#if (EDMA3_RM_RES_CLEAR_ERROR_STATUS_FOR_ALL_CHANNELS == TRUE)
                                /* here write to EMCR to clear the corresponding EMR bits. */
                                ptrEdmaccRegs->EMCR = (1u<<index);
                                /*Clear any SER*/
                                ptrEdmaccRegs->SECR = (1u<<index);
#endif
                            }
                        }
                    ++index;
                    pendingIrqs >>= 1u;
                    }

                index = 0u;
                pendingIrqs = ptrEdmaccRegs->EMRH;
                while (pendingIrqs)
                    {
                    /*Process all the pending interrupts*/
                    if((pendingIrqs & 1u)==TRUE)
                        {
                        unsigned int mappedTcc = 0u;

                        /**
                         * Using the 'index' value (basically the DMA
                         * channel), fetch the corresponding TCC
                         * value, mapped to this DMA channel.
                         */
                        mappedTcc = edma3DmaChTccMapping[rmObj->phyCtrllerInstId][32u+index];

                        /**
                         * Ensure that the mapped tcc is valid and the call
                         * back is not NULL
                         */
                        if (mappedTcc < EDMA3_MAX_TCC)
                            {
                            /**
                             * TCC owned and allocated by RM.
                             * Write to EMCR to clear the corresponding EMR bits.
                             */
                            ptrEdmaccRegs->EMCRH = (1u<<index);
                            /*Clear any SERH*/
                            shadowRegs->SECRH = (1u<<index);

                            /* Call the callback function if registered earlier. */
                            if((edma3IntrParams[rmObj->phyCtrllerInstId][mappedTcc].tccCb) != NULL)
                                {
                                edma3IntrParams[rmObj->phyCtrllerInstId][mappedTcc].tccCb (
                                        mappedTcc,
                                        EDMA3_RM_E_CC_DMA_EVT_MISS,
                                        edma3IntrParams[rmObj->phyCtrllerInstId][mappedTcc].cbData
                                        );
                                }
                            }
                        else
                            {
                            /**
                             * DMA channel not owned by the RM instance.
                             * Check the global error interrupt clearing option.
                             * If it is TRUE, clear the error interupt else leave
                             * it like that.
                             */
#if (EDMA3_RM_RES_CLEAR_ERROR_STATUS_FOR_ALL_CHANNELS == TRUE)
                                /**
                                 * TCC NOT owned by RM.
                                 * Write to EMCRH to clear the corresponding EMRH bits.
                                 */
                                ptrEdmaccRegs->EMCRH = (1u<<index);
                                /*Clear any SERH*/
                                shadowRegs->SECRH = (1u<<index);
#endif
                            }
                        }
                    ++index;
                    pendingIrqs >>= 1u;
                    }

                index = 0u;
                pendingIrqs = ptrEdmaccRegs->QEMR;
                while (pendingIrqs)
                    {
                    /*Process all the pending interrupts*/
                    if((pendingIrqs & 1u)==TRUE)
                        {
                        unsigned int mappedTcc = 0u;

                        /**
                         * Using the 'index' value (basically the QDMA
                         * channel), fetch the corresponding TCC
                         * value, mapped to this QDMA channel.
                         */
                        mappedTcc = edma3QdmaChTccMapping[rmObj->phyCtrllerInstId][index];

                        if (mappedTcc < EDMA3_MAX_TCC)
                           {
                            /* here write to QEMCR to clear the corresponding QEMR bits*/
                            ptrEdmaccRegs->QEMCR = (1u<<index);
                            /*Clear any QSER*/
                            shadowRegs->QSECR = (1u<<index);

                            if((edma3IntrParams[rmObj->phyCtrllerInstId][mappedTcc].tccCb) != NULL)
                                {
                                edma3IntrParams[rmObj->phyCtrllerInstId][mappedTcc].tccCb (
                                        mappedTcc,
                                        EDMA3_RM_E_CC_QDMA_EVT_MISS,
                                        edma3IntrParams[rmObj->phyCtrllerInstId][mappedTcc].cbData
                                        );
                                }
                            }
                        else
                            {
                            /**
                             * QDMA channel not owned by the RM instance.
                             * Check the global error interrupt clearing option.
                             * If it is TRUE, clear the error interupt else leave
                             * the ISR.
                             */
#if (EDMA3_RM_RES_CLEAR_ERROR_STATUS_FOR_ALL_CHANNELS == TRUE)
                                /* here write to QEMCR to clear the corresponding QEMR bits*/
                                ptrEdmaccRegs->QEMCR = (1u<<index);

                                /*Clear any QSER*/
                                ptrEdmaccRegs->QSECR = (1u<<index);
#endif
                            }
                        }
                    ++index;
                    pendingIrqs >>= 1u;
                    }

                index = 0u;
                pendingIrqs = ptrEdmaccRegs->CCERR;
                if (pendingIrqs!=NULL)
                    {
                    /* Process all the pending CC error interrupts. */

                    /* Queue threshold error for different event queues.*/
                    for (evtqueNum = 0u; evtqueNum < rmObj->gblCfgParams.numEvtQueue; evtqueNum++)
                        {
                        if((pendingIrqs & (1u << evtqueNum)) != NULL)
                            {
                            /**
                             * Queue threshold error for queue 'evtqueNum' raised.
                             * Inform all the RM instances working on this region
                             * about the error by calling their global callback functions.
                             */
                            num_rm_instances_opened = resMgrObj[hwId].numOpens;
                            for (resMgrInstIdx = 0u; num_rm_instances_opened; ++resMgrInstIdx)
                                {
                                /* Check whether the RM instance opened working on this region */
                                rm_instance = ((EDMA3_RM_Instance *)(ptrRMIArray) + (hwId*EDMA3_MAX_RM_INSTANCES) + resMgrInstIdx);
                                if (NULL != rm_instance)
                                    {
                                    if (rm_instance->initParam.regionId == edma3RegionId)
                                        {
                                        /* Region id matches, call the callback function */
                                        if (rm_instance->initParam.gblerrCbParams.gblerrCb != NULL)
                                            {
                                            rm_instance->initParam.gblerrCbParams.gblerrCb(
                                                    EDMA3_RM_E_CC_QUE_THRES_EXCEED,
                                                    evtqueNum,
                                                    rm_instance->initParam.gblerrCbParams.gblerrData);
                                            }
                                        }
                                    }

                                /* Check next opened instance */
                                num_rm_instances_opened--;
                                }

                            /* Clear the error interrupt. */
                            ptrEdmaccRegs->CCERRCLR = (1u << evtqueNum);
                            }
                        }


                    /* Transfer completion code error. */
                    if ((pendingIrqs & (1 << EDMA3_CCRL_CCERR_TCCERR_SHIFT))!=NULL)
                        {
                        /**
                         * Transfer completion code error raised.
                         * Inform all the RM instances working on this region
                         * about the error by calling their global callback functions.
                         */
                        num_rm_instances_opened = resMgrObj[hwId].numOpens;
                        for (resMgrInstIdx = 0u; num_rm_instances_opened; ++resMgrInstIdx)
                            {
                            /* Check whether the RM instance opened working on this region */
                            rm_instance = ((EDMA3_RM_Instance *)(ptrRMIArray) + (hwId*EDMA3_MAX_RM_INSTANCES) + resMgrInstIdx);
                            if (NULL != rm_instance)
                                {
                                if (rm_instance->initParam.regionId == edma3RegionId)
                                    {
                                    /* Region id matches, call the callback function */
                                    if (rm_instance->initParam.gblerrCbParams.gblerrCb != NULL)
                                        {
                                        rm_instance->initParam.gblerrCbParams.gblerrCb(
                                                EDMA3_RM_E_CC_TCC,
                                                NULL,
                                                rm_instance->initParam.gblerrCbParams.gblerrData);
                                        }
                                    }
                                }

                            /* Check next opened instance */
                            num_rm_instances_opened--;
                            }

                        ptrEdmaccRegs->CCERRCLR = (1<<EDMA3_CCRL_CCERR_TCCERR_SHIFT);
                        }

                    ++index;
                    }

                Cnt++;
                }


            /**
             * Read the error registers again. If any interrupt is pending,
             * write the EEVAL register.
             * Moreover, according to the global error interrupt clearing
             * option, check either error bits associated with all the
             * DMA/QDMA channels (option is SET) OR check error bits
             * associated with owned DMA/QDMA channels.
             */
#if (EDMA3_RM_RES_CLEAR_ERROR_STATUS_FOR_ALL_CHANNELS == TRUE)
				/* To remove warning. */
				rmInstance = rmInstance;

                /* Check all the error bits. */
                ownedDmaError = ptrEdmaccRegs->EMR;
                ownedDmaHError = ptrEdmaccRegs->EMRH;
                ownedQdmaError = ptrEdmaccRegs->QEMR;
#else
                /* Check ONLY owned error bits. */
                ownedDmaError = (ptrEdmaccRegs->EMR & rmInstance->initParam.rmInstInitConfig->ownDmaChannels[0u]);
                ownedDmaHError = (ptrEdmaccRegs->EMRH & rmInstance->initParam.rmInstInitConfig->ownDmaChannels[1u]);
                ownedQdmaError = (ptrEdmaccRegs->QEMR & rmInstance->initParam.rmInstInitConfig->ownQdmaChannels[0u]);
#endif

            if (((ownedDmaError != 0 ) || (ownedDmaHError != 0 ))
                        || ((ownedQdmaError != 0) || (ptrEdmaccRegs->CCERR != 0)))
                {
                ptrEdmaccRegs->EEVAL=0x1u;
                }

            edma3OsProtectExit (rmObj->phyCtrllerInstId,
								EDMA3_OS_PROTECT_INTERRUPT_CC_ERROR, 
								NULL);
            }
        }

#ifdef EDMA3_INSTRUMENTATION_ENABLED
    EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3_CCERR",
                EDMA3_DVT_DESC(EDMA3_DVT_eINT_END,
                EDMA3_DVT_dNONE,
                EDMA3_DVT_dNONE,
                EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */
    }

void lisrEdma3CCErrHandler0(unsigned int edma3InstanceId)
    {
    /* Invoke CC Error Handler ISR */
    edma3CCErrHandler(&resMgrObj[edma3InstanceId]);
    }



/**
 * \brief   Interrupt handler for Transfer Controller Error.
 *
 * \note    This function first disables its own interrupt to make it non-
 *          entrant. Later, after calling all the callback functions, it
 *          re-enables its own interrupt.
 *
 * \return  None.
 */
static void edma3TCErrHandler (const EDMA3_RM_Obj *rmObj, unsigned int tcNum)
    {
    volatile EDMA3_TCRL_Regs *tcRegs = NULL;
    unsigned int tcMemErrRdWr = 0u;
    unsigned int resMgrInstIdx = 0u;
    EDMA3_RM_Instance *rm_instance = NULL;
    unsigned int hwId = 0u;
    unsigned int num_rm_instances_opened = 0u;

#ifdef EDMA3_INSTRUMENTATION_ENABLED
    EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3_TCERR",
                EDMA3_DVT_DESC(EDMA3_DVT_eINT_START,
                EDMA3_DVT_dNONE,
                EDMA3_DVT_dNONE,
                EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */

    assert ((rmObj != NULL) && (tcNum < rmObj->gblCfgParams.numTcs));

    if (rmObj->gblCfgParams.tcRegs[tcNum] != NULL)
        {
        tcRegs = (volatile EDMA3_TCRL_Regs *)(rmObj->gblCfgParams.tcRegs[tcNum]);
        hwId = rmObj->phyCtrllerInstId;
        }

    if (tcRegs != NULL)
        {
        if(tcRegs->ERRSTAT != 0)
            {
            edma3OsProtectEntry (rmObj->phyCtrllerInstId,
								EDMA3_OS_PROTECT_INTERRUPT_TC_ERROR, 
								&tcNum);

            if((tcRegs->ERRSTAT & (1 << EDMA3_TCRL_ERRSTAT_BUSERR_SHIFT))!=NULL)
                {
                /* Bus error event. */
                /**
                 * EDMA3TC has detected an error at source or destination
                 * address. Error information can be read from the error
                 * details register (ERRDET).
                 */
                tcMemErrRdWr = tcRegs->ERRDET & (EDMA3_TCRL_ERRDET_STAT_MASK);
                if ((tcMemErrRdWr > 0u) && (tcMemErrRdWr < 8u))
                    {
                    /**
                     * Inform all the RM instances working on this region
                     * about the error by calling their global callback functions.
                     */
                    num_rm_instances_opened = resMgrObj[hwId].numOpens;
                    for (resMgrInstIdx = 0u; num_rm_instances_opened; ++resMgrInstIdx)
                        {
                        /* Check whether the RM instance opened working on this region */
                        rm_instance = ((EDMA3_RM_Instance *)(ptrRMIArray) + (hwId*EDMA3_MAX_RM_INSTANCES) + resMgrInstIdx);
                        if (NULL != rm_instance)
                            {
                            if (rm_instance->initParam.regionId == edma3RegionId)
                                {
                                /* Region id matches, call the callback function */
                                if (rm_instance->initParam.gblerrCbParams.gblerrCb != NULL)
                                    {
                                    rm_instance->initParam.gblerrCbParams.gblerrCb(
                                            EDMA3_RM_E_TC_MEM_LOCATION_READ_ERROR,
                                            tcNum,
                                            rm_instance->initParam.gblerrCbParams.gblerrData);
                                    }
                                }
                            }

                            /* Check next opened instance */
                            num_rm_instances_opened--;
                        }
                    }
                else
                    {
                    if ((tcMemErrRdWr >= 8u) && (tcMemErrRdWr <= 0xFu))
                        {
                        /**
                         * Inform all the RM instances working on this region
                         * about the error by calling their global callback functions.
                         */
                        num_rm_instances_opened = resMgrObj[hwId].numOpens;
                        for (resMgrInstIdx = 0u; num_rm_instances_opened; ++resMgrInstIdx)
                            {
                            /* Check whether the RM instance opened working on this region */
                            rm_instance = ((EDMA3_RM_Instance *)(ptrRMIArray) + (hwId*EDMA3_MAX_RM_INSTANCES) + resMgrInstIdx);
                            if (NULL != rm_instance)
                                {
                                if (rm_instance->initParam.regionId == edma3RegionId)
                                    {
                                    /* Region id matches, call the callback function */
                                    if (rm_instance->initParam.gblerrCbParams.gblerrCb != NULL)
                                        {
                                        rm_instance->initParam.gblerrCbParams.gblerrCb(
                                                EDMA3_RM_E_TC_MEM_LOCATION_WRITE_ERROR,
                                                tcNum,
                                                rm_instance->initParam.gblerrCbParams.gblerrData);
                                        }
                                    }
                                }

                                /* Check next opened instance */
                                num_rm_instances_opened--;
                            }
                        }
                    }
                tcRegs->ERRCLR = (1<<EDMA3_TCRL_ERRSTAT_BUSERR_SHIFT);
                }
            else
                {
                /* Transfer request (TR) error event. */
                if((tcRegs->ERRSTAT & (1 << EDMA3_TCRL_ERRSTAT_TRERR_SHIFT))!=NULL)
                    {
                    num_rm_instances_opened = resMgrObj[hwId].numOpens;
                    for (resMgrInstIdx = 0u; num_rm_instances_opened; ++resMgrInstIdx)
                        {
                        /* Check whether the RM instance opened working on this region */
                        rm_instance = ((EDMA3_RM_Instance *)(ptrRMIArray) + (hwId*EDMA3_MAX_RM_INSTANCES) + resMgrInstIdx);
                        if (NULL != rm_instance)
                            {
                            if (rm_instance->initParam.regionId == edma3RegionId)
                                {
                                /* Region id matches, call the callback function */
                                if (rm_instance->initParam.gblerrCbParams.gblerrCb != NULL)
                                    {
                                    rm_instance->initParam.gblerrCbParams.gblerrCb(
                                            EDMA3_RM_E_TC_TR_ERROR,
                                            tcNum,
                                            rm_instance->initParam.gblerrCbParams.gblerrData);
                                    }
                                }
                            }

                            /* Check next opened instance */
                            num_rm_instances_opened--;
                        }

                    tcRegs->ERRCLR = (1<<EDMA3_TCRL_ERRSTAT_TRERR_SHIFT);
                    }
                else
                    {
                    if((tcRegs->ERRSTAT & (1 << EDMA3_TCRL_ERRSTAT_MMRAERR_SHIFT))!=NULL)
                        {
                        num_rm_instances_opened = resMgrObj[hwId].numOpens;
                        for (resMgrInstIdx = 0u; num_rm_instances_opened; ++resMgrInstIdx)
                            {
                            /* Check whether the RM instance opened working on this region */
                            rm_instance = ((EDMA3_RM_Instance *)(ptrRMIArray) + (hwId*EDMA3_MAX_RM_INSTANCES) + resMgrInstIdx);
                            if (NULL != rm_instance)
                                {
                                if (rm_instance->initParam.regionId == edma3RegionId)
                                    {
                                    /* Region id matches, call the callback function */
                                    if (rm_instance->initParam.gblerrCbParams.gblerrCb != NULL)
                                        {
                                        rm_instance->initParam.gblerrCbParams.gblerrCb(
                                                EDMA3_RM_E_TC_INVALID_ADDR,
                                                tcNum,
                                                rm_instance->initParam.gblerrCbParams.gblerrData);
                                        }
                                    }
                                }

                                /* Check next opened instance */
                                num_rm_instances_opened--;
                            }

                        tcRegs->ERRCLR = (1<<EDMA3_TCRL_ERRSTAT_MMRAERR_SHIFT);
                        }
                    }
                }

            edma3OsProtectExit (rmObj->phyCtrllerInstId,
								EDMA3_OS_PROTECT_INTERRUPT_TC_ERROR, 
								tcNum);
            }
        }

#ifdef EDMA3_INSTRUMENTATION_ENABLED
    EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3_TCERR",
                EDMA3_DVT_DESC(EDMA3_DVT_eINT_END,
                EDMA3_DVT_dNONE,
                EDMA3_DVT_dNONE,
                EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */
    }



/*
 *  ======== lisrEdma3TC0ErrHandler0 ========
 *  EDMA3 instance 0 TC0 Error Interrupt Service Routine
 */
void lisrEdma3TC0ErrHandler0(unsigned int edma3InstanceId)
    {
    /* Invoke Error Handler ISR for TC0*/
    edma3TCErrHandler(&resMgrObj[edma3InstanceId], 0u);
    }


/*
 *  ======== lisrEdma3TC1ErrHandler0 ========
 *  EDMA3 instance 0 TC1 Error Interrupt Service Routine
 */
void lisrEdma3TC1ErrHandler0(unsigned int edma3InstanceId)
    {
    /* Invoke Error Handler ISR for TC1*/
    edma3TCErrHandler(&resMgrObj[edma3InstanceId], 1u);
    }

/*
 *  ======== lisrEdma3TC2ErrHandler0 ========
 *  EDMA3 instance 0 TC2 Error Interrupt Service Routine
 */
void lisrEdma3TC2ErrHandler0(unsigned int edma3InstanceId)
    {
    /* Invoke Error Handler ISR for TC2*/
    edma3TCErrHandler(&resMgrObj[edma3InstanceId], 2u);
    }

/*
 *  ======== lisrEdma3TC3ErrHandler0 ========
 *  EDMA3 instance 0 TC3 Error Interrupt Service Routine
 */
void lisrEdma3TC3ErrHandler0(unsigned int edma3InstanceId)
    {
    /* Invoke Error Handler ISR for TC3*/
    edma3TCErrHandler(&resMgrObj[edma3InstanceId], 3u);
    }

/*
 *  ======== lisrEdma3TC4ErrHandler0 ========
 *  EDMA3 instance 0 TC4 Error Interrupt Service Routine
 */
void lisrEdma3TC4ErrHandler0(unsigned int edma3InstanceId)
    {
    /* Invoke Error Handler ISR for TC4*/
    edma3TCErrHandler(&resMgrObj[edma3InstanceId], 4u);
    }


/*
 *  ======== lisrEdma3TC5ErrHandler0 ========
 *  EDMA3 instance 0 TC5 Error Interrupt Service Routine
 */
void lisrEdma3TC5ErrHandler0(unsigned int edma3InstanceId)
    {
    /* Invoke Error Handler ISR for TC5*/
    edma3TCErrHandler(&resMgrObj[edma3InstanceId], 5u);
    }

/*
 *  ======== lisrEdma3TC6ErrHandler0 ========
 *  EDMA3 instance 0 TC6 Error Interrupt Service Routine
 */
/* ARGSUSED */
void lisrEdma3TC6ErrHandler0(unsigned int edma3InstanceId)
    {
    /* Invoke Error Handler ISR for TC6*/
    edma3TCErrHandler(&resMgrObj[edma3InstanceId], 6u);
    }

/*
 *  ======== lisrEdma3TC7ErrHandler0 ========
 *  EDMA3 instance 0 TC7 Error Interrupt Service Routine
 */
void lisrEdma3TC7ErrHandler0(unsigned int edma3InstanceId)
    {
    /* Invoke Error Handler ISR for TC7*/
    edma3TCErrHandler(&resMgrObj[edma3InstanceId], 7u);
    }



/*  Resource Manager Internal functions - Start */

/** Initialization of the Global region registers of the EDMA3 Controller */
static void edma3GlobalRegionInit (unsigned int phyCtrllerInstId)
    {
    unsigned int evtQNum = 0u;
    volatile EDMA3_CCRL_Regs *ptrEdmaccRegs = NULL;

    assert (phyCtrllerInstId < EDMA3_MAX_EDMA3_INSTANCES);

    ptrEdmaccRegs = (volatile EDMA3_CCRL_Regs *)
                    (resMgrObj[phyCtrllerInstId].gblCfgParams.globalRegs);

    if (ptrEdmaccRegs != NULL)
        {
        ptrEdmaccRegs->EMCR = EDMA3_RM_SET_ALL_BITS;
        ptrEdmaccRegs->EMCRH = EDMA3_RM_SET_ALL_BITS;
        ptrEdmaccRegs->QEMCR = 0x000000FFu; // orig: EDMA3_RM_SET_ALL_BITS

        /*
        * Set all Instance-wide EDMA3 parameters (not channel-specific)
        */

        /**
         * Set TC Priority among system-wide bus-masters and Queue
         * Watermark Level
         */
        while (evtQNum <
                    resMgrObj[phyCtrllerInstId].gblCfgParams.numEvtQueue)
            {
            ptrEdmaccRegs->QUEPRI &= EDMA3_RM_QUEPRI_CLR_MASK(evtQNum);
            ptrEdmaccRegs->QUEPRI |= EDMA3_RM_QUEPRI_SET_MASK(evtQNum,
                resMgrObj[phyCtrllerInstId].gblCfgParams.evtQPri[evtQNum]);

            ptrEdmaccRegs->QWMTHRA |= EDMA3_RM_QUEWMTHR_SET_MASK(evtQNum,
                        resMgrObj[phyCtrllerInstId].gblCfgParams.evtQueueWaterMarkLvl[evtQNum]);

            evtQNum++;
            }

        /* Clear CCERR register */
        ptrEdmaccRegs ->CCERRCLR = 0x0001000Fu; // orig: 0xFFFFu
        }

    return;
    }




/** Initialization of the Shadow region registers of the EDMA3 Controller */
static void edma3ShadowRegionInit (const EDMA3_RM_Instance *pRMInstance)
    {
    unsigned int  intState = 0u;
    volatile EDMA3_CCRL_Regs *ptrEdmaccRegs             = NULL;
    volatile EDMA3_CCRL_ShadowRegs *ptrEdmaShadowRegs   = NULL;
    unsigned int phyCtrllerInstId;
    unsigned int regionId;
    const EDMA3_RM_InstanceInitConfig *rmInstInitConfig = pRMInstance->initParam.rmInstInitConfig;

    assert (pRMInstance != NULL);

    if (rmInstInitConfig != NULL)
        {
        phyCtrllerInstId = pRMInstance->pResMgrObjHandle->phyCtrllerInstId;
        regionId = pRMInstance->initParam.regionId;

        ptrEdmaccRegs = (volatile EDMA3_CCRL_Regs *)
                        (resMgrObj[phyCtrllerInstId].gblCfgParams.globalRegs);

        if (ptrEdmaccRegs != NULL)
            {
            ptrEdmaShadowRegs = (volatile EDMA3_CCRL_ShadowRegs *)
                                    (&ptrEdmaccRegs->SHADOW[regionId]);

            ptrEdmaShadowRegs->ECR      = (rmInstInitConfig->ownDmaChannels[0u]
                                            | rmInstInitConfig->ownTccs[0u]);
            ptrEdmaShadowRegs->ECRH     = (rmInstInitConfig->ownDmaChannels[1u]
                                            | rmInstInitConfig->ownTccs[1u]);
            ptrEdmaShadowRegs->EECR     = (rmInstInitConfig->ownDmaChannels[0u]
                                            | rmInstInitConfig->ownTccs[0u]);
            ptrEdmaShadowRegs->SECR     = (rmInstInitConfig->ownDmaChannels[0u]
                                            | rmInstInitConfig->ownTccs[0u]);
            ptrEdmaShadowRegs->SECRH    = (rmInstInitConfig->ownDmaChannels[1u]
                                            | rmInstInitConfig->ownTccs[1u]);
            ptrEdmaShadowRegs->EECR     = (rmInstInitConfig->ownDmaChannels[0u]
                                            | rmInstInitConfig->ownTccs[0u]);
            ptrEdmaShadowRegs->EECRH    = (rmInstInitConfig->ownDmaChannels[1u]
                                            | rmInstInitConfig->ownTccs[1u]);

            ptrEdmaShadowRegs->QEECR    = rmInstInitConfig->ownQdmaChannels[0u];

            ptrEdmaShadowRegs->IECR     = (rmInstInitConfig->ownDmaChannels[0u]
                                            | rmInstInitConfig->ownTccs[0u]);
            ptrEdmaShadowRegs->IECRH    = (rmInstInitConfig->ownDmaChannels[1u]
                                            | rmInstInitConfig->ownTccs[1u]);
            ptrEdmaShadowRegs->ICR      = (rmInstInitConfig->ownDmaChannels[0u]
                                            | rmInstInitConfig->ownTccs[0u]);
            ptrEdmaShadowRegs->ICRH     = (rmInstInitConfig->ownDmaChannels[1u]
                                            | rmInstInitConfig->ownTccs[1u]);

            ptrEdmaShadowRegs->QSECR    = rmInstInitConfig->ownQdmaChannels[0u];

            /*
            * Set all EDMA3 Resource<->Region mapping parameters
            */

            /* 1. Dma Channel (and TCC) <-> Region */
#ifdef EDMA3_RM_DEBUG
            EDMA3_RM_PRINTF("DRAE=%x\r\n",ptrEdmaccRegs->DRA[regionId].DRAE);
            EDMA3_RM_PRINTF("DRAEH=%x\r\n",ptrEdmaccRegs->DRA[regionId].DRAEH);
#endif

            edma3OsProtectEntry (phyCtrllerInstId,
            					EDMA3_OS_PROTECT_INTERRUPT, 
            					&intState);
            ptrEdmaccRegs->DRA[regionId].DRAE = 0u;
            ptrEdmaccRegs->DRA[regionId].DRAEH = 0u;
            edma3OsProtectExit (phyCtrllerInstId,
								EDMA3_OS_PROTECT_INTERRUPT, 
								intState);

#ifdef EDMA3_RM_DEBUG
            EDMA3_RM_PRINTF("DRAE=%x\r\n",ptrEdmaccRegs->DRA[regionId].DRAE);
            EDMA3_RM_PRINTF("DRAEH=%x\r\n",ptrEdmaccRegs->DRA[regionId].DRAEH);
#endif

            /* 2. Qdma Channel <-> Region */
#ifdef EDMA3_RM_DEBUG
            EDMA3_RM_PRINTF("QRAE=%x\r\n",ptrEdmaccRegs->QRAE[regionId]);
#endif

            edma3OsProtectEntry (phyCtrllerInstId,
            					EDMA3_OS_PROTECT_INTERRUPT, 
            					&intState);
            ptrEdmaccRegs->QRAE[regionId] = 0u;
            edma3OsProtectExit (phyCtrllerInstId,
								EDMA3_OS_PROTECT_INTERRUPT, 
								intState);

#ifdef EDMA3_RM_DEBUG
            EDMA3_RM_PRINTF("QRAE=%x\r\n",ptrEdmaccRegs->QRAE[regionId]);
#endif

            }
        }

    return;
    }



/** Local MemSet function */
void edma3MemSet(void *dst, unsigned char data, unsigned int len)
    {
    unsigned int i=0u;
    unsigned char *ds=NULL;

    assert (dst != NULL);

    ds = (unsigned char *)dst;

    for( i=0;i<len;i++)
        {
        *ds=data;
        ds++;
        }

    return;
    }


/* Local MemCopy function */
void edma3MemCpy(void *dst, const void *src, unsigned int len)
    {
    unsigned int i=0u;
    const unsigned char *sr;
    unsigned char *ds;

    assert ((src != NULL) && (dst != NULL));

    sr = (const unsigned char *)src;
    ds = (unsigned char *)dst;

    for( i=0;i<len;i++)
        {
        *ds=*sr;
        ds++;
        sr++;
        }

    return;
    }


/**
 * Finds a particular bit ('0' or '1') in the particular word from 'start'.
 * If found, returns the position, else return -1.
 */
static int findBitInWord (int source, unsigned int start, unsigned short bit)
    {
    unsigned int position = start;
    unsigned short found = 0;
    unsigned int iterations_left = 0;

    switch (bit)
        {
        case 1:
            {
            source >>= (start%32u);

            while ((found==0u) && (source!=0))
                {
                if ((source & 0x1) == 0x1)
                    {
                    /* 1 */
                    found++;
                    }
                else
                    {
                    /* 0 */
                    source >>= 1;
                    position++;
                    }
                }

            }
            break;

        case 0:
            {
            source >>= (start%32u);
            iterations_left = 32u - (start%32u);

            while ((found==0u) && (iterations_left>0u))
                {
                if ((source & 0x1) == 0x1)
                    {
                    /* 1 */
                    source >>= 1;
                    position++;
                    iterations_left--;
                    }
                else
                    {
                    /* 0 */
                    found++;
                    }
                }
            }
            break;

        default:
            break;
        }

    return (found ? (int)position : -1);
    }


/**
 * Finds a particular bit ('0' or '1') in the specified resources' array
 * from 'start' to 'end'. If found, returns the position, else return -1.
 */
static int findBit (unsigned int phyCtrllerInstId,
                            EDMA3_RM_ResType resType,
                            unsigned int start,
                            unsigned int end,
                            unsigned short bit)
    {
    int position = -1;
    unsigned int start_index = start / 32u;
    unsigned int end_index = end / 32u;
    int i;
    unsigned int *resPtr = 0x0;
    int ret = -1;
    EDMA3_RM_Result result = EDMA3_RM_SOK;

    assert (start <= end);

    /**
     * job is to find 'bit' in an array[start_index:end_index]
     * algo used:
     * first search in array[start_index]
     * then search in array[start_index + 1 : end_index - 1]
     * then search in array[end_index]
     */
    switch (resType)
        {
        case EDMA3_RM_RES_DMA_CHANNEL:
            resPtr = &contiguousDmaRes[phyCtrllerInstId][0];
            break;

        case EDMA3_RM_RES_QDMA_CHANNEL:
            resPtr = &contiguousQdmaRes[phyCtrllerInstId][0];
            break;

        case EDMA3_RM_RES_TCC:
            resPtr = &contiguousTccRes[phyCtrllerInstId][0];
            break;

        case EDMA3_RM_RES_PARAM_SET:
            resPtr = &contiguousParamRes[phyCtrllerInstId][0];
            break;

        default:
            result = EDMA3_RM_E_INVALID_PARAM;
            break;
        }

    if (EDMA3_RM_SOK == result)
        {
        switch (bit)
            {
            case 1:
                {
                /* Find '1' in first word. */
                position = findBitInWord (resPtr[start_index], start, 1u);

                if (position != -1)
                    {
                    ret = position;
                    }
                else
                    {
                    /* '1' NOT found, look into other words. */
                    for (i = (int)(start_index + 1u); i <= (int)(end_index - 1u); i++)
                        {
                        position = findBitInWord (resPtr[i], 0u, 1u);
                        if (position != -1)
                            {
                            /* '1' Found... */
                            ret = (position + (i*32));
                            break;
                            }
                        }

                    /* First check whether we have found '1' or not. */
                    if (ret == -1)
                        {
                        /* Still not found, look in the last word. */
                        position = findBitInWord(resPtr[end_index], 0u, 1u);
                        if (position != -1)
                            {
                            /* Finally got it. */
                            ret = (position + (end_index*32u));
                            }
                        else
                            {
                            /* Sorry, could not find it, return -1. */
                            ret = -1;
                            }
                        }
                    }
                }
                break;

            case 0:
                {
                /* Find '0' in first word. */
                position = findBitInWord(resPtr[start_index], start, 0u);
                if (position != -1)
                    {
                    ret = position;
                    }
                else
                    {
                    /* '0' NOT found, look into other words. */
                        for (i = (int)(start_index + 1u); i <= (int)(end_index - 1u); i++)
                        {
                        position = findBitInWord(resPtr[i], 0u, 0u);
                        if (position != -1)
                            {
                            /* '0' found... */
                            ret = (position + (i*32));
                            break;
                            }
                        }

                    /* First check whether we have found '0' or not. */
                    if (ret == -1)
                        {
                        position = findBitInWord(resPtr[end_index], 0u, 0u);
                        if (position != -1)
                            {
                            /* Finally got it. */
                            ret = (position + (end_index*32u));
                            }
                        else
                            {
                            /* Sorry, could not find it, return -1. */
                            ret = -1;
                            }
                        }
                    }
                }
                break;

            default:
                break;
            }
        }



    return (((unsigned int)ret >= start) ? ret : -1);
}



/**
 * If successful, this function returns EDMA3_RM_SOK and the position
 * of first available resource in 'positionRes'. Else returns error.
 */
static EDMA3_RM_Result allocAnyContigRes(unsigned int phyCtrllerInstId,
                                    EDMA3_RM_ResType resType,
                                    unsigned int numResources,
                                    unsigned int *positionRes)
    {
    unsigned short found = 0u;
    int first_one, next_zero;
    unsigned int num_available;
    int ret = -1;
    unsigned int start = 0;
    unsigned int end = 0;
    EDMA3_RM_Result result = EDMA3_RM_SOK;

    assert (positionRes != NULL);

    switch (resType)
        {
        case EDMA3_RM_RES_DMA_CHANNEL:
            end = EDMA3_MAX_DMA_CH - 1u;
            break;

        case EDMA3_RM_RES_QDMA_CHANNEL:
            end = EDMA3_MAX_QDMA_CH - 1u;
            break;

        case EDMA3_RM_RES_TCC:
            end = EDMA3_MAX_TCC - 1u;
            break;

        case EDMA3_RM_RES_PARAM_SET:
            end = edma3NumPaRAMSets - 1u;
            break;

        default:
            result = EDMA3_RM_E_INVALID_PARAM;
            break;
        }

    if (result == EDMA3_RM_SOK)
        {
        /**
         * Algorithm used for finding N contiguous resources.
         * In the resources' array, '1' means available and '0' means
         * not-available.
         * Step a) Find first '1' starting from 'start'. If successful,
         * store it in first_one, else return error.
         * Step b) Find first '0' starting from (first_one+1) to 'end'.
         * If successful, store returned value in next_zero. If '0' could
         * not be located, it means all the resources are available.
         * Store 'end' (i.e. the last resource id) in next_zero.
         * Step c) Count the number of contiguous resources available
         * by subtracting first_one from next_zero.
         * Step d) If result < N, do the whole process again untill you
         * reach end. Else you have found enough resources, return success.
         */
        while((found == 0) && (((end-start)+1u) >= numResources))
            {
            /* Find first '1' starting from 'start' till 'end'. */
            first_one = findBit (phyCtrllerInstId, resType, start, end, 1u);
            if (first_one != -1)
                {
                /* Got first 1, search for first '0' now. */
                next_zero = findBit (phyCtrllerInstId, resType, first_one+1, end, 0u);
                if (next_zero == -1)
                    {
                    /* Unable to find next zero, all 1' are there */
                    next_zero = end + 1u;
                    }

                /* check no of resources available */
                num_available = next_zero - first_one;
                if (num_available >= numResources)
                    {
                    /* hurrah..., we have found enough resources. */
                    found = 1u;
                    ret = first_one;
                    }
                else
                    {
                    /* Not enough resources, try again */
                    start = next_zero + 1;
                    }
                }
            else
                {
                /* do nothing, first 1 is not there, return.  */
                break;
                }
            }
        }


    if (result == EDMA3_RM_SOK)
        {
        if (found == 1u)
            {
            /* required resources found, retrun the first available res id. */
            *positionRes = (unsigned int)ret;
            }
        else
            {
            /* No resources allocated */
            result = EDMA3_RM_E_SPECIFIED_RES_NOT_AVAILABLE;
            }
        }

    return result;
    }



/**
 * Starting from 'firstResIdObj', this function makes the next 'numResources'
 * Resources non-available for future. Also, it does some global resisters'
 * setting also.
 */
static EDMA3_RM_Result gblChngAllocContigRes(EDMA3_RM_Instance *rmInstance,
                                        const EDMA3_RM_ResDesc *firstResIdObj,
                                        unsigned int numResources)
    {
    EDMA3_RM_Result result = EDMA3_RM_SOK;
    volatile EDMA3_CCRL_Regs *gblRegs = NULL;
    EDMA3_RM_Obj *rmObj = NULL;
    unsigned int avlblIdx = 0u;
    unsigned int firstResId=0u;
    unsigned int lastResId=0u;

    assert ((rmInstance != NULL) && (firstResIdObj != NULL));

    rmObj = rmInstance->pResMgrObjHandle;

    if (rmObj == NULL)
        {
        result = EDMA3_RM_E_INVALID_PARAM;
        }

    if (EDMA3_RM_SOK == result)
        {
        gblRegs = (volatile EDMA3_CCRL_Regs *)(rmObj->gblCfgParams.globalRegs);

        if (gblRegs == NULL)
            {
            result = EDMA3_RM_E_INVALID_PARAM;
            }
        }

    if (result == EDMA3_RM_SOK)
        {
        switch (firstResIdObj->type)
            {
            case EDMA3_RM_RES_DMA_CHANNEL:
                {
                firstResId = firstResIdObj->resId;
                lastResId = firstResId + (numResources - 1u);

                for (avlblIdx=firstResId; avlblIdx <= lastResId; ++avlblIdx)
                    {
                    rmInstance->avlblDmaChannels[avlblIdx/32u] &= (unsigned int)(~(1u << (avlblIdx%32u)));

                    /**
                     * Enable the DMA channel in the DRAE/DRAEH registers also.
                     */
                    if (avlblIdx < 32u)
                        {
                        gblRegs->DRA[rmInstance->initParam.regionId].DRAE
                            |= (0x1u << avlblIdx);
                        }
                    else
                        {
                        gblRegs->DRA[rmInstance->initParam.regionId].DRAEH
                            |= (0x1u << (avlblIdx - 32u));
                        }
                    }
                }
                break;

            case EDMA3_RM_RES_QDMA_CHANNEL:
                {
                firstResId = firstResIdObj->resId;
                lastResId = firstResId + (numResources - 1u);

                for (avlblIdx=firstResId; avlblIdx <= lastResId; ++avlblIdx)
                    {
                    rmInstance->avlblQdmaChannels[avlblIdx/32u] &= (unsigned int)(~(1u << (avlblIdx%32u)));

                    /**
                     * Enable the QDMA channel in the QRAE register also.
                     */
                    gblRegs->QRAE[rmInstance->initParam.regionId]
                        |= (0x1u << avlblIdx);
                    }
                }
                break;

            case EDMA3_RM_RES_TCC:
                {
                firstResId = firstResIdObj->resId;
                lastResId = firstResId + (numResources - 1u);

                for (avlblIdx=firstResId; avlblIdx <= lastResId; ++avlblIdx)
                    {
                    rmInstance->avlblTccs[avlblIdx/32u] &= (unsigned int)(~(1u << (avlblIdx%32u)));

                    /**
                     * Enable the Interrupt channel in the DRAE/DRAEH registers.
                     * Also, If the region id coming from this
                     * RM instance is same as the Master RM
                     * Instance's region id, only then we will be
                     * getting the interrupts on the same side.
                     * So save the TCC in the allocatedTCCs[] array.
                     */
                    if (avlblIdx < 32u)
                        {
                        gblRegs->DRA[rmInstance->initParam.regionId].DRAE
                            |= (0x1u << avlblIdx);

                        if (edma3RegionId == rmInstance->initParam.regionId)
                            {
                            allocatedTCCs[rmObj->phyCtrllerInstId][0u] |= (0x1u << avlblIdx);
                            }
                        }
                    else
                        {
                        gblRegs->DRA[rmInstance->initParam.regionId].DRAEH
                            |= (0x1u << (avlblIdx - 32u));

                        if (edma3RegionId == rmInstance->initParam.regionId)
                            {
                            allocatedTCCs[rmObj->phyCtrllerInstId][1u] |= (0x1u << (avlblIdx - 32u));
                            }
                        }
                    }
                }
                break;

            case EDMA3_RM_RES_PARAM_SET:
                {
                firstResId = firstResIdObj->resId;
                lastResId = firstResId + (numResources - 1u);

                for (avlblIdx=firstResId; avlblIdx <= lastResId; ++avlblIdx)
                    {
                    rmInstance->avlblPaRAMSets [avlblIdx/32u] &= (unsigned int)(~(1u << (avlblIdx%32u)));

                    /**
                     * Also, make the actual PARAM Set NULL, checking the flag
                     * whether it is required or not.
                     */
                    if (TRUE == rmInstance->paramInitRequired)
                        {
                        edma3MemSet((void *)(&gblRegs->PARAMENTRY[avlblIdx]),
                                        0x00u,
                                        sizeof(gblRegs->PARAMENTRY[avlblIdx]));
                        }
                    }
                }
                break;

            default:
                result = EDMA3_RM_E_INVALID_PARAM;
                break;
            }
        }


    return result;
    }

/*  Resource Manager Internal functions - End */

/* End of File */
