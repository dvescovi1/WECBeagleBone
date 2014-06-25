//
// Copyright (c) MPC Data Limited 2007. All Rights Reserved.
//
//------------------------------------------------------------------------------
//
//  File:  mcasp.c
//
//  This module provides lower layer McASP functions for accessing the
//  peripheral registers.
//


#include <windows.h>
#include "am33x_mcasp.h"
#include "mcasp.h"

#if 0  // not called. comment out
//------------------------------------------------------------------------------
//  Function:  mcaspGetCurrentXSlot
//
//  This function returns the current transmit time slot count
//
UINT16 mcaspGetCurrentXSlot (PMCASPREGS pMcASPRegs)
{
    if (pMcASPRegs == NULL)
    {
        return 0;
    }
    
    return pMcASPRegs->XSLOT & AM3X_MCASP_XSLOT_MASK;
}

//------------------------------------------------------------------------------
//  Function:  mcaspGetCurrentRSlot
//
//  This function returns the current receive time slot count
//
UINT16 mcaspGetCurrentRSlot (PMCASPREGS pMcASPRegs)
{
    return pMcASPRegs->RSLOT & AM3X_MCASP_RSLOT_MASK;
}

//------------------------------------------------------------------------------
//  Function: mcaspGetXmtErr  
//
//  This function checks whether transmitter error interrupt has occurred
//  or not
//
BOOL mcaspGetXmtErr (PMCASPREGS pMcASPRegs)
{
    if (pMcASPRegs == NULL)
    {
        return FALSE;
    }
    return pMcASPRegs->XSTAT & AM3X_MCASP_XSTAT_TXERR_MASK ? TRUE : FALSE;
}

//------------------------------------------------------------------------------
//  Function: mcaspGetXmtClkFail  
//
//  This function checks whether transmit clock failure flag is set or not
//
BOOL mcaspGetXmtClkFail (PMCASPREGS pMcASPRegs)
{
    if (pMcASPRegs == NULL)
    {
        return FALSE;
    }
    
    return pMcASPRegs->XSTAT & AM3X_MCASP_XSTAT_TXBADCLK_MASK ? TRUE : FALSE;
}

//------------------------------------------------------------------------------
//  Function: mcaspGetXmtSyncErr  
//
//  This function checks whether unexpected transmit frame sync flag is set
//  or not
//
BOOL mcaspGetXmtSyncErr (PMCASPREGS pMcASPRegs)
{
    if (pMcASPRegs == NULL)
    {
        return FALSE;
    }
    
    return pMcASPRegs->XSTAT & AM3X_MCASP_XSTAT_TXUNFSR_MASK ? TRUE : FALSE; 
}

//------------------------------------------------------------------------------
//  Function: mcaspGetXmtUnderrun  
//
//  This function checks whether transmitter underrun flag is set or not
//
BOOL mcaspGetXmtUnderrun (PMCASPREGS pMcASPRegs)
{
    if (pMcASPRegs == NULL)
    {
        return FALSE;
    }
    
    return pMcASPRegs->XSTAT & AM3X_MCASP_XSTAT_TXUNDRN_MASK ? TRUE : FALSE;
}

//------------------------------------------------------------------------------
//  Function: mcaspGetXmtDataReady  
//
//  This function checks whether transmit data ready flag is set or not
//
BOOL mcaspGetXmtDataReady (PMCASPREGS pMcASPRegs)
{
    if (pMcASPRegs == NULL)
    {
        return FALSE;
    }
    
    return pMcASPRegs->XSTAT & AM3X_MCASP_XSTAT_TXDATA_MASK ? TRUE : FALSE;
}

//------------------------------------------------------------------------------
//  Function: mcaspGetRcvErr  
//
//  This function checks whether receiver error interrupt has occurred
//  or not
//
BOOL mcaspGetRcvErr (PMCASPREGS pMcASPRegs)
{
    if (pMcASPRegs == NULL)
    {
        return FALSE;
    }
    
    return pMcASPRegs->RSTAT & AM3X_MCASP_RSTAT_RXERR_MASK ? TRUE : FALSE;
}

//------------------------------------------------------------------------------
//  Function: mcaspGetRcvClkFail  
//
//  This function checks whether receive clock failure flag is set or not
//
BOOL mcaspGetRcvClkFail (PMCASPREGS pMcASPRegs)
{
    if (pMcASPRegs == NULL)
    {
        return FALSE;
    }
    
    return pMcASPRegs->RSTAT & AM3X_MCASP_RSTAT_RXBADCLK_MASK ? TRUE : FALSE;
}

//------------------------------------------------------------------------------
//  Function: mcaspGetRcvSyncErr  
//
//  This function checks whether unexpected receive frame sync flag is set
//  or not
//
BOOL mcaspGetRcvSyncErr (PMCASPREGS pMcASPRegs)
{
    if (pMcASPRegs == NULL)
    {
        return FALSE;
    }
    
    return pMcASPRegs->RSTAT & AM3X_MCASP_RSTAT_RXUNFSR_MASK ? TRUE : FALSE;
}

//------------------------------------------------------------------------------
//  Function: mcaspGetRcvOverrun  
//
//  This function checks whether receiver overrun flag is set or not
//
BOOL mcaspGetRcvOverrun (PMCASPREGS pMcASPRegs)
{
    if (pMcASPRegs == NULL)
    {
        return FALSE;
    }
    
    return pMcASPRegs->RSTAT & AM3X_MCASP_RSTAT_RXOVRN_MASK ? TRUE : FALSE;
}

//------------------------------------------------------------------------------
//  Function: mcaspGetRcvDataReady  
//
//  This function checks whether receive data ready flag is set or not
//
BOOL mcaspGetRcvDataReady (PMCASPREGS pMcASPRegs)
{
    if (pMcASPRegs == NULL)
    {
        return FALSE;
    }
    
    return pMcASPRegs->RSTAT & AM3X_MCASP_RSTAT_RXDATA_MASK ? TRUE : FALSE;
}

//------------------------------------------------------------------------------
//  Function: mcaspGetSerRcvReady  
//
//  This function checks whether receive buffer ready bit of serializer
//  control register is set or not
//
//  
BOOL mcaspGetSerRcvReady (PMCASPREGS pMcASPRegs,
                                      BOOL *serRcvReady,
                                      McaspSerializerNum  serNum)
{
    BOOL bRC = TRUE;

    if (pMcASPRegs == NULL || serRcvReady == NULL)
    {
        return FALSE;
    }
    
    switch (serNum)
    {
        case SERIALIZER_0:                       
            *serRcvReady = pMcASPRegs->SRCTL0 & AM3X_MCASP_SRCTL0_RXSTATE_MASK ? TRUE : FALSE;                       
            break;
        case SERIALIZER_1:
            *serRcvReady = pMcASPRegs->SRCTL1 & AM3X_MCASP_SRCTL1_RXSTATE_MASK ? TRUE : FALSE;
            break;
        case SERIALIZER_2:
            *serRcvReady = pMcASPRegs->SRCTL2 & AM3X_MCASP_SRCTL2_RXSTATE_MASK ? TRUE : FALSE;
            break;
        case SERIALIZER_3:
            *serRcvReady = pMcASPRegs->SRCTL3 & AM3X_MCASP_SRCTL3_RXSTATE_MASK ? TRUE : FALSE;
            break;
        case SERIALIZER_4:
            *serRcvReady = pMcASPRegs->SRCTL4 & AM3X_MCASP_SRCTL4_RXSTATE_MASK ? TRUE : FALSE;
            break;
        case SERIALIZER_5:
            *serRcvReady = pMcASPRegs->SRCTL5 & AM3X_MCASP_SRCTL5_RXSTATE_MASK ? TRUE : FALSE;
            break;
        case SERIALIZER_6:
            *serRcvReady = pMcASPRegs->SRCTL6 & AM3X_MCASP_SRCTL6_RXSTATE_MASK ? TRUE : FALSE;
            break;
        case SERIALIZER_7:
            *serRcvReady = pMcASPRegs->SRCTL7 & AM3X_MCASP_SRCTL7_RXSTATE_MASK ? TRUE : FALSE;
            break;
        case SERIALIZER_8:
            *serRcvReady = pMcASPRegs->SRCTL8 & AM3X_MCASP_SRCTL8_RXSTATE_MASK ? TRUE : FALSE;
            break;
        case SERIALIZER_9:
            *serRcvReady = pMcASPRegs->SRCTL9 & AM3X_MCASP_SRCTL9_RXSTATE_MASK ? TRUE : FALSE;
            break;
        case SERIALIZER_10:
            *serRcvReady = pMcASPRegs->SRCTL10 & AM3X_MCASP_SRCTL10_RXSTATE_MASK ? TRUE : FALSE;
            break;
        case SERIALIZER_11:
            *serRcvReady = pMcASPRegs->SRCTL11 & AM3X_MCASP_SRCTL11_RXSTATE_MASK ? TRUE : FALSE;
            break;
        case SERIALIZER_12:
            *serRcvReady = pMcASPRegs->SRCTL12 & AM3X_MCASP_SRCTL12_RXSTATE_MASK ? TRUE : FALSE;
            break;
        case SERIALIZER_13:
            *serRcvReady = pMcASPRegs->SRCTL13 & AM3X_MCASP_SRCTL13_RXSTATE_MASK ? TRUE : FALSE;
            break;
        case SERIALIZER_14:
            *serRcvReady = pMcASPRegs->SRCTL14 & AM3X_MCASP_SRCTL14_RXSTATE_MASK ? TRUE : FALSE;
            break;
        case SERIALIZER_15:
            *serRcvReady = pMcASPRegs->SRCTL15 & AM3X_MCASP_SRCTL15_RXSTATE_MASK ? TRUE : FALSE;
            break;
        default:
            bRC = FALSE;
    }

    return bRC;
}

//------------------------------------------------------------------------------
//  Function: mcaspGetSerXmtReady  
//
//  This function checks whether transmit buffer ready bit of serializer
//  control register is set or not
//
//  
BOOL mcaspGetSerXmtReady (PMCASPREGS pMcASPRegs,
                                      BOOL *serXmtReady,
                                      McaspSerializerNum  serNum)
{
    BOOL bRC = TRUE;

    if (pMcASPRegs == NULL || serXmtReady == NULL)
    {
        return FALSE;
    }
    
    switch (serNum)
    {
        case SERIALIZER_0:                       
            *serXmtReady = pMcASPRegs->SRCTL0 & AM3X_MCASP_SRCTL0_TXSTATE_MASK ? TRUE : FALSE;                       
            break;
        case SERIALIZER_1:
            *serXmtReady = pMcASPRegs->SRCTL1 & AM3X_MCASP_SRCTL1_TXSTATE_MASK ? TRUE : FALSE;
            break;
        case SERIALIZER_2:
            *serXmtReady = pMcASPRegs->SRCTL2 & AM3X_MCASP_SRCTL2_TXSTATE_MASK ? TRUE : FALSE;
            break;
        case SERIALIZER_3:
            *serXmtReady = pMcASPRegs->SRCTL3 & AM3X_MCASP_SRCTL3_TXSTATE_MASK ? TRUE : FALSE;
            break;
        case SERIALIZER_4:
            *serXmtReady = pMcASPRegs->SRCTL4 & AM3X_MCASP_SRCTL4_TXSTATE_MASK ? TRUE : FALSE;
            break;
        case SERIALIZER_5:
            *serXmtReady = pMcASPRegs->SRCTL5 & AM3X_MCASP_SRCTL5_TXSTATE_MASK ? TRUE : FALSE;
            break;
        case SERIALIZER_6:
            *serXmtReady = pMcASPRegs->SRCTL6 & AM3X_MCASP_SRCTL6_TXSTATE_MASK ? TRUE : FALSE;
            break;
        case SERIALIZER_7:
            *serXmtReady = pMcASPRegs->SRCTL7 & AM3X_MCASP_SRCTL7_TXSTATE_MASK ? TRUE : FALSE;
            break;
        case SERIALIZER_8:
            *serXmtReady = pMcASPRegs->SRCTL8 & AM3X_MCASP_SRCTL8_TXSTATE_MASK ? TRUE : FALSE;
            break;
        case SERIALIZER_9:
            *serXmtReady = pMcASPRegs->SRCTL9 & AM3X_MCASP_SRCTL9_TXSTATE_MASK ? TRUE : FALSE;
            break;
        case SERIALIZER_10:
            *serXmtReady = pMcASPRegs->SRCTL10 & AM3X_MCASP_SRCTL10_TXSTATE_MASK ? TRUE : FALSE;
            break;
        case SERIALIZER_11:
            *serXmtReady = pMcASPRegs->SRCTL11 & AM3X_MCASP_SRCTL11_TXSTATE_MASK ? TRUE : FALSE;
            break;
        case SERIALIZER_12:
            *serXmtReady = pMcASPRegs->SRCTL12 & AM3X_MCASP_SRCTL12_TXSTATE_MASK ? TRUE : FALSE;
            break;
        case SERIALIZER_13:
            *serXmtReady = pMcASPRegs->SRCTL13 & AM3X_MCASP_SRCTL13_TXSTATE_MASK ? TRUE : FALSE;
            break;
        case SERIALIZER_14:
            *serXmtReady = pMcASPRegs->SRCTL14 & AM3X_MCASP_SRCTL14_TXSTATE_MASK ? TRUE : FALSE;
            break;
        case SERIALIZER_15:
            *serXmtReady = pMcASPRegs->SRCTL15 & AM3X_MCASP_SRCTL15_TXSTATE_MASK ? TRUE : FALSE;
            break;
        default:
            bRC = FALSE;
    }

    return bRC;
}

//------------------------------------------------------------------------------
//  Function: mcaspGetSerMode  
//
//  This function gets the serializer mode
//
//  
BOOL mcaspGetSerMode (PMCASPREGS pMcASPRegs, McaspSerMode *serMode,
                                  McaspSerializerNum  serNum)
{
    BOOL bRC = TRUE;

    if (pMcASPRegs == NULL || serMode == NULL)
    {
        return FALSE;
    }
    
    switch (serNum)
    {
        case SERIALIZER_0:                               
            *serMode = (McaspSerMode)(pMcASPRegs->SRCTL0 & AM3X_MCASP_SRCTL0_MODE_MASK);
            break;
        case SERIALIZER_1:
            *serMode = (McaspSerMode)(pMcASPRegs->SRCTL1 & AM3X_MCASP_SRCTL1_MODE_MASK);
            break;
        case SERIALIZER_2:
            *serMode = (McaspSerMode)(pMcASPRegs->SRCTL2 & AM3X_MCASP_SRCTL2_MODE_MASK);
            break;
        case SERIALIZER_3:
            *serMode = (McaspSerMode)(pMcASPRegs->SRCTL3 & AM3X_MCASP_SRCTL3_MODE_MASK);
            break;
        case SERIALIZER_4:
            *serMode = (McaspSerMode)(pMcASPRegs->SRCTL4 & AM3X_MCASP_SRCTL4_MODE_MASK);
            break;
        case SERIALIZER_5:
            *serMode = (McaspSerMode)(pMcASPRegs->SRCTL5 & AM3X_MCASP_SRCTL5_MODE_MASK);
            break;
        case SERIALIZER_6:
            *serMode = (McaspSerMode)(pMcASPRegs->SRCTL6 & AM3X_MCASP_SRCTL6_MODE_MASK);
            break;
        case SERIALIZER_7:
            *serMode = (McaspSerMode)(pMcASPRegs->SRCTL7 & AM3X_MCASP_SRCTL7_MODE_MASK);
            break;
        case SERIALIZER_8:
            *serMode = (McaspSerMode)(pMcASPRegs->SRCTL8 & AM3X_MCASP_SRCTL8_MODE_MASK);
            break;
        case SERIALIZER_9:
            *serMode = (McaspSerMode)(pMcASPRegs->SRCTL9 & AM3X_MCASP_SRCTL9_MODE_MASK);
            break;
        case SERIALIZER_10:
            *serMode = (McaspSerMode)(pMcASPRegs->SRCTL10 & AM3X_MCASP_SRCTL10_MODE_MASK);
            break;
        case SERIALIZER_11:
            *serMode = (McaspSerMode)(pMcASPRegs->SRCTL11 & AM3X_MCASP_SRCTL11_MODE_MASK);
            break;
        case SERIALIZER_12:
            *serMode = (McaspSerMode)(pMcASPRegs->SRCTL12 & AM3X_MCASP_SRCTL12_MODE_MASK);
            break;
        case SERIALIZER_13:
            *serMode = (McaspSerMode)(pMcASPRegs->SRCTL13 & AM3X_MCASP_SRCTL13_MODE_MASK);
            break;
        case SERIALIZER_14:
            *serMode = (McaspSerMode)(pMcASPRegs->SRCTL14 & AM3X_MCASP_SRCTL14_MODE_MASK);
            break;
        case SERIALIZER_15:
            *serMode = (McaspSerMode)(pMcASPRegs->SRCTL15 & AM3X_MCASP_SRCTL15_MODE_MASK);
            break;
        default:
            bRC = FALSE;
        }

    return bRC;
}

