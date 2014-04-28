/*
 * edma3_drv_basic.c
 *
 * EDMA3 Driver Basic Interface Implementation. This file contains
 * beginner-level EDMA3 Driver APIs which are required to:
 * a) Request/free a DMA, QDMA and Link channel.
 * b) Program various fields in the PaRAM Set like source/destination
 * parameters, transfer parameters etc.
 * c) Enable/disable a transfer.
 * These APIs are provided to program a DMA/QDMA channel for simple use-cases
 * and don't expose all the features of EDMA3 hardware. Users who want to go
 * beyond this and have complete control on the EDMA3 hardware are advised
 * to refer edma3_drv_adv.c source file.
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

#include <windows.h>
#include <oal_log.h>

/* EDMa3 Driver Internal Header Files */
#include "edma3.h"
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

#ifndef DEBUG
#define ZONE_ERROR          1
#define ZONE_WARN           1
#define ZONE_FUNCTION       0
#define ZONE_INIT           0
#define ZONE_INFO           0
#define ZONE_IST            0
#define ZONE_IOCTL          0
#define ZONE_VERBOSE        0
#endif

#ifndef ZONE_INIT
#define ZONE_INIT           0
#endif

void ShowPaRAM(volatile EDMA3_CCRL_Regs *globalRegs, int paRAMId, UINT8 which_mask);
void ShowShadowRegs(EDMA3_CCRL_ShadowRegs *pShadowRegs, unsigned int which_mask2, unsigned int which_mask1);
void ShowDCHMAP(volatile EDMA3_CCRL_Regs *globalRegs);
void ShowDMAQNUM(volatile EDMA3_CCRL_Regs *globalRegs);
void ShowQSTAT(volatile EDMA3_CCRL_Regs *globalRegs);
void DupPaRAM(volatile EDMA3_CCRL_Regs *globalRegs, int src_paRAMId, int dst_paRAMId);
void ShowIRs(volatile EDMA3_CCRL_Regs *globalRegs, unsigned int which_mask);
void ShowShadowIRs(volatile EDMA3_CCRL_ShadowRegs *shadowRegs, unsigned int which_mask);


/* Externel Variables */
/*---------------------------------------------------------------------------*/
/**
 * Maximum Resource Manager Instances supported by the EDMA3 Package.
 */
extern const unsigned int EDMA3_MAX_RM_INSTANCES;


/**
 * \brief EDMA3 Resource Manager Objects, tied to each EDMA3 HW Controller.
 *
 * Typically one RM object will cater to one EDMA3 HW controller
 * and will have all the global config information.
 */
extern EDMA3_RM_Obj resMgrObj[EDMA3_MAX_EDMA3_INSTANCES];


/**
 * \brief Region Specific Configuration structure for
 * EDMA3 controller, to provide region specific Information.
 *
 * This configuration info can also be provided by the user at run-time,
 * while calling EDMA3_RM_open (). If not provided at run-time,
 * this info will be taken from the config file "edma3_<PLATFORM_NAME>_cfg.c",
 * for the specified platform.
 */
extern EDMA3_RM_InstanceInitConfig *ptrInitCfgArray;


/**
 * Handles of EDMA3 Resource Manager Instances.
 *
 * Used to maintain information of the EDMA3 RM Instances
 * for each HW controller.
 * There could be a maximum of EDMA3_MAX_RM_INSTANCES instances per
 * EDMA3 HW.
 */
extern EDMA3_RM_Instance *ptrRMIArray;

/** Local MemSet function */
extern void edma3MemSet(void *dst, unsigned char data, unsigned int len);
/** Local MemCpy function */
extern void edma3MemCpy(void *dst, const void *src, unsigned int len);

#ifdef UNDER_CE
extern void edma3TccCallback(unsigned int tcc, EDMA3_RM_TccStatus status, void *appData);
#endif

/**
 * \brief EDMA3 Driver Objects, tied to each EDMA3 HW Controller.
 *
 * Typically one object will cater to one EDMA3 HW controller
 * and will have all regions' (ARM, DSP etc) specific config information.
 */
extern EDMA3_DRV_Object drvObj [EDMA3_MAX_EDMA3_INSTANCES];


/**
 * Handles of EDMA3 Driver Instances.
 *
 * Used to maintain information of the EDMA3 Driver Instances for
 * each region, for each HW controller.
 * There could be as many Driver Instances as there are shadow
 * regions. Multiple EDMA3 Driver instances on the same shadow
 * region are NOT allowed.
 */
extern EDMA3_DRV_Instance drvInstance [EDMA3_MAX_EDMA3_INSTANCES][EDMA3_MAX_REGIONS];


/**
 * \brief Resources bound to a Channel
 *
 * When a request for a channel is made, the resources PaRAM Set and TCC
 * get bound to that channel. This information is needed internally by the
 * driver when a request is made to free the channel (Since it is the
 * responsibility of the driver to free up the channel-associated resources
 * from the Resource Manager layer).
 */
extern EDMA3_DRV_ChBoundResources edma3DrvChBoundRes [EDMA3_MAX_EDMA3_INSTANCES][EDMA3_MAX_LOGICAL_CH];



/* Local functions prototypes */
/*---------------------------------------------------------------------------*/
/** Remove various mappings and do cleanup for DMA/QDMA channels */
static EDMA3_DRV_Result edma3RemoveMapping (EDMA3_DRV_Handle hEdma,
                                 unsigned int channelId);

/*---------------------------------------------------------------------------*/


/**
 *  \brief Request a DMA/QDMA/Link channel.
 *
 *  Each channel (DMA/QDMA/Link) must be requested  before initiating a DMA
 *  transfer on that channel.
 *
 * This API is used to allocate a logical channel (DMA/QDMA/Link) along with
 * the associated resources. For DMA and QDMA channels, TCC and PaRAM Set are
 * also allocated along with the requested channel. For Link channel, ONLY a
 * PaRAM Set is allocated.
 *
 * User can request a specific logical channel by passing the channel id in
 * 'pLCh'. Note that the channel id is the same as the actual resource id in
 * case of DMA channels. To allocate specific QDMA channels, user SHOULD use the
 * defines EDMA3_DRV_QDMA_CHANNEL_X mentioned above.
 *
 * User can also request ANY available logical channel also by specifying the
 * below mentioned values in '*pLCh':
 *  a)  EDMA3_DRV_DMA_CHANNEL_ANY: For DMA channels
 *  b)  EDMA3_DRV_QDMA_CHANNEL_ANY: For QDMA channels, and
 *  c)  EDMA3_DRV_LINK_CHANNEL: For Link channels. Normally user should use this
 *      value to request link channels (PaRAM Sets used for linking purpose
 *      only), unless he wants to use some specific link channels (PaRAM Sets)
 *      which is also allowed.
 *
 * This API internally uses EDMA3_RM_allocResource () to allocate the desired
 * resources (DMA/QDMA channel, PaRAM Set and TCC).
 *
 * This API also registers a specific callback function against the allocated
 * TCC.
 *
 * For DMA/QDMA channels, after allocating all the EDMA3 resources, this API
 * sets the TCC field of the OPT PaRAM Word with the allocated TCC. It also sets
 * the event queue for the channel allocated. The event queue needs to be
 * specified by the user.
 *
 * For DMA channel, it also sets the DCHMAP register, if required.
 *
 * For QDMA channel, it sets the QCHMAP register and CCNT as trigger word and
 * enables the QDMA channel by writing to the QEESR register.
 *
 *  \param  hEdma           [IN]        Handle to the previously opened Driver
 *                                      Instance.
 *  \param  pLCh            [IN/OUT]    Requested logical channel id.
 *                                      Examples:
 *                                      - EDMA3_DRV_HW_CHANNEL_EVENT_0
 *                                      - To request a DMA Master Channel
 *                                        mapped to EDMA Event 0.
 *
 *                                      - EDMA3_DRV_DMA_CHANNEL_ANY
 *                                      - For requesting any DMA Master channel
 *                                        with no event mapping.
 *
 *                                      - EDMA3_DRV_QDMA_CHANNEL_ANY
 *                                      - For requesting any QDMA Master channel
 *
 *                                      - EDMA3_DRV_QDMA_CHANNEL_0
 *                                      - For requesting the QDMA Channel 0.
 *
 *                                      - EDMA3_DRV_LINK_CHANNEL
 *                                      - For requesting a DMA Slave Channel,
 *                                      - to be linked to some other Master
 *                                      - channel.
 *
 *                                      In case user passes a specific channel
 *                                      Id, pLCh value is left unchanged. In
 *                                      case user requests ANY available
 *                                      resource, the allocated channel id is
 *                                      returned in pLCh.
 *
 *  \note   To request  a PaRAM Set for the purpose of
 *          linking to another channel,  call the function with
 *
 *          *pLCh = EDMA3_DRV_LINK_CHANNEL;
 *
 *          This function will update *pLCh with the allocated Link channel
 *          handle. This handle could be DIFFERENT from the actual PaRAM Set
 *          allocated by the Resource Manager internally. So user SHOULD NOT
 *          assume the handle as the PaRAM Set Id.
 *
 *  \param  pTcc             [IN/OUT]   The channel number on which the
 *                                      completion/error interrupt is generated.
 *                                      Not used if user requested for a Link
 *                                      channel.
 *                                      Examples:
 *                                      - EDMA3_DRV_HW_CHANNEL_EVENT_0
 *                                      - To request TCC associated with
 *                                      - DMA Master Channel mapped to EDMA
 *                                      - event 0.
 *
 *                                      - EDMA3_DRV_TCC_ANY
 *                                      - For requesting any TCC with no
 *                                      - channel mapping.
 *                                      In case user passes a specific TCC
 *                                      value, pTcc value is left unchanged.
 *                                      In case user requests ANY available TCC,
 *                                      the allocated one is returned in pTcc
 *
 *  \param  evtQueue         [IN]       Event Queue Number to which the channel
 *                                      will be mapped (valid only for the
 *                                      Master Channel (DMA/QDMA) request)
 *
 *  \param tccCb             [IN]       TCC callback - caters to channel-
 *                                      specific events like "Event Miss Error"
 *                                      or "Transfer Complete"
 *
 *  \param cbData            [IN]       Data which will be passed directly to
 *                                      the tccCb callback function
 *
 *  \return EDMA3_DRV_SOK or EDMA3_DRV Error code
 *
 *  \note    This function internally uses EDMA3 Resource Manager, which
 *           acquires a RM Instance specific semaphore
 *           to prevent simultaneous access to the global pool of resources.
 *           It also disables the global interrupts while modifying
 *           the global CC registers.
 *           It is re-entrant, but SHOULD NOT be called from the user callback
 *           function (ISR context).
 */
