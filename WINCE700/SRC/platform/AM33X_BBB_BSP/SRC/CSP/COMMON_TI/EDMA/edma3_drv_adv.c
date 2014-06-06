/*
 * edma3_drv_adv.c
 *
 * EDMA3 Driver Advanced Interface Implementation. This file contains
 * advanced-level EDMA3 Driver APIs which are required to:
 * a) Link and chain two channels.
 * b) Set/get the whole PaRAM Set in one shot.
 * c) Set/get each individual field of the PaRAM Set.
 * d) Poll mode APIs.
 * e) IOCTL interface.
 * These APIs are provided to have complete control on the EDMA3 hardware and
 * normally advanced users are expected to use them for their specific
 * use-cases.
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


/* EDMA3 Driver Internal Header Files */
#include "edma3.h"
/* Resource Manager Internal Header Files */
#include "edma3resmgr.h"

/* Instrumentation Header File */
#ifdef EDMA3_INSTRUMENTATION_ENABLED
#include "edma3_log.h"
#endif

/* Externel Variables */
/*---------------------------------------------------------------------------*/
/**
 * Maximum Resource Manager Instances supported by the EDMA3 Package.
 */
extern const unsigned int EDMA3_MAX_RM_INSTANCES;
extern const unsigned int g_EDMA3_NUM_INSTANCES;

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


/**
 * \brief  Link two logical channels.
 *
 * This API is used to link two previously allocated logical (DMA/QDMA/Link)
 * channels.
 *
 * It sets the Link field of the PaRAM set associated with first logical
 * channel (lCh1) to point it to the PaRAM set associated with second logical
 * channel (lCh2).
 *
 * It also sets the TCC field of PaRAM set associated with second logical
 * channel to the same as that of the first logical channel.
 *
 * After linking the channels, user should not update any PaRAM Set of the
 * channel.
 *
 * \param   hEdma           [IN]    Handle to the EDMA Driver Instance.
 * \param   lCh1            [IN]    Logical Channel to which particular channel
 *                                  will be linked.
 * \param   lCh2            [IN]    Logical Channel which needs to be linked to
 *                                  the first channel.
 *                                  After the transfer based on the PaRAM set
 *                                  of lCh1 is over, the PaRAM set of lCh2 will
 *                                  be copied to the PaRAM set of lCh1 and
 *                                  transfer will resume.
 *                                  For DMA channels, another sync event is
 *                                  required to initiate the transfer on the
 *                                  Link channel.
 *
 * \return      EDMA3_DRV_SOK or EDMA3_DRV Error Code
 *
 * \note    This function is re-entrant for unique lCh1 & lCh2 values. It is
 *          non-re-entrant for same lCh1 & lCh2 values.
 */