//------------------------------------------------------------------------------
//  Function: mcaspGetXmtStat  
//
//  This function returns the value of XSTAT register.
//
//
UINT16 mcaspGetXmtStat (PMCASPREGS pMcASPRegs)
{
    if (pMcASPRegs == NULL)
    {
        return 0;
    }
    
    return (UINT16)(pMcASPRegs->XSTAT);
}

//------------------------------------------------------------------------------
//  Function: mcaspGetSmFsXmt  
//
//  This function returns the XSMRST and XFRST field values of XGBLCTL
//  register.
//
//  Return Value:
//      0x00 - Both transmit frame generator sync and transmit state
//             machine are reset.
//      0x1  - Only transmit state machine is active.
//      0x10 - Only transmit frame sync generator is active.
//      0x11 - Both transmit frame generator sync and transmit state
//             machine are active.
//
UINT8 mcaspGetSmFsXmt (PMCASPREGS pMcASPRegs)
{

    UINT8 smFsXmt = 0;

    if (pMcASPRegs == NULL)
    {
        return 0;
    }
    
#if 0
    smFsXmt = pMcASPRegs->XGBLCTL & AM3X_MCASP_XGBLCTL_TXFSRST_MASK;
    smFsXmt >>= 8;
    smFsXmt |= (pMcASPRegs->XGBLCTL & AM3X_MCASP_XGBLCTL_TXSMRST_MASK) >> AM3X_MCASP_XGBLCTL_TXSMRST_SHIFT;
#else
    smFsXmt  = (UINT8)((pMcASPRegs->XGBLCTL & AM3X_MCASP_XGBLCTL_TXFSRST_MASK) >> 8);
    smFsXmt |= (UINT8)((pMcASPRegs->XGBLCTL & AM3X_MCASP_XGBLCTL_TXSMRST_MASK) >> AM3X_MCASP_XGBLCTL_TXSMRST_SHIFT);
#endif
    
    return smFsXmt;
}

//------------------------------------------------------------------------------
//  Function: mcaspGetSmFsRcv  
//
//  This function returns the RSMRST and RFRST field values of RGBLCTL
//  register.
//
//  Return Value:
//      0x00 - Both receive frame generator sync and receive state
//             machine are reset.
//      0x01 - Only receive state machine is active.
//      0x10 - Only receive frame sync generator is active.
//      0x11 - Both receive frame generator sync and receive state
//             machine are active.
//
UINT8 mcaspGetSmFsRcv (PMCASPREGS pMcASPRegs)
{

    UINT8 smFsRcv = 0;

    if (pMcASPRegs == NULL)
    {
        return 0;
    }
    
#if 0
    smFsRcv = pMcASPRegs->RGBLCTL & AM3X_MCASP_RGBLCTL_RXFSRST_MASK;
    smFsRcv >>= 8;
    smFsRcv |= (pMcASPRegs->RGBLCTL & AM3X_MCASP_RGBLCTL_RXSMRST_MASK) >> AM3X_MCASP_RGBLCTL_RXSMRST_SHIFT;
#else
    smFsRcv  = (UINT8)(pMcASPRegs->RGBLCTL & AM3X_MCASP_RGBLCTL_RXFSRST_MASK);
    smFsRcv |= (UINT8)((pMcASPRegs->RGBLCTL & AM3X_MCASP_RGBLCTL_RXSMRST_MASK) >> AM3X_MCASP_RGBLCTL_RXSMRST_SHIFT);
#endif

    return smFsRcv;
}

//------------------------------------------------------------------------------
//  Function: mcaspGetDitMode  
//
//  This function returns the status of DITEN bit in DITCTL register.
//  register.
//
//
BOOL mcaspGetDitMode (PMCASPREGS pMcASPRegs)
{
    if (pMcASPRegs == NULL)
    {
        return FALSE;
    }
    
    return pMcASPRegs->DITCTL & AM3X_MCASP_DITCTL_DITEN_MASK ? TRUE : FALSE;
}

//------------------------------------------------------------------------------
//  Function: mcaspSetXmtGbl  
//
// This function configures the transmitter global control register 
//
//
void mcaspSetXmtGbl (PMCASPREGS pMcASPRegs, UINT32 xmtGbl)
{
    if (pMcASPRegs == NULL)
    {
        return;
    }
    /* Configure XGBLCTL with the value passed */
    pMcASPRegs->XGBLCTL = xmtGbl;
}

//------------------------------------------------------------------------------
//  Function: mcaspSetRcvGbl  
//
// This function configures the receiver global control register 
//
//
void mcaspSetRcvGbl (PMCASPREGS pMcASPRegs, UINT32 rcvGbl)
{
    if (pMcASPRegs == NULL)
    {
        return;
    }
    /* Configure RGBLCTL with the value passed */
    pMcASPRegs->RGBLCTL = rcvGbl;
}

//------------------------------------------------------------------------------
//  Function: mcaspResetXmtFSRst  
//
//  This function resets the transmit frame sync generator reset enable bit
//  of transmit global control register 
//
//
void mcaspResetXmtFSRst (PMCASPREGS pMcASPRegs)
{
    if (pMcASPRegs == NULL)
    {
        return;
    }
    
    // Clear TXFSRST bit
    pMcASPRegs->XGBLCTL &= ~AM3X_MCASP_XGBLCTL_TXFSRST_MASK; 
}

//------------------------------------------------------------------------------
//  Function: mcaspResetRcvFSRst  
//
//  This function resets the receive frame sync generator reset enable bit
//  of receive global control register 
//
//
void mcaspResetRcvFSRst (PMCASPREGS pMcASPRegs)
{
    if (pMcASPRegs == NULL)
    {
        return;
    }
    
    // Clear RXFSRST bit
    pMcASPRegs->RGBLCTL &= ~AM3X_MCASP_RGBLCTL_RXFSRST_MASK; 
}

//------------------------------------------------------------------------------
//  Function: mcaspConfigAudioMute  
//
//  This function configures the AMUTE register with specified values 
//
//
void mcaspConfigAudioMute (PMCASPREGS pMcASPRegs, UINT32 audioMute)
{
    if (pMcASPRegs == NULL)
    {
        return;
    }
    
    /* Configure AMUTE register */
    pMcASPRegs->AMUTE = audioMute;
}


//------------------------------------------------------------------------------
//  Function: mcaspConfigLoopBack  
//
//  This function sets the digital loopback mode 
//
//
void mcaspConfigLoopBack (PMCASPREGS pMcASPRegs, BOOL loopBack, UINT8 numSerializers)
{

    BOOL    loopBackEnable      = FALSE;
    BOOL    orderBit            = FALSE;
    INT16   serNum              = 0;

    if (pMcASPRegs == NULL)
    {
        return;
    }
    
    /* Reset the RSRCLR and XSRCLR registers in GBLCTL */
    pMcASPRegs->GBLCTL &= ~AM3X_MCASP_GBLCTL_RXSERCLR_MASK;
    pMcASPRegs->GBLCTL &= ~AM3X_MCASP_GBLCTL_TXSERCLR_MASK;


    /* Reset the RSMRST and XSMRST registers in GBLCTL */
    pMcASPRegs->GBLCTL &= ~AM3X_MCASP_GBLCTL_RXSMRST_MASK;
    pMcASPRegs->GBLCTL &= ~AM3X_MCASP_GBLCTL_TXSMRST_MASK;
    
    /* Reset the RFRST and XFRST registers in GBLCTL */
    pMcASPRegs->GBLCTL &= ~AM3X_MCASP_GBLCTL_RXFSRST_MASK;
    pMcASPRegs->GBLCTL &= ~AM3X_MCASP_GBLCTL_TXFSRST_MASK;

    /* configure loop back mode */
    pMcASPRegs->DLBCTL &= ~AM3X_MCASP_DLBCTL_LBEN_MASK;
    pMcASPRegs->DLBCTL |= AM3X_MCASP_DLBCTL_LBEN_ENABLE;

    loopBackEnable = pMcASPRegs->DLBCTL & AM3X_MCASP_DLBCTL_LBEN_ENABLE ? TRUE : FALSE;
    
    if (loopBackEnable == TRUE)
    {   
        pMcASPRegs->DLBCTL &= ~AM3X_MCASP_DLBCTL_LBGENMODE_MASK;
        pMcASPRegs->DLBCTL |= AM3X_MCASP_DLBCTL_LBGENMODE_TXMTCLK <<
            AM3X_MCASP_DLBCTL_LBGENMODE_SHIFT;
    }

    orderBit = pMcASPRegs->DLBCTL & AM3X_MCASP_DLBCTL_LBORD_MASK ? TRUE : FALSE;
    
    if (orderBit == TRUE)
    {

        while (serNum < numSerializers)
        {


            switch(serNum )
            {

            case 15:
                pMcASPRegs->SRCTL15 &= ~AM3X_MCASP_SRCTL15_MODE_MASK;
                pMcASPRegs->SRCTL15 |= AM3X_MCASP_SRCTL15_MODE_RCV <<
                    AM3X_MCASP_SRCTL15_MODE_SHIFT;
                
                pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR15_MASK;
                
                break;
            case 14:
                pMcASPRegs->SRCTL14 &= ~AM3X_MCASP_SRCTL14_MODE_MASK;
                pMcASPRegs->SRCTL14 |= AM3X_MCASP_SRCTL14_MODE_RCV << 
                    AM3X_MCASP_SRCTL14_MODE_SHIFT;

                pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR14_MASK;
                break;
            case 13:
                pMcASPRegs->SRCTL13 &= ~AM3X_MCASP_SRCTL13_MODE_MASK;
                pMcASPRegs->SRCTL13 |= AM3X_MCASP_SRCTL13_MODE_RCV <<
                    AM3X_MCASP_SRCTL13_MODE_SHIFT;

                pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR13_MASK;
                break;
            case 12:
                pMcASPRegs->SRCTL12 &= ~AM3X_MCASP_SRCTL12_MODE_MASK;
                pMcASPRegs->SRCTL12 |= AM3X_MCASP_SRCTL12_MODE_RCV << 
                    AM3X_MCASP_SRCTL12_MODE_SHIFT;

                pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR12_MASK;
                break;
            case 11:
                pMcASPRegs->SRCTL11 &= ~AM3X_MCASP_SRCTL11_MODE_MASK;
                pMcASPRegs->SRCTL11 |= AM3X_MCASP_SRCTL11_MODE_RCV <<
                    AM3X_MCASP_SRCTL11_MODE_SHIFT;

                pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR11_MASK;

                break;
            case 10:
                pMcASPRegs->SRCTL10 &= ~AM3X_MCASP_SRCTL10_MODE_MASK;
                pMcASPRegs->SRCTL10 |= AM3X_MCASP_SRCTL10_MODE_RCV <<
                    AM3X_MCASP_SRCTL10_MODE_SHIFT;

                pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR10_MASK;
                break;
            case 9:
                pMcASPRegs->SRCTL9 &= ~AM3X_MCASP_SRCTL9_MODE_MASK;
                pMcASPRegs->SRCTL9 |= AM3X_MCASP_SRCTL9_MODE_RCV <<
                    AM3X_MCASP_SRCTL9_MODE_SHIFT;

                pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR9_MASK;
                break;
            case 8:
                pMcASPRegs->SRCTL8 &= ~AM3X_MCASP_SRCTL8_MODE_MASK;
                pMcASPRegs->SRCTL8 |= AM3X_MCASP_SRCTL8_MODE_RCV <<
                    AM3X_MCASP_SRCTL8_MODE_SHIFT;

                pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR8_MASK;
                break;
            case 7:
                pMcASPRegs->SRCTL7 &= ~AM3X_MCASP_SRCTL7_MODE_MASK;
                pMcASPRegs->SRCTL7 |= AM3X_MCASP_SRCTL7_MODE_RCV <<
                    AM3X_MCASP_SRCTL7_MODE_SHIFT;

                pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR7_MASK;
                break;
            case 6:
                pMcASPRegs->SRCTL6 &= ~AM3X_MCASP_SRCTL6_MODE_MASK;
                pMcASPRegs->SRCTL6 |= AM3X_MCASP_SRCTL6_MODE_RCV <<
                    AM3X_MCASP_SRCTL6_MODE_SHIFT;

                pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR6_MASK;
                break;
            case 5:
                pMcASPRegs->SRCTL5 &= ~AM3X_MCASP_SRCTL5_MODE_MASK;
                pMcASPRegs->SRCTL5 |= AM3X_MCASP_SRCTL5_MODE_RCV <<
                    AM3X_MCASP_SRCTL5_MODE_SHIFT;

                pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR5_MASK;
                break;
            case 4:
                pMcASPRegs->SRCTL4 &= ~AM3X_MCASP_SRCTL4_MODE_MASK;
                pMcASPRegs->SRCTL4 |= AM3X_MCASP_SRCTL4_MODE_RCV <<
                    AM3X_MCASP_SRCTL4_MODE_SHIFT;

                pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR4_MASK;
                break;
            case 3:
                pMcASPRegs->SRCTL3 &= ~AM3X_MCASP_SRCTL3_MODE_MASK;
                pMcASPRegs->SRCTL3 |= AM3X_MCASP_SRCTL3_MODE_RCV <<
                    AM3X_MCASP_SRCTL3_MODE_SHIFT;

                pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR3_MASK;
                break;
            case 2:
                pMcASPRegs->SRCTL2 &= ~AM3X_MCASP_SRCTL2_MODE_MASK;
                pMcASPRegs->SRCTL2 |= AM3X_MCASP_SRCTL2_MODE_RCV <<
                    AM3X_MCASP_SRCTL2_MODE_SHIFT;

                pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR2_MASK;
                break;
            case 1:
                pMcASPRegs->SRCTL1 &= ~AM3X_MCASP_SRCTL1_MODE_MASK;
                pMcASPRegs->SRCTL1 |= AM3X_MCASP_SRCTL1_MODE_RCV <<
                    AM3X_MCASP_SRCTL1_MODE_SHIFT;

                pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR1_MASK;
                break;
            case 0:
                pMcASPRegs->SRCTL0 &= ~AM3X_MCASP_SRCTL0_MODE_MASK;
                pMcASPRegs->SRCTL0 |= AM3X_MCASP_SRCTL0_MODE_RCV <<
                    AM3X_MCASP_SRCTL0_MODE_SHIFT;

                pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR0_MASK;
                break;

            default:
                break;
            }
            serNum++;
        }

    }
    else {

        while (serNum < numSerializers)
        {

            switch(serNum)
            {

            case 15:
                pMcASPRegs->SRCTL15 &= ~AM3X_MCASP_SRCTL15_MODE_MASK;
                pMcASPRegs->SRCTL15 |= AM3X_MCASP_SRCTL15_MODE_XMT <<
                    AM3X_MCASP_SRCTL15_MODE_SHIFT;

                pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR15_MASK;
                pMcASPRegs->PDIR |= AM3X_MCASP_PDIR_AFSR_OUTPUT << AM3X_MCASP_PDIR_AXR15_SHIFT;
                break;
            case 14:
                pMcASPRegs->SRCTL14 &= ~AM3X_MCASP_SRCTL14_MODE_MASK;
                pMcASPRegs->SRCTL14 |= AM3X_MCASP_SRCTL14_MODE_XMT <<
                    AM3X_MCASP_SRCTL14_MODE_SHIFT;

                pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR14_MASK;
                pMcASPRegs->PDIR |= AM3X_MCASP_PDIR_AFSR_OUTPUT << AM3X_MCASP_PDIR_AXR14_SHIFT;
                break;
            case 13:
                pMcASPRegs->SRCTL13 &= ~AM3X_MCASP_SRCTL13_MODE_MASK;
                pMcASPRegs->SRCTL13 |= AM3X_MCASP_SRCTL13_MODE_XMT <<
                    AM3X_MCASP_SRCTL13_MODE_SHIFT;

                pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR13_MASK;
                pMcASPRegs->PDIR |= AM3X_MCASP_PDIR_AFSR_OUTPUT << AM3X_MCASP_PDIR_AXR13_SHIFT;
                break;
            case 12:
                pMcASPRegs->SRCTL12 &= ~AM3X_MCASP_SRCTL12_MODE_MASK;
                pMcASPRegs->SRCTL12 |= AM3X_MCASP_SRCTL12_MODE_XMT <<
                    AM3X_MCASP_SRCTL12_MODE_SHIFT;

                pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR12_MASK;
                pMcASPRegs->PDIR |= AM3X_MCASP_PDIR_AFSR_OUTPUT << AM3X_MCASP_PDIR_AXR12_SHIFT;
                break;
            case 11:
                pMcASPRegs->SRCTL11 &= ~AM3X_MCASP_SRCTL11_MODE_MASK;
                pMcASPRegs->SRCTL11 |= AM3X_MCASP_SRCTL11_MODE_XMT <<
                    AM3X_MCASP_SRCTL11_MODE_SHIFT;

                pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR11_MASK;
                pMcASPRegs->PDIR |= AM3X_MCASP_PDIR_AFSR_OUTPUT << AM3X_MCASP_PDIR_AXR11_SHIFT;
                break;

            case 10:
                pMcASPRegs->SRCTL10 &= ~AM3X_MCASP_SRCTL10_MODE_MASK;
                pMcASPRegs->SRCTL10 |= AM3X_MCASP_SRCTL10_MODE_XMT <<
                    AM3X_MCASP_SRCTL10_MODE_SHIFT;

                pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR10_MASK;
                pMcASPRegs->PDIR |= AM3X_MCASP_PDIR_AFSR_OUTPUT << AM3X_MCASP_PDIR_AXR10_SHIFT;
                break;

            case 9:
                pMcASPRegs->SRCTL9 &= ~AM3X_MCASP_SRCTL9_MODE_MASK;
                pMcASPRegs->SRCTL9 |= AM3X_MCASP_SRCTL9_MODE_XMT <<
                    AM3X_MCASP_SRCTL9_MODE_SHIFT;

                pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR9_MASK;
                pMcASPRegs->PDIR |= AM3X_MCASP_PDIR_AFSR_OUTPUT << AM3X_MCASP_PDIR_AXR9_SHIFT;
                break;

            case 8:
                pMcASPRegs->SRCTL8 &= ~AM3X_MCASP_SRCTL8_MODE_MASK;
                pMcASPRegs->SRCTL8 |= AM3X_MCASP_SRCTL8_MODE_XMT <<
                    AM3X_MCASP_SRCTL8_MODE_SHIFT;

                pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR8_MASK;
                pMcASPRegs->PDIR |= AM3X_MCASP_PDIR_AFSR_OUTPUT << AM3X_MCASP_PDIR_AXR8_SHIFT;
                break;

            case 7:
                pMcASPRegs->SRCTL7 &= ~AM3X_MCASP_SRCTL7_MODE_MASK;
                pMcASPRegs->SRCTL7 |= AM3X_MCASP_SRCTL7_MODE_XMT <<
                    AM3X_MCASP_SRCTL7_MODE_SHIFT;

                pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR7_MASK;
                pMcASPRegs->PDIR |= AM3X_MCASP_PDIR_AFSR_OUTPUT << AM3X_MCASP_PDIR_AXR7_SHIFT;
                break;

            case 6:
                pMcASPRegs->SRCTL6 &= ~AM3X_MCASP_SRCTL6_MODE_MASK;
                pMcASPRegs->SRCTL6 |= AM3X_MCASP_SRCTL6_MODE_XMT <<
                    AM3X_MCASP_SRCTL6_MODE_SHIFT;
                
                pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR6_MASK;
                pMcASPRegs->PDIR |= AM3X_MCASP_PDIR_AFSR_OUTPUT << AM3X_MCASP_PDIR_AXR6_SHIFT;
                break;

            case 5:
                pMcASPRegs->SRCTL5 &= ~AM3X_MCASP_SRCTL5_MODE_MASK;
                pMcASPRegs->SRCTL5 |= AM3X_MCASP_SRCTL5_MODE_XMT <<
                    AM3X_MCASP_SRCTL5_MODE_SHIFT;

                pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR5_MASK;
                pMcASPRegs->PDIR |= AM3X_MCASP_PDIR_AFSR_OUTPUT << AM3X_MCASP_PDIR_AXR5_SHIFT;
                break;
            case 4:
                pMcASPRegs->SRCTL4 &= ~AM3X_MCASP_SRCTL4_MODE_MASK;
                pMcASPRegs->SRCTL4 |= AM3X_MCASP_SRCTL4_MODE_XMT <<
                    AM3X_MCASP_SRCTL4_MODE_SHIFT;

                pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR4_MASK;
                pMcASPRegs->PDIR |= AM3X_MCASP_PDIR_AFSR_OUTPUT << AM3X_MCASP_PDIR_AXR4_SHIFT;
                break;
            case 3:
                pMcASPRegs->SRCTL3 &= ~AM3X_MCASP_SRCTL3_MODE_MASK;
                pMcASPRegs->SRCTL3 |= AM3X_MCASP_SRCTL3_MODE_XMT <<
                    AM3X_MCASP_SRCTL3_MODE_SHIFT; 

                pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR3_MASK;
                pMcASPRegs->PDIR |= AM3X_MCASP_PDIR_AFSR_OUTPUT << AM3X_MCASP_PDIR_AXR3_SHIFT;
                break;
            case 2:
                pMcASPRegs->SRCTL2 &= ~AM3X_MCASP_SRCTL2_MODE_MASK;
                pMcASPRegs->SRCTL2 |= AM3X_MCASP_SRCTL2_MODE_XMT <<
                    AM3X_MCASP_SRCTL2_MODE_SHIFT;

                pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR2_MASK;
                pMcASPRegs->PDIR |= AM3X_MCASP_PDIR_AFSR_OUTPUT << AM3X_MCASP_PDIR_AXR2_SHIFT;
                break;
            case 1:
                pMcASPRegs->SRCTL1 &= ~AM3X_MCASP_SRCTL1_MODE_MASK;
                pMcASPRegs->SRCTL1 |= AM3X_MCASP_SRCTL1_MODE_XMT <<
                    AM3X_MCASP_SRCTL1_MODE_SHIFT;

                pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR1_MASK;
                pMcASPRegs->PDIR |= AM3X_MCASP_PDIR_AFSR_OUTPUT << AM3X_MCASP_PDIR_AXR1_SHIFT;
                break;
            case 0:
                pMcASPRegs->SRCTL0 &= ~AM3X_MCASP_SRCTL0_MODE_MASK;
                pMcASPRegs->SRCTL0 |= AM3X_MCASP_SRCTL0_MODE_XMT <<
                    AM3X_MCASP_SRCTL0_MODE_SHIFT;

                pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR0_MASK;
                pMcASPRegs->PDIR |= AM3X_MCASP_PDIR_AFSR_OUTPUT << AM3X_MCASP_PDIR_AXR0_SHIFT;
                break;

            default:
                break;
            }
            serNum++;
        }
    }


}