EDMA3_DRV_Result EDMA3_DRV_requestChannel (EDMA3_DRV_Handle hEdma,
                                    unsigned int *pLCh,
                                    unsigned int *pTcc,
                                    EDMA3_RM_EventQueue evtQueue,
                                    EDMA3_RM_TccCallback tccCb,
                                    void *cbData)
    {
    unsigned int intState=0;
    EDMA3_RM_ResDesc resObj;
    EDMA3_RM_ResDesc channelObj;
    EDMA3_DRV_Result result = EDMA3_DRV_SOK;
    unsigned int linkBcntReld = 0;
    EDMA3_DRV_Instance *drvInst = NULL;
    EDMA3_DRV_Object *drvObject = NULL;
    EDMA3_RM_ResType savedResType;
    unsigned int mappedTcc = EDMA3_DRV_CH_NO_TCC_MAP;
    int paRAMId = (int)EDMA3_RM_RES_ANY;
    EDMA3_DRV_ChannelType chType = EDMA3_DRV_CHANNEL_TYPE_QDMA;
    volatile EDMA3_CCRL_Regs *globalRegs = NULL;
    int mappedPaRAMId;

#ifdef EDMA3_INSTRUMENTATION_ENABLED
    EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3",
                EDMA3_DVT_DESC(EDMA3_DVT_eFUNC_START,
                EDMA3_DVT_dCOUNTER,
                EDMA3_DVT_dNONE,
                EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */

	/* If parameter checking is enabled... */
#ifndef EDMA3_DRV_PARAM_CHECK_DISABLE
    if ((pLCh == NULL) || (hEdma == NULL))
        {
        result = EDMA3_DRV_E_INVALID_PARAM;
        }
#endif

#ifdef UNDER_CE
    /* External callbacks not supported */
    if (tccCb != NULL)
        {
        result = EDMA3_DRV_E_INVALID_PARAM;
        }
    else
        {
        tccCb = &edma3TccCallback;
        cbData = (void*)hEdma;
        }
#endif

    edma3MemSet((void *)(&resObj), 0x00u, sizeof(resObj));

	/* Check if the parameters are OK. */
	if (EDMA3_DRV_SOK == result)
        {
        drvInst = (EDMA3_DRV_Instance *)hEdma;
        drvObject = drvInst->pDrvObjectHandle;

        if ((drvObject == NULL) || (drvObject->gblCfgParams.globalRegs == NULL))
            {
            result = EDMA3_DRV_E_INVALID_PARAM;
            }
        else
            {
            globalRegs = (volatile EDMA3_CCRL_Regs *)(drvObject->gblCfgParams.globalRegs);

			/* If parameter checking is enabled... */
#ifndef EDMA3_DRV_PARAM_CHECK_DISABLE
            if (((*pLCh) != EDMA3_DRV_LINK_CHANNEL) &&
                ((evtQueue >= drvObject->gblCfgParams.numEvtQueue) || (pTcc == NULL)))
                {
                result = EDMA3_DRV_E_INVALID_PARAM;
                }
#endif

			/* Check if the parameters are OK. */
			if (EDMA3_DRV_SOK == result)
                {
                if ((*pLCh) == EDMA3_DRV_LINK_CHANNEL)
                    {
                    /*
                     * Do nothing.  Do not allocate channel.
                     * Typically this option is for request
                     * of a PaRAM Set for linking purpose.
                     */
                    result = EDMA3_DRV_SOK;
                    }
                else if ((*pLCh) == EDMA3_DRV_DMA_CHANNEL_ANY)
                    {
                    /* First request for any available DMA channel */
                    resObj.type = EDMA3_RM_RES_DMA_CHANNEL;
                    resObj.resId = EDMA3_RM_RES_ANY;
                    result = EDMA3_RM_allocResource(drvInst->resMgrInstance, (EDMA3_RM_ResDesc *)&resObj);

                    if (result == EDMA3_RM_SOK)
                        {
                        *pLCh = resObj.resId;
                        mappedPaRAMId = drvObject->gblCfgParams.dmaChannelPaRAMMap[*pLCh];
                        mappedTcc = drvObject->gblCfgParams.dmaChannelTccMap[*pLCh];
                        if (mappedPaRAMId != EDMA3_DRV_CH_NO_PARAM_MAP)
                            {
                            /*
                             * There is a PaRAM Set statically mapped to the returned
                             * channel number.
                             * This imposes a constraint on the PaRAM Set which we
                             * next request for.
                             * We update the PaRAM Set number with the one statically
                             * reserved for the afore-returned channel number.
                             */
                            paRAMId = mappedPaRAMId;
                            }
                        chType = EDMA3_DRV_CHANNEL_TYPE_DMA;

                        /* Save the Resource Type for TCC registeration */
                        channelObj.type = EDMA3_RM_RES_DMA_CHANNEL;
                        }
                    else
                        {
                        result = EDMA3_DRV_E_DMA_CHANNEL_UNAVAIL;
                        }
                    }
                else if ((*pLCh) == EDMA3_DRV_QDMA_CHANNEL_ANY)
                    {
                    /* First request for any available QDMA channel */
                    resObj.type = EDMA3_RM_RES_QDMA_CHANNEL;
                    resObj.resId = EDMA3_RM_RES_ANY;
                    result = EDMA3_RM_allocResource(drvInst->resMgrInstance,
                                                (EDMA3_RM_ResDesc *)&resObj);
                    if (result == EDMA3_DRV_SOK)
                        {
                        (*pLCh) = resObj.resId + EDMA3_DRV_QDMA_CH_MIN_VAL;
                        chType = EDMA3_DRV_CHANNEL_TYPE_QDMA;

                        /* Save the Resource Type for TCC registeration */
                        channelObj.type = EDMA3_RM_RES_QDMA_CHANNEL;
                        }
                    else
                        {
                        result = EDMA3_DRV_E_QDMA_CHANNEL_UNAVAIL;
                        }
                    }
                else if ((*pLCh) <= EDMA3_DRV_DMA_CH_MAX_VAL)
                    {
                    /* Request for a specific DMA channel */
                    resObj.type = EDMA3_RM_RES_DMA_CHANNEL;
                    resObj.resId = *pLCh;
                    result = EDMA3_RM_allocResource(drvInst->resMgrInstance, (EDMA3_RM_ResDesc *)&resObj);
                    if (result != EDMA3_RM_SOK)
                        {
                        result = EDMA3_DRV_E_DMA_CHANNEL_UNAVAIL;
                        }
                    else
                        {
                        mappedPaRAMId = drvObject->gblCfgParams.dmaChannelPaRAMMap[*pLCh];
                        mappedTcc = drvObject->gblCfgParams.dmaChannelTccMap[*pLCh];
                        if (mappedPaRAMId != EDMA3_DRV_CH_NO_PARAM_MAP)
                            {
                            /*
                             * There is a PaRAM Set statically mapped to the returned
                             * channel number.
                             * This imposes a constraint on the PaRAM Set which we
                             * next request for.
                             * We update the PaRAM Set number with the one statically
                             * reserved for the afore-returned channel number.
                             */
                            paRAMId = mappedPaRAMId;
                            }
                        chType = EDMA3_DRV_CHANNEL_TYPE_DMA;

                        /* Save the Resource Type for TCC registeration */
                        channelObj.type = EDMA3_RM_RES_DMA_CHANNEL;
                        }
                    }
                else if (((*pLCh) >= EDMA3_DRV_QDMA_CH_MIN_VAL) && ((*pLCh) <= EDMA3_DRV_QDMA_CH_MAX_VAL))
                    {
                    /* Request for a specific QDMA channel */
                    resObj.type = EDMA3_RM_RES_QDMA_CHANNEL;
                    resObj.resId = (((*pLCh)) - EDMA3_DRV_QDMA_CH_MIN_VAL);
                    result = EDMA3_RM_allocResource(drvInst->resMgrInstance, (EDMA3_RM_ResDesc *)&resObj);
                    if (result != EDMA3_RM_SOK)
                        {
                        result = EDMA3_DRV_E_QDMA_CHANNEL_UNAVAIL;
                        }
                    else
                        {
                        chType = EDMA3_DRV_CHANNEL_TYPE_QDMA;

                        /* Save the Resource Type for TCC registeration */
                        channelObj.type = EDMA3_RM_RES_QDMA_CHANNEL;
                        }
                    }
                else
                    {
                    result = EDMA3_DRV_E_INVALID_PARAM;
                    }

                if (result == EDMA3_DRV_SOK)
                    {
                    /* Request for a PaRAM Set */
                    savedResType = resObj.type;
                    resObj.type = EDMA3_RM_RES_PARAM_SET;
                    resObj.resId = (unsigned int)paRAMId;
                    result = EDMA3_RM_allocResource(drvInst->resMgrInstance, (EDMA3_RM_ResDesc *)&resObj);

                    paRAMId = (int)resObj.resId;
                    if ((*pLCh) == EDMA3_DRV_LINK_CHANNEL)
                        {
                        if (result == EDMA3_RM_SOK)
                            {
                            unsigned int linkCh = EDMA3_DRV_LINK_CH_MIN_VAL;

                            /*
                             * Search for the next Link channel handle available,
                             * starting from EDMA3_DRV_LINK_CH_MIN_VAL.
                             */
                            while ((edma3DrvChBoundRes[drvObject->phyCtrllerInstId][linkCh].paRAMId != -1)
                                    && (linkCh <= EDMA3_DRV_LINK_CH_MAX_VAL))
                                {
                                /* Move to the next handle. */
                                linkCh++;
                                }

                            /* Verify the returned handle, it should lie in the correct range */
                            if (linkCh > EDMA3_DRV_LINK_CH_MAX_VAL)
                                {
                                result = EDMA3_DRV_E_INVALID_PARAM;
                                }
                            else
                                {
                                *pLCh = linkCh;
                                }
                            }
                        }
                    else
                        {
                        if (result != EDMA3_DRV_SOK)
                            {
                            /*
                             * Free the already allocated channel
                             * (only if channel is a Non-Link Channel)
                             */
                            resObj.type = savedResType;
                            resObj.resId = *pLCh;
                            EDMA3_RM_freeResource(drvInst->resMgrInstance, (EDMA3_RM_ResDesc *)&resObj);
                            }
                        else
                            {
                            /* Request for a TCC */
                            resObj.type = EDMA3_RM_RES_TCC;
                            if ((*pTcc) == EDMA3_DRV_TCC_ANY)
                                {
                                if (mappedTcc == EDMA3_DRV_CH_NO_TCC_MAP)
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

                            result = EDMA3_RM_allocResource(drvInst->resMgrInstance, (EDMA3_RM_ResDesc *)&resObj);
                            if (result == EDMA3_DRV_SOK)
                                {
                                *pTcc = resObj.resId;
                                }
                            else
                                {
                                resObj.type = savedResType;
                                resObj.resId = *pLCh;
                                EDMA3_RM_freeResource(drvInst->resMgrInstance, (EDMA3_RM_ResDesc *)&resObj);

                                resObj.type = EDMA3_RM_RES_PARAM_SET;
                                resObj.resId = (unsigned int)paRAMId;
                                EDMA3_RM_freeResource(drvInst->resMgrInstance, (EDMA3_RM_ResDesc *)&resObj);
                                result = EDMA3_DRV_E_TCC_UNAVAIL;
                                }

                            if (result == EDMA3_DRV_SOK)
                                {
                                /* If callback function is not null, register it with the RM. */
                                if (NULL != tccCb)
                                    {
                                    /**
                                     * Fill the resource id, whose associated TCC
                                     * needs to be registered.
                                     * For QDMA channels, pass the actual QDMA
                                     * channel no instead of (*pLCh).
                                     */
                                    if (((*pLCh) >= EDMA3_DRV_QDMA_CH_MIN_VAL)
                                                 && ((*pLCh) <= EDMA3_DRV_QDMA_CH_MAX_VAL))
                                        {
                                        channelObj.resId = (*pLCh) - EDMA3_DRV_QDMA_CH_MIN_VAL;
                                        }
                                    else
                                        {
                                        channelObj.resId = (*pLCh);
                                        }

                                    result = EDMA3_RM_registerTccCb (
                                            drvInst->resMgrInstance,
                                            (EDMA3_RM_ResDesc *)&channelObj,
                                            *pTcc, tccCb, cbData);

                                    if (result != EDMA3_DRV_SOK)
                                        {
                                        EDMA3_DRV_freeChannel (hEdma, *pLCh);
                                        result = EDMA3_DRV_E_TCC_REGISTER_FAIL;
                                        }
                                    }

                                if (result == EDMA3_DRV_SOK)
                                    {
                                    edma3OsProtectEntry(drvObject->phyCtrllerInstId,
														EDMA3_OS_PROTECT_INTERRUPT, 
														&intState);

                                    /* Associate Channel to Event Queue */
                                    if ((*pLCh) < drvObject->gblCfgParams.numDmaChannels)
                                        {
                                        globalRegs->DMAQNUM[(*pLCh) >> 3u] &= EDMA3_DRV_DMAQNUM_CLR_MASK(*pLCh);
                                        globalRegs->DMAQNUM[(*pLCh) >> 3u] |= EDMA3_DRV_DMAQNUM_SET_MASK((*pLCh),evtQueue);
                                        }
                                    else if (((*pLCh) >= EDMA3_DRV_QDMA_CH_MIN_VAL)
                                             && ((*pLCh) <= EDMA3_DRV_QDMA_CH_MAX_VAL))
                                            {
                                            globalRegs->QDMAQNUM &=
                                                EDMA3_DRV_QDMAQNUM_CLR_MASK((*pLCh)-EDMA3_DRV_QDMA_CH_MIN_VAL);
                                            globalRegs->QDMAQNUM |=
                                                EDMA3_DRV_QDMAQNUM_SET_MASK((*pLCh)-EDMA3_DRV_QDMA_CH_MIN_VAL,evtQueue);
                                            }

                                    edma3OsProtectExit(drvObject->phyCtrllerInstId,
														EDMA3_OS_PROTECT_INTERRUPT,
														intState);

                                    /**
                                     * Map the allocated PaRAM Set to the logical
                                     * DMa/QDMA channel.
                                     */
                                    if (chType == EDMA3_DRV_CHANNEL_TYPE_QDMA)
                                        {
                                        result = EDMA3_RM_mapQdmaChannel (drvInst->resMgrInstance,
                                                                        ((*pLCh)-EDMA3_DRV_QDMA_CH_MIN_VAL),
                                                                        paRAMId,
                                                                        EDMA3_RM_QDMA_TRIG_DEFAULT);
                                        }
                                    else
                                        {
                                        /**
                                         * First check whether the mapping feature is supported on the underlying
                                         * platform. In case it is not supported, dont call this API, because this
                                         * API returns error in case the feature is not there.
                                         */
                                        if (TRUE == drvObject->gblCfgParams.dmaChPaRAMMapExists)
                                            {
                                            result = EDMA3_RM_mapEdmaChannel (drvInst->resMgrInstance,
                                                                                 *pLCh,
                                                                                 paRAMId);
                                            }
                                        }

                                    if (result != EDMA3_DRV_SOK)
                                        {
                                        EDMA3_DRV_freeChannel (hEdma, *pLCh);
                                        result = EDMA3_DRV_E_CH_PARAM_BIND_FAIL;
                                        }
                                    else
                                        {
                                        /* Bind the resources PaRAM Set and TCC */
                                        /* Set TCC of Param Set corresponding to specified paramId */
                                        globalRegs->PARAMENTRY [paRAMId].OPT  &= EDMA3_DRV_OPT_TCC_CLR_MASK;
                                        globalRegs->PARAMENTRY [paRAMId].OPT |= EDMA3_DRV_OPT_TCC_SET_MASK(*pTcc);

                                        edma3DrvChBoundRes[drvObject->phyCtrllerInstId][*pLCh].tcc = *pTcc;
                                        }
                                    }
                                }
                            }
                        }

                    if (result == EDMA3_DRV_SOK)
                        {
                        /* Save the PaRAM Id and Trigger mode */
                        edma3DrvChBoundRes[drvObject->phyCtrllerInstId][*pLCh].paRAMId = paRAMId;
                        edma3DrvChBoundRes[drvObject->phyCtrllerInstId][*pLCh].trigMode =
                                                                EDMA3_DRV_TRIG_MODE_NONE;

                        /* Make the Link field NULL */
                        /* Get the Link-bcntReload PaRAM set entry */
                        linkBcntReld = (unsigned int)(*((&globalRegs->PARAMENTRY [paRAMId].OPT) +
                                                (unsigned int)EDMA3_DRV_PARAM_ENTRY_LINK_BCNTRLD));

                        /* Remove any linking */
                        linkBcntReld |= 0xFFFFu;

                        /* Store it back */
                        *((&globalRegs->PARAMENTRY[paRAMId].OPT)
                                    + (unsigned int)EDMA3_DRV_PARAM_ENTRY_LINK_BCNTRLD) = linkBcntReld;

                        /*
                        * For QDMA channels, Enable the transfer.
                        * So that user doesn't have to call EDMA3_DRV_enableTransfer() again.
                        */
                        if (((*pLCh) >= EDMA3_DRV_QDMA_CH_MIN_VAL)
                             && ((*pLCh) <= EDMA3_DRV_QDMA_CH_MAX_VAL))
                            {
                            drvInst->shadowRegs->QEESR = (1u<<((*pLCh)-EDMA3_DRV_QDMA_CH_MIN_VAL));
                            /* save the trigger mode for future use */
                            edma3DrvChBoundRes[drvObject->phyCtrllerInstId][*pLCh].trigMode = EDMA3_DRV_TRIG_MODE_QDMA;
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


/**
 * \brief Free the specified channel (DMA/QDMA/Link) and its associated
 * resources (PaRAM Set, TCC etc) and removes various mappings.
 *
 * This API internally uses EDMA3_RM_freeResource () to free the desired
 * resources.
 *
 * For Link channels, this API only frees the associated PaRAM Set.
 *
 * For DMA/QDMA channels, it does the following operations:
 * a) Disable any ongoing transfer on the channel,
 * b) Unregister the TCC Callback function and disable the interrupts,
 * c) Remove the channel to Event Queue mapping,
 * d) For DMA channels, clear the DCHMAP register, if available
 * e) For QDMA channels, clear the QCHMAP register,
 * f) Frees the DMA/QDMA channel in the end.
 *
 *  \param  hEdma            [IN]     Handle to the EDMA Driver Instance.
 *  \param  channelId        [IN]     Logical Channel number to be freed.
 *
 *  \return EDMA3_DRV_SOK or EDMA3_DRV Error code
 *
 *  \note    This function disables the global interrupts while modifying
 *           the global CC registers and while modifying global data structures,
 *           to prevent simultaneous access to the global pool of resources.
 *           It internally calls EDMA3_RM_freeResource () for resource
 *           de-allocation. It is re-entrant.
 */
EDMA3_DRV_Result EDMA3_DRV_freeChannel (EDMA3_DRV_Handle hEdma,
                                                unsigned int channelId)
    {
    EDMA3_RM_ResDesc resObj;
    EDMA3_DRV_Result result = EDMA3_DRV_SOK;
    EDMA3_DRV_Instance *drvInst = NULL;
    EDMA3_DRV_Object *drvObject = NULL;
    int paRAMId;
    unsigned int tcc;
    EDMA3_DRV_ChannelType chType = EDMA3_DRV_CHANNEL_TYPE_NONE;

#ifdef EDMA3_INSTRUMENTATION_ENABLED
    EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3",
                EDMA3_DVT_DESC(EDMA3_DVT_eFUNC_START,
                EDMA3_DVT_dCOUNTER,
                EDMA3_DVT_dNONE,
                EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */

	/* If parameter checking is enabled... */
#ifndef EDMA3_DRV_PARAM_CHECK_DISABLE
    if ((channelId > EDMA3_DRV_LOG_CH_MAX_VAL) || (hEdma == NULL))
        {
        result = EDMA3_DRV_E_INVALID_PARAM;
        }
#endif

    if (result == EDMA3_DRV_SOK)
        {
        drvInst = (EDMA3_DRV_Instance *)hEdma;
        drvObject = drvInst->pDrvObjectHandle;

        if (drvObject == NULL)
            {
            result = EDMA3_DRV_E_INVALID_PARAM;
            }
        }

    if (result == EDMA3_DRV_SOK)
        {
        /* Check the channel type */
        if (channelId <= EDMA3_DRV_DMA_CH_MAX_VAL)
            {
            /* DMA Channel */
            chType = EDMA3_DRV_CHANNEL_TYPE_DMA;
            }

        if ((channelId >= EDMA3_DRV_LINK_CH_MIN_VAL) && (channelId <= EDMA3_DRV_LINK_CH_MAX_VAL))
            {
            /* LINK Channel */
            chType = EDMA3_DRV_CHANNEL_TYPE_LINK;
            }

        if ((channelId >= EDMA3_DRV_QDMA_CH_MIN_VAL) && (channelId <= EDMA3_DRV_QDMA_CH_MAX_VAL))
            {
            /* QDMA Channel */
            chType = EDMA3_DRV_CHANNEL_TYPE_QDMA;
            }

        if (chType == 0)
            {
            result = EDMA3_DRV_E_INVALID_PARAM;
            }
        }


    if (result == EDMA3_DRV_SOK)
        {
        if (chType == EDMA3_DRV_CHANNEL_TYPE_LINK)
            {
            /* LINK Channel */
            resObj.type = EDMA3_RM_RES_PARAM_SET;

            /* Get the PaRAM id from the book-keeping info. */
            resObj.resId = (unsigned int)(edma3DrvChBoundRes[drvObject->phyCtrllerInstId][channelId].paRAMId);

            result = EDMA3_RM_freeResource(drvInst->resMgrInstance,
                                        (EDMA3_RM_ResDesc *)&resObj);

            if (result == EDMA3_DRV_SOK)
                {
                edma3DrvChBoundRes[drvObject->phyCtrllerInstId][channelId].paRAMId = -1;
                }
            }
        else
            {
            /* DMA/QDMA Channel */
            paRAMId = edma3DrvChBoundRes[drvObject->phyCtrllerInstId][channelId].paRAMId;
            tcc = edma3DrvChBoundRes[drvObject->phyCtrllerInstId][channelId].tcc;

            /* Check the paRAMId and tcc values first */
            if ((paRAMId < 0) || ((unsigned int)paRAMId >= drvObject->gblCfgParams.numPaRAMSets))
                {
                result = EDMA3_DRV_E_INVALID_PARAM;
                }

            if (tcc >= drvObject->gblCfgParams.numTccs)
                {
                result = EDMA3_DRV_E_INVALID_PARAM;
                }

            if (result == EDMA3_DRV_SOK)
                {
                /* Disable the transfer and remove various mappings. */
                result = edma3RemoveMapping(hEdma, channelId);
                }

            if (result == EDMA3_DRV_SOK)
                {
                /* Now Free the PARAM set and TCC */
                resObj.type = EDMA3_RM_RES_PARAM_SET;
                resObj.resId = (unsigned int)paRAMId;
                result = EDMA3_RM_freeResource(drvInst->resMgrInstance, (EDMA3_RM_ResDesc *)&resObj);

                if (result == EDMA3_DRV_SOK)
                    {
                    /* PaRAM Set Freed */
                    edma3DrvChBoundRes[drvObject->phyCtrllerInstId][channelId].paRAMId = -1;

                    /* Free the TCC */
                    resObj.type = EDMA3_RM_RES_TCC;
                    resObj.resId = tcc;
                    result = EDMA3_RM_freeResource(drvInst->resMgrInstance,
                                                (EDMA3_RM_ResDesc *)&resObj);
                    }

                if (result == EDMA3_DRV_SOK)
                    {
                    /* TCC Freed. */
                    edma3DrvChBoundRes[drvObject->phyCtrllerInstId][channelId].tcc = 0;

                    /* Now free the DMA/QDMA Channel in the end. */
                    if (chType == EDMA3_DRV_CHANNEL_TYPE_QDMA)
                        {
                        resObj.type = EDMA3_RM_RES_QDMA_CHANNEL;
                        resObj.resId = (channelId - EDMA3_DRV_QDMA_CH_MIN_VAL);
                        result = EDMA3_RM_freeResource(drvInst->resMgrInstance,
                                                    (EDMA3_RM_ResDesc *)&resObj);
                        }
                    else
                        {
                        resObj.type = EDMA3_RM_RES_DMA_CHANNEL;
                        resObj.resId = channelId;
                        result = EDMA3_RM_freeResource(drvInst->resMgrInstance,
                                                    (EDMA3_RM_ResDesc *)&resObj);
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


/**
 *  \brief  Clears Event Register and Error Register for a specific
 *  DMA channel and brings back EDMA3 to its initial state.
 *
 *  This API clears the Event register, Event Miss register Event Enable
 *  register for a specific DMA channel. It also clears the CC Error register.
 *
 *  \param  hEdma            [IN]     Handle to the EDMA Driver Instance.
 *  \param  channelId        [IN]     DMA Channel needs to be cleaned.
 *
 *  \return EDMA3_DRV_SOK or EDMA3_DRV Error code
 *
 *  \note    This function is re-entrant for unique channelId values. It is non-
 *          re-entrant for same channelId value.
 */
EDMA3_DRV_Result EDMA3_DRV_clearErrorBits (EDMA3_DRV_Handle hEdma,
                                                unsigned int channelId)
{
    EDMA3_DRV_Result result = EDMA3_DRV_SOK;
    EDMA3_DRV_Instance *drvInst = NULL;
    EDMA3_DRV_Object *drvObject = NULL;
    volatile EDMA3_CCRL_Regs *globalRegs = NULL;
    unsigned int count;
    unsigned int value = 0;

#ifdef EDMA3_INSTRUMENTATION_ENABLED
    EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3",
                EDMA3_DVT_DESC(EDMA3_DVT_eFUNC_START,
                EDMA3_DVT_dCOUNTER,
                EDMA3_DVT_dNONE,
                EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */

	/* If parameter checking is enabled... */
#ifndef EDMA3_DRV_PARAM_CHECK_DISABLE
    if (hEdma == NULL)
        {
        result = EDMA3_DRV_E_INVALID_PARAM;
        }
#endif

	/* Check if the parameters are OK. */
	if (EDMA3_DRV_SOK == result)
        {
        drvInst = (EDMA3_DRV_Instance *)hEdma;
        drvObject = drvInst->pDrvObjectHandle;

        if ((drvObject == NULL) || (drvObject->gblCfgParams.globalRegs == NULL))
            {
            result = EDMA3_DRV_E_INVALID_PARAM;
            }
        else
            {
			/* If parameter checking is enabled... */
#ifndef EDMA3_DRV_PARAM_CHECK_DISABLE
            if (channelId > EDMA3_DRV_DMA_CH_MAX_VAL)
                {
                result = EDMA3_DRV_E_INVALID_PARAM;
                }
#endif

			/* Check if the parameters are OK. */
			if (EDMA3_DRV_SOK == result)
                {
                globalRegs = (volatile EDMA3_CCRL_Regs *)(drvObject->gblCfgParams.globalRegs);

#ifdef EDMA3_DRV_DEBUG
            EDMA3_DRV_PRINTF("EMR =%l\r\n", globalRegs->EMR);
#endif
                if(channelId < 32u)
                    {
                    /* Clear the Event Register */
                    drvInst->shadowRegs->ECR = (1u << channelId);
                    /* Write to EMCR to clear the corresponding EMR bit*/
                    globalRegs->EMCR = (1u << channelId);
                    /* Clear any SER*/
                    drvInst->shadowRegs->SECR = (1u << channelId);
                    /* Clear any EER */
                    drvInst->shadowRegs->EECR = (1u << channelId);
                    }
                else
                    {
#ifdef EDMA3_DRV_DEBUG
                    EDMA3_DRV_PRINTF("EMRH =%l\r\n", globalRegs->EMRH);
#endif
                    /* Clear the Event Register */
                    drvInst->shadowRegs->ECRH = (1u << (channelId - 32u));
                    /* Write to EMCR to clear the corresponding EMR bit*/
                    globalRegs->EMCRH = (1u << (channelId - 32u));
                    /* Clear any SER*/
                    drvInst->shadowRegs->SECRH = (1u << (channelId - 32u));
                    /* Clear any EER */
                    drvInst->shadowRegs->EECRH = (1u << (channelId - 32u));
                    }

                /* Clear the global CC Error Register */
                for (count = 0; count < drvObject->gblCfgParams.numEvtQueue; count++)
                    {
                    value |= (1u << count);
                    }

                globalRegs->CCERRCLR = (EDMA3_CCRL_CCERR_TCCERR_MASK | value);
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
 * \brief   Set a particular OPT field in the PaRAM set associated with the
 *          logical channel 'lCh'.
 *
 * This API can be used to set various optional parameters for an EDMA3
 * transfer. Like enable/disable completion interrupts, enable/disable chaining,
 * setting the transfer mode (A/AB Sync), setting the FIFO width etc.
 *
 * \param   hEdma               [IN]        Handle to the EDMA Driver Instance.
 * \param   lCh                 [IN]        Logical Channel, bound to which
 *                                          PaRAM set OPT field needs to be set.
 * \param   optField            [IN]        The particular field of OPT Word
 *                                          that needs setting
 * \param   newOptFieldVal      [IN]        The new OPT field value
 *
 * \return      EDMA3_DRV_SOK or EDMA3_DRV Error Code
 *
 * \note    This function is re-entrant for unique lCh values. It is non-
 *          re-entrant for same lCh value.
 */
EDMA3_DRV_Result EDMA3_DRV_setOptField (EDMA3_DRV_Handle hEdma,
                    unsigned int lCh,
                    EDMA3_DRV_OptField optField,
                    unsigned int newOptFieldVal)
    {
    EDMA3_DRV_Result result = EDMA3_DRV_SOK;
    EDMA3_DRV_Instance *drvInst = NULL;
    EDMA3_DRV_Object *drvObject = NULL;
    unsigned int newOptVal = 0;
    unsigned int oldOptVal = 0;
    int paRAMId = 0;
    volatile EDMA3_CCRL_Regs *globalRegs = NULL;

#ifdef EDMA3_INSTRUMENTATION_ENABLED
    EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3",
                EDMA3_DVT_DESC(EDMA3_DVT_eFUNC_START,
                EDMA3_DVT_dCOUNTER,
                EDMA3_DVT_dNONE,
                EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */

	/* If parameter checking is enabled... */
#ifndef EDMA3_DRV_PARAM_CHECK_DISABLE
    if (((lCh > EDMA3_DRV_LOG_CH_MAX_VAL) || (hEdma == NULL))
        || ((optField < EDMA3_DRV_OPT_FIELD_SAM)
        || (optField > EDMA3_DRV_OPT_FIELD_ITCCHEN)))
        {
        result = EDMA3_DRV_E_INVALID_PARAM;
        }
#endif

	/* Check if the parameters are OK. */
	if (EDMA3_DRV_SOK == result)
        {
        drvInst = (EDMA3_DRV_Instance *)hEdma;
        drvObject = drvInst->pDrvObjectHandle;

        if ((drvObject == NULL) || (drvObject->gblCfgParams.globalRegs == NULL))
            {
            result = EDMA3_DRV_E_INVALID_PARAM;
            }
        else
            {
            paRAMId = edma3DrvChBoundRes[drvObject->phyCtrllerInstId][lCh].paRAMId;
            globalRegs = (volatile EDMA3_CCRL_Regs *)(drvObject->gblCfgParams.globalRegs);

            if ((paRAMId < 0) || ((unsigned int)paRAMId >= drvObject->gblCfgParams.numPaRAMSets))
                {
                result = EDMA3_DRV_E_INVALID_PARAM;
                }

            if (result == EDMA3_DRV_SOK)
                {
                oldOptVal = (unsigned int)(*(&globalRegs->PARAMENTRY [paRAMId].OPT));

                switch (optField)
                    {
                    case EDMA3_DRV_OPT_FIELD_SAM :
                        newOptVal = (oldOptVal & EDMA3_DRV_OPT_SAM_CLR_MASK)
                                    |
                                    (EDMA3_DRV_OPT_SAM_SET_MASK(newOptFieldVal));
                        break;
                    case EDMA3_DRV_OPT_FIELD_DAM :
                        newOptVal = (oldOptVal & EDMA3_DRV_OPT_DAM_CLR_MASK)
                                    |
                                    (EDMA3_DRV_OPT_DAM_SET_MASK(newOptFieldVal));
                        break;
                    case EDMA3_DRV_OPT_FIELD_SYNCDIM :
                        newOptVal = (oldOptVal & EDMA3_DRV_OPT_SYNCDIM_CLR_MASK)
                                    |
                                    (EDMA3_DRV_OPT_SYNCDIM_SET_MASK(newOptFieldVal));
                        break;
                    case EDMA3_DRV_OPT_FIELD_STATIC :
                        newOptVal = (oldOptVal & EDMA3_DRV_OPT_STATIC_CLR_MASK)
                                    |
                                    (EDMA3_DRV_OPT_STATIC_SET_MASK(newOptFieldVal));
                        break;
                    case EDMA3_DRV_OPT_FIELD_FWID :
                        newOptVal = (oldOptVal & EDMA3_DRV_OPT_FWID_CLR_MASK)
                                    |
                                    (EDMA3_DRV_OPT_FWID_SET_MASK(newOptFieldVal));
                        break;
                    case EDMA3_DRV_OPT_FIELD_TCCMODE :
                        newOptVal = (oldOptVal & EDMA3_DRV_OPT_TCCMODE_CLR_MASK)
                                    |
                                    (EDMA3_DRV_OPT_TCCMODE_SET_MASK(newOptFieldVal));
                        break;
                    case EDMA3_DRV_OPT_FIELD_TCC :
                        newOptVal = (oldOptVal & EDMA3_DRV_OPT_TCC_CLR_MASK)
                                    |
                                    (EDMA3_DRV_OPT_TCC_SET_MASK(newOptFieldVal));
                        break;
                    case EDMA3_DRV_OPT_FIELD_TCINTEN :
                        newOptVal = (oldOptVal & EDMA3_DRV_OPT_TCINTEN_CLR_MASK)
                                    |
                                    (EDMA3_DRV_OPT_TCINTEN_SET_MASK(newOptFieldVal));
                        break;
                    case EDMA3_DRV_OPT_FIELD_ITCINTEN :
                        newOptVal = (oldOptVal & EDMA3_DRV_OPT_ITCINTEN_CLR_MASK)
                                    |
                                    (EDMA3_DRV_OPT_ITCINTEN_SET_MASK(newOptFieldVal));
                        break;
                    case EDMA3_DRV_OPT_FIELD_TCCHEN :
                        newOptVal = (oldOptVal & EDMA3_DRV_OPT_TCCHEN_CLR_MASK)
                                    |
                                    (EDMA3_DRV_OPT_TCCHEN_SET_MASK(newOptFieldVal));
                        break;
                    case EDMA3_DRV_OPT_FIELD_ITCCHEN :
                        newOptVal = (oldOptVal & EDMA3_DRV_OPT_ITCCHEN_CLR_MASK)
                                    |
                                    (EDMA3_DRV_OPT_ITCCHEN_SET_MASK(newOptFieldVal));
                        break;
                    default:
                        break;
                    }

                *(&globalRegs->PARAMENTRY[paRAMId].OPT) = newOptVal;
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
 * \brief   Get a particular OPT field in the PaRAM set associated with the
 *          logical channel 'lCh'.
 *
 * This API can be used to read various optional parameters for an EDMA3
 * transfer. Like enable/disable completion interrupts, enable/disable chaining,
 * setting the transfer mode (A/AB Sync), setting the FIFO width etc.
 *
 * \param   hEdma               [IN]        Handle to the EDMA Driver Instance.
 * \param   lCh                 [IN]        Logical Channel, bound to which
 *                                          PaRAM set OPT field is required.
 * \param   optField            [IN]        The particular field of OPT Word
 *                                          that is needed
 * \param   optFieldVal         [IN/OUT]    Value of the OPT field
 *
 * \return  EDMA3_DRV_SOK or EDMA3_DRV Error Code
 *
 * \note    This function is re-entrant.
 */
EDMA3_DRV_Result EDMA3_DRV_getOptField (EDMA3_DRV_Handle hEdma,
                    unsigned int lCh,
                    EDMA3_DRV_OptField optField,
                    unsigned int *optFieldVal)
    {
    EDMA3_DRV_Result result = EDMA3_DRV_SOK;
    EDMA3_DRV_Instance *drvInst = NULL;
    EDMA3_DRV_Object *drvObject = NULL;
    unsigned int optVal = 0;
    int paRAMId = 0;
    volatile EDMA3_CCRL_Regs *globalRegs = NULL;

#ifdef EDMA3_INSTRUMENTATION_ENABLED
    EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3",
                EDMA3_DVT_DESC(EDMA3_DVT_eFUNC_START,
                EDMA3_DVT_dCOUNTER,
                EDMA3_DVT_dNONE,
                EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */

	/* If parameter checking is enabled... */
#ifndef EDMA3_DRV_PARAM_CHECK_DISABLE
    if (((lCh > EDMA3_DRV_LOG_CH_MAX_VAL)
        || ((hEdma == NULL) || (optFieldVal == NULL)))
        || ((optField < EDMA3_DRV_OPT_FIELD_SAM)
        || (optField > EDMA3_DRV_OPT_FIELD_ITCCHEN)))
        {
        result = EDMA3_DRV_E_INVALID_PARAM;
        }
#endif

	/* Check if the parameters are OK. */
	if (EDMA3_DRV_SOK == result)
        {
        drvInst = (EDMA3_DRV_Instance *)hEdma;
        drvObject = drvInst->pDrvObjectHandle;

        if ((drvObject == NULL) || (drvObject->gblCfgParams.globalRegs == NULL))
            {
            result = EDMA3_DRV_E_INVALID_PARAM;
            }
        else
            {
            paRAMId = edma3DrvChBoundRes[drvObject->phyCtrllerInstId][lCh].paRAMId;
            globalRegs = (volatile EDMA3_CCRL_Regs *)(drvObject->gblCfgParams.globalRegs);

            if ((paRAMId < 0) || ((unsigned int)paRAMId >= drvObject->gblCfgParams.numPaRAMSets))
                {
                result = EDMA3_DRV_E_INVALID_PARAM;
                }

            if (result == EDMA3_DRV_SOK)
                {
                optVal = (unsigned int)(*(&globalRegs->PARAMENTRY [paRAMId].OPT));

                switch (optField)
                    {
                    case EDMA3_DRV_OPT_FIELD_SAM :
                        *optFieldVal = EDMA3_DRV_OPT_SAM_GET_MASK(optVal);
                        break;
                    case EDMA3_DRV_OPT_FIELD_DAM :
                        *optFieldVal = EDMA3_DRV_OPT_DAM_GET_MASK(optVal);
                        break;
                    case EDMA3_DRV_OPT_FIELD_SYNCDIM :
                        *optFieldVal = EDMA3_DRV_OPT_SYNCDIM_GET_MASK(optVal);
                        break;
                    case EDMA3_DRV_OPT_FIELD_STATIC :
                        *optFieldVal = EDMA3_DRV_OPT_STATIC_GET_MASK(optVal);
                        break;
                    case EDMA3_DRV_OPT_FIELD_FWID :
                        *optFieldVal = EDMA3_DRV_OPT_FWID_GET_MASK(optVal);
                        break;
                    case EDMA3_DRV_OPT_FIELD_TCCMODE :
                        *optFieldVal = EDMA3_DRV_OPT_TCCMODE_GET_MASK(optVal);
                        break;
                    case EDMA3_DRV_OPT_FIELD_TCC :
                        *optFieldVal = EDMA3_DRV_OPT_TCC_GET_MASK(optVal);
                        break;
                    case EDMA3_DRV_OPT_FIELD_TCINTEN :
                        *optFieldVal = EDMA3_DRV_OPT_TCINTEN_GET_MASK(optVal);
                        break;
                    case EDMA3_DRV_OPT_FIELD_ITCINTEN :
                        *optFieldVal = EDMA3_DRV_OPT_ITCINTEN_GET_MASK(optVal);
                        break;
                    case EDMA3_DRV_OPT_FIELD_TCCHEN :
                        *optFieldVal = EDMA3_DRV_OPT_TCCHEN_GET_MASK(optVal);
                        break;
                    case EDMA3_DRV_OPT_FIELD_ITCCHEN :
                        *optFieldVal = EDMA3_DRV_OPT_ITCCHEN_GET_MASK(optVal);
                        break;
                    default:
                        break;
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



/**
 * \brief  DMA source parameters setup
 *
 * It is used to program the source address, source side addressing mode
 * (INCR or FIFO) and the FIFO width in case the addressing mode is FIFO.
 *
 * In FIFO Addressing mode, memory location must be 32 bytes aligned.
 *
 * \param   hEdma       [IN]    Handle to the EDMA Driver Instance
 * \param   lCh         [IN]    Logical Channel for which the source parameters
 *                              are to be configured
 * \param   srcAddr     [IN]    Source address
 * \param   addrMode    [IN]    Address mode [FIFO or Increment]
 * \param   fifoWidth   [IN]    Width of FIFO (Valid only if addrMode is FIFO)
 *                                  -# 0 - 8 bit
 *                                  -# 1 - 16 bit
 *                                  -# 2 - 32 bit
 *                                  -# 3 - 64 bit
 *                                  -# 4 - 128 bit
 *                                  -# 5 - 256 bit
 *
 * \return      EDMA3_DRV_SOK or EDMA3_DRV Error Code
 *
 * \note    This function is re-entrant for unique lCh values. It is non-
 *          re-entrant for same lCh value.
 */
EDMA3_DRV_Result EDMA3_DRV_setSrcParams (EDMA3_DRV_Handle hEdma,
                    unsigned int lCh,
                    unsigned int srcAddr,
                    EDMA3_DRV_AddrMode addrMode,
                    EDMA3_DRV_FifoWidth fifoWidth)
    {
    EDMA3_DRV_Result result = EDMA3_DRV_SOK;
    unsigned int opt = 0;
    EDMA3_DRV_Instance *drvInst = NULL;
    EDMA3_DRV_Object *drvObject = NULL;
    int paRAMId = 0;
    volatile EDMA3_CCRL_Regs *globalRegs = NULL;
    unsigned int mappedEvtQ = 0;
    unsigned int defaultBurstSize = 0;

    OALMSG(ZONE_INIT,(_T("+++EDMA3_DRV_setSrcParams: ch %d src 0x%X addrMode %d wid %d\r\n"), 
        lCh, srcAddr, addrMode, fifoWidth
             ));

#ifdef EDMA3_INSTRUMENTATION_ENABLED
    EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3",
                EDMA3_DVT_DESC(EDMA3_DVT_eFUNC_START,
                EDMA3_DVT_dCOUNTER,
                EDMA3_DVT_dNONE,
                EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */

	/* If parameter checking is enabled... */
#ifndef EDMA3_DRV_PARAM_CHECK_DISABLE
    if ((((lCh > EDMA3_DRV_LOG_CH_MAX_VAL) || (hEdma == NULL))
        || ((addrMode < EDMA3_DRV_ADDR_MODE_INCR) || (addrMode > EDMA3_DRV_ADDR_MODE_FIFO)))
        || ((fifoWidth < EDMA3_DRV_W8BIT) || (fifoWidth > EDMA3_DRV_W256BIT)))
        {
        result = EDMA3_DRV_E_INVALID_PARAM;
        }

    /** In FIFO Addressing mode, memory location must be 32 bytes aligned */
    if ((addrMode == EDMA3_DRV_ADDR_MODE_FIFO)
        && ((void *)(srcAddr & 0x1Fu) != NULL))
        {
        /** Memory is not 32 bytes aligned */
        result = EDMA3_DRV_E_ADDRESS_NOT_ALIGNED;
        }
#endif

	/* Check if the parameters are OK. */
	if (EDMA3_DRV_SOK == result)
        {
        drvInst = (EDMA3_DRV_Instance *)hEdma;
        drvObject = drvInst->pDrvObjectHandle;

        if ((drvObject == NULL) || (drvObject->gblCfgParams.globalRegs == NULL))
            {
            result =  EDMA3_DRV_E_INVALID_PARAM;
            }
        else
            {
            paRAMId = edma3DrvChBoundRes[drvObject->phyCtrllerInstId][lCh].paRAMId;
            globalRegs = (volatile EDMA3_CCRL_Regs *)(drvObject->gblCfgParams.globalRegs);

            if ((paRAMId < 0) || ((unsigned int)paRAMId >= drvObject->gblCfgParams.numPaRAMSets))
                {
                result = EDMA3_DRV_E_INVALID_PARAM;
                }

            if (result == EDMA3_DRV_SOK)
                {
                /**
                  * If request is for FIFO mode, check whether the FIFO size
                  * is supported by the Transfer Controller which will be used for
                  * this transfer or not.
                  */
                if (addrMode == EDMA3_DRV_ADDR_MODE_FIFO)
                    {
                    if (lCh <= EDMA3_DRV_DMA_CH_MAX_VAL)
                        {
                        mappedEvtQ = ((globalRegs->DMAQNUM[lCh >> 3u])
                                        & (~(EDMA3_DRV_DMAQNUM_CLR_MASK(lCh))))
                                          >> ((lCh%8u)*4u);
                        }
                    else
                        {
                        if ((lCh >= EDMA3_DRV_QDMA_CH_MIN_VAL)
                             &&(lCh <= EDMA3_DRV_QDMA_CH_MAX_VAL))
                            {
                            mappedEvtQ = ((globalRegs->QDMAQNUM)
                                            & (~(EDMA3_DRV_QDMAQNUM_CLR_MASK(lCh -EDMA3_DRV_QDMA_CH_MIN_VAL))))
                                           >> (lCh*4u);
                            }
                        }

                    /**
                       * mappedEvtQ contains the event queue and hence the TC which will
                       * process this transfer request. Check whether this TC supports the
                       * FIFO size or not.
                       */
                    defaultBurstSize = 1u << fifoWidth;
                    if (defaultBurstSize > drvObject->gblCfgParams.tcDefaultBurstSize[mappedEvtQ])
                        {
                        result = EDMA3_DRV_E_FIFO_WIDTH_NOT_SUPPORTED;
                        }
                    }

                if (EDMA3_DRV_SOK == result)
                    {
                    /* Set Src Address */
                    *((&globalRegs->PARAMENTRY[paRAMId].OPT) +
                                (unsigned int)EDMA3_DRV_PARAM_ENTRY_SRC) = srcAddr;

                    opt = (unsigned int)(*(&globalRegs->PARAMENTRY [paRAMId].OPT));

                    /* Set SAM */
                    opt &= EDMA3_DRV_OPT_SAM_CLR_MASK;
                    opt |= EDMA3_DRV_OPT_SAM_SET_MASK(addrMode);
                    /* Set FIFO Width */
                    opt &= EDMA3_DRV_OPT_FWID_CLR_MASK;
                    opt |= EDMA3_DRV_OPT_FWID_SET_MASK(fifoWidth);

                    /* Set the OPT */
                    *(&globalRegs->PARAMENTRY[paRAMId].OPT) = opt;

                    //+++ ShowPaRAM(globalRegs, paRAMId, 0xff);

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



/**
 * \brief  DMA Destination parameters setup
 *
 * It is used to program the destination address, destination side addressing
 * mode (INCR or FIFO) and the FIFO width in case the addressing mode is FIFO.
 *
 * In FIFO Addressing mode, memory location must be 32 bytes aligned.
 *
 * \param   hEdma       [IN]    Handle to the EDMA Driver Instance
 * \param   lCh         [IN]    Logical Channel for which the destination
 *                              parameters are to be configured
 * \param   destAddr    [IN]    Destination address
 * \param   addrMode    [IN]    Address mode [FIFO or Increment]
 * \param   fifoWidth   [IN]    Width of FIFO (Valid only if addrMode is FIFO)
 *                                  -# 0 - 8 bit
 *                                  -# 1 - 16 bit
 *                                  -# 2 - 32 bit
 *                                  -# 3 - 64 bit
 *                                  -# 4 - 128 bit
 *                                  -# 5 - 256 bit
 *
 * \return      EDMA3_DRV_SOK or EDMA3_DRV Error Code
 *
 * \note    This function is re-entrant for unique lCh values. It is non-
 *          re-entrant for same lCh value.
 */
EDMA3_DRV_Result EDMA3_DRV_setDestParams (EDMA3_DRV_Handle hEdma,
                    unsigned int lCh,
                    unsigned int destAddr,
                    EDMA3_DRV_AddrMode addrMode,
                    EDMA3_DRV_FifoWidth fifoWidth)
    {
    EDMA3_DRV_Result result = EDMA3_DRV_SOK;
    unsigned int opt = 0;
    EDMA3_DRV_Instance *drvInst = NULL;
    EDMA3_DRV_Object *drvObject = NULL;
    int paRAMId = 0;
    volatile EDMA3_CCRL_Regs *globalRegs = NULL;
    unsigned int mappedEvtQ = 0;
    unsigned int defaultBurstSize = 0;

#ifdef EDMA3_INSTRUMENTATION_ENABLED
    EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3",
                EDMA3_DVT_DESC(EDMA3_DVT_eFUNC_START,
                EDMA3_DVT_dCOUNTER,
                EDMA3_DVT_dNONE,
                EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */

	/* If parameter checking is enabled... */
#ifndef EDMA3_DRV_PARAM_CHECK_DISABLE
    if ((((lCh > EDMA3_DRV_LOG_CH_MAX_VAL) || (hEdma == NULL))
        || ((addrMode < EDMA3_DRV_ADDR_MODE_INCR) || (addrMode > EDMA3_DRV_ADDR_MODE_FIFO)))
        || ((fifoWidth < EDMA3_DRV_W8BIT) || (fifoWidth > EDMA3_DRV_W256BIT)))
        {
        result = EDMA3_DRV_E_INVALID_PARAM;
        }

    /** In FIFO Addressing mode, memory location must be 32 bytes aligned */
    if ((addrMode == EDMA3_DRV_ADDR_MODE_FIFO)
        && ((void *)(destAddr & 0x1Fu)!=NULL))
        {
        /** Memory is not 32 bytes aligned */
        result = EDMA3_DRV_E_ADDRESS_NOT_ALIGNED;
        }
#endif

	/* Check if the parameters are OK. */
	if (EDMA3_DRV_SOK == result)
        {
        drvInst = (EDMA3_DRV_Instance *)hEdma;
        drvObject = drvInst->pDrvObjectHandle;

        if ((drvObject == NULL) || (drvObject->gblCfgParams.globalRegs == NULL))
            {
            result =  EDMA3_DRV_E_INVALID_PARAM;
            }
        else
            {
            paRAMId = edma3DrvChBoundRes[drvObject->phyCtrllerInstId][lCh].paRAMId;
            globalRegs = (volatile EDMA3_CCRL_Regs *)(drvObject->gblCfgParams.globalRegs);

            if ((paRAMId < 0) || ((unsigned int)paRAMId >= drvObject->gblCfgParams.numPaRAMSets))
                {
                result = EDMA3_DRV_E_INVALID_PARAM;
                }

            if (result == EDMA3_DRV_SOK)
                {
                /**
                  * If request is for FIFO mode, check whether the FIFO size
                  * is supported by the Transfer Controller which will be used for
                  * this transfer or not.
                  */
                if (addrMode == EDMA3_DRV_ADDR_MODE_FIFO)
                    {
                    if (lCh <= EDMA3_DRV_DMA_CH_MAX_VAL)
                        {
                        mappedEvtQ = ((globalRegs->DMAQNUM[lCh >> 3u])
                                        & (~(EDMA3_DRV_DMAQNUM_CLR_MASK(lCh))))
                                          >> ((lCh%8u)*4u);
                        }
                    else
                        {
                        if ((lCh >= EDMA3_DRV_QDMA_CH_MIN_VAL)
                             &&(lCh <= EDMA3_DRV_QDMA_CH_MAX_VAL))
                            {
                            mappedEvtQ = ((globalRegs->QDMAQNUM)
                                            & (~(EDMA3_DRV_QDMAQNUM_CLR_MASK(lCh -EDMA3_DRV_QDMA_CH_MIN_VAL))))
                                           >> (lCh*4u);
                            }
                        }

                    /**
                       * mappedEvtQ contains the event queue and hence the TC which will
                       * process this transfer request. Check whether this TC supports the
                       * FIFO size or not.
                       */
                    defaultBurstSize = 1u << fifoWidth;
                    if (defaultBurstSize > drvObject->gblCfgParams.tcDefaultBurstSize[mappedEvtQ])
                        {
                        result = EDMA3_DRV_E_FIFO_WIDTH_NOT_SUPPORTED;
                        }
                    }

                if (EDMA3_DRV_SOK == result)
                    {
                    /* Set the Dest address */
                    *((&globalRegs->PARAMENTRY[paRAMId].OPT) +
                                (unsigned int)EDMA3_DRV_PARAM_ENTRY_DST) = destAddr;

                    opt = (unsigned int)(*(&globalRegs->PARAMENTRY [paRAMId].OPT));

                    /* Set DAM */
                    opt &= EDMA3_DRV_OPT_DAM_CLR_MASK;
                    opt |= EDMA3_DRV_OPT_DAM_SET_MASK(addrMode);
                    /* Set FIFO Width */
                    opt &= EDMA3_DRV_OPT_FWID_CLR_MASK;
                    opt |= EDMA3_DRV_OPT_FWID_SET_MASK(fifoWidth);

                    /* Set the OPT */
                    *(&globalRegs->PARAMENTRY[paRAMId].OPT) = opt;

                    //+++ ShowPaRAM(globalRegs, paRAMId, 0xff);
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



/**
 * \brief   DMA source index setup
 *
 * It is used to program the source B index and source C index.
 *
 * SRCBIDX is a 16-bit signed value (2s complement) used for source address
 * modification between each array in the 2nd dimension. Valid values for
 * SRCBIDX are between -32768 and 32767. It provides a byte address offset
 * from the beginning of the source array to the beginning of the next source
 * array. It applies to both A-synchronized and AB-synchronized transfers.
 *
 * SRCCIDX is a 16-bit signed value (2s complement) used for source address
 * modification in the 3rd dimension. Valid values for SRCCIDX are between
 * -32768 and 32767. It provides a byte address offset from the beginning of
 * the current array (pointed to by SRC address) to the beginning of the first
 * source array in the next frame. It applies to both A-synchronized and
 * AB-synchronized transfers. Note that when SRCCIDX is applied, the current
 * array in an A-synchronized transfer is the last array in the frame, while
 * the current array in an AB-synchronized transfer is the first array in the
 * frame.
 *
 * \param   hEdma           [IN]            Handle to the EDMA Driver Instance
 * \param   lCh             [IN]            Logical Channel for which source
 *                                          indices are to be configured
 * \param   srcBIdx         [IN]            Source B index
 * \param   srcCIdx         [IN]            Source C index
 *
 * \return      EDMA3_DRV_SOK or EDMA3_DRV Error Code
 *
 * \note    This function is re-entrant for unique lCh values. It is non-
 *          re-entrant for same lCh value.
 */
EDMA3_DRV_Result EDMA3_DRV_setSrcIndex (EDMA3_DRV_Handle hEdma,
                    unsigned int lCh,
                    int srcBIdx, int srcCIdx)
    {
    EDMA3_DRV_Result result = EDMA3_DRV_SOK;
    unsigned int srcDstBidx;
    unsigned int srcDstCidx;
    EDMA3_DRV_Instance *drvInst = NULL;
    EDMA3_DRV_Object *drvObject = NULL;
    int paRAMId = 0;
    volatile EDMA3_CCRL_Regs *globalRegs = NULL;

#ifdef EDMA3_INSTRUMENTATION_ENABLED
    EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3",
                EDMA3_DVT_DESC(EDMA3_DVT_eFUNC_START,
                EDMA3_DVT_dCOUNTER,
                EDMA3_DVT_dNONE,
                EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */

	/* If parameter checking is enabled... */
#ifndef EDMA3_DRV_PARAM_CHECK_DISABLE
    if ((lCh > EDMA3_DRV_LOG_CH_MAX_VAL) || (hEdma == NULL))
        {
        result = EDMA3_DRV_E_INVALID_PARAM;
        }

    if (((srcBIdx > EDMA3_DRV_SRCBIDX_MAX_VAL)
        || (srcBIdx < EDMA3_DRV_SRCBIDX_MIN_VAL))
        || ((srcCIdx > EDMA3_DRV_SRCCIDX_MAX_VAL)
        || (srcCIdx < EDMA3_DRV_SRCCIDX_MIN_VAL)))
        {
        result = EDMA3_DRV_E_INVALID_PARAM;
        }
#endif

	/* Check if the parameters are OK. */
	if (EDMA3_DRV_SOK == result)
        {
        drvInst = (EDMA3_DRV_Instance *)hEdma;
        drvObject = drvInst->pDrvObjectHandle;

        if ((drvObject == NULL) || (drvObject->gblCfgParams.globalRegs == NULL))
            {
            result =  EDMA3_DRV_E_INVALID_PARAM;
            }
        else
            {
            paRAMId = edma3DrvChBoundRes[drvObject->phyCtrllerInstId][lCh].paRAMId;
            globalRegs = (volatile EDMA3_CCRL_Regs *)(drvObject->gblCfgParams.globalRegs);

            if ((paRAMId < 0) || ((unsigned int)paRAMId >= drvObject->gblCfgParams.numPaRAMSets))
                {
                result = EDMA3_DRV_E_INVALID_PARAM;
                }

            if (result == EDMA3_DRV_SOK)
                {
                /* Get SrcDestBidx PaRAM Set entry */
                srcDstBidx = (unsigned int)(*((&globalRegs->PARAMENTRY [paRAMId].OPT) +
                                                        (unsigned int)EDMA3_DRV_PARAM_ENTRY_SRC_DST_BIDX));

                srcDstBidx &= 0xFFFF0000u;
                /* Update it */
                srcDstBidx |= (unsigned int)(srcBIdx & 0xFFFF);

                /* Store it back */
                *((&globalRegs->PARAMENTRY[paRAMId].OPT)
                            + (unsigned int)EDMA3_DRV_PARAM_ENTRY_SRC_DST_BIDX) = srcDstBidx;

                /* Get SrcDestCidx PaRAM Set entry */
                srcDstCidx = (unsigned int)(*((&globalRegs->PARAMENTRY [paRAMId].OPT) +
                                                        (unsigned int)EDMA3_DRV_PARAM_ENTRY_SRC_DST_CIDX));

                srcDstCidx &= 0xFFFF0000u;
                /* Update it */
                srcDstCidx |= (unsigned int)(srcCIdx & 0xFFFF);

                /* Store it back */
                *((&globalRegs->PARAMENTRY[paRAMId].OPT)
                            + (unsigned int)EDMA3_DRV_PARAM_ENTRY_SRC_DST_CIDX) = srcDstCidx;

                //+++ ShowPaRAM(globalRegs, paRAMId, 0xff);

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
 * \brief   DMA destination index setup
 *
 * It is used to program the destination B index and destination C index.
 *
 * DSTBIDX is a 16-bit signed value (2s complement) used for destination
 * address modification between each array in the 2nd dimension. Valid values
 * for DSTBIDX are between -32768 and 32767. It provides a byte address offset
 * from the beginning of the destination array to the beginning of the next
 * destination array within the current frame. It applies to both
 * A-synchronized and AB-synchronized transfers.
 *
 * DSTCIDX is a 16-bit signed value (2s complement) used for destination address
 * modification in the 3rd dimension. Valid values are between -32768 and 32767.
 * It provides a byte address offset from the beginning of the current array
 * (pointed to by DST address) to the beginning of the first destination array
 * TR in the next frame. It applies to both A-synchronized and AB-synchronized
 * transfers. Note that when DSTCIDX is applied, the current array in an
 * A-synchronized transfer is the last array in the frame, while the current
 * array in a AB-synchronized transfer is the first array in the frame
 *
 * \param   hEdma           [IN]            Handle to the EDMA Driver Instance
 * \param   lCh             [IN]            Logical Channel for which dest
 *                                          indices are to be configured
 * \param   destBIdx        [IN]            Destination B index
 * \param   destCIdx        [IN]            Destination C index
 *
 * \return      EDMA3_DRV_SOK or EDMA3_DRV Error Code
 *
 * \note    This function is re-entrant for unique lCh values. It is non-
 *          re-entrant for same lCh value.
 */
EDMA3_DRV_Result  EDMA3_DRV_setDestIndex (EDMA3_DRV_Handle hEdma, unsigned int lCh,
                            int destBIdx, int destCIdx)
    {
    EDMA3_DRV_Result result = EDMA3_DRV_SOK;
    unsigned int srcDstBidx;
    unsigned int srcDstCidx;
    EDMA3_DRV_Instance *drvInst = NULL;
    EDMA3_DRV_Object *drvObject = NULL;
    int paRAMId = 0;
    volatile EDMA3_CCRL_Regs *globalRegs = NULL;

#ifdef EDMA3_INSTRUMENTATION_ENABLED
    EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3",
                EDMA3_DVT_DESC(EDMA3_DVT_eFUNC_START,
                EDMA3_DVT_dCOUNTER,
                EDMA3_DVT_dNONE,
                EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */

	/* If parameter checking is enabled... */
#ifndef EDMA3_DRV_PARAM_CHECK_DISABLE
    if ((lCh > EDMA3_DRV_LOG_CH_MAX_VAL) || (hEdma == NULL))
        {
        result = EDMA3_DRV_E_INVALID_PARAM;
        }

    if (((destBIdx > EDMA3_DRV_DSTBIDX_MAX_VAL)
        || (destBIdx < EDMA3_DRV_DSTBIDX_MIN_VAL))
        || ((destCIdx > EDMA3_DRV_DSTCIDX_MAX_VAL)
        || (destCIdx < EDMA3_DRV_DSTCIDX_MIN_VAL)))
        {
        result = EDMA3_DRV_E_INVALID_PARAM;
        }
#endif

	/* Check if the parameters are OK. */
	if (EDMA3_DRV_SOK == result)
        {
        drvInst = (EDMA3_DRV_Instance *)hEdma;
        drvObject = drvInst->pDrvObjectHandle;

        if ((drvObject == NULL) || (drvObject->gblCfgParams.globalRegs == NULL))
            {
            result =  EDMA3_DRV_E_INVALID_PARAM;
            }
        else
            {
            paRAMId = edma3DrvChBoundRes[drvObject->phyCtrllerInstId][lCh].paRAMId;
            globalRegs = (volatile EDMA3_CCRL_Regs *)(drvObject->gblCfgParams.globalRegs);

            if ((paRAMId < 0) || ((unsigned int)paRAMId >= drvObject->gblCfgParams.numPaRAMSets))
                {
                result = EDMA3_DRV_E_INVALID_PARAM;
                }

            if (result == EDMA3_DRV_SOK)
                {
                /* Get SrcDestBidx PaRAM Set entry */
                srcDstBidx = (unsigned int)(*((&globalRegs->PARAMENTRY [paRAMId].OPT) +
                                                        (unsigned int)EDMA3_DRV_PARAM_ENTRY_SRC_DST_BIDX));

                srcDstBidx &= 0xFFFFu;
                /* Update it */
                srcDstBidx |= (unsigned int)((destBIdx & 0xFFFF) << 16u);

                /* Store it back */
                *((&globalRegs->PARAMENTRY[paRAMId].OPT)
                            + (unsigned int)EDMA3_DRV_PARAM_ENTRY_SRC_DST_BIDX) = srcDstBidx;

                /* Get SrcDestCidx PaRAM Set entry */
                srcDstCidx = (unsigned int)(*((&globalRegs->PARAMENTRY [paRAMId].OPT) +
                                                        (unsigned int)EDMA3_DRV_PARAM_ENTRY_SRC_DST_CIDX));

                srcDstCidx &= 0xFFFFu;
                /* Update it */
                srcDstCidx |= (unsigned int)((destCIdx & 0xFFFF) << 16u);

                /* Store it back */
                *((&globalRegs->PARAMENTRY[paRAMId].OPT)
                            + (unsigned int)EDMA3_DRV_PARAM_ENTRY_SRC_DST_CIDX) = srcDstCidx;

                //+++ ShowPaRAM(globalRegs, paRAMId, 0xff);

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
 * \brief       DMA transfer parameters setup
 *
 * It is used to specify the various counts (ACNT, BCNT and CCNT), B count
 * reload and the synchronization type
 *
 * ACNT represents the number of bytes within the 1st dimension of a transfer.
 * ACNT is a 16-bit unsigned value with valid values between 0 and 65535.
 * Therefore, the maximum number of bytes in an array is 65535 bytes (64K - 1
 * bytes). ACNT must be greater than or equal to 1 for a TR to be submitted to
 * EDMA3 Transfer Controller.
 * An ACNT equal to 0 is considered either a null or dummy transfer. A dummy or
 * null transfer generates a completion code depending on the settings of the
 * completion bit fields in OPT.

 * BCNT is a 16-bit unsigned value that specifies the number of arrays of length
 * ACNT. For normal operation, valid values for BCNT are between 1 and 65535.
 * Therefore, the maximum number of arrays in a frame is 65535 (64K - 1 arrays).
 * A BCNT equal to 0 is considered either a null or dummy transfer. A dummy or
 * null transfer generates a completion code depending on the settings of the
 * completion bit fields in OPT.
 *
 * CCNT is a 16-bit unsigned value that specifies the number of frames in a
 * block. Valid values for CCNT are between 1 and 65535. Therefore, the maximum
 * number of frames in a block is 65535 (64K - 1 frames). A CCNT equal to 0 is
 * considered either a null or dummy transfer. A dummy or null transfer
 * generates a completion code depending on the settings of the completion bit
 * fields in OPT. A CCNT value of 0 is considered either a null or dummy
 * transfer.
 *
 * BCNTRLD is a 16-bit unsigned value used to reload the BCNT field once the
 * last array in the 2nd dimension is transferred. This field is only used for
 * A-synchronized transfers. In this case, the EDMA3CC decrements the BCNT
 * value by 1 on each TR submission. When BCNT (conceptually) reaches 0, the
 * EDMA3CC decrements CCNT and uses the BCNTRLD value to reinitialize the BCNT
 * value.
 * For AB-synchronized transfers, the EDMA3CC submits the BCNT in the TR and the
 * EDMA3TC decrements BCNT appropriately. For AB-synchronized transfers,
 * BCNTRLD is not used.

 * \param       hEdma           [IN]    Handle to the EDMA Driver Instance
 * \param       lCh             [IN]    Logical Channel for which transfer
 *                                      parameters are to be configured
 * \param       aCnt            [IN]    Count for 1st Dimension.
 * \param       bCnt            [IN]    Count for 2nd Dimension.
 * \param       cCnt            [IN]    Count for 3rd Dimension.
 * \param       bCntReload      [IN]    Reload value for bCnt.
 * \param       syncType        [IN]    Transfer synchronization dimension
 *                                      0: A-synchronized. Each event triggers
 *                                      the transfer of a single array of
 *                                      ACNT bytes.
 *                                      1: AB-synchronized. Each event triggers
 *                                      the transfer of BCNT arrays of ACNT
 *                                      bytes.
 *
 * \return      EDMA3_DRV_SOK or EDMA3_DRV Error Code
 *
 * \note    This function is re-entrant for unique lCh values. It is non-
 *          re-entrant for same lCh value.
 */
EDMA3_DRV_Result EDMA3_DRV_setTransferParams (EDMA3_DRV_Handle hEdma,
        unsigned int lCh, unsigned int aCnt, unsigned int bCnt, unsigned int cCnt,
        unsigned int bCntReload, EDMA3_DRV_SyncType syncType)
    {
    EDMA3_DRV_Result result = EDMA3_DRV_SOK;
    unsigned int abCnt = 0;
    unsigned int linkBCntReld = 0;
    unsigned int opt = 0;
    EDMA3_DRV_Instance *drvInst = NULL;
    EDMA3_DRV_Object *drvObject = NULL;
    int paRAMId = 0;
    volatile EDMA3_CCRL_Regs *globalRegs = NULL;

#ifdef EDMA3_INSTRUMENTATION_ENABLED
    EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3",
                EDMA3_DVT_DESC(EDMA3_DVT_eFUNC_START,
                EDMA3_DVT_dCOUNTER,
                EDMA3_DVT_dNONE,
                EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */

	/* If parameter checking is enabled... */
#ifndef EDMA3_DRV_PARAM_CHECK_DISABLE
    if ((lCh > EDMA3_DRV_LOG_CH_MAX_VAL) || (hEdma == NULL))
        {
        result = EDMA3_DRV_E_INVALID_PARAM;
        }

    if ((((aCnt > EDMA3_DRV_ACNT_MAX_VAL)
        || (bCnt > EDMA3_DRV_BCNT_MAX_VAL))
        || ((cCnt > EDMA3_DRV_CCNT_MAX_VAL)
        || (bCntReload > EDMA3_DRV_BCNTRELD_MAX_VAL)))
        || ((syncType < EDMA3_DRV_SYNC_A) || (syncType > EDMA3_DRV_SYNC_AB)))
        {
        result = EDMA3_DRV_E_INVALID_PARAM;
        }
#endif

	/* Check if the parameters are OK. */
	if (EDMA3_DRV_SOK == result)
        {
        drvInst = (EDMA3_DRV_Instance *)hEdma;
        drvObject = drvInst->pDrvObjectHandle;

        if ((drvObject == NULL) || (drvObject->gblCfgParams.globalRegs == NULL))
            {
            result =  EDMA3_DRV_E_INVALID_PARAM;
            }
        else
            {
            abCnt = aCnt | ((bCnt&0xFFFFu) << 16u);
            paRAMId = edma3DrvChBoundRes[drvObject->phyCtrllerInstId][lCh].paRAMId;
            globalRegs = (volatile EDMA3_CCRL_Regs *)(drvObject->gblCfgParams.globalRegs);

            if ((paRAMId < 0) || ((unsigned int)paRAMId >= drvObject->gblCfgParams.numPaRAMSets))
                {
                result = EDMA3_DRV_E_INVALID_PARAM;
                }

            if (result == EDMA3_DRV_SOK)
                {
                /* Set aCnt and bCnt */
                *((&globalRegs->PARAMENTRY[paRAMId].OPT)
                            + (unsigned int)EDMA3_DRV_PARAM_ENTRY_ACNT_BCNT) = abCnt;

                /* Set cCnt */
                *((&globalRegs->PARAMENTRY[paRAMId].OPT)
                            + (unsigned int)EDMA3_DRV_PARAM_ENTRY_CCNT) = cCnt;


                linkBCntReld = (unsigned int)(*((&globalRegs->PARAMENTRY [paRAMId].OPT) +
                                                        (unsigned int)EDMA3_DRV_PARAM_ENTRY_LINK_BCNTRLD));

                linkBCntReld |= ((bCntReload & 0xFFFFu) << 16u);

                /* Set bCntReload */
                *((&globalRegs->PARAMENTRY[paRAMId].OPT)
                            + (unsigned int)EDMA3_DRV_PARAM_ENTRY_LINK_BCNTRLD) = linkBCntReld;

                opt = (unsigned int)(*(&globalRegs->PARAMENTRY [paRAMId].OPT));

                /* Set Sync Type */
                opt &= EDMA3_DRV_OPT_SYNCDIM_CLR_MASK;
                opt |= EDMA3_DRV_OPT_SYNCDIM_SET_MASK(syncType);

                *(&globalRegs->PARAMENTRY[paRAMId].OPT) = opt;

                //+++ ShowPaRAM(globalRegs, paRAMId, 0xff);

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
 * \brief       Start EDMA transfer on the specified channel.
 *
 * There are multiple ways to trigger an EDMA3 transfer. The triggering mode
 * option allows choosing from the available triggering modes: Event,
 * Manual or QDMA.
 *
 * In event triggered, a peripheral or an externally generated event triggers
 * the transfer. This API clears the Event and Event Miss Register and then
 * enables the DMA channel by writing to the EESR.
 *
 * In manual triggered mode, CPU manually triggers a transfer by writing a 1
 * in the Event Set Register (ESR/ESRH). This API writes to the ESR/ESRH to
 * start the transfer.
 *
 * In QDMA triggered mode, a QDMA transfer is triggered when a CPU (or other
 * EDMA3 programmer) writes to the trigger word of the QDMA channel PaRAM set
 * (auto-triggered) or when the EDMA3CC performs a link update on a PaRAM set
 * that has been mapped to a QDMA channel (link triggered). This API enables
 * the QDMA channel by writing to the QEESR register.
 *
 * \param  hEdma        [IN]    Handle to the EDMA Driver Instance
 * \param  lCh          [IN]    Channel on which transfer has to be started
 * \param  trigMode     [IN]    Mode of triggering start of transfer (Manual,
 *                              QDMA or Event)
 *
 * \return      EDMA3_DRV_SOK or EDMA3_DRV Error Code
 *
 * \note    This function is re-entrant for unique lCh values. It is non-
 *          re-entrant for same lCh value.
 */
EDMA3_DRV_Result EDMA3_DRV_enableTransfer (EDMA3_DRV_Handle hEdma,
                                        unsigned int lCh,
                                        EDMA3_DRV_TrigMode trigMode)
    {
    EDMA3_DRV_Instance *drvInst = NULL;
    EDMA3_DRV_Object *drvObject = NULL;
    EDMA3_DRV_Result result = EDMA3_DRV_SOK;
    volatile EDMA3_CCRL_Regs *globalRegs = NULL;

#if 0
    unsigned int qevt_entry[4][16]; //+++
    int i, j;
#endif

#ifdef EDMA3_INSTRUMENTATION_ENABLED
    EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3",
                EDMA3_DVT_DESC(EDMA3_DVT_eFUNC_START,
                EDMA3_DVT_dCOUNTER,
                EDMA3_DVT_dNONE,
                EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */

	/* If parameter checking is enabled... */
#ifndef EDMA3_DRV_PARAM_CHECK_DISABLE
    if ((lCh > EDMA3_DRV_LOG_CH_MAX_VAL) || (hEdma == NULL))
        {
        result = EDMA3_DRV_E_INVALID_PARAM;
        }

    /* Trigger type is Manual */
    if ((EDMA3_DRV_TRIG_MODE_MANUAL == trigMode)
    	&& (lCh > EDMA3_DRV_DMA_CH_MAX_VAL))
        {
        /* Channel Id lies outside DMA channel range */
        result = EDMA3_DRV_E_INVALID_PARAM;
        }

    /* Trigger type is QDMA */
    if ((EDMA3_DRV_TRIG_MODE_QDMA == trigMode)
    	&& ((lCh < EDMA3_DRV_QDMA_CH_MIN_VAL)
    	|| (lCh > EDMA3_DRV_QDMA_CH_MAX_VAL)))
        {
        /* Channel Id lies outside QDMA channel range */
        result = EDMA3_DRV_E_INVALID_PARAM;
        }
#endif

	/* Check if the parameters are OK. */
	if (EDMA3_DRV_SOK == result)
        {
        drvInst = (EDMA3_DRV_Instance *)hEdma;
        drvObject = drvInst->pDrvObjectHandle;

        if ((drvObject == NULL) || (drvObject->gblCfgParams.globalRegs == NULL))
            {
            result = EDMA3_DRV_E_INVALID_PARAM;
            }
        else
            {
            globalRegs = (volatile EDMA3_CCRL_Regs *)(drvObject->gblCfgParams.globalRegs);

            switch (trigMode)
                {
                case EDMA3_DRV_TRIG_MODE_MANUAL :
                    {
    //+++
#if 0
                    OALMSG(ZONE_INIT,(_T("+++EDMA3_DRV_enableTransfer: ch %d, shadowRegs 0x%08X\r\n"), 
                           lCh, drvInst->shadowRegs));
                    //DupPaRAM(globalRegs, 6, 0);
                    ShowPaRAM(globalRegs, 0, 0xff);
#endif
    
                    if (lCh < 32u)
                        {
                        drvInst->shadowRegs->ESR = (1UL << lCh);
                        }
                    else
                        {
                        drvInst->shadowRegs->ESRH = (1UL << (lCh-32u));
                        }
                    edma3DrvChBoundRes[drvObject->phyCtrllerInstId][lCh].trigMode = EDMA3_DRV_TRIG_MODE_MANUAL;

    //+++
#if 0
    for(i=0; i<4; i++)
        for(j=0; j<16; j++)
            qevt_entry[i][j] = globalRegs->QUEEVTENTRY[i][j].QUEEVT_ENTRY;

    for(i=0; i<4; i++)
        for(j=0; j<16; j++)
            OALMSG(ZONE_INIT,(_T("+++QUEEVTENTRY[%d][%d]=%d %d\r\n"), 
               i, j, ((qevt_entry[i][j] & 0x000000C0) >> 6), ((qevt_entry[i][j] & 0x0000003F)) ));
#endif

                    }
                    break;

                case EDMA3_DRV_TRIG_MODE_QDMA :
                    {
                    drvInst->shadowRegs->QEESR = (1u<<(lCh - EDMA3_DRV_QDMA_CH_MIN_VAL));
                    edma3DrvChBoundRes[drvObject->phyCtrllerInstId][lCh].trigMode = EDMA3_DRV_TRIG_MODE_QDMA;
                    }
                    break;

                case EDMA3_DRV_TRIG_MODE_EVENT :
                    {
                    /* Trigger type is Event */
/* If parameter checking is enabled... */
#ifndef EDMA3_DRV_PARAM_CHECK_DISABLE
                    if (((drvObject->gblCfgParams.dmaChannelHwEvtMap [lCh/32u]
                         & (1u<<(lCh%32u))) == FALSE))
                    	{
                        /* Channel was not mapped to any Hw Event. */
                        result = EDMA3_DRV_E_INVALID_PARAM;
                        }
#endif

					if (EDMA3_DRV_SOK == result)
						{
	                    if (lCh < 32u)
	                       {
	                        /*clear SECR to clean any previous NULL request */
	                        drvInst->shadowRegs->SECR = (1UL << lCh);

	                        /*clear EMCR to clean any previous NULL request */
	                        globalRegs->EMCR = (1UL << lCh);

	                        drvInst->shadowRegs->EESR = (1UL << lCh);
	                        }
	                    else
	                        {
	                        /*clear SECR to clean any previous NULL request */
	                        drvInst->shadowRegs->SECRH = (1UL << (lCh-32u));

	                        /*clear EMCR to clean any previous NULL request */
	                        globalRegs->EMCRH = (1UL << (lCh-32u));

	                        drvInst->shadowRegs->EESRH = (1UL << (lCh-32u));
	                        }

	                    edma3DrvChBoundRes[drvObject->phyCtrllerInstId][lCh].trigMode = EDMA3_DRV_TRIG_MODE_EVENT;
                    	}
                    }
                    break;

                default :
                    result = EDMA3_DRV_E_INVALID_PARAM;
                    break;
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
 * \brief       Disable DMA transfer on the specified channel
 *
 * There are multiple ways by which an EDMA3 transfer could be triggered.
 * The triggering mode option allows choosing from the available triggering
 * modes: Event, Manual or QDMA.
 *
 * To disable a channel which was previously triggered in manual mode,
 * this API clears the Secondary Event Register and Event Miss Register,
 * if set, for the specific DMA channel.
 *
 * To disable a channel which was previously triggered in QDMA mode, this
 * API clears the QDMA Even Enable Register, for the specific QDMA channel.
 *
 * To disable a channel which was previously triggered in event mode, this API
 * clears the Event Enable Register, Event Register, Secondary Event Register
 * and Event Miss Register, if set, for the specific DMA channel.

 * \param   hEdma       [IN]    Handle to the EDMA Driver Instance
 * \param   lCh         [IN]    Channel on which transfer has to be stopped
 * \param   trigMode    [IN]    Mode of triggering start of transfer
 *
 * \return      EDMA3_DRV_SOK or EDMA3_DRV Error Code
 *
 * \note    This function is re-entrant for unique lCh values. It is non-
 *          re-entrant for same lCh value.
 */
EDMA3_DRV_Result EDMA3_DRV_disableTransfer (EDMA3_DRV_Handle hEdma,
                                unsigned int lCh, EDMA3_DRV_TrigMode trigMode)
{
    EDMA3_DRV_Result result = EDMA3_DRV_SOK;
    EDMA3_DRV_Instance *drvInst = NULL;
    EDMA3_DRV_Object *drvObject = NULL;
    volatile EDMA3_CCRL_Regs *globalRegs = NULL;

#ifdef EDMA3_INSTRUMENTATION_ENABLED
    EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3",
                EDMA3_DVT_DESC(EDMA3_DVT_eFUNC_START,
                EDMA3_DVT_dCOUNTER,
                EDMA3_DVT_dNONE,
                EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */

	/* If parameter checking is enabled... */
#ifndef EDMA3_DRV_PARAM_CHECK_DISABLE
    if ((lCh > EDMA3_DRV_LOG_CH_MAX_VAL) || (hEdma == NULL))
        {
        result = EDMA3_DRV_E_INVALID_PARAM;
        }

    /* Trigger type is Manual */
    if ((EDMA3_DRV_TRIG_MODE_MANUAL == trigMode)
    	&& (lCh > EDMA3_DRV_DMA_CH_MAX_VAL))
        {
        /* Channel Id lies outside DMA channel range */
        result = EDMA3_DRV_E_INVALID_PARAM;
        }

    /* Trigger type is QDMA */
    if ((EDMA3_DRV_TRIG_MODE_QDMA == trigMode)
    	&& ((lCh < EDMA3_DRV_QDMA_CH_MIN_VAL)
    	|| (lCh > EDMA3_DRV_QDMA_CH_MAX_VAL)))
        {
        /* Channel Id lies outside QDMA channel range */
        result = EDMA3_DRV_E_INVALID_PARAM;
        }
#endif

	/* Check if the parameters are OK. */
	if (EDMA3_DRV_SOK == result)
        {
        drvInst = (EDMA3_DRV_Instance *)hEdma;
        drvObject = drvInst->pDrvObjectHandle;

        if ((drvObject == NULL) || (drvObject->gblCfgParams.globalRegs == NULL))
            {
            result = EDMA3_DRV_E_INVALID_PARAM;
            }
        else
            {
            globalRegs = (volatile EDMA3_CCRL_Regs *)(drvObject->gblCfgParams.globalRegs);

            switch (trigMode)
                {
                case EDMA3_DRV_TRIG_MODE_MANUAL :
                    {
                    if (lCh < 32u)
                        {
                        if((drvInst->shadowRegs->SER & (1u<<lCh))!=FALSE)
                            {
                            drvInst->shadowRegs->SECR = (1u<<lCh);
                            }
                        if((globalRegs->EMR & (1u<<lCh))!=FALSE)
                            {
                            globalRegs->EMCR = (1u<<lCh);
                            }
                        }
                    else
                        {
                        if((drvInst->shadowRegs->SERH & (1u<<(lCh-32u)))!=FALSE)
                            {
                            drvInst->shadowRegs->SECRH = (1u<<(lCh-32u));
                            }

                        if((globalRegs->EMRH & (1u<<(lCh-32u)))!=FALSE)
                            {
                            globalRegs->EMCRH = (1u<<(lCh-32u));
                            }
                        }
                    }
                    break;

                case EDMA3_DRV_TRIG_MODE_QDMA :
                    {
                    drvInst->shadowRegs->QEECR = (1u<<(lCh - EDMA3_DRV_QDMA_CH_MIN_VAL));
                    }
                    break;

                case EDMA3_DRV_TRIG_MODE_EVENT :
                    {
                    /* Trigger type is Event */
/* If parameter checking is enabled... */
#ifndef EDMA3_DRV_PARAM_CHECK_DISABLE
                    if (((drvObject->gblCfgParams.dmaChannelHwEvtMap [lCh/32u]
                         & (1u<<(lCh%32u))) == FALSE))
                    	{
                        /* Channel was not mapped to any Hw Event. */
                        result = EDMA3_DRV_E_INVALID_PARAM;
                        }
#endif

					if (EDMA3_DRV_SOK == result)
						{
	                    if (lCh < 32u)
	                        {
	                        drvInst->shadowRegs->EECR = (1u << lCh);

	                        if((drvInst->shadowRegs->ER & (1u<<lCh))!=FALSE)
	                            {
	                            drvInst->shadowRegs->ECR = (1u<<lCh);
	                            }
	                        if((drvInst->shadowRegs->SER & (1u<<lCh))!=FALSE)
	                            {
	                            drvInst->shadowRegs->SECR = (1u<<lCh);
	                            }
	                        if((globalRegs->EMR & (1u<<lCh))!=FALSE)
	                            {
	                            globalRegs->EMCR = (1u<<lCh);
	                            }
	                        }
	                    else
	                        {
	                        drvInst->shadowRegs->EECRH = (1u << (lCh-32u));
	                        if((drvInst->shadowRegs->ERH & (1u<<(lCh-32u)))!=FALSE)
	                            {
	                            drvInst->shadowRegs->ECRH = (1u<<(lCh-32u));
	                            }

	                        if((drvInst->shadowRegs->SERH & (1u<<(lCh-32u)))!=FALSE)
	                            {
	                            drvInst->shadowRegs->SECRH = (1u<<(lCh-32u));
	                            }

	                        if((globalRegs->EMRH & (1u<<(lCh-32u)))!=FALSE)
	                            {
	                            globalRegs->EMCRH = (1u<<(lCh-32u));
	                            }
	                        }
                        }
                    }
                    break;

                default :
                    result = EDMA3_DRV_E_INVALID_PARAM;
                    break;
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
 * \brief Disable the event driven DMA channel or QDMA channel
 *
 * This API disables the DMA channel (which was previously triggered in event
 * mode) by clearing the Event Enable Register; it disables the QDMA channel by
 * clearing the QDMA Event Enable Register.
 *
 * This API should NOT be used for DMA channels which are not mapped to any
 * hardware events and are used for memory-to-memory copy based transfers. In
 * case of that, this API returns error.
 *
 * \param   hEdma       [IN]    Handle to the EDMA Driver Instance
 * \param   lCh         [IN]    DMA/QDMA Channel which needs to be disabled
 * \param   trigMode    [IN]    Mode of triggering start of transfer
 *
 * \return      EDMA3_DRV_SOK or EDMA3_DRV Error Code
 *
 * \note    This function is re-entrant for unique lCh values. It is non-
 *          re-entrant for same lCh value.
 */
EDMA3_DRV_Result EDMA3_DRV_disableLogicalChannel (EDMA3_DRV_Handle hEdma,
                                unsigned int lCh, EDMA3_DRV_TrigMode trigMode)
{
    EDMA3_DRV_Result result = EDMA3_DRV_SOK;
    EDMA3_DRV_Instance *drvInst = NULL;
    EDMA3_DRV_Object *drvObject = NULL;

#ifdef EDMA3_INSTRUMENTATION_ENABLED
    EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3",
                EDMA3_DVT_DESC(EDMA3_DVT_eFUNC_START,
                EDMA3_DVT_dCOUNTER,
                EDMA3_DVT_dNONE,
                EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */

	/* If parameter checking is enabled... */
#ifndef EDMA3_DRV_PARAM_CHECK_DISABLE
    if ((lCh > EDMA3_DRV_LOG_CH_MAX_VAL) || (hEdma == NULL))
        {
        result = EDMA3_DRV_E_INVALID_PARAM;
        }

    /* Trigger type is Manual */
    if (EDMA3_DRV_TRIG_MODE_MANUAL == trigMode)
        result = EDMA3_DRV_E_INVALID_PARAM;

    /* Trigger type is QDMA */
    if ((EDMA3_DRV_TRIG_MODE_QDMA == trigMode)
    	&& ((lCh < EDMA3_DRV_QDMA_CH_MIN_VAL)
    	|| (lCh > EDMA3_DRV_QDMA_CH_MAX_VAL)))
        {
        /* Channel Id lies outside QDMA channel range */
        result = EDMA3_DRV_E_INVALID_PARAM;
        }
#endif

	/* Check if the parameters are OK. */
	if (EDMA3_DRV_SOK == result)
        {
        drvInst = (EDMA3_DRV_Instance *)hEdma;
        drvObject = drvInst->pDrvObjectHandle;

        if (drvObject == NULL)
            {
            result = EDMA3_DRV_E_INVALID_PARAM;
            }
        else
            {
            switch (trigMode)
                {
                case EDMA3_DRV_TRIG_MODE_QDMA:
                    {
                    drvInst->shadowRegs->QEECR = (1u<<(lCh - EDMA3_DRV_QDMA_CH_MIN_VAL));
                    }
                    break;

                case EDMA3_DRV_TRIG_MODE_EVENT:
                    {
                    /* Trigger type is Event */
/* If parameter checking is enabled... */
#ifndef EDMA3_DRV_PARAM_CHECK_DISABLE
                    if (((drvObject->gblCfgParams.dmaChannelHwEvtMap [lCh/32u]
                         & (1u<<(lCh%32u))) == FALSE))
                    	{
                        /* Channel was not mapped to any Hw Event. */
                        result = EDMA3_DRV_E_INVALID_PARAM;
                        }
#endif

					if (EDMA3_DRV_SOK == result)
						{
	                    if (lCh < 32u)
	                        drvInst->shadowRegs->EECR = (1u << lCh);
	                    else
	                        drvInst->shadowRegs->EECRH = (1u << (lCh-32u));
                        }
                    }
                    break;

                default :
                    result = EDMA3_DRV_E_INVALID_PARAM;
                    break;
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

/* Definitions of Local functions - Start */
/** Remove various mappings and do cleanup for DMA/QDMA channels */
static EDMA3_DRV_Result edma3RemoveMapping (EDMA3_DRV_Handle hEdma,
                                 unsigned int channelId)
    {
    unsigned int intState;
    EDMA3_DRV_Instance *drvInst = NULL;
    EDMA3_DRV_Object *drvObject = NULL;
    EDMA3_DRV_Result result = EDMA3_DRV_SOK;
    volatile EDMA3_CCRL_Regs *globalRegs = NULL;
    EDMA3_RM_ResDesc channelObj;

    assert ((hEdma != NULL) && (channelId <= EDMA3_DRV_LOG_CH_MAX_VAL));

    drvInst = (EDMA3_DRV_Instance *)hEdma;
    drvObject = drvInst->pDrvObjectHandle;

    if ((drvObject == NULL) || (drvObject->gblCfgParams.globalRegs == NULL))
        {
        result = EDMA3_DRV_E_INVALID_PARAM;
        }
    else
        {
        globalRegs = (volatile EDMA3_CCRL_Regs *)
                                (drvObject->gblCfgParams.globalRegs);

        /**
         * Disable any ongoing transfer on the channel, if transfer was
         * enabled earlier.
         */
        if (EDMA3_DRV_TRIG_MODE_NONE !=
            edma3DrvChBoundRes[drvObject->phyCtrllerInstId][channelId].trigMode)
            {
            result = EDMA3_DRV_disableTransfer(hEdma, channelId,
                edma3DrvChBoundRes[drvObject->phyCtrllerInstId][channelId].trigMode);
            }

        if (result == EDMA3_DRV_SOK)
            {
            /*
            * Unregister the TCC Callback function and disable the interrupts.
            */
            if (channelId < drvObject->gblCfgParams.numDmaChannels)
                {
                /* DMA channel */
                channelObj.type = EDMA3_RM_RES_DMA_CHANNEL;
                channelObj.resId = channelId;
                }
            else
                {
                /* QDMA channel */
                channelObj.type = EDMA3_RM_RES_QDMA_CHANNEL;
                channelObj.resId = channelId - EDMA3_DRV_QDMA_CH_MIN_VAL;
                }

            result = EDMA3_RM_unregisterTccCb(drvInst->resMgrInstance,
                        (EDMA3_RM_ResDesc *)&channelObj);

            if (result == EDMA3_RM_SOK)
                {
                edma3OsProtectEntry(drvObject->phyCtrllerInstId,
									EDMA3_OS_PROTECT_INTERRUPT, 
									&intState);

                if (channelId <= EDMA3_DRV_DMA_CH_MAX_VAL)
                    {
                    /* DMA channel */
                    /* Remove the channel to Event Queue mapping */
                    globalRegs->DMAQNUM[channelId >> 3u] &=
                                    EDMA3_DRV_DMAQNUM_CLR_MASK(channelId);

                    /**
                     * If DMA channel to PaRAM Set mapping exists,
                     * remove it too.
                     */
                    if (TRUE == drvObject->gblCfgParams.dmaChPaRAMMapExists)
                        {
                        globalRegs->DCHMAP[channelId] &=
                                                EDMA3_RM_DCH_PARAM_CLR_MASK;
                        }
                    }
                else
                    {
                    /* QDMA channel */
                    /* Remove the channel to Event Queue mapping */
                    globalRegs->QDMAQNUM = (globalRegs->QDMAQNUM) &
                        (EDMA3_DRV_QDMAQNUM_CLR_MASK(channelId-EDMA3_DRV_QDMA_CH_MIN_VAL));

                    /* Remove the channel to PARAM set mapping */
                    /* Unmap PARAM Set Number for specified channelId */
                    globalRegs->QCHMAP[channelId-EDMA3_DRV_QDMA_CH_MIN_VAL] &=
                                                EDMA3_RM_QCH_PARAM_CLR_MASK;

                    /* Reset the Trigger Word */
                    globalRegs->QCHMAP[channelId-EDMA3_DRV_QDMA_CH_MIN_VAL] &=
                                                EDMA3_RM_QCH_TRWORD_CLR_MASK;
                    }

                edma3OsProtectExit(drvObject->phyCtrllerInstId,
									EDMA3_OS_PROTECT_INTERRUPT, 
									intState);
                }
            }
        }

    return result;
    }
/* Definitions of Local functions - End */

void ShowPaRAM(volatile EDMA3_CCRL_Regs *globalRegs, int paRAMId, UINT8 which_mask)
{
    volatile EDMA3_CCRL_ParamentryRegs *pPaRAM;

    pPaRAM = &(globalRegs->PARAMENTRY[paRAMId]);

    OALMSG(ZONE_INIT,(_T("+++ParRAM[%d]:\r\n"), paRAMId));

    if (which_mask & 0x01)
    {
        OALMSG(ZONE_INIT,(_T("OPT 0x%X\r\n"), pPaRAM->OPT));
    }

    if (which_mask & 0x02)
    {
        OALMSG(ZONE_INIT,(_T("SRC 0x%X\r\n"), pPaRAM->SRC ));
    }

    if (which_mask & 0x04)
    {
        OALMSG(ZONE_INIT,(_T("A_B_CNT 0x%X\r\n"), pPaRAM->A_B_CNT ));
    }

    if (which_mask & 0x08)
    {
        OALMSG(ZONE_INIT,(_T("DST 0x%X\r\n"), pPaRAM->DST ));
    }

    if (which_mask & 0x10)
    {
        OALMSG(ZONE_INIT,(_T("SRC_DST_BIDX 0x%X\r\n"), pPaRAM->SRC_DST_BIDX ));
    }

    if (which_mask & 0x20)
    {
        OALMSG(ZONE_INIT,(_T("LINK_BCNTRLD 0x%X\r\n"), pPaRAM->LINK_BCNTRLD ));
    }

    if (which_mask & 0x40)
    {
        OALMSG(ZONE_INIT,(_T("SRC_DST_CIDX 0x%X\r\n"), pPaRAM->SRC_DST_CIDX ));
    }

    if (which_mask & 0x80)
    {
        OALMSG(ZONE_INIT,(_T("CCNT 0x%X\r\n"), pPaRAM->CCNT ));
    }

}


void ShowShadowRegs(EDMA3_CCRL_ShadowRegs *pShadowRegs, unsigned int which_mask2, unsigned int which_mask1)
{
    OALMSG(ZONE_INIT,(_T("+++ShadowRegs 0x%08X\r\n"), pShadowRegs));

    if (which_mask1 & 0x00000003)
    {
        OALMSG(ZONE_INIT,(_T("ER=0x%08X ERH=0x%08X\r\n"), pShadowRegs->ER, pShadowRegs->ERH ));
    }

    if (which_mask1 & 0x0000000C)
    {
        OALMSG(ZONE_INIT,(_T("ECR=0x%08X ECRH=0x%08X\r\n"), pShadowRegs->ECR, pShadowRegs->ECRH ));
    }

    if (which_mask1 & 0x00000030)
    {
        OALMSG(ZONE_INIT,(_T("ESR=0x%08X ESRH=0x%08X\r\n"), pShadowRegs->ESR, pShadowRegs->ESRH ));
    }

    if (which_mask1 & 0x000000C0)
    {
        OALMSG(ZONE_INIT,(_T("CER=0x%08X CERH=0x%08X\r\n"), pShadowRegs->CER, pShadowRegs->CERH ));
    }

    if (which_mask1 & 0x00000300)
    {
        OALMSG(ZONE_INIT,(_T("EER=0x%08X EERH=0x%08X\r\n"), pShadowRegs->EER, pShadowRegs->EERH ));
    }

    if (which_mask1 & 0x00000C00)
    {
        OALMSG(ZONE_INIT,(_T("EECR=0x%08X EECRH=0x%08X\r\n"), pShadowRegs->EECR, pShadowRegs->EECRH ));
    }

    if (which_mask1 & 0x00003000)
    {
        OALMSG(ZONE_INIT,(_T("EESR=0x%08X EESRH=0x%08X\r\n"), pShadowRegs->EESR, pShadowRegs->EESRH ));
    }

    if (which_mask1 & 0x0000C000)
    {
        OALMSG(ZONE_INIT,(_T("SER=0x%08X SERH=0x%08X\r\n"), pShadowRegs->SER, pShadowRegs->SERH ));
    }

    if (which_mask1 & 0x00030000)
    {
        OALMSG(ZONE_INIT,(_T("SECR=0x%08X SECRH=0x%08X\r\n"), pShadowRegs->SECR, pShadowRegs->SECRH ));
    }
}


void ShowDCHMAP(volatile EDMA3_CCRL_Regs *globalRegs)
{
    int i;

    for (i=0; i<64; i++)
    {
        OALMSG(ZONE_INIT,(_T("DCHMAP[%d] 0x%08X\r\n"), i, globalRegs->DCHMAP[i] ));
    }
}


void ShowDMAQNUM(volatile EDMA3_CCRL_Regs *globalRegs)
{
    int i;

    for (i=0; i<8; i++)
    {
        OALMSG(ZONE_INIT,(_T("DMAQNUM[%d] 0x%08X\r\n"), i, globalRegs->DMAQNUM[i] ));
    }
}


void ShowQSTAT(volatile EDMA3_CCRL_Regs *globalRegs)
{
    int i;

    for (i=0; i<4; i++)
    {
        OALMSG(ZONE_INIT,(_T("QSTAT[%d] 0x%08X\r\n"), i, globalRegs->QSTAT[i] ));
    }
}


void DupPaRAM(volatile EDMA3_CCRL_Regs *globalRegs, int src_paRAMId, int dst_paRAMId)
{
    volatile EDMA3_CCRL_ParamentryRegs *pSrc;
    volatile EDMA3_CCRL_ParamentryRegs *pDst;

    pSrc = &(globalRegs->PARAMENTRY[src_paRAMId]);
    pDst = &(globalRegs->PARAMENTRY[dst_paRAMId]);

    pDst->OPT           = pSrc->OPT ;
    pDst->SRC           = pSrc->SRC ;
    pDst->A_B_CNT       = pSrc->A_B_CNT ;
    pDst->DST           = pSrc->DST ;
    pDst->SRC_DST_BIDX  = pSrc->SRC_DST_BIDX ;
    pDst->LINK_BCNTRLD  = pSrc->LINK_BCNTRLD ;
    pDst->SRC_DST_CIDX  = pSrc->SRC_DST_CIDX ;
    pDst->CCNT          = pSrc->CCNT ;

}


void ShowIRs(volatile EDMA3_CCRL_Regs *globalRegs, unsigned int which_mask1)
{
    if (which_mask1 & 0x00000001)  // IER
    {
        OALMSG(ZONE_INIT,(_T("IER=0x%08X IERH=0x%08X\r\n"), globalRegs->IER, globalRegs->IERH ));
    }
    
    if (which_mask1 & 0x00000002)  // IPR
    {
        OALMSG(ZONE_INIT,(_T("IPR=0x%08X IPRH=0x%08X\r\n"), globalRegs->IPR, globalRegs->IPRH ));
    }
}


void ShowShadowIRs(volatile EDMA3_CCRL_ShadowRegs *shadowRegs, unsigned int which_mask1)
{
    if (which_mask1 & 0x00000001)  // IER
    {
        OALMSG(ZONE_INIT,(_T("shd IER=0x%08X IERH=0x%08X\r\n"), shadowRegs->IER, shadowRegs->IERH ));
    }
    
    if (which_mask1 & 0x00000002)  // IPR
    {
        OALMSG(ZONE_INIT,(_T("shd IPR=0x%08X IPRH=0x%08X\r\n"), shadowRegs->IPR, shadowRegs->IPRH ));
    }
}


#define GLOBAL_REGS(h)  ((volatile EDMA3_CCRL_Regs *)((((EDMA3_DRV_Instance *)(h))->pDrvObjectHandle)->gblCfgParams.globalRegs))
#define SHADOW_REGS(h)  (((EDMA3_DRV_Instance *)hEdma)->shadowRegs)

void EDMA3_DRV_ShowQSTAT (EDMA3_DRV_Handle hEdma)
{
    volatile EDMA3_CCRL_Regs *globalRegs = NULL;

    globalRegs = GLOBAL_REGS(hEdma);

    ShowQSTAT(globalRegs);
}


void EDMA3_DRV_ShowCCCFG (EDMA3_DRV_Handle hEdma)
{
    OALMSG(ZONE_INIT,(_T("CCCFG 0x%08X\r\n"), GLOBAL_REGS(hEdma)->CCCFG ));
}

void EDMA3_DRV_ShowIRs (EDMA3_DRV_Handle hEdma)
{
    ShowIRs(GLOBAL_REGS(hEdma), 0x00000003);
}

void EDMA3_DRV_ShowShadowIRs (EDMA3_DRV_Handle hEdma)
{
    ShowShadowIRs(SHADOW_REGS(hEDMA), 0x00000003);
}



/* End of File */