EDMA3_DRV_Result EDMA3_DRV_linkChannel (EDMA3_DRV_Handle hEdma,
                        unsigned int lCh1, unsigned int lCh2)
    {
    EDMA3_DRV_Result result = EDMA3_DRV_SOK;
    EDMA3_DRV_Instance *drvInst = NULL;
    EDMA3_DRV_Object *drvObject = NULL;
    unsigned int linkBcntReld;
    int paRAM1Id = 0;
    int paRAM2Id = 0;
    unsigned int oldTccVal = 0;
    unsigned int optVal = 0;
    unsigned int newOptVal = 0;
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
    if (((lCh1 > EDMA3_DRV_LOG_CH_MAX_VAL)
    	|| (lCh2 > EDMA3_DRV_LOG_CH_MAX_VAL))
        || (hEdma == NULL))
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
            paRAM1Id = edma3DrvChBoundRes[drvObject->phyCtrllerInstId][lCh1].paRAMId;
            paRAM2Id = edma3DrvChBoundRes[drvObject->phyCtrllerInstId][lCh2].paRAMId;
            globalRegs = (volatile EDMA3_CCRL_Regs *)(drvObject->gblCfgParams.globalRegs);

            if ((paRAM1Id < 0) || ((unsigned int)paRAM1Id >= drvObject->gblCfgParams.numPaRAMSets))
                {
                result = EDMA3_DRV_E_INVALID_PARAM;
                }

            if ((paRAM2Id < 0) || ((unsigned int)paRAM2Id >= drvObject->gblCfgParams.numPaRAMSets))
                {
                result = EDMA3_DRV_E_INVALID_PARAM;
                }

            if (result == EDMA3_DRV_SOK)
                {
                /* Get the Link-bcntReload PaRAM set entry */
                linkBcntReld = (unsigned int)(*((&globalRegs->PARAMENTRY [paRAM1Id].OPT) +
                                    (unsigned int)EDMA3_DRV_PARAM_ENTRY_LINK_BCNTRLD));
                linkBcntReld &= 0xFFFF0000u;
                /* Update the Link field with lch2 PaRAM set */
                linkBcntReld |= (0xFFFFu & (unsigned int)(&(globalRegs->PARAMENTRY [paRAM2Id].OPT)));

                /* Store it back */
                *((&globalRegs->PARAMENTRY[paRAM1Id].OPT)
                            + (unsigned int)EDMA3_DRV_PARAM_ENTRY_LINK_BCNTRLD) = linkBcntReld;

                /*
                * Set the TCC field of PaRAM set associated with lch2 to
                * the same as that of lch1.
                */
                /* for channel 1 */
                optVal = (unsigned int)(*(&globalRegs->PARAMENTRY [paRAM1Id].OPT));
                oldTccVal = EDMA3_DRV_OPT_TCC_GET_MASK(optVal);

                /* for channel 2 */
                optVal = (unsigned int)(*(&globalRegs->PARAMENTRY [paRAM2Id].OPT));
                newOptVal = (optVal & EDMA3_DRV_OPT_TCC_CLR_MASK)
                            |
                            (EDMA3_DRV_OPT_TCC_SET_MASK(oldTccVal));
                *(&globalRegs->PARAMENTRY[paRAM2Id].OPT) = newOptVal;
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
 * \brief  Unlink the channel from the earlier linked logical channel.
 *
 *         This function breaks the link between the specified
 *         channel and the earlier linked logical channel
 *         by clearing the Link Address field.
 *
 * \param  hEdma             [IN]    Handle to the EDMA Driver Instance.
 * \param  lCh               [IN]    Channel for which linking has to be removed
 *
 * \return      EDMA3_DRV_SOK or EDMA3_DRV Error Code
 *
 * \note    This function is re-entrant for unique lCh values. It is non-
 *          re-entrant for same lCh value.
 */
EDMA3_DRV_Result EDMA3_DRV_unlinkChannel (EDMA3_DRV_Handle hEdma, unsigned int lCh)
    {
    EDMA3_DRV_Result result = EDMA3_DRV_SOK;
    unsigned int linkBcntReld;
    int paRAMId = 0;
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
                /* Get the Link-bcntReload PaRAM set entry */
                linkBcntReld = (unsigned int)(*((&globalRegs->PARAMENTRY [paRAMId].OPT) +
                                                        (unsigned int)EDMA3_DRV_PARAM_ENTRY_LINK_BCNTRLD));

                /* Remove any linking */
                linkBcntReld |= 0xFFFFu;

                /* Store it back */
                *((&globalRegs->PARAMENTRY[paRAMId].OPT)
                            + (unsigned int)EDMA3_DRV_PARAM_ENTRY_LINK_BCNTRLD) = linkBcntReld;
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
 * \brief   Chain the two specified channels.
 *
 * This API is used to chain two previously allocated logical (DMA/QDMA)
 * channels.
 *
 * Chaining is different from Linking. The EDMA3 link feature reloads the
 * current channel parameter set with the linked parameter set. The EDMA3
 * chaining feature does not modify or update any channel parameter set;
 * it provides a synchronization event to the chained channel.
 *
 * \param   hEdma               [IN]    Handle to the EDMA Driver Instance.
 *
 * \param   lCh1                [IN]    Channel to which particular channel
 *                                      will be chained.
 * \param   lCh2                [IN]    Channel which needs to be chained to
 *                                      the first channel.
 * \param   chainOptions        [IN]    Options such as intermediate interrupts
 *                                      are required or not, intermediate/final
 *                                      chaining is enabled or not etc.
 *
 * \return      EDMA3_DRV_SOK or EDMA3_DRV Error Code
 *
 * \note    This function is re-entrant for unique lCh1 & lCh2 values. It is
 *          non-re-entrant for same lCh1 & lCh2 values.
 */
EDMA3_DRV_Result EDMA3_DRV_chainChannel (EDMA3_DRV_Handle hEdma,
                            unsigned int lCh1,
                            unsigned int lCh2,
                            const EDMA3_DRV_ChainOptions *chainOptions)
    {
    EDMA3_DRV_Result result = EDMA3_DRV_SOK;
    unsigned int opt = 0x0;
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
    if (((lCh1 > EDMA3_DRV_LOG_CH_MAX_VAL) || (lCh2 > EDMA3_DRV_LOG_CH_MAX_VAL))
        || (hEdma == NULL))
        {
        result = EDMA3_DRV_E_INVALID_PARAM;
        }

    if (chainOptions == NULL)
        {
        result = EDMA3_DRV_E_INVALID_PARAM;
        }
#endif

    if (result == EDMA3_DRV_SOK)
        {
        drvInst = (EDMA3_DRV_Instance *)hEdma;
        drvObject = drvInst->pDrvObjectHandle;

        if ((drvObject == NULL) || (drvObject->gblCfgParams.globalRegs == NULL))
            {
            result =  EDMA3_DRV_E_INVALID_PARAM;
            }
        }

    if (result == EDMA3_DRV_SOK)
        {
        paRAMId = edma3DrvChBoundRes[drvObject->phyCtrllerInstId][lCh1].paRAMId;
        globalRegs = (volatile EDMA3_CCRL_Regs *)(drvObject->gblCfgParams.globalRegs);

        if ((paRAMId < 0) || ((unsigned int)paRAMId >= drvObject->gblCfgParams.numPaRAMSets))
            {
            result = EDMA3_DRV_E_INVALID_PARAM;
            }
        }

    if (result == EDMA3_DRV_SOK)
        {
        opt = (unsigned int)(*(&globalRegs->PARAMENTRY [paRAMId].OPT));

        /* set Transfer complete chaining */
        if (chainOptions->tcchEn == EDMA3_DRV_TCCHEN_EN)
            {
            opt |= EDMA3_DRV_OPT_TCCHEN_SET_MASK(1u);
            }
        else
            {
            opt &= EDMA3_DRV_OPT_TCCHEN_CLR_MASK;
            }

        /*set Intermediate transfer completion chaining */
        if (chainOptions->itcchEn == EDMA3_DRV_ITCCHEN_EN)
            {
            opt |= EDMA3_DRV_OPT_ITCCHEN_SET_MASK(1u);
            }
        else
            {
            opt &= EDMA3_DRV_OPT_ITCCHEN_CLR_MASK;
            }

        /*set Transfer complete interrupt */
        if (chainOptions->tcintEn == EDMA3_DRV_TCINTEN_EN)
            {
            opt |= EDMA3_DRV_OPT_TCINTEN_SET_MASK(1u);
            }
        else
            {
            opt &= EDMA3_DRV_OPT_TCINTEN_CLR_MASK;
            }

        /*set Intermediate transfer completion interrupt */
        if (chainOptions->itcintEn == EDMA3_DRV_ITCINTEN_EN)
            {
            opt |= EDMA3_DRV_OPT_ITCINTEN_SET_MASK(1u);
            }
        else
            {
            opt &= EDMA3_DRV_OPT_ITCINTEN_CLR_MASK;
            }

        opt &= EDMA3_DRV_OPT_TCC_CLR_MASK;
        opt |= EDMA3_DRV_OPT_TCC_SET_MASK(lCh2);

        *(&globalRegs->PARAMENTRY[paRAMId].OPT) = opt;

        /* Set the trigger mode of lch2 as the same as of lch1 */
        edma3DrvChBoundRes[drvObject->phyCtrllerInstId][lCh2].trigMode =
                    edma3DrvChBoundRes[drvObject->phyCtrllerInstId][lCh1].trigMode;
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
 * \brief   Unchain the two channels.
 *
 * \param   hEdma               [IN]    Handle to the EDMA Driver Instance.
 * \param   lCh                 [IN]    Channel whose chaining with the other
 *                                      channel has to be removed.
 *
 * \return      EDMA3_DRV_SOK or EDMA3_DRV Error Code
 *
 * \note    This function is re-entrant for unique lCh values. It is non-
 *          re-entrant for same lCh value.
 */
EDMA3_DRV_Result EDMA3_DRV_unchainChannel (EDMA3_DRV_Handle hEdma,
                    unsigned int lCh)
    {
    EDMA3_DRV_Result result = EDMA3_DRV_SOK;
    unsigned int opt;
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
#endif

    if (result == EDMA3_DRV_SOK)
        {
        drvInst = (EDMA3_DRV_Instance *)hEdma;
        drvObject = drvInst->pDrvObjectHandle;

        if ((drvObject == NULL) || (drvObject->gblCfgParams.globalRegs == NULL))
            {
            result =  EDMA3_DRV_E_INVALID_PARAM;
            }
        }

    if (result == EDMA3_DRV_SOK)
        {
        paRAMId = edma3DrvChBoundRes[drvObject->phyCtrllerInstId][lCh].paRAMId;
        globalRegs = (volatile EDMA3_CCRL_Regs *)(drvObject->gblCfgParams.globalRegs);

        if ((paRAMId < 0) || ((unsigned int)paRAMId >= drvObject->gblCfgParams.numPaRAMSets))
            {
            result = EDMA3_DRV_E_INVALID_PARAM;
            }
        }

    if (result == EDMA3_DRV_SOK)
        {
        opt = (unsigned int)(*(&globalRegs->PARAMENTRY [paRAMId].OPT));

        /* Reset TCCHEN */
        opt &= EDMA3_DRV_OPT_TCCHEN_CLR_MASK;
        /* Reset ITCCHEN */
        opt &= EDMA3_DRV_OPT_ITCCHEN_CLR_MASK;

        *(&globalRegs->PARAMENTRY[paRAMId].OPT) = opt;
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
 * \brief  Assign a Trigger Word to the specified QDMA channel
 *
 * This API sets the Trigger word for the specific QDMA channel in the QCHMAP
 * Register. Default QDMA trigger word is CCNT.
 *
 * \param   hEdma      [IN]    Handle to the EDMA Instance object
 * \param   lCh        [IN]    QDMA Channel which needs to be assigned
 *                             the Trigger Word
 * \param   trigWord   [IN]    The Trigger Word for the QDMA channel.
 *                             Trigger Word is the word in the PaRAM
 *                             Register Set which, when written to by CPU,
 *                             will start the QDMA transfer automatically.
 *
 * \return  EDMA3_DRV_SOK or EDMA3_DRV Error Code
 *
 * \note    This function is re-entrant for unique lCh values. It is non-
 *          re-entrant for same lCh value.
 */
EDMA3_DRV_Result EDMA3_DRV_setQdmaTrigWord (EDMA3_DRV_Handle hEdma,
                    unsigned int lCh,
                    EDMA3_RM_QdmaTrigWord trigWord)
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
    if((hEdma == NULL)
        || (((lCh < EDMA3_DRV_QDMA_CH_MIN_VAL)
        || (lCh > EDMA3_DRV_QDMA_CH_MAX_VAL))
        || ((trigWord < EDMA3_RM_QDMA_TRIG_OPT)
        || (trigWord > EDMA3_RM_QDMA_TRIG_CCNT))))
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
            globalRegs = (volatile EDMA3_CCRL_Regs *)(drvObject->gblCfgParams.globalRegs);

            globalRegs->QCHMAP[lCh -EDMA3_DRV_QDMA_CH_MIN_VAL] &= EDMA3_DRV_QCH_TRWORD_CLR_MASK;
            globalRegs->QCHMAP[lCh -EDMA3_DRV_QDMA_CH_MIN_VAL] |= EDMA3_DRV_QCH_TRWORD_SET_MASK(trigWord);
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
 * \param   hEdma       [IN]  Handle to the EDMA Instance object
 * \param   lCh         [IN]  Logical Channel for which new PaRAM set is
 *                            specified
 * \param   newPaRAM    [IN]  Parameter RAM set to be copied onto existing PaRAM
 *
 * \return  EDMA3_DRV_SOK or EDMA3_DRV Error Code
 *
 * \note    This function is re-entrant for unique lCh values. It is non-
 *          re-entrant for same lCh value.
 */
EDMA3_DRV_Result EDMA3_DRV_setPaRAM (EDMA3_DRV_Handle hEdma,
                    unsigned int lCh,
                    const EDMA3_DRV_PaRAMRegs *newPaRAM)
    {
    EDMA3_DRV_Result result = EDMA3_DRV_SOK;
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
    if (((lCh > EDMA3_DRV_LOG_CH_MAX_VAL) || (hEdma == NULL))
        || (newPaRAM == NULL))
        {
        result = EDMA3_DRV_E_INVALID_PARAM;
        }
#endif

    if (result == EDMA3_DRV_SOK)
        {
        drvInst = (EDMA3_DRV_Instance *)hEdma;
        drvObject = drvInst->pDrvObjectHandle;

        if ((drvObject == NULL) || (drvObject->gblCfgParams.globalRegs == NULL))
            {
            result = EDMA3_DRV_E_INVALID_PARAM;
            }
        }

    if (result == EDMA3_DRV_SOK)
        {
        globalRegs = (volatile EDMA3_CCRL_Regs *)(drvObject->gblCfgParams.globalRegs);
        paRAMId = edma3DrvChBoundRes[drvObject->phyCtrllerInstId][lCh].paRAMId;

        if ((paRAMId < 0) || ((unsigned int)paRAMId >= drvObject->gblCfgParams.numPaRAMSets))
            {
            result = EDMA3_DRV_E_INVALID_PARAM;
            }
        }

    if (result == EDMA3_DRV_SOK)
        {
        edma3MemCpy ((void *)(&(globalRegs->PARAMENTRY[paRAMId].OPT)),
            (const void *)newPaRAM,
            sizeof(EDMA3_CCRL_ParamentryRegs));
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
 * \brief   Retrieve existing PaRAM set associated with specified logical
 *          channel (DMA/QDMA/Link).
 *
 * \param   hEdma           [IN]     Handle to the EDMA Instance object
 * \param   lCh             [IN]     Logical Channel whose PaRAM set is
 *                                   requested
 * \param   currPaRAM       [IN/OUT] User gets the existing PaRAM here
 *
 * \return  EDMA3_DRV_SOK or EDMA3_DRV Error Code
 *
 * \note    This function is re-entrant.
 */
EDMA3_DRV_Result EDMA3_DRV_getPaRAM (EDMA3_DRV_Handle hEdma,
                    unsigned int lCh,
                    EDMA3_DRV_PaRAMRegs *currPaRAM)
    {
    EDMA3_DRV_Result result = EDMA3_DRV_SOK;
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
    if (((lCh > EDMA3_DRV_LOG_CH_MAX_VAL) || (hEdma == NULL))
        || (currPaRAM == NULL))
        {
        result = EDMA3_DRV_E_INVALID_PARAM;
        }
#endif

    if (result == EDMA3_DRV_SOK)
        {
        drvInst = (EDMA3_DRV_Instance *)hEdma;
        drvObject = drvInst->pDrvObjectHandle;

        if ((drvObject == NULL) || (drvObject->gblCfgParams.globalRegs == NULL))
            {
            result = EDMA3_DRV_E_INVALID_PARAM;
            }
        }

    if (result == EDMA3_DRV_SOK)
        {
        globalRegs = (volatile EDMA3_CCRL_Regs *)(drvObject->gblCfgParams.globalRegs);
        paRAMId = edma3DrvChBoundRes[drvObject->phyCtrllerInstId][lCh].paRAMId;

        if ((paRAMId < 0) || ((unsigned int)paRAMId >= drvObject->gblCfgParams.numPaRAMSets))
            {
            result = EDMA3_DRV_E_INVALID_PARAM;
            }
        }

    if (result == EDMA3_DRV_SOK)
        {
        edma3MemCpy ((void *)currPaRAM ,
            (const void *)(&(globalRegs->PARAMENTRY [paRAMId].OPT)),
            sizeof(EDMA3_CCRL_ParamentryRegs));
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
 * \brief   Set a particular PaRAM set entry of the specified PaRAM set
 *
 *
 * \param   hEdma       [IN]    Handle to the EDMA Driver Instance
 * \param   lCh         [IN]    Logical Channel bound to the Parameter RAM set
 *                              whose specified field needs to be set
 * \param   paRAMEntry  [IN]    Specify the PaRAM set entry which needs
 *                              to be set
 * \param   newPaRAMEntryVal [IN]    The new field setting
 *
 * \return  EDMA3_DRV_SOK or EDMA3_DRV Error Code
 *
 * \note    This API should be used while setting the PaRAM set entry
 *          for QDMA channels. If EDMA3_DRV_setPaRAMField () used,
 *          it will trigger the QDMA channel before complete
 *          PaRAM set entry is written. For DMA channels, no such
 *          constraint is there.
 *
 *          This function is re-entrant for unique lCh values. It is non-
 *          re-entrant for same lCh value.
 */
EDMA3_DRV_Result EDMA3_DRV_setPaRAMEntry (EDMA3_DRV_Handle hEdma,
                    unsigned int lCh,
                    EDMA3_DRV_PaRAMEntry paRAMEntry,
                    unsigned int newPaRAMEntryVal)
    {
    EDMA3_DRV_Result result = EDMA3_DRV_SOK;
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
    if(((lCh > EDMA3_DRV_LOG_CH_MAX_VAL) || (hEdma == NULL))
        || ((paRAMEntry < EDMA3_DRV_PARAM_ENTRY_OPT)
        || (paRAMEntry > EDMA3_DRV_PARAM_ENTRY_CCNT)))
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
            else
                {
                *((&globalRegs->PARAMENTRY[paRAMId].OPT) + (unsigned int)paRAMEntry) = newPaRAMEntryVal;
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
 * \brief   Get a particular PaRAM set entry of the specified PaRAM set
 *
 *
 * \param   hEdma       [IN]    Handle to the EDMA Driver Instance
 * \param   lCh         [IN]    Logical Channel bound to the Parameter RAM set
 *                              whose specified field value is needed
 * \param   paRAMEntry  [IN]    Specify the PaRAM set entry which needs
 *                              to be obtained
 * \param   paRAMEntryVal [IN/OUT]  The value of the field is returned here
 *
 * \return  EDMA3_DRV_SOK or EDMA3_DRV Error Code
 *
 * \note    This function is re-entrant.
 */
EDMA3_DRV_Result EDMA3_DRV_getPaRAMEntry (EDMA3_DRV_Handle hEdma,
                    unsigned int lCh,
                    EDMA3_DRV_PaRAMEntry paRAMEntry,
                    unsigned int *paRAMEntryVal)
    {
    EDMA3_DRV_Result result = EDMA3_DRV_SOK;
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
    if(((lCh > EDMA3_DRV_LOG_CH_MAX_VAL)
        || ((hEdma == NULL) || (paRAMEntryVal == NULL)))
        || ((paRAMEntry < EDMA3_DRV_PARAM_ENTRY_OPT)
        || (paRAMEntry > EDMA3_DRV_PARAM_ENTRY_CCNT)))
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
            else
                {
                *paRAMEntryVal = (unsigned int)(*((&globalRegs->PARAMENTRY [paRAMId].OPT) + (unsigned int)paRAMEntry));
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
 * \brief   Set a particular PaRAM set field of the specified PaRAM set
 *
 *
 * \param   hEdma               [IN]    Handle to the EDMA Driver Instance
 * \param   lCh                 [IN]    Logical Channel bound to the PaRAM set
 *                                      whose specified field needs to be set
 * \param   paRAMField          [IN]    Specify the PaRAM set field which needs
 *                                      to be set
 * \param   newPaRAMFieldVal    [IN]    The new field setting
 *
 * \return  EDMA3_DRV_SOK or EDMA3_DRV Error Code
 *
 * \note    This API CANNOT be used while setting the PaRAM set
 *          field for QDMA channels. It can trigger the QDMA channel before
 *          complete PaRAM set ENTRY (4-bytes field) is written (for eg, as
 *          soon one sets the ACNT field for QDMA channel, transfer is started,
 *          before one modifies the BCNT field). For DMA channels, no such
 *          constraint is there.
 *
 *          This function is re-entrant for unique lCh values. It is non-
 *          re-entrant for same lCh value.
 */
EDMA3_DRV_Result EDMA3_DRV_setPaRAMField (EDMA3_DRV_Handle hEdma,
                        unsigned int lCh,
                        EDMA3_DRV_PaRAMField paRAMField,
                        unsigned int newPaRAMFieldVal)
    {
    EDMA3_DRV_Result result = EDMA3_DRV_SOK;
    EDMA3_DRV_Instance *drvInst = NULL;
    EDMA3_DRV_Object *drvObject = NULL;
    unsigned int paramEntryVal = 0;
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
    /*
    * THIS API IS NOT ALLOWED FOR QDMA CHANNELS.
    * Reason being setting one PaRAM field might trigger the
    * transfer if the word written happends to be the trigger
    * word. One should use EDMA3_DRV_setPaRAMEntry ()
    * API instead to write the whole 32 bit word.
    */
    if ((lCh >= EDMA3_DRV_QDMA_CHANNEL_0) && (lCh <= EDMA3_DRV_QDMA_CHANNEL_7))
        {
        result =  EDMA3_DRV_E_INVALID_PARAM;
        }

    if(lCh > EDMA3_DRV_LOG_CH_MAX_VAL)
        {
        result = EDMA3_DRV_E_INVALID_PARAM;
        }

    if((hEdma == NULL)
        || ((paRAMField < EDMA3_DRV_PARAM_FIELD_OPT)
        || (paRAMField > EDMA3_DRV_PARAM_FIELD_CCNT)))
        {
        result =  EDMA3_DRV_E_INVALID_PARAM;
        }
#endif

    if (result == EDMA3_DRV_SOK)
        {
        drvInst = (EDMA3_DRV_Instance *)hEdma;
        drvObject = drvInst->pDrvObjectHandle;

        if ((drvObject == NULL) || (drvObject->gblCfgParams.globalRegs == NULL))
            {
            result =  EDMA3_DRV_E_INVALID_PARAM;
            }
        }

    if (result == EDMA3_DRV_SOK)
        {
        paRAMId = edma3DrvChBoundRes[drvObject->phyCtrllerInstId][lCh].paRAMId;
        globalRegs = (volatile EDMA3_CCRL_Regs *)(drvObject->gblCfgParams.globalRegs);

        if ((paRAMId < 0) || ((unsigned int)paRAMId >= drvObject->gblCfgParams.numPaRAMSets))
            {
            result = EDMA3_DRV_E_INVALID_PARAM;
            }
        }


    if (result == EDMA3_DRV_SOK)
        {
        switch (paRAMField)
            {
            case EDMA3_DRV_PARAM_FIELD_OPT:
                *((&globalRegs->PARAMENTRY[paRAMId].OPT) + (unsigned)EDMA3_DRV_PARAM_ENTRY_OPT) = newPaRAMFieldVal;
                break;

            case EDMA3_DRV_PARAM_FIELD_SRCADDR:
                *((&globalRegs->PARAMENTRY[paRAMId].OPT) + (unsigned)EDMA3_DRV_PARAM_ENTRY_SRC) = newPaRAMFieldVal;
                break;

            case EDMA3_DRV_PARAM_FIELD_ACNT:
                paramEntryVal = (unsigned int)(*((&globalRegs->PARAMENTRY [paRAMId].OPT) + (unsigned)EDMA3_DRV_PARAM_ENTRY_ACNT_BCNT));
                paramEntryVal &= 0xFFFF0000u;
                newPaRAMFieldVal &= 0x0000FFFFu;
                paramEntryVal |= newPaRAMFieldVal;
                *((&globalRegs->PARAMENTRY[paRAMId].OPT) + (unsigned)EDMA3_DRV_PARAM_ENTRY_ACNT_BCNT) = paramEntryVal;
                break;

            case EDMA3_DRV_PARAM_FIELD_BCNT:
                paramEntryVal = (unsigned int)(*((&globalRegs->PARAMENTRY [paRAMId].OPT) + (unsigned)EDMA3_DRV_PARAM_ENTRY_ACNT_BCNT));
                paramEntryVal &= 0x0000FFFFu;
                newPaRAMFieldVal <<= 0x10u;
                paramEntryVal |= newPaRAMFieldVal;
                *((&globalRegs->PARAMENTRY[paRAMId].OPT) + (unsigned)EDMA3_DRV_PARAM_ENTRY_ACNT_BCNT) = paramEntryVal;
                break;

            case EDMA3_DRV_PARAM_FIELD_DESTADDR:
                *((&globalRegs->PARAMENTRY[paRAMId].OPT) + (unsigned)EDMA3_DRV_PARAM_ENTRY_DST) = newPaRAMFieldVal;
                break;

            case EDMA3_DRV_PARAM_FIELD_SRCBIDX:
                paramEntryVal = (unsigned int)(*((&globalRegs->PARAMENTRY [paRAMId].OPT) + (unsigned)EDMA3_DRV_PARAM_ENTRY_SRC_DST_BIDX));
                paramEntryVal &= 0xFFFF0000u;
                newPaRAMFieldVal &= 0x0000FFFFu;
                paramEntryVal |= newPaRAMFieldVal;
                *((&globalRegs->PARAMENTRY[paRAMId].OPT) + (unsigned)EDMA3_DRV_PARAM_ENTRY_SRC_DST_BIDX) = paramEntryVal;
                break;

            case EDMA3_DRV_PARAM_FIELD_DESTBIDX:
                paramEntryVal = (unsigned int)(*((&globalRegs->PARAMENTRY [paRAMId].OPT) + (unsigned)EDMA3_DRV_PARAM_ENTRY_SRC_DST_BIDX));
                paramEntryVal &= 0x0000FFFFu;
                newPaRAMFieldVal <<= 0x10u;
                paramEntryVal |= newPaRAMFieldVal;
                *((&globalRegs->PARAMENTRY[paRAMId].OPT) + (unsigned)EDMA3_DRV_PARAM_ENTRY_SRC_DST_BIDX) = paramEntryVal;
                break;

            case EDMA3_DRV_PARAM_FIELD_LINKADDR:
                paramEntryVal = (unsigned int)(*((&globalRegs->PARAMENTRY [paRAMId].OPT) + (unsigned)EDMA3_DRV_PARAM_ENTRY_LINK_BCNTRLD));
                paramEntryVal &= 0xFFFF0000u;
                newPaRAMFieldVal &= 0x0000FFFFu;
                paramEntryVal |= newPaRAMFieldVal;
                *((&globalRegs->PARAMENTRY[paRAMId].OPT) + (unsigned)EDMA3_DRV_PARAM_ENTRY_LINK_BCNTRLD) = paramEntryVal;
                break;

            case EDMA3_DRV_PARAM_FIELD_BCNTRELOAD:
                paramEntryVal = (unsigned int)(*((&globalRegs->PARAMENTRY [paRAMId].OPT) + (unsigned)EDMA3_DRV_PARAM_ENTRY_LINK_BCNTRLD));
                paramEntryVal &= 0x0000FFFFu;
                newPaRAMFieldVal <<= 0x10u;
                paramEntryVal |= newPaRAMFieldVal;
                *((&globalRegs->PARAMENTRY[paRAMId].OPT) + (unsigned)EDMA3_DRV_PARAM_ENTRY_LINK_BCNTRLD) = paramEntryVal;
                break;

            case EDMA3_DRV_PARAM_FIELD_SRCCIDX:
                paramEntryVal = (unsigned int)(*((&globalRegs->PARAMENTRY [paRAMId].OPT) + (unsigned)EDMA3_DRV_PARAM_ENTRY_SRC_DST_CIDX));
                paramEntryVal &= 0xFFFF0000u;
                newPaRAMFieldVal &= 0x0000FFFFu;
                paramEntryVal |= newPaRAMFieldVal;
                *((&globalRegs->PARAMENTRY[paRAMId].OPT) + (unsigned)EDMA3_DRV_PARAM_ENTRY_SRC_DST_CIDX) = paramEntryVal;
                break;

            case EDMA3_DRV_PARAM_FIELD_DESTCIDX:
                paramEntryVal = (unsigned int)(*((&globalRegs->PARAMENTRY [paRAMId].OPT) + (unsigned)EDMA3_DRV_PARAM_ENTRY_SRC_DST_CIDX));
                paramEntryVal &= 0x0000FFFFu;
                newPaRAMFieldVal <<= 0x10u;
                paramEntryVal |= newPaRAMFieldVal;
                *((&globalRegs->PARAMENTRY[paRAMId].OPT) + (unsigned)EDMA3_DRV_PARAM_ENTRY_SRC_DST_CIDX) = paramEntryVal;
                break;

            case EDMA3_DRV_PARAM_FIELD_CCNT:
                newPaRAMFieldVal &= 0x0000FFFFu;
                *((&globalRegs->PARAMENTRY[paRAMId].OPT) + (unsigned)EDMA3_DRV_PARAM_ENTRY_CCNT) = newPaRAMFieldVal;
                break;

            default:
                break;
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
 * \brief   Get a particular PaRAM set field of the specified PaRAM set
 *
 *
 * \param   hEdma               [IN]    Handle to the EDMA Driver Instance
 * \param   lCh                 [IN]    Logical Channel bound to the PaRAM set
 *                                      whose specified field value is needed
 * \param   paRAMField          [IN]    Specify the PaRAM set field which needs
 *                                      to be obtained
 * \param   currPaRAMFieldVal [IN/OUT]  The value of the field is returned here
 *
 * \return  EDMA3_DRV_SOK or EDMA3_DRV Error Code
 *
 * \note    This function is re-entrant.
 */
EDMA3_DRV_Result EDMA3_DRV_getPaRAMField (EDMA3_DRV_Handle hEdma,
                        unsigned int lCh,
                        EDMA3_DRV_PaRAMField paRAMField,
                        unsigned int *currPaRAMFieldVal)
    {
    EDMA3_DRV_Result result = EDMA3_DRV_SOK;
    EDMA3_DRV_Instance *drvInst = NULL;
    EDMA3_DRV_Object *drvObject = NULL;
    unsigned int paramEntryVal = 0;
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
    if(((lCh > EDMA3_DRV_LOG_CH_MAX_VAL)
       || ((hEdma == NULL) || (currPaRAMFieldVal == NULL)))
       || ((paRAMField < EDMA3_DRV_PARAM_FIELD_OPT)
       || (paRAMField > EDMA3_DRV_PARAM_FIELD_CCNT)))
        {
        result =  EDMA3_DRV_E_INVALID_PARAM;
        }
#endif

    if (result == EDMA3_DRV_SOK)
        {
        drvInst = (EDMA3_DRV_Instance *)hEdma;
        drvObject = drvInst->pDrvObjectHandle;

        if ((drvObject == NULL) || (drvObject->gblCfgParams.globalRegs == NULL))
            {
            result =  EDMA3_DRV_E_INVALID_PARAM;
            }
        }

    if (result == EDMA3_DRV_SOK)
        {
        paRAMId = edma3DrvChBoundRes[drvObject->phyCtrllerInstId][lCh].paRAMId;
        globalRegs = (volatile EDMA3_CCRL_Regs *)(drvObject->gblCfgParams.globalRegs);

        if ((paRAMId < 0) || ((unsigned int)paRAMId >= drvObject->gblCfgParams.numPaRAMSets))
            {
            result = EDMA3_DRV_E_INVALID_PARAM;
            }
        }

    if (result == EDMA3_DRV_SOK)
        {
        switch (paRAMField)
            {
            case EDMA3_DRV_PARAM_FIELD_OPT:
                *currPaRAMFieldVal = (unsigned int)(*((&globalRegs->PARAMENTRY [paRAMId].OPT) + (unsigned)EDMA3_DRV_PARAM_ENTRY_OPT));
                break;

            case EDMA3_DRV_PARAM_FIELD_SRCADDR:
                *currPaRAMFieldVal = (unsigned int)(*((&globalRegs->PARAMENTRY [paRAMId].OPT) + (unsigned)EDMA3_DRV_PARAM_ENTRY_SRC));
                break;

            case EDMA3_DRV_PARAM_FIELD_ACNT:
                paramEntryVal = (unsigned int)(*((&globalRegs->PARAMENTRY [paRAMId].OPT) + (unsigned)EDMA3_DRV_PARAM_ENTRY_ACNT_BCNT));
                paramEntryVal &= 0x0000FFFFu;
                *currPaRAMFieldVal = paramEntryVal;
                break;

            case EDMA3_DRV_PARAM_FIELD_BCNT:
                paramEntryVal = (unsigned int)(*((&globalRegs->PARAMENTRY [paRAMId].OPT) + (unsigned)EDMA3_DRV_PARAM_ENTRY_ACNT_BCNT));
                paramEntryVal = paramEntryVal >> 0x10u;
                *currPaRAMFieldVal = paramEntryVal;
                break;

            case EDMA3_DRV_PARAM_FIELD_DESTADDR:
                *currPaRAMFieldVal = (unsigned int)(*((&globalRegs->PARAMENTRY [paRAMId].OPT) + (unsigned)EDMA3_DRV_PARAM_ENTRY_DST));
                break;

            case EDMA3_DRV_PARAM_FIELD_SRCBIDX:
                paramEntryVal = (unsigned int)(*((&globalRegs->PARAMENTRY [paRAMId].OPT) + (unsigned)EDMA3_DRV_PARAM_ENTRY_SRC_DST_BIDX));
                paramEntryVal &= 0x0000FFFFu;
                *currPaRAMFieldVal = paramEntryVal;
                break;

            case EDMA3_DRV_PARAM_FIELD_DESTBIDX:
                paramEntryVal = (unsigned int)(*((&globalRegs->PARAMENTRY [paRAMId].OPT) + (unsigned)EDMA3_DRV_PARAM_ENTRY_SRC_DST_BIDX));
                paramEntryVal = paramEntryVal >> 0x10u;
                *currPaRAMFieldVal = paramEntryVal;
                break;

            case EDMA3_DRV_PARAM_FIELD_LINKADDR:
                paramEntryVal = (unsigned int)(*((&globalRegs->PARAMENTRY [paRAMId].OPT) + (unsigned)EDMA3_DRV_PARAM_ENTRY_LINK_BCNTRLD));
                paramEntryVal &= 0x0000FFFFu;
                *currPaRAMFieldVal = paramEntryVal;
                break;

            case EDMA3_DRV_PARAM_FIELD_BCNTRELOAD:
                paramEntryVal = (unsigned int)(*((&globalRegs->PARAMENTRY [paRAMId].OPT) + (unsigned)EDMA3_DRV_PARAM_ENTRY_LINK_BCNTRLD));
                paramEntryVal = paramEntryVal >> 0x10u;
                *currPaRAMFieldVal = paramEntryVal;
                break;

            case EDMA3_DRV_PARAM_FIELD_SRCCIDX:
                paramEntryVal = (unsigned int)(*((&globalRegs->PARAMENTRY [paRAMId].OPT) + (unsigned)EDMA3_DRV_PARAM_ENTRY_SRC_DST_CIDX));
                paramEntryVal &= 0x0000FFFFu;
                *currPaRAMFieldVal = paramEntryVal;
                break;

            case EDMA3_DRV_PARAM_FIELD_DESTCIDX:
                paramEntryVal = (unsigned int)(*((&globalRegs->PARAMENTRY [paRAMId].OPT) + (unsigned)EDMA3_DRV_PARAM_ENTRY_SRC_DST_CIDX));
                paramEntryVal = paramEntryVal >> 0x10u;
                *currPaRAMFieldVal = paramEntryVal;
                break;

            case EDMA3_DRV_PARAM_FIELD_CCNT:
                *currPaRAMFieldVal = (unsigned int)(*((&globalRegs->PARAMENTRY [paRAMId].OPT) + (unsigned)EDMA3_DRV_PARAM_ENTRY_CCNT));
                break;

            default:
                break;
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
 * \brief   Sets EDMA TC priority
 *
 * User can program the priority of the Event Queues at a system-wide level.
 * This means that the user can set the priority of an IO initiated by either
 * of the TCs (Transfer Ctrllers) relative to IO initiated by the other bus
 * masters on the device (ARM, DSP, USB, etc)
 *
 * \param   hEdma           [IN]    Handle to the EDMA Driver Instance
 * \param   evtQPriObj     [IN]    Priority of the Event Queues
 *
 * \return  EDMA3_DRV_SOK or EDMA3_DRV Error Code
 *
 * \note    This function disables the global interrupts while modifying
 *          the global CC Registers, to make it re-entrant.
 */
EDMA3_DRV_Result EDMA3_DRV_setEvtQPriority (EDMA3_DRV_Handle hEdma,
                        const EDMA3_DRV_EvtQuePriority *evtQPriObj)
    {
    unsigned int intState;
    EDMA3_DRV_Instance *drvInst = NULL;
    EDMA3_DRV_Object *drvObject = NULL;
    EDMA3_DRV_Result result = EDMA3_DRV_SOK;
    unsigned int evtQNum = 0;
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
    if ((hEdma == NULL) || (evtQPriObj== NULL))
        {
        result =  EDMA3_DRV_E_INVALID_PARAM;
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
            globalRegs = (volatile EDMA3_CCRL_Regs *)(drvObject->gblCfgParams.globalRegs);

			/* If parameter checking is enabled... */
#ifndef EDMA3_DRV_PARAM_CHECK_DISABLE
            /* check event queue priority first*/
            while (evtQNum < drvObject->gblCfgParams.numEvtQueue)
                {
                if (evtQPriObj->evtQPri[evtQNum] > EDMA3_DRV_QPRIORITY_MAX_VAL)
                    {
                    result = EDMA3_DRV_E_INVALID_PARAM;
                    break;
                    }
                evtQNum++;
                }
#endif

            if (result == EDMA3_DRV_SOK)
                {
                edma3OsProtectEntry (drvObject->phyCtrllerInstId,
									EDMA3_OS_PROTECT_INTERRUPT,
									&intState);

                /* Set TC Priority among system-wide bus-masters and Queue Watermark Level */
                evtQNum = 0;
                while (evtQNum < drvObject->gblCfgParams.numEvtQueue)
                    {
                    globalRegs->QUEPRI = globalRegs->QUEPRI & (unsigned int)EDMA3_RM_QUEPRI_CLR_MASK(evtQNum);
                    globalRegs->QUEPRI |= EDMA3_RM_QUEPRI_SET_MASK(evtQNum, evtQPriObj->evtQPri[evtQNum]);

                    evtQNum++;
                    }

                edma3OsProtectExit (drvObject->phyCtrllerInstId,
									EDMA3_OS_PROTECT_INTERRUPT,
									intState);
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
 * \brief   Associate Channel to Event Queue
 *
 *
 * \param   hEdma       [IN]     Handle to the EDMA Driver Instance
 * \param   channelId   [IN]     Logical Channel to which the Event
 *                               Queue is to be mapped
 * \param   eventQ      [IN]     The Event Queue which is to be mapped
 *                               to the DMA channel
 *
 * \return      EDMA3_DRV_SOK or EDMA3_DRV Error Code
 *
 * \note        There should not be any data transfer going on
 *              while setting the mapping. Results could be unpredictable.
 *
 *              This function disables the global interrupts while modifying
 *              the global CC Registers, to make it re-entrant.
 */
EDMA3_DRV_Result EDMA3_DRV_mapChToEvtQ(EDMA3_DRV_Handle hEdma,
                                unsigned int channelId,
                                EDMA3_RM_EventQueue eventQ)
    {
    EDMA3_DRV_Instance *drvInst = NULL;
    EDMA3_DRV_Object *drvObject = NULL;
    unsigned int intState;
    EDMA3_DRV_Result result = EDMA3_DRV_SOK;
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
    if (hEdma == NULL)
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
        if (drvObject->gblCfgParams.globalRegs == NULL)
            {
            result = EDMA3_DRV_E_INVALID_PARAM;
            }
        }

    if (result == EDMA3_DRV_SOK)
        {
        globalRegs = (volatile EDMA3_CCRL_Regs *)(drvObject->gblCfgParams.globalRegs);

		/* If parameter checking is enabled... */
#ifndef EDMA3_DRV_PARAM_CHECK_DISABLE
        /* Check the event queue */
        if (eventQ >= drvObject->gblCfgParams.numEvtQueue)
            {
            result = EDMA3_DRV_E_INVALID_PARAM;
            }
#endif
        }

    if (result == EDMA3_DRV_SOK)
        {
        if (channelId <= EDMA3_DRV_DMA_CH_MAX_VAL)
            {
            /* DMA channel */
            edma3OsProtectEntry (drvObject->phyCtrllerInstId,
            					EDMA3_OS_PROTECT_INTERRUPT,
            					&intState);

            globalRegs->DMAQNUM[channelId >> 3u] &=
                                        EDMA3_DRV_DMAQNUM_CLR_MASK(channelId);
            globalRegs->DMAQNUM[channelId >> 3u] |=
                                EDMA3_DRV_DMAQNUM_SET_MASK(channelId, eventQ);

            edma3OsProtectExit(drvObject->phyCtrllerInstId,
								EDMA3_OS_PROTECT_INTERRUPT,
								intState);
            }
        else
            {
            if ((channelId >= EDMA3_DRV_QDMA_CH_MIN_VAL)
                     && (channelId <= EDMA3_DRV_QDMA_CH_MAX_VAL))
                {
                /* QDMA channel */
                edma3OsProtectEntry (drvObject->phyCtrllerInstId,
                					EDMA3_OS_PROTECT_INTERRUPT,
                					&intState);

                globalRegs->QDMAQNUM &=
                            EDMA3_DRV_QDMAQNUM_CLR_MASK(channelId-EDMA3_DRV_QDMA_CH_MIN_VAL);
                globalRegs->QDMAQNUM |=
                            EDMA3_DRV_QDMAQNUM_SET_MASK(channelId-EDMA3_DRV_QDMA_CH_MIN_VAL, eventQ);

                edma3OsProtectExit(drvObject->phyCtrllerInstId,
									EDMA3_OS_PROTECT_INTERRUPT,
									intState);
                }
            else
                {
                /* API valid for DMA/QDMA channel only, return error... */
                result = EDMA3_DRV_E_INVALID_PARAM;
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
 * \brief   Get the Event Queue mapped to the specified DMA/QDMA channel.
 *
 * \param   hEdma       [IN]     Handle to the EDMA Driver Instance
 * \param   channelId   [IN]     Logical Channel whose associated
 *                               Event Queue is needed
 * \param   mappedEvtQ  [IN/OUT] The Event Queue which is mapped
 *                               to the DMA/QDMA channel
 *
 * \return  EDMA3_DRV_SOK or EDMA3_DRV Error Code
 *
 * \note    This function is re-entrant.
 */
EDMA3_DRV_Result EDMA3_DRV_getMapChToEvtQ (EDMA3_DRV_Handle hEdma,
                                    unsigned int channelId,
                                    unsigned int *mappedEvtQ)
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
    if (((hEdma == NULL) || (mappedEvtQ == NULL))
        || (channelId > EDMA3_DRV_LOG_CH_MAX_VAL))
        {
        result =  EDMA3_DRV_E_INVALID_PARAM;
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

            if (channelId <= EDMA3_DRV_DMA_CH_MAX_VAL)
                {
                *mappedEvtQ = ((globalRegs->DMAQNUM[channelId >> 3u])
                                & (~(EDMA3_DRV_DMAQNUM_CLR_MASK(channelId))))
                                  >> ((channelId%8u)*4u);
                }
            else
                {
                if ((channelId >= EDMA3_DRV_QDMA_CH_MIN_VAL)
                     &&(channelId <= EDMA3_DRV_QDMA_CH_MAX_VAL))
                    {
                    *mappedEvtQ = ((globalRegs->QDMAQNUM)
                                    & (~(EDMA3_DRV_QDMAQNUM_CLR_MASK(channelId -EDMA3_DRV_QDMA_CH_MIN_VAL))))
                                   >> (channelId*4u);
                    }
                else
                    {
                    /* Only valid for DMA/QDMA channel, return error... */
                    result = EDMA3_DRV_E_INVALID_PARAM;
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
 * \brief   Set the Channel Controller (CC) Register value
 *
 * \param   hEdma       [IN]     Handle to the EDMA Driver Instance
 * \param   regOffset   [IN]     CC Register offset whose value needs to be set
 * \param   newRegValue [IN]     New CC Register Value
 *
 * \return  EDMA3_DRV_SOK or EDMA3_DRV Error Code
 *
 * \note    This function is non re-entrant for users using the same
 *          EDMA handle i.e. working on the same shadow region.
 *          Before modifying a register, it tries to acquire a semaphore
 *          (Driver instance specific), to protect simultaneous
 *          modification of the same register by two different users.
 *          After the successful change, it releases the semaphore.
 *          For users working on different shadow regions, thus different
 *          EDMA handles, this function is re-entrant.
 */
EDMA3_DRV_Result EDMA3_DRV_setCCRegister (EDMA3_DRV_Handle hEdma,
                    unsigned int regOffset,
                    unsigned int newRegValue)
    {
    EDMA3_DRV_Result result = EDMA3_DRV_SOK;
    EDMA3_DRV_Instance *drvInst = NULL;
    EDMA3_DRV_Object *drvObject = NULL;
    volatile unsigned int regPhyAddr = 0x0u;


#ifdef EDMA3_INSTRUMENTATION_ENABLED
    EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3",
                EDMA3_DVT_DESC(EDMA3_DVT_eFUNC_START,
                EDMA3_DVT_dCOUNTER,
                EDMA3_DVT_dNONE,
                EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */


	/* If parameter checking is enabled... */
#ifndef EDMA3_DRV_PARAM_CHECK_DISABLE
    if ((hEdma == NULL) || ((regOffset % 4u) != 0))
        {
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
            if (drvObject->gblCfgParams.globalRegs != NULL)
                {
                /**
                  * Take the instance specific semaphore, to prevent simultaneous
                  * access to the shared resources.
                  */
                result = edma3OsSemTake(drvInst->drvSemHandle,
                                        EDMA3_OSSEM_NO_TIMEOUT);

                if (EDMA3_DRV_SOK == result)
                    {
                    /* Semaphore taken successfully, modify the registers. */
                    regPhyAddr = (unsigned int)(drvObject->gblCfgParams.globalRegs) + regOffset;

                    *(unsigned int *)regPhyAddr = newRegValue;

                    /* Return the semaphore back */
                    result = edma3OsSemGive(drvInst->drvSemHandle);
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
 * \brief   Get the Channel Controller (CC) Register value
 *
 * \param   hEdma       [IN]     Handle to the EDMA Driver Instance
 * \param   regOffset   [IN]     CC Register offset whose value is needed
 * \param   regValue    [IN/OUT] CC Register Value
 *
 * \return  EDMA3_DRV_SOK or EDMA3_DRV Error Code
 *
 * \note    This function is re-entrant.
 */
EDMA3_DRV_Result EDMA3_DRV_getCCRegister ( EDMA3_DRV_Handle hEdma,
                    unsigned int regOffset,
                    unsigned int *regValue)
    {
    EDMA3_DRV_Result result = EDMA3_DRV_SOK;
    EDMA3_DRV_Instance *drvInst = NULL;
    EDMA3_DRV_Object *drvObject = NULL;
    volatile unsigned int regPhyAddr = 0x0u;


#ifdef EDMA3_INSTRUMENTATION_ENABLED
    EDMA3_LOG_EVENT(&DVTEvent_Log,"EDMA3",
                EDMA3_DVT_DESC(EDMA3_DVT_eFUNC_START,
                EDMA3_DVT_dCOUNTER,
                EDMA3_DVT_dNONE,
                EDMA3_DVT_dNONE));
#endif /* EDMA3_INSTRUMENTATION_ENABLED */


	/* If parameter checking is enabled... */
#ifndef EDMA3_DRV_PARAM_CHECK_DISABLE
    if (((hEdma == NULL) || (regValue == NULL))
        || ((regOffset % 4u) != 0))
        {
        result =  EDMA3_DRV_E_INVALID_PARAM;
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
            if (drvObject->gblCfgParams.globalRegs != NULL)
                {
                regPhyAddr = (unsigned int)(drvObject->gblCfgParams.globalRegs) + regOffset;

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



/**
 * \brief   Wait for a transfer completion interrupt to occur and clear it.
 *
 * This is a blocking function that returns when the IPR/IPRH bit corresponding
 * to the tccNo specified, is SET. It clears the corresponding bit while
 * returning also.
 *
 * This function waits for the specific bit indefinitely in a tight loop, with
 * out any delay in between. USE IT CAUTIOUSLY.
 *
 * \param   hEdma       [IN]     Handle to the EDMA Driver Instance
 * \param   tccNo       [IN]     TCC, specific to which the function
 *                               waits on a IPR/IPRH bit.
 *
 * \return  EDMA3_DRV_SOK or EDMA3_DRV Error Code
 *
 * \note    This function is re-entrant for different tccNo.
 */
EDMA3_DRV_Result EDMA3_DRV_waitAndClearTcc (EDMA3_DRV_Handle hEdma,
                    unsigned int tccNo)
    {
    EDMA3_DRV_Result result = EDMA3_DRV_SOK;
    EDMA3_DRV_Instance *drvInst = NULL;
    EDMA3_DRV_Object *drvObject = NULL;
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
            if (tccNo >= drvObject->gblCfgParams.numTccs)
                {
                result = EDMA3_DRV_E_INVALID_PARAM;
                }
#endif

			/* Check if the parameters are OK. */
			if (EDMA3_DRV_SOK == result)
                {
                globalRegs = (volatile EDMA3_CCRL_Regs *)(drvObject->gblCfgParams.globalRegs);
                shadowRegs = (volatile EDMA3_CCRL_ShadowRegs *)
                                        (&globalRegs->SHADOW[drvInst->regionId]);


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


/**
 * \brief   Returns the status of a previously initiated transfer.
 *
 * This is a non-blocking function that returns the status of a previously
 * initiated transfer, based on the IPR/IPRH bit. This bit corresponds to
 * the tccNo specified by the user. It clears the corresponding bit, if SET,
 * while returning also.
 *
 * \param   hEdma       [IN]     Handle to the EDMA Driver Instance
 * \param   tccNo       [IN]     TCC, specific to which the function
 *                               checks the status of the IPR/IPRH bit.
 * \param   tccStatus   [IN/OUT] Status of the transfer is returned here.
 *                               Returns "TRUE" if the transfer has
 *                               completed (IPR/IPRH bit SET),
 *                               "FALSE" if the transfer has not completed
 *                               successfully (IPR/IPRH bit NOT SET).
 *
 * \return  EDMA3_DRV_SOK or EDMA3_DRV Error Code
 *
 * \note    This function is re-entrant for different tccNo.
 */
EDMA3_DRV_Result EDMA3_DRV_checkAndClearTcc (EDMA3_DRV_Handle hEdma,
                    unsigned int tccNo,
                    unsigned short *tccStatus)
    {
    EDMA3_DRV_Result result = EDMA3_DRV_SOK;
    EDMA3_DRV_Instance *drvInst = NULL;
    EDMA3_DRV_Object *drvObject = NULL;
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
#ifndef EDMA3_DRV_PARAM_CHECK_DISABLE
    if ((hEdma == NULL) || (tccStatus == NULL))
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
            if (tccNo >= drvObject->gblCfgParams.numTccs)
                {
                result = EDMA3_DRV_E_INVALID_PARAM;
                }
#endif

			/* Check if the parameters are OK. */
			if (EDMA3_DRV_SOK == result)
                {
                globalRegs = (volatile EDMA3_CCRL_Regs *)(drvObject->gblCfgParams.globalRegs);
                shadowRegs = (volatile EDMA3_CCRL_ShadowRegs *)
                                        (&globalRegs->SHADOW[drvInst->regionId]);

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



/**
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
 *
 * \param   hEdma       [IN]            Handle to the EDMA Driver Instance
 * \param   lCh         [IN]            Logical Channel for which the PaRAM set
 *                                      physical address is required
 * \param   paramPhyAddr [IN/OUT]       PaRAM Set physical address is returned
 *                                      here.
 *
 * \return  EDMA3_DRV_SOK or EDMA3_DRV Error Code
 *
 * \note    This function is re-entrant.
 */
EDMA3_DRV_Result EDMA3_DRV_getPaRAMPhyAddr(EDMA3_DRV_Handle hEdma,
                    unsigned int lCh,
                    unsigned int *paramPhyAddr)
    {
    EDMA3_DRV_Result result = EDMA3_DRV_SOK;
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
    if (((lCh > EDMA3_DRV_LOG_CH_MAX_VAL) || (hEdma == NULL))
        || (paramPhyAddr == NULL))
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
            globalRegs = (volatile EDMA3_CCRL_Regs *)(drvObject->gblCfgParams.globalRegs);
            paRAMId = edma3DrvChBoundRes[drvObject->phyCtrllerInstId][lCh].paRAMId;

            if ((paRAMId < 0) || ((unsigned int)paRAMId >= drvObject->gblCfgParams.numPaRAMSets))
                {
                result = EDMA3_DRV_E_INVALID_PARAM;
                }
            else
                {
                *paramPhyAddr = (unsigned int)&(globalRegs->PARAMENTRY [paRAMId].OPT);
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
 *  \brief EDMA3 Driver IOCTL
 *
 *  This function provides IOCTL functionality for EDMA3 Driver.
 *
 *  \param   hEdma          [IN]        Handle to the EDMA Driver Instance
 *  \param  cmd             [IN]        IOCTL command to be performed
 *  \param  cmdArg          [IN/OUT]    IOCTL command argument (if any)
 *  \param  param           [IN/OUT]    Device/Cmd specific argument
 *
 *  \return EDMA3_DRV_SOK or EDMA3_DRV Error Code
 */
EDMA3_DRV_Result EDMA3_DRV_Ioctl(
                      EDMA3_DRV_Handle       hEdma,
                      EDMA3_DRV_IoctlCmd     cmd,
                      void                  *cmdArg,
                      void                  *param
                     )
    {
    EDMA3_DRV_Result result = EDMA3_DRV_SOK;
    EDMA3_DRV_Instance *drvInst = NULL;

    /* To remove CCS warnings */
    (void)param;

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

    if ((cmd <= EDMA3_DRV_IOCTL_MIN_IOCTL)
        || (cmd >= EDMA3_DRV_IOCTL_MAX_IOCTL))
        {
        result = EDMA3_DRV_E_INVALID_PARAM;
        }
#endif

    if (result == EDMA3_DRV_SOK)
        {
        drvInst = (EDMA3_DRV_Instance *)hEdma;

        if (drvInst == NULL)
            {
            result = EDMA3_DRV_E_INVALID_PARAM;
            }
        }

    if (result == EDMA3_DRV_SOK)
        {
        switch (cmd)
            {
            case EDMA3_DRV_IOCTL_SET_PARAM_CLEAR_OPTION:
                {
                result = EDMA3_RM_Ioctl (drvInst->resMgrInstance, EDMA3_RM_IOCTL_SET_PARAM_CLEAR_OPTION, cmdArg, param);

                break;
                }

            case EDMA3_DRV_IOCTL_GET_PARAM_CLEAR_OPTION:
                {
                if (NULL == cmdArg)
                    {
                    result = EDMA3_DRV_E_INVALID_PARAM;
                    }
                else
                    {
                    result = EDMA3_RM_Ioctl (drvInst->resMgrInstance, EDMA3_RM_IOCTL_GET_PARAM_CLEAR_OPTION, cmdArg, param);
                    }

                break;
                }

            default:
                /* You passed invalid IOCTL cmd */
                result = EDMA3_DRV_E_INVALID_PARAM;
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
 * \brief	Return the previously opened EDMA3 Driver Instance handle
 *
 * This API is used to return the previously opened EDMA3 Driver's
 * Instance Handle (region specific), which could be used to call other
 * EDMA3 Driver APIs. Since EDMA3 Driver does not allow multiple instances,
 * for a single shadow region, this API is provided. This API is meant
 * for users who DO NOT want to / could not open a new Driver Instance and
 * hence re-use the existing Driver Instance to allocate EDMA3 resources
 * and use various other EDMA3 Driver APIs.
 *
 * In case the Driver Instance is not yet opened, NULL is returned as the
 * function return value whereas EDMA3_DRV_E_INST_NOT_OPENED is returned
 * in the errorCode.
 *
 * \param   phyCtrllerInstId    [IN]    EDMA3 Controller Instance Id (Hardware
 *                                      instance id, starting from 0).
 * \param   regionId			[IN]    Shadow Region id for which the previously
 *										opened driver's instance handle is
 *										required.
 * \param   errorCode           [OUT]   Error code while returning Driver Instance
 *										Handle.
 *
 * \return EDMA3_DRV_Handle : If successful, this API will return the
 *                            driver's instance handle.
 *
 * \note    1) This API returns the previously opened EDMA3 Driver's Instance
 * 		handle. The instance, if exists, could have been opened by some other
 *		user (most probably) or may be by the same user calling this API. If
 *		it was opened by some other user, then that user can very well close
 *		this instance anytime, without even knowing that the same instance
 *		handle is being used by other users as well. In that case, the
 *		handle becomes INVALID and user has to open a valid driver
 *		instance for his/her use.
 *
 * 			2) This function is re-entrant.
 */
EDMA3_DRV_Handle EDMA3_DRV_getInstHandle(unsigned int phyCtrllerInstId,
								EDMA3_RM_RegionId regionId,
								EDMA3_DRV_Result *errorCode)
	{
    EDMA3_DRV_Result result = EDMA3_DRV_SOK;
	EDMA3_DRV_Object *drvObject = NULL;
	EDMA3_DRV_Instance *drvInstanceHandle = NULL;

	/* If parameter checking is enabled... */
#ifndef EDMA3_DRV_PARAM_CHECK_DISABLE
    if ((phyCtrllerInstId >= g_EDMA3_NUM_INSTANCES)
        || (errorCode == NULL))
        {
        result = EDMA3_DRV_E_INVALID_PARAM;
        }
#endif

	if (EDMA3_DRV_SOK == result)
		{
		drvObject = &drvObj[phyCtrllerInstId];

		if (NULL == drvObject)
            {
            result = EDMA3_DRV_E_INVALID_PARAM;
            }

		/* If parameter checking is enabled... */
#ifndef EDMA3_DRV_PARAM_CHECK_DISABLE
	/* Check regionId. */
    if (regionId >= drvObject->gblCfgParams.numRegions)
        {
        result = EDMA3_DRV_E_INVALID_PARAM;
        }
#endif
		}

	if (EDMA3_DRV_SOK == result)
		{
        /* If the driver instance is already opened for this specific region,
        *  return it, else return an error.
        */
		drvInstanceHandle = &drvInstance[phyCtrllerInstId][regionId];

        if (NULL == drvInstanceHandle->pDrvObjectHandle)
            {
            /* Instance not opened yet!!! */
            drvInstanceHandle = NULL;
            result = EDMA3_DRV_E_INST_NOT_OPENED;
            }
		}

	*errorCode = result;
	return (EDMA3_DRV_Handle)drvInstanceHandle;
	}

/* End of File */