//------------------------------------------------------------------------------
//  Function: mcaspConfigRcvSlot  
//
//  This function configures receive slot with value passed. 
//
//
void mcaspConfigRcvSlot (PMCASPREGS pMcASPRegs, UINT32 rcvSlot)
{
    if (pMcASPRegs == NULL)
    {
        return;
    }
    
    /* configure the RTDM register */
    pMcASPRegs->RTDM = rcvSlot;
}

//------------------------------------------------------------------------------
//  Function: mcaspConfigxMTSlot  
//
//  This function configures transmit slot with value passed. 
//
//
void mcaspConfigXmtSlot (PMCASPREGS pMcASPRegs, UINT32 xmtSlot)
{
    if (pMcASPRegs == NULL)
    {
        return;
    }
    
    /* configure the XTDM register */
    pMcASPRegs->XTDM = xmtSlot;
}

//------------------------------------------------------------------------------
//  Function: mcaspConfigRcvInt  
//
//  This function configures the receiver interrupt control register with
//  specified value. 
//
//
void mcaspConfigRcvInt (PMCASPREGS pMcASPRegs, UINT32 rcvInt)
{
    if (pMcASPRegs == NULL)
    {
        return;
    }
    
    /* configure the RINTCTL register */
    pMcASPRegs->RINTCTL = rcvInt;
}

//------------------------------------------------------------------------------
//  Function: mcaspConfigXmtInt  
//
//  This function configures the transmit interrupt control register with
//  specified value. 
//
//
void mcaspConfigXmtInt (PMCASPREGS pMcASPRegs, UINT32 xmtInt)
{
    if (pMcASPRegs == NULL)
    {
        return;
    }
    
    /* configure the XINTCTL register */
    pMcASPRegs->XINTCTL = xmtInt;
}

//------------------------------------------------------------------------------
//  Function: mcaspResetRcvClk  
//
//  This function resets the receive clock circuitry 
//
//
void mcaspResetRcvClk (PMCASPREGS pMcASPRegs)
{
    if (pMcASPRegs == NULL)
    {
        return;
    }
    
    /* Reset RXCLKRCTL */
    pMcASPRegs->GBLCTL &= ~AM3X_MCASP_GBLCTL_RXCLKRST_MASK;
    /* Reset RXHCLKRCTL */
    pMcASPRegs->GBLCTL &= ~AM3X_MCASP_GBLCTL_RXHCLKRST_MASK;
}

//------------------------------------------------------------------------------
//  Function: mcaspResetXmtClk  
//
//  This function resets the transmit clock circuitry 
//
//
void mcaspResetXmtClk (PMCASPREGS pMcASPRegs)
{
    if (pMcASPRegs == NULL)
    {
        return;
    }
    
    /* Reset TXCLKRCTL */
    pMcASPRegs->GBLCTL &= ~AM3X_MCASP_GBLCTL_TXCLKRST_MASK;
    /* Reset TXHCLKRCTL */
    pMcASPRegs->GBLCTL &= ~AM3X_MCASP_GBLCTL_TXHCLKRST_MASK;
}

//------------------------------------------------------------------------------
//  Function: mcaspSetRcvClk  
//
//  This function configures the receive clock circuitry with specified
//  values 
//
//
void mcaspSetRcvClk (PMCASPREGS pMcASPRegs, McaspHwSetupDataClk *rcvClkSet)
{

    UINT32 bitValue = 0;

    if (pMcASPRegs == NULL || rcvClkSet == NULL)
    {
        return;
    }
    
    /* Reset the bits in GBLCTL */
    pMcASPRegs->GBLCTL &= ~AM3X_MCASP_GBLCTL_RXHCLKRST_MASK;
    pMcASPRegs->GBLCTL &= ~AM3X_MCASP_GBLCTL_RXCLKRST_MASK;
    pMcASPRegs->GBLCTL &= ~AM3X_MCASP_GBLCTL_RXSERCLR_MASK;
    pMcASPRegs->GBLCTL &= ~AM3X_MCASP_GBLCTL_RXSMRST_MASK;
    pMcASPRegs->GBLCTL &= ~AM3X_MCASP_GBLCTL_RXFSRST_MASK;
    
    /* Set the High frequency serial clock */
    pMcASPRegs->AHCLKRCTL = rcvClkSet->clkSetupHiClk;
    
    if (AM3X_MCASP_AHCLKRCTL_AHCLKRE_INTERNAL ==
        (pMcASPRegs->AHCLKRCTL & AM3X_MCASP_AHCLKRCTL_AHCLKRE_MASK) >>
         AM3X_MCASP_AHCLKRCTL_AHCLKRE_SHIFT)
    {   
        pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AHCLKR_MASK;
        pMcASPRegs->PDIR |= AM3X_MCASP_PDIR_AHCLKR_OUTPUT << AM3X_MCASP_PDIR_AHCLKR_SHIFT;    
    }

    /* Set the serial clock */
    pMcASPRegs->ACLKRCTL = rcvClkSet->clkSetupClk;

    if (AM3X_MCASP_ACLKRCTL_ACLKRE_INTERNAL == 
        (pMcASPRegs->ACLKRCTL & AM3X_MCASP_ACLKRCTL_ACLKRE_MASK) >> 
            AM3X_MCASP_ACLKRCTL_ACLKRE_SHIFT)
    {
        pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_ACLKR_MASK;
        pMcASPRegs->PDIR |= AM3X_MCASP_PDIR_ACLKR_OUTPUT << AM3X_MCASP_PDIR_ACLKR_SHIFT;
    }

    /* Start the serial clock */
    pMcASPRegs->GBLCTL &= ~AM3X_MCASP_GBLCTL_RXCLKRST_MASK;
    pMcASPRegs->GBLCTL |= AM3X_MCASP_GBLCTL_RXCLKRST_ACTIVE << AM3X_MCASP_GBLCTL_RXCLKRST_SHIFT;
    {
        while (AM3X_MCASP_GBLCTL_RXCLKRST_ACTIVE != bitValue)
        {
            bitValue = (pMcASPRegs->GBLCTL & AM3X_MCASP_GBLCTL_RXCLKRST_MASK) >>
                AM3X_MCASP_GBLCTL_RXCLKRST_SHIFT; 
        }
    }

    /* Start the high frequency clock */
    pMcASPRegs->GBLCTL &= ~AM3X_MCASP_GBLCTL_RXHCLKRST_MASK;
    pMcASPRegs->GBLCTL |= AM3X_MCASP_GBLCTL_RXHCLKRST_ACTIVE << AM3X_MCASP_GBLCTL_RXHCLKRST_SHIFT;
    {
        bitValue = 0;
        while (AM3X_MCASP_GBLCTL_RXHCLKRST_ACTIVE != bitValue)
        {
            bitValue = (pMcASPRegs->GBLCTL & AM3X_MCASP_GBLCTL_RXHCLKRST_MASK) >>
                AM3X_MCASP_GBLCTL_RXHCLKRST_SHIFT;
        }
    }

    /* Set up the receive clock check control register */
    pMcASPRegs->RCLKCHK = rcvClkSet->clkChk;
}

//------------------------------------------------------------------------------
//  Function: mcaspSetXmtClk  
//
//  This function configures the transmit clock circuitry with specified
//  values 
//
//
void mcaspSetXmtClk (PMCASPREGS pMcASPRegs, McaspHwSetupDataClk *xmtClkSet)
{

    UINT32 bitValue = 0;

    if (pMcASPRegs == NULL || xmtClkSet == NULL)
    {
        return;
    }
    
    /* Reset the bits in GBLCTL */
    pMcASPRegs->GBLCTL &= ~AM3X_MCASP_GBLCTL_TXHCLKRST_MASK;
    pMcASPRegs->GBLCTL &= ~AM3X_MCASP_GBLCTL_TXCLKRST_MASK;
    pMcASPRegs->GBLCTL &= ~AM3X_MCASP_GBLCTL_TXSERCLR_MASK;
    pMcASPRegs->GBLCTL &= ~AM3X_MCASP_GBLCTL_TXSMRST_MASK;
    pMcASPRegs->GBLCTL &= ~AM3X_MCASP_GBLCTL_TXFSRST_MASK;
    
    /* Set the High frequency serial clock */
    pMcASPRegs->AHCLKXCTL = xmtClkSet->clkSetupHiClk;
    
    if (AM3X_MCASP_AHCLKXCTL_AHCLKXE_INTERNAL ==
        (pMcASPRegs->AHCLKXCTL & AM3X_MCASP_AHCLKXCTL_AHCLKXE_MASK) >>
         AM3X_MCASP_AHCLKXCTL_AHCLKXE_SHIFT)
    {   
        pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AHCLKX_MASK;
        pMcASPRegs->PDIR |= AM3X_MCASP_PDIR_AHCLKX_OUTPUT << AM3X_MCASP_PDIR_AHCLKX_SHIFT;    
    }

    /* Set the serial clock */
    pMcASPRegs->ACLKXCTL = xmtClkSet->clkSetupClk;

    if (AM3X_MCASP_ACLKXCTL_ACLKXE_INTERNAL == 
        (pMcASPRegs->ACLKXCTL & AM3X_MCASP_ACLKXCTL_ACLKXE_MASK) >> 
            AM3X_MCASP_ACLKXCTL_ACLKXE_SHIFT)
    {
        pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_ACLKX_MASK;
        pMcASPRegs->PDIR |= AM3X_MCASP_PDIR_ACLKX_OUTPUT << AM3X_MCASP_PDIR_ACLKX_SHIFT;
    }

    /* Start the serial clock */
    pMcASPRegs->GBLCTL &= ~AM3X_MCASP_GBLCTL_TXCLKRST_MASK;
    pMcASPRegs->GBLCTL |= AM3X_MCASP_GBLCTL_TXCLKRST_ACTIVE << AM3X_MCASP_GBLCTL_TXCLKRST_SHIFT;
    {
        while (AM3X_MCASP_GBLCTL_TXCLKRST_ACTIVE != bitValue)
        {
            bitValue = (pMcASPRegs->GBLCTL & AM3X_MCASP_GBLCTL_TXCLKRST_MASK) >>
                AM3X_MCASP_GBLCTL_TXCLKRST_SHIFT; 
        }
    }

    /* Start the high frequency clock */
    pMcASPRegs->GBLCTL &= ~AM3X_MCASP_GBLCTL_TXHCLKRST_MASK;
    pMcASPRegs->GBLCTL |= AM3X_MCASP_GBLCTL_TXHCLKRST_ACTIVE << AM3X_MCASP_GBLCTL_TXHCLKRST_SHIFT;
    {
        bitValue = 0;
        while (AM3X_MCASP_GBLCTL_TXHCLKRST_ACTIVE != bitValue)
        {
            bitValue = (pMcASPRegs->GBLCTL & AM3X_MCASP_GBLCTL_TXHCLKRST_MASK) >>
                AM3X_MCASP_GBLCTL_TXHCLKRST_SHIFT;
        }
    }

    /* Set up the transmit clock check control register */
    pMcASPRegs->XCLKCHK = xmtClkSet->clkChk;
}
#endif  // not called. comment out

//------------------------------------------------------------------------------
//  Function: mcaspConfigXmtSection  
//
//  This function configures format, frame sync, and other parameters
//  related to the xmt section. Also configures the xmt clk section. 
//
//
void mcaspConfigXmtSection (PMCASPREGS pMcASPRegs, McaspHwSetupData  *xmtData)
{

    volatile UINT32 bitValue = 0;

    if (pMcASPRegs == NULL || xmtData == NULL)
    {
        return;
    }
    
    /* Configure TXMASK register */
    pMcASPRegs->XMASK     = xmtData->mask;

    /* Reset the XSMRST bit in GBLCTL register */
    pMcASPRegs->GBLCTL &= ~AM3X_MCASP_GBLCTL_TXSMRST_MASK;

    /* Reset the RSMRST bit in GBLCTL register */
//    pMcASPRegs->GBLCTL &= ~AM3X_MCASP_GBLCTL_RXSMRST_MASK;

    /* Configure XFMT register */
    pMcASPRegs->XFMT      = xmtData->fmt;

    /* Reset the XFRST register in GBLCTL */
    pMcASPRegs->GBLCTL &= ~AM3X_MCASP_GBLCTL_TXFSRST_MASK;
    
    /* Configure TXFMCTL register */
    pMcASPRegs->AFSXCTL = xmtData->frSyncCtl;
    
    if (pMcASPRegs->AFSXCTL & AM3X_MCASP_AFSXCTL_AFSXE_MASK)
    {
        // Internal
        pMcASPRegs->PDIR |= AM3X_MCASP_PDIR_AFSX_OUTPUT << AM3X_MCASP_PDIR_AFSX_SHIFT;
    }

    /* Reset XHCLKRST, XCLKRST, XSRCLR  in GBLCTL */
    pMcASPRegs->GBLCTL &= ~AM3X_MCASP_GBLCTL_TXHCLKRST_MASK;
    pMcASPRegs->GBLCTL &= ~AM3X_MCASP_GBLCTL_TXCLKRST_MASK;
    pMcASPRegs->GBLCTL &= ~AM3X_MCASP_GBLCTL_TXSERCLR_MASK;
    
    /* Configure ACLKXCTL register */
    pMcASPRegs->ACLKXCTL  = xmtData->clk.clkSetupClk;

    if (pMcASPRegs->ACLKXCTL & AM3X_MCASP_ACLKXCTL_ACLKXE_MASK)
    {
        // Internal
        pMcASPRegs->PDIR |= AM3X_MCASP_PDIR_ACLKX_OUTPUT << AM3X_MCASP_PDIR_ACLKX_SHIFT; 
    }

    /* Configure AHCLKXCTL register */
    pMcASPRegs->AHCLKXCTL = xmtData->clk.clkSetupHiClk;

    if (pMcASPRegs->AHCLKXCTL & AM3X_MCASP_AHCLKXCTL_AHCLKXE_MASK)
    {
        // Internal
        pMcASPRegs->PDIR |= AM3X_MCASP_PDIR_AHCLKX_OUTPUT << AM3X_MCASP_PDIR_AHCLKX_SHIFT;
    }
    
    /* Configure XTDM register */
    pMcASPRegs->XTDM =  xmtData->tdm;

    /* Configure EVTCTLX register */
    pMcASPRegs->XINTCTL =  xmtData->intCtl;

    /* Configure XCLKCHK register */
    pMcASPRegs->XCLKCHK = xmtData->clk.clkChk;

    /* Configure XEVTCTL register */
    pMcASPRegs->XEVTCTL =  xmtData->evtCtl;

    /* Sequence of start: starting hclk first*/
    pMcASPRegs->XGBLCTL &= ~AM3X_MCASP_XGBLCTL_TXHCLKRST_MASK;
    pMcASPRegs->XGBLCTL |= AM3X_MCASP_XGBLCTL_TXHCLKRST_ACTIVE << 
            AM3X_MCASP_XGBLCTL_TXHCLKRST_SHIFT;
    {
        bitValue = 0;
        while (AM3X_MCASP_XGBLCTL_TXHCLKRST_ACTIVE != bitValue)
        {
            bitValue = (pMcASPRegs->GBLCTL &  AM3X_MCASP_GBLCTL_TXHCLKRST_MASK) >>
                AM3X_MCASP_GBLCTL_TXHCLKRST_SHIFT;
        }
    }

    /* start ACLKX only if internal clock is used*/
    if (pMcASPRegs->ACLKXCTL & AM3X_MCASP_ACLKXCTL_ACLKXE_MASK)
    {
        pMcASPRegs->XGBLCTL &= ~AM3X_MCASP_XGBLCTL_TXCLKRST_MASK;
        pMcASPRegs->XGBLCTL |= AM3X_MCASP_XGBLCTL_TXCLKRST_ACTIVE << 
            AM3X_MCASP_XGBLCTL_TXCLKRST_SHIFT;
        {
            bitValue = 0;
            while (AM3X_MCASP_XGBLCTL_TXHCLKRST_ACTIVE != bitValue)
            {
                bitValue = (pMcASPRegs->GBLCTL &  AM3X_MCASP_GBLCTL_TXCLKRST_MASK) >>
                AM3X_MCASP_GBLCTL_TXCLKRST_SHIFT;
            }
        }
    }
    
// RETAILMSG(1, (TEXT("<= mcaspConfigXmtSection\r\n" )));
}

//------------------------------------------------------------------------------
//  Function: mcaspConfigRcvSection  
//
//  This function configures format, frame sync, and other parameters
//  related to the rcv section. Also configures the rcv clk section. 
//
//
void mcaspConfigRcvSection (PMCASPREGS pMcASPRegs, McaspHwSetupData  *rcvData)
{

    volatile UINT32 bitValue = 0;

    if (pMcASPRegs == NULL || rcvData == NULL)
    {
        return;
    }
    
    /* Configure RMASK register */
    pMcASPRegs->RMASK     = rcvData->mask;

    /* Reset the RSMRST bit in GBLCTL register */
    pMcASPRegs->GBLCTL &= ~AM3X_MCASP_GBLCTL_RXSMRST_MASK;

    /* Configure RFMT register */
    pMcASPRegs->RFMT      = rcvData->fmt;

    /* Reset the RFRST register in GBLCTL */
    pMcASPRegs->GBLCTL &= ~AM3X_MCASP_GBLCTL_RXFSRST_MASK;
    
    /* Configure RXFMCTL register */
    pMcASPRegs->AFSRCTL = rcvData->frSyncCtl;
    
    if (pMcASPRegs->AFSRCTL & AM3X_MCASP_AFSRCTL_AFSRE_MASK)
    {
        // Internal
        pMcASPRegs->PDIR |= AM3X_MCASP_PDIR_AFSR_OUTPUT << AM3X_MCASP_PDIR_AFSR_SHIFT;
    }

    /* Reset RHCLKRST, RCLKRST, RSRCLR  in GBLCTL */
    pMcASPRegs->GBLCTL &= ~AM3X_MCASP_GBLCTL_RXHCLKRST_MASK;
    pMcASPRegs->GBLCTL &= ~AM3X_MCASP_GBLCTL_RXCLKRST_MASK;
    pMcASPRegs->GBLCTL &= ~AM3X_MCASP_GBLCTL_RXSERCLR_MASK;
    
    /* Configure ACLKRCTL register */
    pMcASPRegs->ACLKRCTL  = rcvData->clk.clkSetupClk;

    if (pMcASPRegs->ACLKRCTL & AM3X_MCASP_ACLKRCTL_ACLKRE_MASK)
    {
        // Internal
        pMcASPRegs->PDIR |= AM3X_MCASP_PDIR_ACLKR_OUTPUT << AM3X_MCASP_PDIR_ACLKR_SHIFT; 
    }

    /* Configure AHCLKRCTL register */
    pMcASPRegs->AHCLKRCTL = rcvData->clk.clkSetupHiClk;

    if (pMcASPRegs->AHCLKRCTL & AM3X_MCASP_AHCLKRCTL_AHCLKRE_MASK)
    {
        // Internal
        pMcASPRegs->PDIR |= AM3X_MCASP_PDIR_AHCLKR_OUTPUT << AM3X_MCASP_PDIR_AHCLKR_SHIFT;
    }
    
    /* Configure RTDM register */
    pMcASPRegs->RTDM =  rcvData->tdm;

    /* Configure RINTCTL register */
    pMcASPRegs->RINTCTL =  rcvData->intCtl;

    /* Configure RCLKCHK register */
    pMcASPRegs->RCLKCHK = rcvData->clk.clkChk;

    /* Configure REVTCTL register */
    pMcASPRegs->REVTCTL =  rcvData->evtCtl;

    /* Sequence of start: starting hclk first*/
    pMcASPRegs->RGBLCTL &= ~AM3X_MCASP_RGBLCTL_RXHCLKRST_MASK;
    pMcASPRegs->RGBLCTL |= AM3X_MCASP_RGBLCTL_RXHCLKRST_ACTIVE << 
            AM3X_MCASP_RGBLCTL_RXHCLKRST_SHIFT;
    {
        bitValue = 0;
        while (AM3X_MCASP_RGBLCTL_RXHCLKRST_ACTIVE != bitValue)
        {
            bitValue = (pMcASPRegs->GBLCTL &  AM3X_MCASP_GBLCTL_RXHCLKRST_MASK) >>
                AM3X_MCASP_GBLCTL_RXHCLKRST_SHIFT;
        }
    }

    /* start ACLKR only if internal clock is used*/
if (pMcASPRegs->ACLKXCTL & AM3X_MCASP_ACLKXCTL_ASYNC_MASK)
{   // Not src from XCLK
    if (pMcASPRegs->ACLKRCTL & AM3X_MCASP_ACLKRCTL_ACLKRE_MASK)
    {
        pMcASPRegs->RGBLCTL &= ~AM3X_MCASP_RGBLCTL_RXCLKRST_MASK;
        pMcASPRegs->RGBLCTL |= AM3X_MCASP_RGBLCTL_RXCLKRST_ACTIVE << 
            AM3X_MCASP_RGBLCTL_RXCLKRST_SHIFT;
        {
            bitValue = 0;
            while (AM3X_MCASP_RGBLCTL_RXHCLKRST_ACTIVE != bitValue)
            {
                bitValue = (pMcASPRegs->GBLCTL &  AM3X_MCASP_GBLCTL_RXCLKRST_MASK) >>
                AM3X_MCASP_GBLCTL_RXCLKRST_SHIFT;
            }
        }
    }
}
    
// RETAILMSG(1, (TEXT("<= mcaspConfigRcvSection\r\n" )));
}

//------------------------------------------------------------------------------
//  Function: mcaspSetSerXmt  
//
//  This function sets a particular serializer to act as transmitter 
//
//
void mcaspSetSerXmt (PMCASPREGS pMcASPRegs, McaspSerializerNum  serNum)
{

    if (pMcASPRegs == NULL)
    {
        return;
    }
    
    switch (serNum)
    {
        case SERIALIZER_0:
            pMcASPRegs->SRCTL0 &= ~AM3X_MCASP_SRCTL0_MODE_MASK;
            pMcASPRegs->SRCTL0 |= AM3X_MCASP_SRCTL0_MODE_XMT << AM3X_MCASP_SRCTL0_MODE_SHIFT;
            pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR0_MASK;
            pMcASPRegs->PDIR |= AM3X_MCASP_PDIR_AXR0_OUTPUT << AM3X_MCASP_PDIR_AXR0_SHIFT;
            break;

        case SERIALIZER_1:
            pMcASPRegs->SRCTL1 &= ~AM3X_MCASP_SRCTL1_MODE_MASK;
            pMcASPRegs->SRCTL1 |= AM3X_MCASP_SRCTL1_MODE_XMT << AM3X_MCASP_SRCTL1_MODE_SHIFT;
            pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR1_MASK;
            pMcASPRegs->PDIR |= AM3X_MCASP_PDIR_AXR1_OUTPUT << AM3X_MCASP_PDIR_AXR1_SHIFT;
            break;

        case SERIALIZER_2:
            pMcASPRegs->SRCTL2 &= ~AM3X_MCASP_SRCTL2_MODE_MASK;
            pMcASPRegs->SRCTL2 |= AM3X_MCASP_SRCTL2_MODE_XMT << AM3X_MCASP_SRCTL2_MODE_SHIFT; 
            pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR2_MASK;
            pMcASPRegs->PDIR |= AM3X_MCASP_PDIR_AXR2_OUTPUT << AM3X_MCASP_PDIR_AXR2_SHIFT;
            break;

        case SERIALIZER_3:
            pMcASPRegs->SRCTL3 &= ~AM3X_MCASP_SRCTL3_MODE_MASK;
            pMcASPRegs->SRCTL3 |= AM3X_MCASP_SRCTL3_MODE_XMT << AM3X_MCASP_SRCTL3_MODE_SHIFT;
            pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR3_MASK;
            pMcASPRegs->PDIR |= AM3X_MCASP_PDIR_AXR3_OUTPUT << AM3X_MCASP_PDIR_AXR3_SHIFT;
            break;

        case SERIALIZER_4:
            pMcASPRegs->SRCTL4 &= ~AM3X_MCASP_SRCTL4_MODE_MASK;
            pMcASPRegs->SRCTL4 |= AM3X_MCASP_SRCTL4_MODE_XMT << AM3X_MCASP_SRCTL4_MODE_SHIFT;
            pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR4_MASK;
            pMcASPRegs->PDIR |= AM3X_MCASP_PDIR_AXR4_OUTPUT << AM3X_MCASP_PDIR_AXR4_SHIFT;
            break;

        case SERIALIZER_5:
            pMcASPRegs->SRCTL5 &= ~AM3X_MCASP_SRCTL5_MODE_MASK;
            pMcASPRegs->SRCTL5 |= AM3X_MCASP_SRCTL5_MODE_XMT << AM3X_MCASP_SRCTL5_MODE_SHIFT;
            pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR5_MASK;
            pMcASPRegs->PDIR |= AM3X_MCASP_PDIR_AXR5_OUTPUT << AM3X_MCASP_PDIR_AXR5_SHIFT;
            break;

        case SERIALIZER_6:
            pMcASPRegs->SRCTL6 &= ~AM3X_MCASP_SRCTL6_MODE_MASK;
            pMcASPRegs->SRCTL6 |= AM3X_MCASP_SRCTL6_MODE_XMT << AM3X_MCASP_SRCTL6_MODE_SHIFT;
            pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR6_MASK;
            pMcASPRegs->PDIR |= AM3X_MCASP_PDIR_AXR6_OUTPUT << AM3X_MCASP_PDIR_AXR6_SHIFT;
            break;

        case SERIALIZER_7:
            pMcASPRegs->SRCTL7 &= ~AM3X_MCASP_SRCTL7_MODE_MASK;
            pMcASPRegs->SRCTL7 |= AM3X_MCASP_SRCTL7_MODE_XMT << AM3X_MCASP_SRCTL7_MODE_SHIFT;
            pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR7_MASK;
            pMcASPRegs->PDIR |= AM3X_MCASP_PDIR_AXR7_OUTPUT << AM3X_MCASP_PDIR_AXR7_SHIFT;
            break;

        case SERIALIZER_8:
            pMcASPRegs->SRCTL8 &= ~AM3X_MCASP_SRCTL8_MODE_MASK;
            pMcASPRegs->SRCTL8 |= AM3X_MCASP_SRCTL8_MODE_XMT << AM3X_MCASP_SRCTL8_MODE_SHIFT;
            pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR8_MASK;
            pMcASPRegs->PDIR |= AM3X_MCASP_PDIR_AXR8_OUTPUT << AM3X_MCASP_PDIR_AXR8_SHIFT;
            break;

        case SERIALIZER_9:
            pMcASPRegs->SRCTL9 &= ~AM3X_MCASP_SRCTL9_MODE_MASK;
            pMcASPRegs->SRCTL9 |= AM3X_MCASP_SRCTL9_MODE_XMT << AM3X_MCASP_SRCTL9_MODE_SHIFT;
            pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR9_MASK;
            pMcASPRegs->PDIR |= AM3X_MCASP_PDIR_AXR9_OUTPUT << AM3X_MCASP_PDIR_AXR9_SHIFT;
            break;

        case SERIALIZER_10:
            pMcASPRegs->SRCTL10 &= ~AM3X_MCASP_SRCTL10_MODE_MASK;
            pMcASPRegs->SRCTL10 |= AM3X_MCASP_SRCTL10_MODE_XMT << AM3X_MCASP_SRCTL10_MODE_SHIFT;
            pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR10_MASK;
            pMcASPRegs->PDIR |= AM3X_MCASP_PDIR_AXR10_OUTPUT << AM3X_MCASP_PDIR_AXR10_SHIFT;
            break;

        case SERIALIZER_11:
            pMcASPRegs->SRCTL11 &= ~AM3X_MCASP_SRCTL11_MODE_MASK;
            pMcASPRegs->SRCTL11 |= AM3X_MCASP_SRCTL11_MODE_XMT << AM3X_MCASP_SRCTL11_MODE_SHIFT;
            pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR11_MASK;
            pMcASPRegs->PDIR |= AM3X_MCASP_PDIR_AXR11_OUTPUT << AM3X_MCASP_PDIR_AXR11_SHIFT;
            break;

        case SERIALIZER_12:
            pMcASPRegs->SRCTL12 &= ~AM3X_MCASP_SRCTL12_MODE_MASK;
            pMcASPRegs->SRCTL12 |= AM3X_MCASP_SRCTL12_MODE_XMT << AM3X_MCASP_SRCTL12_MODE_SHIFT;
            pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR12_MASK;
            pMcASPRegs->PDIR |= AM3X_MCASP_PDIR_AXR12_OUTPUT << AM3X_MCASP_PDIR_AXR12_SHIFT;
            break;

        case SERIALIZER_13:
            pMcASPRegs->SRCTL13 &= ~AM3X_MCASP_SRCTL13_MODE_MASK;
            pMcASPRegs->SRCTL13 |= AM3X_MCASP_SRCTL13_MODE_XMT << AM3X_MCASP_SRCTL13_MODE_SHIFT;
            pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR13_MASK;
            pMcASPRegs->PDIR |= AM3X_MCASP_PDIR_AXR13_OUTPUT << AM3X_MCASP_PDIR_AXR13_SHIFT;
            break;

        case SERIALIZER_14:
            pMcASPRegs->SRCTL14 &= ~AM3X_MCASP_SRCTL14_MODE_MASK;
            pMcASPRegs->SRCTL14 |= AM3X_MCASP_SRCTL14_MODE_XMT << AM3X_MCASP_SRCTL14_MODE_SHIFT;
            pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR14_MASK;
            pMcASPRegs->PDIR |= AM3X_MCASP_PDIR_AXR14_OUTPUT << AM3X_MCASP_PDIR_AXR14_SHIFT;
            break;

        case SERIALIZER_15:
            pMcASPRegs->SRCTL15 &= ~AM3X_MCASP_SRCTL15_MODE_MASK;
            pMcASPRegs->SRCTL15 |= AM3X_MCASP_SRCTL15_MODE_XMT << AM3X_MCASP_SRCTL15_MODE_SHIFT;
            pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR15_MASK;
            pMcASPRegs->PDIR |= AM3X_MCASP_PDIR_AXR15_OUTPUT << AM3X_MCASP_PDIR_AXR15_SHIFT;
            break;
        default :
            break;
    }

// RETAILMSG(1, (TEXT("<= mcaspSetSerXmt %d\r\n" ), serNum));
}

//------------------------------------------------------------------------------
//  Function: mcaspSerXmtEn  
//
//  This function enables the Xmt serialisers 
//
//
void mcaspSerXmtEn(PMCASPREGS pMcASPRegs)
{
    UINT32 bitValue;
    
    if (pMcASPRegs == NULL)
    {
        return;
    }
    
    bitValue = AM3X_MCASP_GBLCTL_TXSERCLR_ACTIVE <<
        AM3X_MCASP_GBLCTL_TXSERCLR_SHIFT;
        
    pMcASPRegs->XGBLCTL |= bitValue;
        
    while ((pMcASPRegs->GBLCTL & bitValue) != bitValue);

// RETAILMSG(1, (TEXT("<= mcaspSerXmtEn\r\n" )));
}

//------------------------------------------------------------------------------
//  Function: mcaspSerRcvEn  
//
//  This function enables the Rcv serialisers 
//
//
void mcaspSerRcvEn(PMCASPREGS pMcASPRegs)
{
    UINT32 bitValue;
    
    bitValue = AM3X_MCASP_GBLCTL_RXSERCLR_ACTIVE <<
                                        AM3X_MCASP_GBLCTL_RXSERCLR_SHIFT;
        
    pMcASPRegs->GBLCTL |= bitValue;
        
    while ((pMcASPRegs->GBLCTL & bitValue) != bitValue);
}

//------------------------------------------------------------------------------
//  Function: mcaspSetSerRcv  
//
//  This function sets a particular serializer to act as receiver 
//
//
void mcaspSetSerRcv (PMCASPREGS pMcASPRegs, McaspSerializerNum  serNum)
{

    if (pMcASPRegs == NULL)
    {
        return;
    }
    
    switch (serNum)
    {
        case SERIALIZER_0:
            pMcASPRegs->SRCTL0 &= ~AM3X_MCASP_SRCTL0_MODE_MASK;
            pMcASPRegs->SRCTL0 |= AM3X_MCASP_SRCTL0_MODE_RCV << AM3X_MCASP_SRCTL0_MODE_SHIFT;
            pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR0_MASK;
            pMcASPRegs->PDIR |= AM3X_MCASP_PDIR_AXR0_INPUT << AM3X_MCASP_PDIR_AXR0_SHIFT;
            break;

        case SERIALIZER_1:
            pMcASPRegs->SRCTL1 &= ~AM3X_MCASP_SRCTL1_MODE_MASK;
            pMcASPRegs->SRCTL1 |= AM3X_MCASP_SRCTL1_MODE_RCV << AM3X_MCASP_SRCTL1_MODE_SHIFT;
            pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR1_MASK;
            pMcASPRegs->PDIR |= AM3X_MCASP_PDIR_AXR1_INPUT << AM3X_MCASP_PDIR_AXR1_SHIFT;
            break;

        case SERIALIZER_2:
            pMcASPRegs->SRCTL2 &= ~AM3X_MCASP_SRCTL2_MODE_MASK;
            pMcASPRegs->SRCTL2 |= AM3X_MCASP_SRCTL2_MODE_RCV << AM3X_MCASP_SRCTL2_MODE_SHIFT;
            pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR2_MASK;
            pMcASPRegs->PDIR |= AM3X_MCASP_PDIR_AXR2_INPUT << AM3X_MCASP_PDIR_AXR2_SHIFT;
            break;

        case SERIALIZER_3:
            pMcASPRegs->SRCTL3 &= ~AM3X_MCASP_SRCTL3_MODE_MASK;
            pMcASPRegs->SRCTL3 |= AM3X_MCASP_SRCTL3_MODE_RCV << AM3X_MCASP_SRCTL3_MODE_SHIFT;
            pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR3_MASK;
            pMcASPRegs->PDIR |= AM3X_MCASP_PDIR_AXR3_INPUT << AM3X_MCASP_PDIR_AXR3_SHIFT;
            break;

        case SERIALIZER_4:
            pMcASPRegs->SRCTL4 &= ~AM3X_MCASP_SRCTL4_MODE_MASK;
            pMcASPRegs->SRCTL4 |= AM3X_MCASP_SRCTL4_MODE_RCV << AM3X_MCASP_SRCTL4_MODE_SHIFT;
            pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR4_MASK;
            pMcASPRegs->PDIR |= AM3X_MCASP_PDIR_AXR4_INPUT << AM3X_MCASP_PDIR_AXR4_SHIFT;
            break;

        case SERIALIZER_5:
            pMcASPRegs->SRCTL5 &= ~AM3X_MCASP_SRCTL5_MODE_MASK;
            pMcASPRegs->SRCTL5 |= AM3X_MCASP_SRCTL5_MODE_RCV << AM3X_MCASP_SRCTL5_MODE_SHIFT;
            pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR5_MASK;
            pMcASPRegs->PDIR |= AM3X_MCASP_PDIR_AXR5_INPUT << AM3X_MCASP_PDIR_AXR5_SHIFT;
            break;

        case SERIALIZER_6:
            pMcASPRegs->SRCTL6 &= ~AM3X_MCASP_SRCTL6_MODE_MASK;
            pMcASPRegs->SRCTL6 |= AM3X_MCASP_SRCTL6_MODE_RCV << AM3X_MCASP_SRCTL6_MODE_SHIFT;
            pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR6_MASK;
            pMcASPRegs->PDIR |= AM3X_MCASP_PDIR_AXR6_INPUT << AM3X_MCASP_PDIR_AXR6_SHIFT;
            break;

        case SERIALIZER_7:
            pMcASPRegs->SRCTL7 &= ~AM3X_MCASP_SRCTL7_MODE_MASK;
            pMcASPRegs->SRCTL7 |= AM3X_MCASP_SRCTL7_MODE_RCV << AM3X_MCASP_SRCTL7_MODE_SHIFT;
            pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR7_MASK;
            pMcASPRegs->PDIR |= AM3X_MCASP_PDIR_AXR7_INPUT << AM3X_MCASP_PDIR_AXR7_SHIFT;
            break;

        case SERIALIZER_8:
            pMcASPRegs->SRCTL8 &= ~AM3X_MCASP_SRCTL8_MODE_MASK;
            pMcASPRegs->SRCTL8 |= AM3X_MCASP_SRCTL8_MODE_RCV << AM3X_MCASP_SRCTL8_MODE_SHIFT;
            pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR8_MASK;
            pMcASPRegs->PDIR |= AM3X_MCASP_PDIR_AXR8_INPUT << AM3X_MCASP_PDIR_AXR8_SHIFT;
            break;

        case SERIALIZER_9:
            pMcASPRegs->SRCTL9 &= ~AM3X_MCASP_SRCTL9_MODE_MASK;
            pMcASPRegs->SRCTL9 |= AM3X_MCASP_SRCTL9_MODE_RCV << AM3X_MCASP_SRCTL9_MODE_SHIFT;
            pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR9_MASK;
            pMcASPRegs->PDIR |= AM3X_MCASP_PDIR_AXR9_INPUT << AM3X_MCASP_PDIR_AXR9_SHIFT;
            break;

        case SERIALIZER_10:
            pMcASPRegs->SRCTL10 &= ~AM3X_MCASP_SRCTL10_MODE_MASK;
            pMcASPRegs->SRCTL10 |= AM3X_MCASP_SRCTL10_MODE_RCV << AM3X_MCASP_SRCTL10_MODE_SHIFT;
            pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR10_MASK;
            pMcASPRegs->PDIR |= AM3X_MCASP_PDIR_AXR10_INPUT << AM3X_MCASP_PDIR_AXR10_SHIFT;
            break;

        case SERIALIZER_11:
            pMcASPRegs->SRCTL11 &= ~AM3X_MCASP_SRCTL11_MODE_MASK;
            pMcASPRegs->SRCTL11 |= AM3X_MCASP_SRCTL11_MODE_RCV << AM3X_MCASP_SRCTL11_MODE_SHIFT;
            pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR11_MASK;
            pMcASPRegs->PDIR |= AM3X_MCASP_PDIR_AXR11_INPUT << AM3X_MCASP_PDIR_AXR11_SHIFT;
            break;

        case SERIALIZER_12:
            pMcASPRegs->SRCTL12 &= ~AM3X_MCASP_SRCTL12_MODE_MASK;
            pMcASPRegs->SRCTL12 |= AM3X_MCASP_SRCTL12_MODE_RCV << AM3X_MCASP_SRCTL12_MODE_SHIFT;
            pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR12_MASK;
            pMcASPRegs->PDIR |= AM3X_MCASP_PDIR_AXR12_INPUT << AM3X_MCASP_PDIR_AXR12_SHIFT;
            break;

        case SERIALIZER_13:
            pMcASPRegs->SRCTL13 &= ~AM3X_MCASP_SRCTL13_MODE_MASK;
            pMcASPRegs->SRCTL13 |= AM3X_MCASP_SRCTL13_MODE_RCV << AM3X_MCASP_SRCTL13_MODE_SHIFT;
            pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR13_MASK;
            pMcASPRegs->PDIR |= AM3X_MCASP_PDIR_AXR13_INPUT << AM3X_MCASP_PDIR_AXR13_SHIFT;
            break;

        case SERIALIZER_14:
            pMcASPRegs->SRCTL14 &= ~AM3X_MCASP_SRCTL14_MODE_MASK;
            pMcASPRegs->SRCTL14 |= AM3X_MCASP_SRCTL14_MODE_RCV << AM3X_MCASP_SRCTL14_MODE_SHIFT;
            pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR14_MASK;
            pMcASPRegs->PDIR |= AM3X_MCASP_PDIR_AXR14_INPUT << AM3X_MCASP_PDIR_AXR14_SHIFT;
            break;

        case SERIALIZER_15:
            pMcASPRegs->SRCTL15 &= ~AM3X_MCASP_SRCTL15_MODE_MASK;
            pMcASPRegs->SRCTL15 |= AM3X_MCASP_SRCTL15_MODE_RCV << AM3X_MCASP_SRCTL15_MODE_SHIFT;
            pMcASPRegs->PDIR &= ~AM3X_MCASP_PDIR_AXR15_MASK;
            pMcASPRegs->PDIR |= AM3X_MCASP_PDIR_AXR15_INPUT << AM3X_MCASP_PDIR_AXR15_SHIFT;
            break;
        default :
            break;
    }

// RETAILMSG(1, (TEXT("<= mcaspSetSerRcv\r\n" )));
}

#if 0  // not called. comment out
//------------------------------------------------------------------------------
//  Function: mcaspSetSerIna  
//
//  This function sets a particular serializer to be inactive 
//
//
void mcaspSetSerIna (PMCASPREGS pMcASPRegs, McaspSerializerNum serNum)
{

    if (pMcASPRegs == NULL)
    {
        return;
    }
    
    switch (serNum)
    {
        case SERIALIZER_0:
            pMcASPRegs->SRCTL0 &= ~AM3X_MCASP_SRCTL0_MODE_MASK;
            break;

        case SERIALIZER_1:
            pMcASPRegs->SRCTL1 &= ~AM3X_MCASP_SRCTL1_MODE_MASK;
            break;

        case SERIALIZER_2:
            pMcASPRegs->SRCTL2 &= ~AM3X_MCASP_SRCTL2_MODE_MASK;
            break;

        case SERIALIZER_3:
            pMcASPRegs->SRCTL3 &= ~AM3X_MCASP_SRCTL3_MODE_MASK;
            break;

        case SERIALIZER_4:
            pMcASPRegs->SRCTL4 &= ~AM3X_MCASP_SRCTL4_MODE_MASK;
            break;

        case SERIALIZER_5:
            pMcASPRegs->SRCTL5 &= ~AM3X_MCASP_SRCTL5_MODE_MASK;
            break;

        case SERIALIZER_6:
            pMcASPRegs->SRCTL6 &= ~AM3X_MCASP_SRCTL6_MODE_MASK;
            break;

        case SERIALIZER_7:
            pMcASPRegs->SRCTL7 &= ~AM3X_MCASP_SRCTL7_MODE_MASK;
            break;

        case SERIALIZER_8:
            pMcASPRegs->SRCTL8 &= ~AM3X_MCASP_SRCTL8_MODE_MASK;
            break;

        case SERIALIZER_9:
            pMcASPRegs->SRCTL9 &= ~AM3X_MCASP_SRCTL9_MODE_MASK;
            break;

        case SERIALIZER_10:
            pMcASPRegs->SRCTL10 &= ~AM3X_MCASP_SRCTL10_MODE_MASK;
            break;

        case SERIALIZER_11:
            pMcASPRegs->SRCTL11 &= ~AM3X_MCASP_SRCTL11_MODE_MASK;
            break;

        case SERIALIZER_12:
            pMcASPRegs->SRCTL12 &= ~AM3X_MCASP_SRCTL12_MODE_MASK;
            break;

        case SERIALIZER_13:
            pMcASPRegs->SRCTL13 &= ~AM3X_MCASP_SRCTL13_MODE_MASK;
            break;

        case SERIALIZER_14:
            pMcASPRegs->SRCTL14 &= ~AM3X_MCASP_SRCTL14_MODE_MASK;
            break;

        case SERIALIZER_15:
            pMcASPRegs->SRCTL15 &= ~AM3X_MCASP_SRCTL15_MODE_MASK;
            break;
        default :
            break;
    }

}

//------------------------------------------------------------------------------
//  Function: mcaspWriteChanStatRam  
//
//  This function writes to the Channel status RAM (DITCSRA/B0-5) 
//
//
void mcaspWriteChanStatRam (PMCASPREGS pMcASPRegs, McaspChStatusRam *chanStatRam)
{

    if (pMcASPRegs == NULL || chanStatRam == NULL)
    {
        return;
    }
    
    /* Configure the DIT left channel status registers */
    pMcASPRegs->DITCSRA0 = chanStatRam->chStatusLeft[DIT_REGISTER_0];

    pMcASPRegs->DITCSRA1 = chanStatRam->chStatusLeft[DIT_REGISTER_1];

    pMcASPRegs->DITCSRA2 = chanStatRam->chStatusLeft[DIT_REGISTER_2];

    pMcASPRegs->DITCSRA3 = chanStatRam->chStatusLeft[DIT_REGISTER_3];

    pMcASPRegs->DITCSRA4 = chanStatRam->chStatusLeft[DIT_REGISTER_4];

    pMcASPRegs->DITCSRA5 = chanStatRam->chStatusLeft[DIT_REGISTER_5];

    /* Configure the DIT right channel status registers */
    pMcASPRegs->DITCSRB0 = chanStatRam->chStatusRight[DIT_REGISTER_0];

    pMcASPRegs->DITCSRB1 = chanStatRam->chStatusRight[DIT_REGISTER_1];

    pMcASPRegs->DITCSRB2 = chanStatRam->chStatusRight[DIT_REGISTER_2];

    pMcASPRegs->DITCSRB3 = chanStatRam->chStatusRight[DIT_REGISTER_3];

    pMcASPRegs->DITCSRB4 = chanStatRam->chStatusRight[DIT_REGISTER_4];

    pMcASPRegs->DITCSRB5 = chanStatRam->chStatusRight[DIT_REGISTER_5];

}

//------------------------------------------------------------------------------
//  Function: mcaspWriteUserDataRam  
//
//  This function writes to the User Data RAM (DITUDRA/B0-5) 
//
//
void mcaspWriteUserDataRam (PMCASPREGS pMcASPRegs, McaspUserDataRam *userDataRam)
{
    if (pMcASPRegs == NULL || userDataRam == NULL)
    {
        return;
    }
    
    /* Configure the DIT left user data registers */
    pMcASPRegs->DITUDRA0 = userDataRam->userDataLeft[DIT_REGISTER_0];

    pMcASPRegs->DITUDRA1 = userDataRam->userDataLeft[DIT_REGISTER_1];

    pMcASPRegs->DITUDRA2 = userDataRam->userDataLeft[DIT_REGISTER_2];

    pMcASPRegs->DITUDRA3 = userDataRam->userDataLeft[DIT_REGISTER_3];

    pMcASPRegs->DITUDRA4 = userDataRam->userDataLeft[DIT_REGISTER_4];

    pMcASPRegs->DITUDRA5 = userDataRam->userDataLeft[DIT_REGISTER_5];

    /* Configure the DIT right user data registers */
    pMcASPRegs->DITUDRB0 = userDataRam->userDataRight[DIT_REGISTER_0];

    pMcASPRegs->DITUDRB1 = userDataRam->userDataRight[DIT_REGISTER_1];

    pMcASPRegs->DITUDRB2 = userDataRam->userDataRight[DIT_REGISTER_2];

    pMcASPRegs->DITUDRB3 = userDataRam->userDataRight[DIT_REGISTER_3];

    pMcASPRegs->DITUDRB4 = userDataRam->userDataRight[DIT_REGISTER_4];

    pMcASPRegs->DITUDRB5 = userDataRam->userDataRight[DIT_REGISTER_5];

}
#endif  // not called. comment out

//------------------------------------------------------------------------------
//  Function: mcaspResetXmt  
//
//  This function resets the bits related to transmit in XGBLCTL. 
//
//
void mcaspResetXmt (PMCASPREGS pMcASPRegs)
{
    if (pMcASPRegs == NULL)
    {
        return;
    }
    
    pMcASPRegs->XGBLCTL &= ~AM3X_MCASP_XGBLCTL_TXCLKRST_MASK;;
    pMcASPRegs->XGBLCTL &= ~AM3X_MCASP_XGBLCTL_TXHCLKRST_MASK;
    pMcASPRegs->XGBLCTL &= ~AM3X_MCASP_XGBLCTL_TXSERCLR_MASK;
    pMcASPRegs->XGBLCTL &= ~AM3X_MCASP_XGBLCTL_TXSMRST_MASK;
    pMcASPRegs->XGBLCTL &= ~AM3X_MCASP_XGBLCTL_TXFSRST_MASK;

// RETAILMSG(1, (TEXT("<= mcaspResetXmt\r\n" )));
}

//------------------------------------------------------------------------------
//  Function: mcaspResetRcv  
//
//  This function resets the bits related to receive in RGBLCTL. 
//
//
void mcaspResetRcv (PMCASPREGS pMcASPRegs)
{
    if (pMcASPRegs == NULL)
    {
        return;
    }
    
    pMcASPRegs->RGBLCTL &= ~AM3X_MCASP_RGBLCTL_RXCLKRST_MASK;
    while (pMcASPRegs->GBLCTL & AM3X_MCASP_RGBLCTL_RXCLKRST_MASK);
    
    pMcASPRegs->RGBLCTL &= ~AM3X_MCASP_RGBLCTL_RXHCLKRST_MASK;
    while (pMcASPRegs->GBLCTL & AM3X_MCASP_RGBLCTL_RXHCLKRST_MASK);
    
    pMcASPRegs->RGBLCTL &= ~AM3X_MCASP_RGBLCTL_RXSERCLR_MASK;
    while (pMcASPRegs->GBLCTL & AM3X_MCASP_RGBLCTL_RXSERCLR_MASK);
    
    pMcASPRegs->RGBLCTL &= ~AM3X_MCASP_RGBLCTL_RXSMRST_MASK;
    while (pMcASPRegs->GBLCTL & AM3X_MCASP_RGBLCTL_RXSMRST_MASK);
    
    pMcASPRegs->RGBLCTL &= ~AM3X_MCASP_RGBLCTL_RXFSRST_MASK;
    while (pMcASPRegs->GBLCTL & AM3X_MCASP_RGBLCTL_RXFSRST_MASK);

// RETAILMSG(1, (TEXT("<= mcaspResetRcv\r\n" )));
}

//------------------------------------------------------------------------------
//  Function: mcaspResetSmFsXmt  
//
//  This function resets the XFRST and XSMRST bits in XGBLCTL. 
//
//
void mcaspResetSmFsXmt (PMCASPREGS pMcASPRegs)
{
    if (pMcASPRegs == NULL)
    {
        return;
    }
    
    pMcASPRegs->XGBLCTL &= ~AM3X_MCASP_XGBLCTL_TXSMRST_MASK;
    pMcASPRegs->XGBLCTL &= ~AM3X_MCASP_XGBLCTL_TXFSRST_MASK;
}

//------------------------------------------------------------------------------
//  Function: mcaspResetSmFsRcv  
//
//  This function resets the RFRST and RSMRST bits in RGBLCTL. 
//
//
void mcaspResetSmFsRcv (PMCASPREGS pMcASPRegs)
{
    if (pMcASPRegs == NULL)
    {
        return;
    }
    
    pMcASPRegs->RGBLCTL &= ~AM3X_MCASP_RGBLCTL_RXSMRST_MASK;
    pMcASPRegs->RGBLCTL &= ~AM3X_MCASP_RGBLCTL_RXFSRST_MASK;
}

//------------------------------------------------------------------------------
//  Function: mcaspActivateXmtClkSer  
//
//  This function sets the bits related to transmit in XGBLCTL. 
//
//
void mcaspActivateXmtClkSer (PMCASPREGS pMcASPRegs)
{

    UINT32 bitValue = 0;

    if (pMcASPRegs == NULL)
    {
        return;
    }
    
    /* Sequence of start: starting hclk first */
    /* XHCLKRST */
    pMcASPRegs->XGBLCTL &= ~AM3X_MCASP_XGBLCTL_TXHCLKRST_MASK;
    pMcASPRegs->XGBLCTL |= AM3X_MCASP_XGBLCTL_TXHCLKRST_ACTIVE << AM3X_MCASP_XGBLCTL_TXHCLKRST_SHIFT;
    {
        bitValue = 0;
        while (AM3X_MCASP_GBLCTL_TXHCLKRST_ACTIVE != bitValue)
        {              
            bitValue = (pMcASPRegs->GBLCTL & AM3X_MCASP_GBLCTL_TXHCLKRST_MASK) >>
                AM3X_MCASP_GBLCTL_TXHCLKRST_SHIFT;           
        }
    }

     /* start ACLKX only if internal clock is used*/
    if (pMcASPRegs->ACLKXCTL & AM3X_MCASP_ACLKXCTL_ACLKXE_MASK)
    {
       /* XCLKRST */
       pMcASPRegs->XGBLCTL |= AM3X_MCASP_XGBLCTL_TXCLKRST_ACTIVE <<
           AM3X_MCASP_XGBLCTL_TXCLKRST_SHIFT;
       {
           bitValue = 0;
           while (AM3X_MCASP_XGBLCTL_TXCLKRST_ACTIVE != bitValue)
           {                 
              bitValue = (pMcASPRegs->GBLCTL & AM3X_MCASP_GBLCTL_TXCLKRST_MASK) >>
                  AM3X_MCASP_GBLCTL_TXCLKRST_SHIFT;
           }
       }
    }
// RETAILMSG(1, (TEXT("<= mcaspActivateXmtClkSer AM3X_MCASP_XGBLCTL_TXCLKRST_ACTIVE\r\n" )));
 
    /* XSRCLR */
    pMcASPRegs->XGBLCTL |= AM3X_MCASP_XGBLCTL_TXSERCLR_ACTIVE <<
        AM3X_MCASP_XGBLCTL_TXSERCLR_SHIFT;
    {
        bitValue = 0;
        while (AM3X_MCASP_GBLCTL_TXSERCLR_ACTIVE != bitValue)
        {
            bitValue = (pMcASPRegs->GBLCTL & AM3X_MCASP_GBLCTL_TXSERCLR_MASK) >>
                AM3X_MCASP_GBLCTL_TXSERCLR_SHIFT;
        }
    }
// RETAILMSG(1, (TEXT("<= mcaspActivateXmtClkSer AM3X_MCASP_GBLCTL_TXSERCLR_ACTIVE\r\n" )));

}

#if 0  // not called. comment out
//------------------------------------------------------------------------------
//  Function: mcaspActivateRcvClkSer  
//
//  This function sets the bits related to receive in RGBLCTL. 
//
//
void mcaspActivateRcvClkSer (PMCASPREGS pMcASPRegs)
{

    UINT32 bitValue = 0;

    if (pMcASPRegs == NULL)
    {
        return;
    }
    
    /* Sequence of start: starting hclk first*/
    pMcASPRegs->RGBLCTL &= ~AM3X_MCASP_RGBLCTL_RXHCLKRST_MASK;
    pMcASPRegs->RGBLCTL |= AM3X_MCASP_RGBLCTL_RXHCLKRST_ACTIVE << AM3X_MCASP_RGBLCTL_RXHCLKRST_SHIFT;
    {
        bitValue = 0;
        while (AM3X_MCASP_GBLCTL_RXHCLKRST_ACTIVE != bitValue)
        {              
            bitValue = (pMcASPRegs->GBLCTL & AM3X_MCASP_GBLCTL_RXHCLKRST_MASK) >>
                AM3X_MCASP_GBLCTL_RXHCLKRST_SHIFT;           
        }
    }

     /* start ACLKR only if internal clock is used*/
    if (pMcASPRegs->ACLKRCTL & AM3X_MCASP_ACLKRCTL_ACLKRE_MASK)
    {
       pMcASPRegs->RGBLCTL |= AM3X_MCASP_RGBLCTL_RXCLKRST_ACTIVE <<
           AM3X_MCASP_RGBLCTL_RXCLKRST_SHIFT;
       {
           bitValue = 0;
           while (AM3X_MCASP_RGBLCTL_RXCLKRST_ACTIVE != bitValue)
           {                 
              bitValue = (pMcASPRegs->GBLCTL & AM3X_MCASP_GBLCTL_RXCLKRST_MASK) >>
                  AM3X_MCASP_GBLCTL_RXCLKRST_SHIFT;
           }
       }
    }
 
    pMcASPRegs->RGBLCTL |= AM3X_MCASP_RGBLCTL_RXSERCLR_ACTIVE <<
        AM3X_MCASP_RGBLCTL_RXSERCLR_SHIFT;
    {
        bitValue = 0;
        while (AM3X_MCASP_GBLCTL_RXSERCLR_ACTIVE != bitValue)
        {
            bitValue = (pMcASPRegs->GBLCTL & AM3X_MCASP_GBLCTL_RXSERCLR_MASK) >>
                AM3X_MCASP_GBLCTL_RXSERCLR_SHIFT;
        }
    }

}

//------------------------------------------------------------------------------
//  Function: mcaspActivateSmRcvXmt  
//
//  This function sets the RSMRST and XSMRST bits in GBLCTL. 
//
//
void mcaspActivateSmRcvXmt (PMCASPREGS pMcASPRegs)
{

    UINT16 selectMask;

    if (pMcASPRegs == NULL)
    {
        return;
    }
    
    selectMask = AM3X_MCASP_GBLCTL_RXSMRST_MASK |
                 AM3X_MCASP_GBLCTL_TXSMRST_MASK;

    pMcASPRegs->GBLCTL |= selectMask;
    
    while (selectMask != (pMcASPRegs->GBLCTL & selectMask))
    {}

}

//------------------------------------------------------------------------------
//  Function: mcaspActivateFsRcvXmt  
//
//  This function resets the RFRST and XFRST bits in GBLCTL. 
//
//
void mcaspActivateFsRcvXmt (PMCASPREGS pMcASPRegs)
{
    UINT16 selectMask;

    if (pMcASPRegs == NULL)
    {
        return;
    }
    
    selectMask = AM3X_MCASP_GBLCTL_RXFSRST_MASK |
                 AM3X_MCASP_GBLCTL_TXFSRST_MASK;

    pMcASPRegs->GBLCTL |= selectMask;
    while (selectMask != (pMcASPRegs->GBLCTL & selectMask))
    {}

}

//------------------------------------------------------------------------------
//  Function: mcaspSetDitMode  
//
//  This function enables/disables the DIT mode. 
//
//
void mcaspSetDitMode (PMCASPREGS pMcASPRegs, BOOL ditFlag)
{
    if (pMcASPRegs == NULL)
    {
        return;
    }
    
    pMcASPRegs->GBLCTL &= ~AM3X_MCASP_GBLCTL_TXSMRST_MASK;
    pMcASPRegs->GBLCTL &= ~AM3X_MCASP_GBLCTL_TXFSRST_MASK;
    
    pMcASPRegs->DITCTL &= ~AM3X_MCASP_DITCTL_DITEN_MASK;
    if (ditFlag)
    {
        pMcASPRegs->DITCTL |= AM3X_MCASP_DITCTL_DITEN_DIT << AM3X_MCASP_DITCTL_DITEN_SHIFT;
    } 
}

//------------------------------------------------------------------------------
//  Function: mcaspGetAmute  
//
//  This function returns the value of AMUTE register. 
//
//
UINT16 mcaspGetAmute (PMCASPREGS pMcASPRegs)
{
    if (pMcASPRegs == NULL)
    {
        return 0;
    }
    
    return (UINT16)pMcASPRegs->AMUTE;
}


//------------------------------------------------------------------------------
//  Function: mcaspActivateClkRcvXmt  
//
//  This function activates both the receive and transmit clocks 
//
//
void mcaspActivateClkRcvXmt(PMCASPREGS pMcASPRegs)
{

    UINT32 bitValue = 0;
    
    if (pMcASPRegs == NULL)
    {
        return;
    }
    
    /* Sequence of start: starting hclk first*/
    /* start AHCLKR */
    pMcASPRegs->GBLCTL |= AM3X_MCASP_GBLCTL_RXHCLKRST_ACTIVE << AM3X_MCASP_GBLCTL_RXHCLKRST_SHIFT;
    {
        bitValue = 0;
        while (AM3X_MCASP_GBLCTL_RXHCLKRST_ACTIVE != bitValue)
        {
            bitValue = (pMcASPRegs->GBLCTL & AM3X_MCASP_GBLCTL_RXHCLKRST_MASK) >>
                     AM3X_MCASP_GBLCTL_RXHCLKRST_SHIFT;
        }
    }

    /* start ACLKR only if internal clock is used*/
    if (AM3X_MCASP_ACLKRCTL_ACLKRE_INTERNAL == 
        (pMcASPRegs->ACLKRCTL & AM3X_MCASP_ACLKRCTL_ACLKRE_MASK) >>
             AM3X_MCASP_ACLKRCTL_ACLKRE_SHIFT)
    {
        pMcASPRegs->GBLCTL |= AM3X_MCASP_GBLCTL_RXCLKRST_ACTIVE << 
            AM3X_MCASP_GBLCTL_RXCLKRST_SHIFT;
        {
            bitValue = 0;
            while (AM3X_MCASP_GBLCTL_RXCLKRST_ACTIVE != bitValue)
            {
                bitValue = (pMcASPRegs->GBLCTL & AM3X_MCASP_GBLCTL_RXCLKRST_MASK) >>
                    AM3X_MCASP_GBLCTL_RXCLKRST_SHIFT;
            }
        }
    }


    /* Sequence of start: starting hclk first*/
    pMcASPRegs->XGBLCTL |= AM3X_MCASP_XGBLCTL_TXHCLKRST_ACTIVE <<
        AM3X_MCASP_XGBLCTL_TXHCLKRST_SHIFT;
    {
        bitValue = 0;
        while (AM3X_MCASP_GBLCTL_TXHCLKRST_ACTIVE != bitValue)
        {
            bitValue = (pMcASPRegs->GBLCTL & AM3X_MCASP_GBLCTL_TXHCLKRST_MASK) >>
                AM3X_MCASP_GBLCTL_TXHCLKRST_SHIFT;
         }
    }

    /* start ACLKX only if internal clock is used*/
    if (AM3X_MCASP_ACLKXCTL_ACLKXE_INTERNAL ==
        (pMcASPRegs->ACLKXCTL & AM3X_MCASP_ACLKXCTL_ACLKXE_MASK) >>
        AM3X_MCASP_ACLKXCTL_ACLKXE_SHIFT)
    {
        pMcASPRegs->XGBLCTL |= AM3X_MCASP_XGBLCTL_TXCLKRST_ACTIVE <<
            AM3X_MCASP_XGBLCTL_TXCLKRST_SHIFT;
        {
            bitValue = 0;
            while (AM3X_MCASP_XGBLCTL_TXCLKRST_ACTIVE != bitValue)
            {
                bitValue = (pMcASPRegs->GBLCTL & AM3X_MCASP_GBLCTL_TXCLKRST_MASK) >>
                AM3X_MCASP_GBLCTL_TXCLKRST_SHIFT;
            }
        }
    }
}

//------------------------------------------------------------------------------
//  Function: mcaspRegReset  
//
//  This Function fills the registers for the McAsp instance with
//  default reset values 
//
//
void mcaspRegReset (PMCASPREGS pMcASPRegs, UINT8 numSerializers)

{
    INT16  serNum = 0;
    
    if (pMcASPRegs == NULL)
    {
        return;
    }
    
    pMcASPRegs->GBLCTL = AM3X_MCASP_GBLCTL_RESETVAL;
    pMcASPRegs->PWRDEMU = AM3X_MCASP_PWRDEMU_RESETVAL;
    pMcASPRegs->RMASK = AM3X_MCASP_RMASK_RESETVAL;
    pMcASPRegs->RFMT = AM3X_MCASP_RFMT_RESETVAL;
    pMcASPRegs->AFSRCTL = AM3X_MCASP_AFSRCTL_RESETVAL;
    pMcASPRegs->ACLKRCTL = AM3X_MCASP_ACLKRCTL_RESETVAL;    
    pMcASPRegs->AHCLKRCTL = AM3X_MCASP_AHCLKRCTL_RESETVAL;
    pMcASPRegs->RTDM = AM3X_MCASP_RTDM_RESETVAL;
    pMcASPRegs->RINTCTL = AM3X_MCASP_RINTCTL_RESETVAL;
    pMcASPRegs->RCLKCHK = AM3X_MCASP_RCLKCHK_RESETVAL;
    
    pMcASPRegs->XMASK = AM3X_MCASP_XMASK_RESETVAL;    
    pMcASPRegs->XFMT = AM3X_MCASP_XFMT_RESETVAL;
    pMcASPRegs->AFSXCTL = AM3X_MCASP_AFSXCTL_RESETVAL;
    pMcASPRegs->ACLKXCTL = AM3X_MCASP_ACLKXCTL_RESETVAL;
    pMcASPRegs->AHCLKXCTL = AM3X_MCASP_AHCLKXCTL_RESETVAL;
    pMcASPRegs->XTDM = AM3X_MCASP_XTDM_RESETVAL;
    pMcASPRegs->XINTCTL = AM3X_MCASP_XINTCTL_RESETVAL;
    pMcASPRegs->XCLKCHK = AM3X_MCASP_XCLKCHK_RESETVAL;

    

    while (serNum < numSerializers)
    {
    
        switch(serNum)
        {
        case 15:
        {
            pMcASPRegs->SRCTL15 = AM3X_MCASP_SRCTL15_RESETVAL;
            break;
        }
        case 14:
        {
            pMcASPRegs->SRCTL14 = AM3X_MCASP_SRCTL14_RESETVAL;
            break;
        }
        case 13:
        {
            pMcASPRegs->SRCTL13 = AM3X_MCASP_SRCTL13_RESETVAL;
            break;
        }
        case 12:
        {
            pMcASPRegs->SRCTL12 = AM3X_MCASP_SRCTL12_RESETVAL;
            break;
        }
        case 11:
        {
            pMcASPRegs->SRCTL11 = AM3X_MCASP_SRCTL11_RESETVAL;
            break;
        }
        case 10:
        {
            pMcASPRegs->SRCTL10 = AM3X_MCASP_SRCTL10_RESETVAL;
            break;
        }
        case 9:
        {
            pMcASPRegs->SRCTL9 = AM3X_MCASP_SRCTL9_RESETVAL;
            break;
        }
        case 8:
        {
            pMcASPRegs->SRCTL8 = AM3X_MCASP_SRCTL8_RESETVAL;
            break;
        }
        case 7:
        {
            pMcASPRegs->SRCTL7 = AM3X_MCASP_SRCTL7_RESETVAL;
            break;
        }
        case 6:
        {
            pMcASPRegs->SRCTL6 = AM3X_MCASP_SRCTL6_RESETVAL;
            break;
        }
        case 5:
        {
            pMcASPRegs->SRCTL5 = AM3X_MCASP_SRCTL5_RESETVAL;
            break;
        }
        case 4:
        {
            pMcASPRegs->SRCTL4 = AM3X_MCASP_SRCTL4_RESETVAL;
            break;
        }
        case 3:
        {
            pMcASPRegs->SRCTL3 = AM3X_MCASP_SRCTL3_RESETVAL;
            break;
        }
        case 2:
        {
            pMcASPRegs->SRCTL2 = AM3X_MCASP_SRCTL2_RESETVAL;
            break;
        }
        case 1:
        {
            pMcASPRegs->SRCTL1 = AM3X_MCASP_SRCTL1_RESETVAL;
            break;
        }
        case 0:
        {
            pMcASPRegs->SRCTL0 = AM3X_MCASP_SRCTL0_RESETVAL;
            break;
        }
        default:
        {
            break;
        }
    
        }
        serNum++;
    }
    
    pMcASPRegs->PFUNC = AM3X_MCASP_PFUNC_RESETVAL;
    pMcASPRegs->PDIR = AM3X_MCASP_PDIR_RESETVAL;
    pMcASPRegs->DITCTL = AM3X_MCASP_DITCTL_RESETVAL;
    pMcASPRegs->DLBCTL = AM3X_MCASP_DLBCTL_RESETVAL;
    pMcASPRegs->AMUTE = AM3X_MCASP_AMUTE_RESETVAL;
    pMcASPRegs->RSTAT = AM3X_MCASP_RSTAT_RESETVAL;
    pMcASPRegs->RINTCTL = AM3X_MCASP_RINTCTL_RESETVAL;
    pMcASPRegs->XSTAT = AM3X_MCASP_XSTAT_RESETVAL;
    pMcASPRegs->XINTCTL = AM3X_MCASP_XINTCTL_RESETVAL;
    pMcASPRegs->PDOUT = AM3X_MCASP_PDOUT_RESETVAL;
    pMcASPRegs->PDSET = AM3X_MCASP_PDSET_RESETVAL;
    pMcASPRegs->PDCLR = AM3X_MCASP_PDCLR_RESETVAL;

}
#endif  // not called. comment out

//------------------------------------------------------------------------------
//  Function: mcaspHwSetup  
//
//  It configures the  McASP instance registers as per the values passed
//  in the hardware setup structure. 
//
//
BOOL mcaspHwSetup(PMCASPREGS pMcASPRegs, McaspHwSetup *myHwSetup, UINT8 numSerializers)
{
    INT16 serNum = 0;
    BOOL bRC = TRUE;
    
    if ( (NULL == pMcASPRegs) || (NULL == myHwSetup) )
    {
        bRC = FALSE;
    }
    else
    {

        /* Reset McASP to default values by setting GBLCTL = 0 */
        pMcASPRegs->GBLCTL = 0x0000;

        /* Initialize the powerdown and emulation management register*/
        pMcASPRegs->PWRDEMU &= ~AM3X_MCASP_PWRDEMU_FREE_MASK;
        pMcASPRegs->PWRDEMU |= myHwSetup->emu << AM3X_MCASP_PWRDEMU_FREE_SHIFT;

        /* Configure the RMASK register */
        pMcASPRegs->RMASK = (UINT32)myHwSetup->rx.mask;

        /* Configure RXFMT */
        pMcASPRegs->RFMT = myHwSetup->rx.fmt;

        /* Configure RXFMCTL */
        pMcASPRegs->AFSRCTL = myHwSetup->rx.frSyncCtl;

        /* Configure ACLKRCTL */
        pMcASPRegs->ACLKRCTL = myHwSetup->rx.clk.clkSetupClk;

        /* Configure AHCLKRCTL */
        pMcASPRegs->AHCLKRCTL = myHwSetup->rx.clk.clkSetupHiClk;

        /* Configure RXTDM */
        pMcASPRegs->RTDM = myHwSetup->rx.tdm;

        /* Configure RINTCTL */
        pMcASPRegs->RINTCTL = myHwSetup->rx.intCtl;

        /* Configure RCLKCHK */
        pMcASPRegs->RCLKCHK = myHwSetup->rx.clk.clkChk;

        /* Configure TXMASK */
        pMcASPRegs->XMASK = myHwSetup->tx.mask;

        /* Configure TXFMT */
        pMcASPRegs->XFMT = myHwSetup->tx.fmt;

        /* Configure TXFMCTL */
        pMcASPRegs->AFSXCTL = myHwSetup->tx.frSyncCtl;

        /* Configure ACLKXCTL */
        pMcASPRegs->ACLKXCTL = myHwSetup->tx.clk.clkSetupClk;

        /* Configure AHCLKXCTL */
        pMcASPRegs->AHCLKXCTL = myHwSetup->tx.clk.clkSetupHiClk;

        /* Configure XTDM */
        pMcASPRegs->XTDM = myHwSetup->tx.tdm;

        /* Configure XINTCTL */
        pMcASPRegs->XINTCTL = myHwSetup->tx.intCtl;

        /* Configure XCLKCHK */
        pMcASPRegs->XCLKCHK = myHwSetup->tx.clk.clkChk;


        while (serNum < numSerializers)
        {

            switch(serNum)
            {
            case 15:
                pMcASPRegs->SRCTL15 = (UINT32)myHwSetup->glb.serSetup[serNum];
                break;
            case 14:
                pMcASPRegs->SRCTL14 = (UINT32)myHwSetup->glb.serSetup[serNum];
                break;
            case 13:
                pMcASPRegs->SRCTL13 = (UINT32)myHwSetup->glb.serSetup[serNum];
                break;    
            case 12:
                pMcASPRegs->SRCTL12 = (UINT32)myHwSetup->glb.serSetup[serNum];
                break;
            case 11:
                pMcASPRegs->SRCTL11 = (UINT32)myHwSetup->glb.serSetup[serNum];
                break;
            case 10:
                pMcASPRegs->SRCTL10 = (UINT32)myHwSetup->glb.serSetup[serNum];
                break;
            case 9:
                pMcASPRegs->SRCTL9 = (UINT32)myHwSetup->glb.serSetup[serNum];
                break;
            case 8:
                pMcASPRegs->SRCTL8 = (UINT32)myHwSetup->glb.serSetup[serNum];
                break;
            case 7:
                pMcASPRegs->SRCTL7 = (UINT32)myHwSetup->glb.serSetup[serNum];
                break;
            case 6:
                pMcASPRegs->SRCTL6 = (UINT32)myHwSetup->glb.serSetup[serNum];
                break;
            case 5:
                pMcASPRegs->SRCTL5 = (UINT32)myHwSetup->glb.serSetup[serNum];
                break;
            case 4:
                pMcASPRegs->SRCTL4 = (UINT32)myHwSetup->glb.serSetup[serNum];
                break;
            case 3:
                pMcASPRegs->SRCTL3 = (UINT32)myHwSetup->glb.serSetup[serNum];
                break;
            case 2:
                pMcASPRegs->SRCTL2 = (UINT32)myHwSetup->glb.serSetup[serNum];
                break;
            case 1:
                pMcASPRegs->SRCTL1 = (UINT32)myHwSetup->glb.serSetup[serNum];
                break;
            case 0:
                pMcASPRegs->SRCTL0 = (UINT32)myHwSetup->glb.serSetup[serNum];
                break;
            default:
                break;
            }
            serNum++;
        }

        /* Configure pin function register */
        pMcASPRegs->PFUNC = myHwSetup->glb.pfunc;

        /* Configure pin direction register */
        pMcASPRegs->PDIR = myHwSetup->glb.pdir;

        /* Configure DITCTL */
        pMcASPRegs->DITCTL = myHwSetup->glb.ditCtl;

        /* Configure LBCTL */
        pMcASPRegs->DLBCTL = myHwSetup->glb.dlbMode;

        /* Configure AMUTE */
        pMcASPRegs->AMUTE  =  myHwSetup->glb.amute;

        /* Configure RSTAT and XSTAT */
        pMcASPRegs->RSTAT = myHwSetup->rx.stat;
        pMcASPRegs->XSTAT = myHwSetup->tx.stat;

        /* Configure REVTCTL and XEVTCTL */
        pMcASPRegs->REVTCTL = myHwSetup->rx.evtCtl;
        pMcASPRegs->XEVTCTL =  myHwSetup->tx.evtCtl;

        /* Initialize the global control register */
        pMcASPRegs->GBLCTL = myHwSetup->glb.ctl;
    }

    return bRC;
}

#if 0  // not called. comment out
//------------------------------------------------------------------------------
//  Function: mcaspConfigGpio  
//
//  This function enables/disables the GPIO mode (configure GPIO mode). 
//
//
void mcaspConfigGpio(PMCASPREGS pMcASPRegs, UINT32 setVal)
{
    if (pMcASPRegs == NULL)
    {
        return;
    }
    
    /* Configure PDIR Pin direction with the value passed */
    pMcASPRegs->PFUNC = 0xFFFFFFFF;
    pMcASPRegs->PDIR = setVal;
}

//------------------------------------------------------------------------------
//  Function: mcaspGpioRead  
//
//  This function reads the value of the GPIOS (PDIN) 
//
//
UINT32 mcaspGpioRead (PMCASPREGS pMcASPRegs)
{
    if (pMcASPRegs == NULL)
    {
        return 0;
    }
       
    return pMcASPRegs->PDIN;
}

//------------------------------------------------------------------------------
//  Function: mcaspGpioWrite  
//
//  This function writes to the GPIOs (PDOUT) 
//
//
void mcaspGpioWrite (PMCASPREGS pMcASPRegs, UINT32 setVal)
{
    if (pMcASPRegs == NULL)
    {
        return;
    }
    
    /* Write data to PDOUT */
    pMcASPRegs->PDOUT =  setVal;
}

//------------------------------------------------------------------------------
//  Function: mcaspReadXmtConfig  
//
//  Reads the transmit configuration 
//
//
void mcaspReadXmtConfig (PMCASPREGS pMcASPRegs, McaspHwSetupData *xmtData)
{
    if ((NULL != pMcASPRegs) && (NULL != xmtData))
    {
        /* Read XMASK register */
        xmtData->mask = (UINT32)(pMcASPRegs->XMASK);

        /* Read XFMT register */
        xmtData->fmt = (UINT32)(pMcASPRegs->XFMT);
        
        /* Read AFSXCTL register */
        xmtData->frSyncCtl = (UINT32)(pMcASPRegs->AFSXCTL);

        xmtData->clk.clkSetupClk = (UINT32)(pMcASPRegs->ACLKXCTL);

        /* Read AHCLKXCTL register */
        xmtData->clk.clkSetupHiClk = (UINT32)(pMcASPRegs->AHCLKXCTL);

        /* Read XTDM register */
        xmtData->tdm = (UINT32)(pMcASPRegs->XTDM);

        /* Read XINTCTL register */
        xmtData->intCtl = (UINT32)(pMcASPRegs->XINTCTL);

        /* Read XCLKCHK register */
        xmtData->clk.clkChk = (UINT32)(pMcASPRegs->XCLKCHK);

        /* Read XSTAT register */
        xmtData->stat = (UINT32)(pMcASPRegs->XSTAT);

        /* Read XEVTCTL register */
        xmtData->evtCtl = (UINT32)(pMcASPRegs->XEVTCTL);
    }
}

//------------------------------------------------------------------------------
//  Function: mcaspReadRcvConfig  
//
//  Reads the receive configuration 
//
//
void mcaspReadRcvConfig (PMCASPREGS pMcASPRegs, McaspHwSetupData *rcvData)
{
    if ((NULL != pMcASPRegs) && (NULL != rcvData))
    {
        /* Read RMASK register */
        rcvData->mask = (UINT32)(pMcASPRegs->RMASK);

        /* Read RFMT register */
        rcvData->fmt = (UINT32)(pMcASPRegs->RFMT);
        
        /* Read AFSRCTL register */
        rcvData->frSyncCtl = (UINT32)(pMcASPRegs->AFSRCTL);

        rcvData->clk.clkSetupClk = (UINT32)(pMcASPRegs->ACLKRCTL);

        /* Read AHCLKRCTL register */
        rcvData->clk.clkSetupHiClk = (UINT32)(pMcASPRegs->AHCLKRCTL);

        /* Read RTDM register */
        rcvData->tdm = (UINT32)(pMcASPRegs->RTDM);

        /* Read RINTCTL register */
        rcvData->intCtl = (UINT32)(pMcASPRegs->RINTCTL);

        /* Read RCLKCHK register */
        rcvData->clk.clkChk = (UINT32)(pMcASPRegs->RCLKCHK);

        /* Read RSTAT register */
        rcvData->stat = (UINT32)(pMcASPRegs->RSTAT);

        /* Read REVTCTL register */
        rcvData->evtCtl = (UINT32)(pMcASPRegs->REVTCTL);
    }
}
#endif   // comment out.  not called

//------------------------------------------------------------------------------
//  Function: mcaspActivateSmFsXmt  
//
//  This function sets the XSMRST and XFSRST bits in GBLCTL. 
//
//
void mcaspActivateSmFsXmt  (PMCASPREGS pMcASPRegs)
{
    UINT16 selectMask;

    selectMask = AM3X_MCASP_XGBLCTL_TXSMRST_MASK |
                 AM3X_MCASP_XGBLCTL_TXFSRST_MASK;   

    //+++DBG: done on XGBLCTL in CCS test code
    pMcASPRegs->XGBLCTL /*+++DBG GBLCTL*/ |= selectMask;
    
    while (selectMask != (pMcASPRegs->GBLCTL & selectMask))
    {}
}

//------------------------------------------------------------------------------
//  Function: mcaspActivateSmFsRcv  
//
//  This function sets the RSMRST and RFSRST bits in GBLCTL. 
//
//
void mcaspActivateSmFsRcv  (PMCASPREGS pMcASPRegs)
{
    UINT16 selectMask;

    if (pMcASPRegs == NULL)
    {
        return;
    }
    
    selectMask = AM3X_MCASP_RGBLCTL_RXSMRST_MASK |
                 AM3X_MCASP_RGBLCTL_RXFSRST_MASK;   

    pMcASPRegs->RGBLCTL |= selectMask;
    
    while (selectMask != (pMcASPRegs->GBLCTL & selectMask))
    {}
}


#if 0
void mcaspShowPadConfig(void)
{
    PHYSICAL_ADDRESS          physicalAddress;
    AM387X_SYSC_PADCONFS_REGS *pPadRegs = NULL;


    // Print McASP PAD configs
    physicalAddress.HighPart = 0;
    physicalAddress.LowPart  = AM387X_SYSC_PADCONFS_REGS_PA; //0x48180800
	pPadRegs = (AM387X_SYSC_PADCONFS_REGS *)MmMapIoSpace(physicalAddress, sizeof(AM387X_SYSC_PADCONFS_REGS), FALSE);

    RETAILMSG(1,  (L">>>>> AM387X_SYSC_PADCONFS_REGS(0x%08X): \r\n", pPadRegs ));


    RETAILMSG(1,  (L"      XREF_CLK2 (AHCLKX)  (@0x%08X)  0x%08X \r\n", 
        &(pPadRegs->CONTROL_PADCONF_XREF_CLK2), pPadRegs->CONTROL_PADCONF_XREF_CLK2 ));

    RETAILMSG(1,  (L"      MCASP2_ACLKX        (@0x%08X)  0x%08X \r\n", 
        &(pPadRegs->CONTROL_PADCONF_MCASP2_ACLKX), pPadRegs->CONTROL_PADCONF_MCASP2_ACLKX));

    RETAILMSG(1,  (L"      MCASP2_FSX        (@0x%08X)  0x%08X \r\n", 
        &(pPadRegs->CONTROL_PADCONF_MCASP2_FSX), pPadRegs->CONTROL_PADCONF_MCASP2_FSX));

    RETAILMSG(1,  (L"      MCASP2_AXR0        (@0x%08X)  0x%08X \r\n", 
        &(pPadRegs->CONTROL_PADCONF_MCASP2_AXR0), pPadRegs->CONTROL_PADCONF_MCASP2_AXR0));

    RETAILMSG(1,  (L"      MCASP2_AXR1        (@0x%08X)  0x%08X \r\n", 
        &(pPadRegs->CONTROL_PADCONF_MCASP2_AXR1), pPadRegs->CONTROL_PADCONF_MCASP2_AXR1));

    RETAILMSG(1,  (L"      XREF_CLK2        (@0x%08X)  0x%08X \r\n", 
        &(pPadRegs->), pPadRegs->));

}
#endif


#if 0
static UINT16 sinetable[48] = {
    0x0000, 0x10b4, 0x2120, 0x30fb, 0x3fff, 0x4dea, 0x5a81, 0x658b,
    0x6ed8, 0x763f, 0x7ba1, 0x7ee5, 0x7ffd, 0x7ee5, 0x7ba1, 0x763f,
    0x6ed8, 0x658b, 0x5a81, 0x4dea, 0x3fff, 0x30fb, 0x2120, 0x10b4,
    0x0000, 0xef4c, 0xdee0, 0xcf06, 0xc002, 0xb216, 0xa57f, 0x9a75,
    0x9128, 0x89c1, 0x845f, 0x811b, 0x8002, 0x811b, 0x845f, 0x89c1,
    0x9128, 0x9a76, 0xa57f, 0xb216, 0xc002, 0xcf06, 0xdee0, 0xef4c
};

void mcaspTestAudio(PMCASPREGS pMcASPRegs)
{
	UINT16 u16Sample = 0;
	UINT16 u16MilliSec = 0;
	UINT16 u16Seconds = 0;
    UINT32  wait;
    
    UINT32  XSTAT1, XSTAT2, XSTAT3;

    
    UINT32 left_wait, right_wait, samples_written=0;

    RETAILMSG(1, (TEXT("=> mcaspTestAudio\r\n" )));

//    mcaspShowPadConfig();

	/* Initialize MCASP2 */
    /* ---------------------------------------------------------------- *
     *                                                                  *
     *  McASP2 is in MASTER mode.                                       *
     *      BCLK & WCLK come from McASP2                                *
     *      DIN is used by write16/write32                              *
     *      DOUT is usec by read16/read32                               *
     *                                                                  *
     * ---------------------------------------------------------------- */
	pMcASPRegs->GBLCTL = 0;		// Reset the McASP
    for (wait=0; wait < 0x0000ffff; wait++){}
	pMcASPRegs->RGBLCTL = 0; 	// Reset the Rx section
    for (wait=0; wait < 0x0000ffff; wait++){}
	pMcASPRegs->XGBLCTL	= 0;	// Reset the Tx section
    for (wait=0; wait < 0x0000ffff; wait++){}
	
	/* Rx side initialization */
	pMcASPRegs->RMASK	= 0xFFFFFFFF;	// No padding used
	pMcASPRegs->RFMT	= 0x00018078; // MSB 16bit, 1-delay, no pad, CFGBus
	pMcASPRegs->AFSRCTL = 0x00000112; // 2TDM, 1bit Rising, INTERNAL FS, word
	pMcASPRegs->ACLKRCTL  = 0x000000AF; // Rising INTERNAL CLK (from tx side)
    pMcASPRegs->AHCLKRCTL  = 0x00000000; // INT CLK (from tx side)
    pMcASPRegs->RTDM       = 0x00000003; // Slots 0,1
    pMcASPRegs->RINTCTL    = 0x00000000; // Not used
    pMcASPRegs->RCLKCHK    = 0x00FF0008; // 255-MAX 0-MIN, div-by-256

    /* TX */
    pMcASPRegs->XMASK      = 0xffffffff; // No padding used
    pMcASPRegs->XFMT       = 0x00018078; // MSB 16bit, 1-delay, no pad, CFGBus
    pMcASPRegs->AFSXCTL    = 0x00000110; // 2TDM, 1bit Rising edge INTERNAL FS, word
    pMcASPRegs->ACLKXCTL   = 0x00000080; // ASYNC, Rising INTERNAL CLK, div-by-16
    pMcASPRegs->AHCLKXCTL  = 0x00000000; // EXTERNAL CLK, div-by-1
    pMcASPRegs->XTDM       = 0x00000003; // Slots 0,1
    pMcASPRegs->XINTCTL    = 0x00000000; // Not used
    pMcASPRegs->XCLKCHK    = 0x00FF0008; // 255-MAX 0-MIN, div-by-256

    pMcASPRegs->SRCTL0     = 0x000D;     // MCASP2.AXR0 --> DIN
    pMcASPRegs->SRCTL1     = 0x000E;     // MCASP2.AXR1 <-- DOUT
    pMcASPRegs->PFUNC      = 0;          // All MCASPs
    pMcASPRegs->PDIR       = 0x00000001; // All inputs except AXR0, ACLKX1, AFSX1,

    pMcASPRegs->DITCTL     = 0x00000000; // Not used
    pMcASPRegs->DLBCTL     = 0x00000000; // Not used
    pMcASPRegs->AMUTE      = 0x00000000; // Not used

    /* Starting sections of the McASP*/
    pMcASPRegs->XGBLCTL |= 0x0200;                                    // HS Clk
    while ( ( pMcASPRegs->XGBLCTL & 0x0200 ) != 0x0200 );  
    pMcASPRegs->RGBLCTL |= 0x0002;                                    // HS Clk
    while ( ( pMcASPRegs->RGBLCTL & 0x0002 ) != 0x0002 );
   
    pMcASPRegs->XGBLCTL |= 0x0100;                                     // Clk
    while ( ( pMcASPRegs->XGBLCTL & 0x0100 ) != 0x0100 );
    pMcASPRegs->RGBLCTL |= 0x0001;                                     // Clk
    while ( ( pMcASPRegs->RGBLCTL & 0x0001 ) != 0x0001 );

    pMcASPRegs->XSTAT = 0x0000ffff;        // Clear all
    pMcASPRegs->RSTAT = 0x0000ffff;        // Clear all

    pMcASPRegs->XGBLCTL |= 0x0400;                                   // Serialize
    while ( ( pMcASPRegs->XGBLCTL & 0x0400 ) != 0x0400 );
    pMcASPRegs->RGBLCTL |= 0x0004;                                      // Serialize
    while ( ( pMcASPRegs->RGBLCTL & 0x0004 ) != 0x0004 );

    /* Write a 0, so that no underrun occurs after releasing the state machine */
    XSTAT1 = pMcASPRegs->XSTAT;
    pMcASPRegs->XBUF0 = 0;
    XSTAT2 = pMcASPRegs->XSTAT;
    //MCASP2_RBUF0 = 0;

    pMcASPRegs->XGBLCTL |= 0x0800;                                       // State Machine  XSMRST
    pMcASPRegs->RGBLCTL |= 0x0008;                                       // State Machine  RSMRST
    pMcASPRegs->XGBLCTL |= 0x1000;                                        // Frame Sync XFRST
    pMcASPRegs->RGBLCTL |= 0x0010;                                        // Frame Sync RFRST
    while ( ( pMcASPRegs->GBLCTL & 0x1818 ) != 0x1818 );

    XSTAT3 = pMcASPRegs->XSTAT;
    
     /* Start by sending a dummy write */
    while( ! ( pMcASPRegs->SRCTL0 & 0x10 ) );  // Check for Tx ready
	pMcASPRegs->XBUF0 = 0;

    /* Play Tone */
    for ( u16Seconds = 0 ; u16Seconds < 5 ; u16Seconds++ )
    {
        for ( u16MilliSec = 0 ; u16MilliSec < 1000 ; u16MilliSec++ )
        {
            for ( u16Sample = 0 ; u16Sample < 48 ; u16Sample++ )
            {
                /* Send a sample to the left channel */
                left_wait=0; while ( ! ( pMcASPRegs->SRCTL0 & 0x10 ) && left_wait < 0xfffff ) ++left_wait;
                if(left_wait >= 0xffff)
                {
                    RETAILMSG(1, (TEXT("left_wait, samples_written=%d, XSTAT=0x%08X, PID=0x%08X\r\n"), 
                        samples_written, pMcASPRegs->XSTAT, pMcASPRegs->PID));
                    return;
                }
                pMcASPRegs->XBUF0 = (sinetable[u16Sample] << 16);
                samples_written++;                

                /* Send a sample to the right channel */
                right_wait=0; while ( ! ( pMcASPRegs->SRCTL0 & 0x10 ) && right_wait < 0xfffff ) ++right_wait;
                if(right_wait >= 0xffff)
                {
                    RETAILMSG(1, (TEXT("right_wait, samples_written=%d, XSTAT=0x%08X, PID=0x%08X\r\n"), 
                        samples_written, pMcASPRegs->XSTAT, pMcASPRegs->PID));
                    return;
                }
                pMcASPRegs->XBUF0 = (sinetable[u16Sample] << 16);
                samples_written++;
                
			}
        }
    }

    RETAILMSG(1, (TEXT("<= mcaspTestAudio samples_written=%d\r\n" ),samples_written));
}
#endif

