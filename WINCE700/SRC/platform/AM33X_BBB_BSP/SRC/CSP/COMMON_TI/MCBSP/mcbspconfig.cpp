// All rights reserved ADENEO EMBEDDED 2010
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this source code is subject to the terms of the Microsoft end-user
// license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
// If you did not accept the terms of the EULA, you are not authorized to use
// this source code. For a copy of the EULA, please see the LICENSE.RTF on your
// install media.
//
// Portions Copyright (c) Texas Instruments.  All rights reserved.
//
//------------------------------------------------------------------------------
//
//  File: mcbspconfig.c
//

#include <windows.h>
#include <ceddk.h>
#include <ceddkex.h>
#include <mcbsp.h>

#include <omap.h>
#include "debug.h"
#include "mcbsptypes.h"
#include "soc_cfg.h"

//------------------------------------------------------------------------------
//
// Defines the word length in terms of enum McBSPWordLength_e.
//
//
static const int f_rgWordLength[] =
{
     8,        /* serial word length 8 bits */
    12,        /* serial word length 12 bits */
    16,        /* serial word length 16 bits */
    20,        /* serial word length 20 bits */
    24,        /* serial word length 24 bits */
    32         /* serial word length 32 bits */
};



//------------------------------------------------------------------------------
//
// mcbsp_ConfigCommonRegisters
//
void
mcbsp_ConfigCommonRegisters(
    McBSPDevice_t* pDevice
    )
{
    DEBUGMSG(ZONE_FUNCTION, (L"MCP:+%S(0x%08x)\r\n", __FUNCTION__, pDevice));

    // initialize shadow registers
    //
    mcbsp_ResetShadowRegisters(pDevice);

    // overwrite default configuration with registry values
    //
    if (pDevice->useRegistryForMcbsp)
        {
        mcbsp_GetRegistryValues(pDevice);
        }

    mcbsp_ResetSampleRateGenerator(pDevice);
    mcbsp_ResetTransmitter(pDevice);
    mcbsp_ResetReceiver(pDevice);

    mcbsp_ConfigureSampleRateGenerator(pDevice);
    mcbsp_ConfigureTransmitter(pDevice);
    mcbsp_ConfigureReceiver(pDevice);
    mcbsp_ClearIRQStatus(pDevice);
    mcbsp_UpdateRegisters(pDevice);

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:-%S\r\n", __FUNCTION__));
}


//------------------------------------------------------------------------------
//
// mcbsp_GetRegistryValues
//
void
mcbsp_GetRegistryValues(
    McBSPDevice_t* pDevice
    )
{
    DEBUGMSG(ZONE_FUNCTION, (L"MCP:+%S(0x%08x)\r\n", __FUNCTION__, pDevice));

    //------------------------
    // SRG values
    //------------------------

    // use SRG clock source from registry
    switch (pDevice->clockSourceSRG)
        {
        case kMcBSP_SRG_SRC_CLKS_PIN_RISE:
            pDevice->pConfigInfo->SRGClkSrc = kMcBSP_SRG_SRC_CLKS_PIN_RISE;
            break;
        case kMcBSP_SRG_SRC_CLKS_PIN_FALL:
            pDevice->pConfigInfo->SRGClkSrc = kMcBSP_SRG_SRC_CLKS_PIN_FALL;
            break;
        case kMcBSP_SRG_SRC_CPU_CLK:
            pDevice->pConfigInfo->SRGClkSrc = kMcBSP_SRG_SRC_CPU_CLK;
            break;
        case kMcBSP_SRG_SRC_CLKR_PIN:
            pDevice->pConfigInfo->SRGClkSrc = kMcBSP_SRG_SRC_CLKR_PIN;
            break;
        case kMcBSP_SRG_SRC_CLKX_PIN:
            pDevice->pConfigInfo->SRGClkSrc = kMcBSP_SRG_SRC_CLKX_PIN;
            break;
        default:
            pDevice->pConfigInfo->SRGClkSrc = kMcBSP_SRG_SRC_CLKX_PIN;
        }

    // use frame width from registry
    pDevice->pConfigInfo->SRGFrameWidth = (UINT32)pDevice->frameWidthSRG;

    // use clock divider from registry
    pDevice->pConfigInfo->SRGClkDivFactor = (UINT32)pDevice->clockDividerSRG;

    // use clock resync from registry
    if (pDevice->clockResyncSRG)
        {
        pDevice->pConfigInfo->SRGClkSyncMode = MCBSP_SRGR2_GSYNC;
        }
    else
        {
        pDevice->pConfigInfo->SRGClkSyncMode = 0;
        }

    //------------------------
    // Transmit and receive values
    //------------------------

    // use frame length from registry
    pDevice->pConfigInfo->TxFrameLength = (UINT32)pDevice->wordsPerFrame;
    pDevice->pConfigInfo->RxFrameLength = (UINT32)pDevice->wordsPerFrame;

    // use word length from registry
    switch (pDevice->wordLength)
        {
        case 8:
            pDevice->pConfigInfo->TxWordLength = kMcBSP_Word_Length_8;
            pDevice->pConfigInfo->RxWordLength = kMcBSP_Word_Length_8;
            break;
        case 12:
            pDevice->pConfigInfo->TxWordLength = kMcBSP_Word_Length_12;
            pDevice->pConfigInfo->RxWordLength = kMcBSP_Word_Length_12;
            break;
        case 16:
            pDevice->pConfigInfo->TxWordLength = kMcBSP_Word_Length_16;
            pDevice->pConfigInfo->RxWordLength = kMcBSP_Word_Length_16;
            break;
        case 20:
            pDevice->pConfigInfo->TxWordLength = kMcBSP_Word_Length_20;
            pDevice->pConfigInfo->RxWordLength = kMcBSP_Word_Length_20;
            break;
        case 24:
            pDevice->pConfigInfo->TxWordLength = kMcBSP_Word_Length_24;
            pDevice->pConfigInfo->RxWordLength = kMcBSP_Word_Length_24;
            break;
        case 32:
            pDevice->pConfigInfo->TxWordLength = kMcBSP_Word_Length_32;
            pDevice->pConfigInfo->RxWordLength = kMcBSP_Word_Length_32;
            break;
        default:
            pDevice->pConfigInfo->TxWordLength = kMcBSP_Word_Length_16;
            pDevice->pConfigInfo->RxWordLength = kMcBSP_Word_Length_16;
        }

    // use transmit dual phase from registry
    if (pDevice->phaseTx)
        {
        pDevice->pConfigInfo->TxPhase = MCBSP_PHASE_DUAL;

        // use transmit word length from registry
        switch (pDevice->wordLength2)
            {
            case 8:
                pDevice->pConfigInfo->TxWordLength2 = kMcBSP_Word_Length_8;
                break;
            case 12:
                pDevice->pConfigInfo->TxWordLength2 = kMcBSP_Word_Length_12;
                break;
            case 16:
                pDevice->pConfigInfo->TxWordLength2 = kMcBSP_Word_Length_16;
                break;
            case 20:
                pDevice->pConfigInfo->TxWordLength2 = kMcBSP_Word_Length_20;
                break;
            case 24:
                pDevice->pConfigInfo->TxWordLength2 = kMcBSP_Word_Length_24;
                break;
            case 32:
                pDevice->pConfigInfo->TxWordLength2 = kMcBSP_Word_Length_32;
                break;
            default:
                pDevice->pConfigInfo->TxWordLength2 = kMcBSP_Word_Length_16;
            }
        }
    else
        {
        pDevice->pConfigInfo->TxPhase = MCBSP_PHASE_SINGLE;
        }

     // use receive phase from registry
    if (pDevice->phaseRx)
        {
        pDevice->pConfigInfo->RxPhase = MCBSP_PHASE_DUAL;

        // use transmit word length from registry
        switch (pDevice->wordLength2)
            {
            case 8:
                pDevice->pConfigInfo->RxWordLength2 = kMcBSP_Word_Length_8;
                break;
            case 12:
                pDevice->pConfigInfo->RxWordLength2 = kMcBSP_Word_Length_12;
                break;
            case 16:
                pDevice->pConfigInfo->RxWordLength2 = kMcBSP_Word_Length_16;
                break;
            case 20:
                pDevice->pConfigInfo->RxWordLength2 = kMcBSP_Word_Length_20;
                break;
            case 24:
                pDevice->pConfigInfo->RxWordLength2 = kMcBSP_Word_Length_24;
                break;
            case 32:
                pDevice->pConfigInfo->RxWordLength2 = kMcBSP_Word_Length_32;
                break;
            default:
                pDevice->pConfigInfo->RxWordLength2 = kMcBSP_Word_Length_16;
            }
        }
    else
        {
        pDevice->pConfigInfo->RxPhase = MCBSP_PHASE_SINGLE;
        }

    // use data delay from registry
    switch (pDevice->dataDelayTx)
        {
        case MCBSP_DATDLY_0BIT:
            pDevice->pConfigInfo->TxDataDelay = MCBSP_DATDLY_0BIT;
            break;
        case MCBSP_DATDLY_1BIT:
            pDevice->pConfigInfo->TxDataDelay = MCBSP_DATDLY_1BIT;
            break;
        case MCBSP_DATDLY_2BIT:
            pDevice->pConfigInfo->TxDataDelay = MCBSP_DATDLY_2BIT;
            break;
        default:
            pDevice->pConfigInfo->TxDataDelay = MCBSP_DATDLY_1BIT;
        }

    // use data delay from registry
    switch (pDevice->dataDelayRx)
        {
        case MCBSP_DATDLY_0BIT:
            pDevice->pConfigInfo->RxDataDelay = MCBSP_DATDLY_0BIT;
            break;
        case MCBSP_DATDLY_1BIT:
            pDevice->pConfigInfo->RxDataDelay = MCBSP_DATDLY_1BIT;
            break;
        case MCBSP_DATDLY_2BIT:
            pDevice->pConfigInfo->RxDataDelay = MCBSP_DATDLY_2BIT;
            break;
        default:
            pDevice->pConfigInfo->RxDataDelay = MCBSP_DATDLY_1BIT;
        }

    //------------------------
    // Transmit values
    //------------------------

    // use transmit frame sync source from registry
    switch (pDevice->frameSyncSourceTx)
        {
        case kMcBSP_Tx_FS_SRC_External:
            pDevice->pConfigInfo->TxFrameSyncSource =
                kMcBSP_Tx_FS_SRC_External;
            break;
        case kMcBSP_Tx_FS_SRC_DSR_XSR_COPY:
            pDevice->pConfigInfo->TxFrameSyncSource =
                kMcBSP_Tx_FS_SRC_DSR_XSR_COPY;
            break;
        case kMcBSP_Tx_FS_SRC_SRG:
            pDevice->pConfigInfo->TxFrameSyncSource =
                kMcBSP_Tx_FS_SRC_SRG;
            break;
        default:
            pDevice->pConfigInfo->TxFrameSyncSource =
                kMcBSP_Tx_FS_SRC_External;
        }

    // use CLKXM from registry
    if (pDevice->clockModeTx)
        {
        pDevice->pConfigInfo->TxClockSource = MCBSP_PCR_CLKXM;
        }
    else
        {
        pDevice->pConfigInfo->TxClockSource = 0;
        }

    // use FSXP from registry
    if (pDevice->frameSyncPolarityTx)
        {
        pDevice->pConfigInfo->TxFrameSyncPolarity = MCBSP_PCR_FSXP;
        }
    else
        {
        pDevice->pConfigInfo->TxFrameSyncPolarity = 0;
        }

    // use CLKXP from registry
    if (pDevice->clockPolarityTx)
        {
        pDevice->pConfigInfo->TxClkPolarity = MCBSP_PCR_CLKXP;
        }
    else
        {
        pDevice->pConfigInfo->TxClkPolarity = 0;
        }

    // use Reverse Mode from registry
    if (pDevice->reverseModeTx)
        {
        pDevice->pConfigInfo->TxReverse = MCBSP_REVERSE_LSB_FIRST;
        }
    else
        {
        pDevice->pConfigInfo->TxReverse = MCBSP_REVERSE_MSB_FIRST;
        }

    //------------------------
    // Receive values
    //------------------------

    // use FSRM from registry
    if (pDevice->frameSyncSourceRx)
        {
        pDevice->pConfigInfo->RxFrameSyncSource = MCBSP_PCR_FSRM;
        }
    else
        {
        pDevice->pConfigInfo->RxFrameSyncSource = 0;
        }

    // use CLKRM from registry
    if (pDevice->clockModeRx)
        {
        pDevice->pConfigInfo->RxClockSource = MCBSP_PCR_CLKRM;
        }
    else
        {
        pDevice->pConfigInfo->RxClockSource = 0;
        }

    // use FSRP from registry
    if (pDevice->frameSyncPolarityRx)
        {
        pDevice->pConfigInfo->RxFrameSyncPolarity = MCBSP_PCR_FSRP;
        }
    else
        {
        pDevice->pConfigInfo->RxFrameSyncPolarity = 0;
        }

    // use CLKRP from registry
    if (pDevice->clockPolarityRx)
        {
        pDevice->pConfigInfo->RxClkPolarity = MCBSP_PCR_CLKRP;
        }
    else
        {
        pDevice->pConfigInfo->RxClkPolarity = 0;
        }

    // use loopback mode from registry
    if (pDevice->loopbackMode)
        {
        pDevice->pConfigInfo->AnalogLoopBackMode = MCBSP_SPCR1_ALB_ENABLE;
        }
    else
        {
        pDevice->pConfigInfo->AnalogLoopBackMode = 0;
        }

    // use justification mode from registry
    switch (pDevice->justificationMode << 13)
        {
        case MCBSP_SPCR1_RJUST_RJ_ZEROFILL:
            pDevice->pConfigInfo->JustificationMode =
                MCBSP_SPCR1_RJUST_RJ_ZEROFILL;
            break;
        case MCBSP_SPCR1_RJUST_RJ_SIGNFILL:
            pDevice->pConfigInfo->JustificationMode =
                MCBSP_SPCR1_RJUST_RJ_SIGNFILL;
            break;
        case MCBSP_SPCR1_RJUST_LJ_ZEROFILL:
            pDevice->pConfigInfo->JustificationMode =
                MCBSP_SPCR1_RJUST_LJ_ZEROFILL;
            break;
        default:
            pDevice->pConfigInfo->JustificationMode =
                MCBSP_SPCR1_RJUST_RJ_ZEROFILL;
        }

    // use Reverse Mode from registry
    if (pDevice->reverseModeRx)
        {
        pDevice->pConfigInfo->RxReverse = MCBSP_REVERSE_LSB_FIRST;
        }
    else
        {
        pDevice->pConfigInfo->RxReverse = MCBSP_REVERSE_MSB_FIRST;
        }

    pDevice->pConfigInfo->RxSyncError = MCBSP_SPCR1_RINTM_RSYNCERR;
    pDevice->pConfigInfo->TxSyncError = MCBSP_SPCR2_XINTM_XSYNCERR;

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:-%S\r\n", __FUNCTION__));
}


//------------------------------------------------------------------------------
//
// mcbsp_ConfigureForMaster
//
void
mcbsp_ConfigureForMaster(
    McBSPDevice_t* pDevice
    )
{
    DEBUGMSG(ZONE_FUNCTION, (L"MCP:+%S(0x%08x)\r\n", __FUNCTION__, pDevice));

    //------------------------
    // SRG values
    //------------------------

    // SRG clock source
    // value: 0(rising CLKS), 1(falling CLKS), 2(CPU), 3(CLKRI),  4(CLKXI)
    pDevice->pConfigInfo->SRGClkSrc = kMcBSP_SRG_SRC_CLKS_PIN_FALL;

    // frame width
    pDevice->pConfigInfo->SRGFrameWidth = 16;

    // use clock divider based on frequency (input clk is 256 * 48KHz fclock)
    // sampling frequency = 48Khz
    //clkgdv = (clk rate /(sample rate * (bit_per_sample * 2))) - 1
    pDevice->pConfigInfo->SRGClkDivFactor = 7;

    // clock resync
    // Value: MCBSP_SRGR2_GSYNC or 0
    pDevice->pConfigInfo->SRGClkSyncMode = 0;

    //------------------------
    // Transmit and receive values
    //------------------------
#if 1
    // Single phase

    // frame length
    pDevice->pConfigInfo->TxFrameLength = 1;
    pDevice->pConfigInfo->RxFrameLength = 1;

    // word length
    pDevice->pConfigInfo->TxWordLength = kMcBSP_Word_Length_32;
    pDevice->pConfigInfo->RxWordLength = kMcBSP_Word_Length_32;

    // dual phase
    // value: MCBSP_PHASE_DUAL or MCBSP_PHASE_SINGLE
    pDevice->pConfigInfo->TxPhase = MCBSP_PHASE_SINGLE;
    pDevice->pConfigInfo->RxPhase = MCBSP_PHASE_SINGLE;

#else
    // Dual phase

    // frame length
    pDevice->pConfigInfo->TxFrameLength = 1;
    pDevice->pConfigInfo->RxFrameLength = 1;

    // word length
    pDevice->pConfigInfo->TxWordLength = kMcBSP_Word_Length_16;
    pDevice->pConfigInfo->RxWordLength = kMcBSP_Word_Length_16;

    // dual phase
    // value: MCBSP_PHASE_DUAL or MCBSP_PHASE_SINGLE
    pDevice->pConfigInfo->TxPhase = MCBSP_PHASE_DUAL;
    pDevice->pConfigInfo->RxPhase = MCBSP_PHASE_DUAL;

    // word length 2 ( for dual phase)
    pDevice->pConfigInfo->TxWordLength2 = kMcBSP_Word_Length_16;
    pDevice->pConfigInfo->RxWordLength2 = kMcBSP_Word_Length_16;
#endif
    // use data delay from registry
    // Value: 0, 1, or 2 bits
    pDevice->pConfigInfo->TxDataDelay = 1;
    pDevice->pConfigInfo->RxDataDelay = 1;

    //------------------------
    // Transmit values
    //------------------------

    // transmit frame sync source
    // Value: 0 (ext), 2 (int xmit), or 3 (int SRG)
    pDevice->pConfigInfo->TxFrameSyncSource = kMcBSP_Tx_FS_SRC_SRG;

    // CLKXM
    // Value: MCBSP_PCR_CLKXM or 0(external clock drivers CLKX)
    pDevice->pConfigInfo->TxClockSource = MCBSP_PCR_CLKXM;

    // FSXP
    // Value: MCBSP_PCR_FSXP or 0(active high)
    pDevice->pConfigInfo->TxFrameSyncPolarity = 0;

    // CLKXP
    // Value: MCBSP_PCR_CLKXP or 0(rising edge)
    pDevice->pConfigInfo->TxClkPolarity = 0;

    // Reverse Mode
    // value: MCBSP_REVERSE_MSB_FIRST or MCBSP_REVERSE_LSB_FIRST
    pDevice->pConfigInfo->TxReverse = MCBSP_REVERSE_MSB_FIRST;

    //------------------------
    // Receive values
    //------------------------

    //  FSRM
    // Value: MCBSP_PCR_FSRM or 0(external clock drivers frame sync)
    pDevice->pConfigInfo->RxFrameSyncSource = MCBSP_PCR_FSRM;

    // CLKRM from registry
    // Value: MCBSP_PCR_CLKRM or 0(external clock drivers CLKR)
    pDevice->pConfigInfo->RxClockSource = MCBSP_PCR_CLKRM;

    // FSRP
    // Value: MCBSP_PCR_FSRP or 0(active high)
    pDevice->pConfigInfo->RxFrameSyncPolarity = 0;

    // CLKRP
    // Value: MCBSP_PCR_CLKRP or 0(falling edge)
    pDevice->pConfigInfo->RxClkPolarity = MCBSP_PCR_CLKRP;

    // loopback mode (MCBSP_SPCR1_ALB_ENABLE or 0)
    pDevice->pConfigInfo->AnalogLoopBackMode = 0;

    // justification mode
    //(right, zero fill), (right, sign fill), or (left)
    pDevice->pConfigInfo->JustificationMode = MCBSP_SPCR1_RJUST_RJ_ZEROFILL;

    // Reverse Mode
    // value: MCBSP_REVERSE_MSB_FIRST or MCBSP_REVERSE_LSB_FIRST
    pDevice->pConfigInfo->RxReverse = MCBSP_REVERSE_MSB_FIRST;

    pDevice->pConfigInfo->RxSyncError = 0;
    pDevice->pConfigInfo->TxSyncError = 0;

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:-%S\r\n", __FUNCTION__));
}


//------------------------------------------------------------------------------
//
// mcbsp_ResetShadowRegisters
//
void
mcbsp_ResetShadowRegisters(
    McBSPDevice_t* pDevice
    )
{
    DEBUGMSG(ZONE_FUNCTION, (L"MCP:+%S(0x%08x)\r\n", __FUNCTION__, pDevice));

    UINT counter = 0;
    // Do a software reset of McBSP
    pDevice->shadowRegs.SYSCONFIG = MCBSP_SYSCONFIG_SOFTRESET;
    OUTREG32(&pDevice->pMcbspRegs->SYSCONFIG, pDevice->shadowRegs.SYSCONFIG);

    // Read to confirm if the reset is done
    while (INREG32(&pDevice->pMcbspRegs->SYSCONFIG) & MCBSP_SYSCONFIG_SOFTRESET)
        {
        Sleep(1);
        if (counter++ > MAX_WAIT_FOR_RESET)
            {
            DEBUGMSG(ZONE_ERROR, (L"ERROR: MCP:%S: Reset Timeout\r\n",
                __FUNCTION__));
            break;
            }
        }

    pDevice->shadowRegs.SPCR2 = 0;
    pDevice->shadowRegs.SPCR1 = 0;
    pDevice->shadowRegs.RCR2 = 0;
    pDevice->shadowRegs.RCR1 = 0;
    pDevice->shadowRegs.XCR2 = 0;
    pDevice->shadowRegs.XCR1 = 0;
    pDevice->shadowRegs.SRGR2 = MCBSP_SRGR2_CLKSM;
    pDevice->shadowRegs.SRGR1 = 1;
    pDevice->shadowRegs.PCR = 0;
    pDevice->shadowRegs.THRSH1 = 0;
    pDevice->shadowRegs.THRSH2 = 0;
    pDevice->shadowRegs.SYSCONFIG = 0;
    pDevice->shadowRegs.WAKEUPEN = 0;
    pDevice->shadowRegs.MCR1 = 0;
    pDevice->shadowRegs.MCR2 = 0;
    pDevice->shadowRegs.RCERA = 0;
    pDevice->shadowRegs.XCERA = 0;

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:-%S\r\n", __FUNCTION__));
}


//------------------------------------------------------------------------------
//
// mcbsp_ConfigureSampleRateGenerator
//
void
mcbsp_ConfigureSampleRateGenerator(
    McBSPDevice_t* pDevice
    )
{
    DEBUGMSG(ZONE_FUNCTION, (L"MCP:+%S(0x%08x)\r\n", __FUNCTION__, pDevice));

    // update register: SRGR1
    //------------------------
    // set FWID
    // set CLKGDIV
    //
    ASSERT(pDevice->pConfigInfo->SRGFrameWidth > 0 &&
           pDevice->pConfigInfo->SRGFrameWidth <= 256 &&
           pDevice->pConfigInfo->SRGClkDivFactor < 256);
    pDevice->shadowRegs.SRGR1 |=
        MCBSP_SRGR1_FWID(pDevice->pConfigInfo->SRGFrameWidth) |
        pDevice->pConfigInfo->SRGClkDivFactor;

    // update register: SRGR2
    //------------------------
    // set GSYNC
    // set FPER
    //
    // for auto calculation of the frame period
    // assumes transmit and receive word lengths are the same
    // assumes transmit and receive words per frame are the same
    // assumes transmit and receive phases are the same (either single or
    //  dual phase)

    if (pDevice->pConfigInfo->TxPhase == MCBSP_PHASE_DUAL)
        {
        // in dual phase transmit1 and transmit2 words per frame are the same
        pDevice->shadowRegs.SRGR2 |= pDevice->pConfigInfo->SRGClkSyncMode |
            ((((pDevice->pConfigInfo->TxFrameLength) *
            (f_rgWordLength[pDevice->pConfigInfo->TxWordLength])) +
            ((pDevice->pConfigInfo->TxFrameLength) *
            (f_rgWordLength[pDevice->pConfigInfo->TxWordLength2]))) -1);
        }
    else
        {
        pDevice->shadowRegs.SRGR2 |= pDevice->pConfigInfo->SRGClkSyncMode |
            (((pDevice->pConfigInfo->TxFrameLength) *
            (f_rgWordLength[pDevice->pConfigInfo->TxWordLength])) -1);
        }

    // set CLKSM
    //
    if (pDevice->pConfigInfo->SRGClkSrc == kMcBSP_SRG_SRC_CPU_CLK ||
        pDevice->pConfigInfo->SRGClkSrc == kMcBSP_SRG_SRC_CLKX_PIN)
        {
        // Sample-rate generator source is either CLKXI(SCLKME=1) or
        // FCLK(SCLKME=0)
        //
        pDevice->shadowRegs.SRGR2 |= MCBSP_SRGR2_CLKSM;
        }
    else
        {
        // Sample-rate generator source is either CLKRI(SCLKME=1) or
        // CLKS(SCLKME=0)
        //
        pDevice->shadowRegs.SRGR2 &= ~MCBSP_SRGR2_CLKSM;
        if (pDevice->pConfigInfo->SRGClkSrc == kMcBSP_SRG_SRC_CLKS_PIN_FALL)
            {
            // Falling CLKS
            //
            pDevice->shadowRegs.SRGR2 |= MCBSP_SRGR2_CLKSP;
            }
        else if (pDevice->pConfigInfo->SRGClkSrc ==
            kMcBSP_SRG_SRC_CLKS_PIN_RISE)
            {
            // Rising CLKS
            //
            pDevice->shadowRegs.SRGR2 &= ~MCBSP_SRGR2_CLKSP;
            }
        }

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:-%S\r\n", __FUNCTION__));
}


//------------------------------------------------------------------------------
//
// mcbsp_ConfigureTransmitter
//
void
mcbsp_ConfigureTransmitter(
    McBSPDevice_t* pDevice
    )
{
    DEBUGMSG(ZONE_FUNCTION, (L"MCP:+%S(0x%08x)\r\n", __FUNCTION__, pDevice));

    // update register: SPCR1
    //------------------------
    // set DLB
    // set DXENA
    //
    pDevice->shadowRegs.SPCR1 |= pDevice->pConfigInfo->AnalogLoopBackMode |
        MCBSP_SPCR1_DXENA;

    // update register: XCR1
    //------------------------
    // set XFRLEN1
    // set XWDLEN1
    //
    ASSERT(pDevice->pConfigInfo->TxFrameLength <= 128 &&
           pDevice->pConfigInfo->TxFrameLength > 0);
    pDevice->shadowRegs.XCR1 |=
        MCBSP_FRAME_LENGTH(pDevice->pConfigInfo->TxFrameLength)|
        MCBSP_WORD_LENGTH(pDevice->pConfigInfo->TxWordLength);

    // update register: XCR2
    //------------------------
    // NOTE:
    //      We are always ignoring Tx frame sync errors
    //
    // set XPHASE
    // set XFRLEN2
    // set XWDLEN2
    // set XREVERSE
    // set XDATDLY
    // set XREVERSE
    //
    // in dual phase transmit1 and transmit2 words per frame are the same

    pDevice->shadowRegs.XCR2 |= pDevice->pConfigInfo->TxPhase |
        MCBSP_FRAME_LENGTH(pDevice->pConfigInfo->TxFrameLength)|
        MCBSP_WORD_LENGTH(pDevice->pConfigInfo->TxWordLength2) |
        pDevice->pConfigInfo->TxReverse |
        pDevice->pConfigInfo->TxDataDelay;

    // update register: PCR
    //------------------------
    // set CLKXM
    // set FSXP
    // set CLKXP
    //
    pDevice->shadowRegs.PCR |= pDevice->pConfigInfo->TxClockSource |
        pDevice->pConfigInfo->TxFrameSyncPolarity |
        pDevice->pConfigInfo->TxClkPolarity;


    // set SCLKME
    //
    if (pDevice->pConfigInfo->SRGClkSrc == kMcBSP_SRG_SRC_CLKX_PIN ||
        pDevice->pConfigInfo->SRGClkSrc == kMcBSP_SRG_SRC_CLKR_PIN)
        {
        // Sample-rate generator source is either CLKXI(CLKSM=1) or
        // CLKRI(CLKSM=0)
        //
        pDevice->shadowRegs.PCR |= MCBSP_PCR_SCLKME;
        }
    else
        {
        pDevice->shadowRegs.PCR &= ~MCBSP_PCR_SCLKME;
        }

    // update register: PCR
    //------------------------
    // set FSXM
    //
    // update register: SRGR2
    //------------------------
    // set FSGM
    //
    switch (pDevice->pConfigInfo->TxFrameSyncSource)
        {
        case kMcBSP_Tx_FS_SRC_SRG:
            pDevice->shadowRegs.SRGR2 |= MCBSP_SRGR2_FSGM;

            // fall-through

        case kMcBSP_Tx_FS_SRC_DSR_XSR_COPY:
            pDevice->shadowRegs.PCR |= MCBSP_PCR_FSXM;
        }


    // update register: SPCR2
    //------------------------
    // NOTE:
    //      Only set XINTM when in DMA mode
    //
    // set XINTM
    //
    pDevice->shadowRegs.SPCR2 |= pDevice->pConfigInfo->TxSyncError;

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:-%S\r\n", __FUNCTION__));
}



//------------------------------------------------------------------------------
//
// mcbsp_ConfigureReceiver
//
void
mcbsp_ConfigureReceiver(
    McBSPDevice_t* pDevice
    )
{
    DEBUGMSG(ZONE_FUNCTION, (L"MCP:+%S(0x%08x)\r\n", __FUNCTION__, pDevice));

    // update register: SPCR1
    //------------------------
    // set RJUST
    //
    pDevice->shadowRegs.SPCR1 |= pDevice->pConfigInfo->JustificationMode;

    // update register: RCR1
    //------------------------
    // set RFRLEN1
    // set RWDLEN1
    //
    ASSERT(pDevice->pConfigInfo->RxFrameLength <= 128 &&
           pDevice->pConfigInfo->RxFrameLength > 0);

    pDevice->shadowRegs.RCR1 |=
        MCBSP_FRAME_LENGTH(pDevice->pConfigInfo->RxFrameLength)|
        MCBSP_WORD_LENGTH(pDevice->pConfigInfo->RxWordLength);

    // update register: RCR2
    //------------------------
    // NOTE:
    //      We are always ignoring Rx frame sync errors
    //
    // set RPHASE
    // set RFRLEN2
    // set RWDLEN2
    // set RREVERSE
    // set RDATDLY
    // set RREVERSE
    //
    // in dual phase receive1 and receive2 words per frame are the same

    pDevice->shadowRegs.RCR2 |= pDevice->pConfigInfo->RxPhase |
        MCBSP_FRAME_LENGTH(pDevice->pConfigInfo->RxFrameLength)|
        MCBSP_WORD_LENGTH(pDevice->pConfigInfo->RxWordLength2) |
        pDevice->pConfigInfo->RxReverse |
        pDevice->pConfigInfo->RxDataDelay;

    // update register: PCR
    //-----------------------
    // set FSRM
    // set FSRP
    // set CLKRP
    // set CLKRM
    //
    pDevice->shadowRegs.PCR |= pDevice->pConfigInfo->RxFrameSyncSource |
        pDevice->pConfigInfo->RxFrameSyncPolarity |
        pDevice->pConfigInfo->RxClkPolarity |
        pDevice->pConfigInfo->RxClockSource;

    // update register: SPCR2
    //------------------------
    // NOTE:
    //      Only set RINTM when in DMA mode
    //
    // set RINTM
    //
    pDevice->shadowRegs.SPCR1 |= pDevice->pConfigInfo->RxSyncError;

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:-%S\r\n", __FUNCTION__));
}


//------------------------------------------------------------------------------
//
// mcbsp_ClearIRQStatus
//
void
mcbsp_ClearIRQStatus(
    McBSPDevice_t* pDevice
    )
{
    DEBUGMSG(ZONE_FUNCTION, (L"MCP:+%S(0x%08x)\r\n", __FUNCTION__, pDevice));

    // Clear the IRQ status
    //
    OUTREG32(&pDevice->pMcbspRegs->IRQSTATUS, 0xFFFF);

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:-%S\r\n", __FUNCTION__));
}

//------------------------------------------------------------------------------
//
// mcbsp_UpdateRegisters
//
void
mcbsp_UpdateRegisters(
    McBSPDevice_t* pDevice
    )
{
    DEBUGMSG(ZONE_FUNCTION, (L"MCP:+%S(0x%08x)\r\n", __FUNCTION__, pDevice));

    OUTREG32(&pDevice->pMcbspRegs->SRGR1, pDevice->shadowRegs.SRGR1);
    OUTREG32(&pDevice->pMcbspRegs->SRGR2, pDevice->shadowRegs.SRGR2);
    OUTREG32(&pDevice->pMcbspRegs->PCR, pDevice->shadowRegs.PCR);
    OUTREG32(&pDevice->pMcbspRegs->XCR1, pDevice->shadowRegs.XCR1);
    OUTREG32(&pDevice->pMcbspRegs->XCR2, pDevice->shadowRegs.XCR2);
    OUTREG32(&pDevice->pMcbspRegs->RCR1, pDevice->shadowRegs.RCR1);
    OUTREG32(&pDevice->pMcbspRegs->RCR2, pDevice->shadowRegs.RCR2);
    OUTREG32(&pDevice->pMcbspRegs->SPCR2, pDevice->shadowRegs.SPCR2);
    OUTREG32(&pDevice->pMcbspRegs->SPCR1, pDevice->shadowRegs.SPCR1);

    //McBSP RX Threshold settings for FIFO
    //
    pDevice->shadowRegs.THRSH1 = pDevice->fifoThresholdRx;
    OUTREG32(&pDevice->pMcbspRegs->THRSH1, pDevice->shadowRegs.THRSH1);

    //McBSP TX Threshold settings for FIFO
    //
    pDevice->shadowRegs.THRSH2 = pDevice->fifoThresholdTx;
    OUTREG32(&pDevice->pMcbspRegs->THRSH2, pDevice->shadowRegs.THRSH2);

    // Set McBSP in smart Idle mode and enable mcbsp wakeup
    // Also the clockactivity to be set to 0 ( McBSP2_ICLK clock can be switched
    // off and PRCM functional clock can be switched off
    //
    pDevice->shadowRegs.SYSCONFIG = MCBSP_SYSCONFIG_SMARTIDLE |
                                    MCBSP_SYSCONFIG_ENAWAKEUP |
                                    MCBSP_SYSCONFIG_CLOCKACTIVITY(0);
    OUTREG32(&pDevice->pMcbspRegs->SYSCONFIG, pDevice->shadowRegs.SYSCONFIG);

    // Clocks in McBSP are shutoff when both IDLE_EN=1 and its power domain is
    // in idle mode(Force idle or Smart idle)
    pDevice->shadowRegs.PCR |= MCBSP_PCR_IDLE_EN;
    OUTREG32(&pDevice->pMcbspRegs->PCR, pDevice->shadowRegs.PCR);

    // Set McBSP wake up enable register
    //
    pDevice->shadowRegs.WAKEUPEN = MCBSP_WAKEUPEN_XEMPTYEOFEN |
                                   MCBSP_WAKEUPEN_XRDYEN |
                                   MCBSP_WAKEUPEN_RRDYEN;
    OUTREG32(&pDevice->pMcbspRegs->WAKEUPEN, pDevice->shadowRegs.WAKEUPEN);

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:-%S\r\n", __FUNCTION__));
}

//------------------------------------------------------------------------------
//
// mcbsp_ResetSampleRateGenerator
//
void
mcbsp_ResetSampleRateGenerator(
    McBSPDevice_t* pDevice
    )
{
    DEBUGMSG(ZONE_FUNCTION, (L"MCP:+%S(0x%08x)\r\n", __FUNCTION__, pDevice));

    pDevice->shadowRegs.SPCR2 &=
        ~(MCBSP_SPCR2_GRST_RSTCLR | MCBSP_SPCR2_FRST_RSTCLR);
    OUTREG32(&pDevice->pMcbspRegs->SPCR2, pDevice->shadowRegs.SPCR2);

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:-%S\r\n", __FUNCTION__));
}

//------------------------------------------------------------------------------
//
// mcbsp_EnableSampleRateGenerator
//
void
mcbsp_EnableSampleRateGenerator(
    McBSPDevice_t* pDevice
    )
{
    DEBUGMSG(ZONE_FUNCTION, (L"MCP:+%S(0x%08x)\r\n", __FUNCTION__, pDevice));

    pDevice->shadowRegs.SPCR2 |=
        MCBSP_SPCR2_GRST_RSTCLR | MCBSP_SPCR2_FRST_RSTCLR;
    OUTREG32(&pDevice->pMcbspRegs->SPCR2, pDevice->shadowRegs.SPCR2);

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:-%S\r\n", __FUNCTION__));
}


//------------------------------------------------------------------------------
//
// mcbsp_ResetTransmitter
//
void
mcbsp_ResetTransmitter(
    McBSPDevice_t* pDevice
    )
{
    DEBUGMSG(ZONE_FUNCTION, (L"MCP:+%S(0x%08x)\r\n", __FUNCTION__, pDevice));

    pDevice->shadowRegs.SPCR2 &= ~MCBSP_SPCR2_XRST_RSTCLR;
    OUTREG32(&pDevice->pMcbspRegs->SPCR2, pDevice->shadowRegs.SPCR2);

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:-%S\r\n", __FUNCTION__));
}


//------------------------------------------------------------------------------
//
// mcbsp_EnableTransmitter
//
void
mcbsp_EnableTransmitter(
    McBSPDevice_t* pDevice
    )
{
    DEBUGMSG(ZONE_FUNCTION, (L"MCP:+%S(0x%08x)\r\n", __FUNCTION__, pDevice));

    pDevice->shadowRegs.SPCR2 |= MCBSP_SPCR2_XRST_RSTCLR;
    OUTREG32(&pDevice->pMcbspRegs->SPCR2, pDevice->shadowRegs.SPCR2);

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:-%S\r\n", __FUNCTION__));
}


//------------------------------------------------------------------------------
//
// mcbsp_ResetReceiver
//
void
mcbsp_ResetReceiver(
    McBSPDevice_t* pDevice
    )
{
    DEBUGMSG(ZONE_FUNCTION, (L"MCP:+%S(0x%08x)\r\n", __FUNCTION__, pDevice));

    pDevice->shadowRegs.SPCR1 &= ~MCBSP_SPCR1_RRST_RSTCLR;
    OUTREG32(&pDevice->pMcbspRegs->SPCR1, pDevice->shadowRegs.SPCR1);

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:-%S\r\n", __FUNCTION__));
}


//------------------------------------------------------------------------------
//
// mcbsp_EnableReceiver
//
void
mcbsp_EnableReceiver(
    McBSPDevice_t* pDevice
    )
{
    DEBUGMSG(ZONE_FUNCTION, (L"MCP:+%S(0x%08x)\r\n", __FUNCTION__, pDevice));

    pDevice->shadowRegs.SPCR1 |= MCBSP_SPCR1_RRST_RSTCLR;
    OUTREG32(&pDevice->pMcbspRegs->SPCR1, pDevice->shadowRegs.SPCR1);

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:-%S\r\n", __FUNCTION__));
}

//------------------------------------------------------------------------------
//
// mcbsp_ConfigI2SProfile
//
void
mcbsp_ConfigI2SProfile(
    McBSPDevice_t* pDevice
    )
{
    DEBUGMSG(ZONE_FUNCTION, (L"MCP:+%S(0x%08x)\r\n", __FUNCTION__, pDevice));

    pDevice->shadowRegs.MCR1 = 0;
    pDevice->shadowRegs.MCR2 = 0;
    pDevice->shadowRegs.RCERA = 0;
    pDevice->shadowRegs.XCERA = 0;

    OUTREG32(&pDevice->pMcbspRegs->MCR1, pDevice->shadowRegs.MCR1);
    OUTREG32(&pDevice->pMcbspRegs->MCR2, pDevice->shadowRegs.MCR1);
    OUTREG32(&pDevice->pMcbspRegs->RCERA, pDevice->shadowRegs.RCERA);
    OUTREG32(&pDevice->pMcbspRegs->XCERA, pDevice->shadowRegs.XCERA);

    pDevice->shadowRegs.XCR1 |= MCBSP_FRAME_LENGTH(pDevice->pConfigInfo->TxFrameLength)|
                               MCBSP_WORD_LENGTH(pDevice->pConfigInfo->TxWordLength);
    OUTREG32(&pDevice->pMcbspRegs->XCR1, pDevice->shadowRegs.XCR1);

    pDevice->shadowRegs.RCR1 |= MCBSP_FRAME_LENGTH(pDevice->pConfigInfo->RxFrameLength)|
                               MCBSP_WORD_LENGTH(pDevice->pConfigInfo->RxWordLength);
    OUTREG32(&pDevice->pMcbspRegs->RCR1, pDevice->shadowRegs.RCR1);

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:-%S\r\n", __FUNCTION__));
}

//------------------------------------------------------------------------------
//
// mcbsp_ConfigTDMProfile
//
void
mcbsp_ConfigTDMProfile(
    McBSPDevice_t* pDevice
    )
{
    DWORD regVal = 0;

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:+%S(0x%08x)\r\n", __FUNCTION__, pDevice));

    // Configure McBSP phase TDM profile should be configured in single phase
    // only, reason refer TRM McBSP TDM mode section.
    //
    regVal = INREG32(&pDevice->pMcbspRegs->XCR2);
    regVal |= MCBSP_PHASE_SINGLE;
    OUTREG32(&pDevice->pMcbspRegs->XCR2, regVal);

    regVal = INREG32(&pDevice->pMcbspRegs->RCR2);
    regVal |= MCBSP_PHASE_SINGLE;
    OUTREG32(&pDevice->pMcbspRegs->RCR2, regVal);

    // Configure frame length and word length
    //
    regVal = (MCBSP_FRAME_LENGTH (pDevice->tdmWordsPerFrame) |
        MCBSP_WORD_LENGTH (kMcBSP_Word_Length_16));
    OUTREG32(&pDevice->pMcbspRegs->XCR1, regVal);

    regVal = (MCBSP_FRAME_LENGTH (pDevice->tdmWordsPerFrame) |
        MCBSP_WORD_LENGTH (kMcBSP_Word_Length_16));
    OUTREG32(&pDevice->pMcbspRegs->RCR1, regVal);

    // Configure partition mode, multichannel selection
    //
    if ( pDevice->partitionMode == kMcBSP_2PartitionMode)
        {
        // Rx partition mode and multichannel config
        //
        regVal = INREG32(&pDevice->pMcbspRegs->MCR1);

        regVal &= ~(MCBSP_PARTITION_MODE);
        regVal |= (MCBSP_PARTITION_A_BLOCK(0) |
            MCBSP_PARTITION_B_BLOCK(0) |
            MCBSP_MCR1_RMCM_RX);

        OUTREG32(&pDevice->pMcbspRegs->MCR1, regVal);

        // Tx partition mode and multichannel config
        //
        regVal = INREG32(&pDevice->pMcbspRegs->MCR2);

        regVal &= ~(MCBSP_PARTITION_MODE);
        regVal |= (MCBSP_PARTITION_A_BLOCK(0) |
            MCBSP_PARTITION_B_BLOCK(0) |
            MCBSP_MCR2_RMCM_TX(1));

        OUTREG32(&pDevice->pMcbspRegs->MCR2, regVal);
        }
    else if (pDevice->partitionMode == kMcBSP_8PartitionMode)
        {
        // Rx partition mode and multichannel config
        //
        regVal = INREG32(&pDevice->pMcbspRegs->MCR1);

        regVal |= (MCBSP_PARTITION_MODE);
        regVal |= (MCBSP_PARTITION_A_BLOCK(0) |
            MCBSP_PARTITION_B_BLOCK(0) |
            MCBSP_MCR1_RMCM_RX);

        OUTREG32(&pDevice->pMcbspRegs->MCR1, regVal);

        // Tx partition mode and multichannel config
        //
        regVal = INREG32(&pDevice->pMcbspRegs->MCR2);

        regVal |= (MCBSP_PARTITION_MODE);
        regVal |= (MCBSP_PARTITION_A_BLOCK(0) |
            MCBSP_PARTITION_B_BLOCK(0) |
            MCBSP_MCR2_RMCM_TX(1));

        OUTREG32(&pDevice->pMcbspRegs->MCR2, regVal);
        }
    else
        DEBUGMSG(ZONE_ERROR, (L"MCP: ERROR: mcbsp_ConfigTDMProfile: "
                L"Partition Mode unknown\r\n"
                ));

    // Enable the requested channels for Tx/Rx
    //
    mcbsp_ConfigTDMTxChannels(pDevice);
    mcbsp_ConfigTDMRxChannels(pDevice);

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:-%S\r\n", __FUNCTION__));
}

//------------------------------------------------------------------------------
//
// mcbsp_ConfigTDMTxChannels
//
void
mcbsp_ConfigTDMTxChannels(
    McBSPDevice_t* pDevice
    )
{
    DWORD channelToEnable = 0;
    UINT  nCount = 0;
    DWORD regVal = 0;

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:+%S(0x%08x)\r\n", __FUNCTION__, pDevice));

    // Channel selection among the available 128 channels
    // which is distributed equally among 8 blocks.
    //
    for (nCount= 0; nCount < pDevice->numOfTxChannels; nCount++)
        {
        switch ((pDevice->requestedTxChannels[nCount]) / MAX_CHANNEL_PER_BLOCK)
            {
            case kMcBSP_Block0:
                channelToEnable = pDevice->requestedTxChannels[nCount] %
                    (kMcBSP_Block0 + MAX_CHANNEL_PER_BLOCK);

                regVal = INREG32(&pDevice->pMcbspRegs->XCERA);
                regVal |= (1<<channelToEnable);
                OUTREG32(&pDevice->pMcbspRegs->XCERA, regVal);
                break;

            case kMcBSP_Block1:
                channelToEnable = pDevice->requestedTxChannels[nCount] %
                    (kMcBSP_Block1 + MAX_CHANNEL_PER_BLOCK);

                regVal = INREG32(&pDevice->pMcbspRegs->XCERB);
                regVal |= (1<<channelToEnable);
                OUTREG32(&pDevice->pMcbspRegs->XCERB, regVal);
                break;

            case kMcBSP_Block2:
                channelToEnable = pDevice->requestedTxChannels[nCount] %
                    (kMcBSP_Block2 + MAX_CHANNEL_PER_BLOCK);

                regVal = INREG32(&pDevice->pMcbspRegs->XCERC);
                regVal |= (1<<channelToEnable);
                OUTREG32(&pDevice->pMcbspRegs->XCERC, regVal);
                break;

            case kMcBSP_Block3:
                channelToEnable = pDevice->requestedTxChannels[nCount] %
                    (kMcBSP_Block3 + MAX_CHANNEL_PER_BLOCK);

                regVal = INREG32(&pDevice->pMcbspRegs->XCERD);
                regVal |= (1<<channelToEnable);
                OUTREG32(&pDevice->pMcbspRegs->XCERD, regVal);
                break;

            case kMcBSP_Block4:
                channelToEnable = pDevice->requestedTxChannels[nCount] %
                    (kMcBSP_Block4 + MAX_CHANNEL_PER_BLOCK);

                regVal = INREG32(&pDevice->pMcbspRegs->XCERE);
                regVal |= (1<<channelToEnable);
                OUTREG32(&pDevice->pMcbspRegs->XCERE, regVal);
                break;

            case kMcBSP_Block5:
                channelToEnable = pDevice->requestedTxChannels[nCount] %
                    (kMcBSP_Block5 + MAX_CHANNEL_PER_BLOCK);

                regVal = INREG32(&pDevice->pMcbspRegs->XCERF);
                regVal |= (1<<channelToEnable);
                OUTREG32(&pDevice->pMcbspRegs->XCERF, regVal);
                break;

            case kMcBSP_Block6:
                channelToEnable = pDevice->requestedTxChannels[nCount] %
                    (kMcBSP_Block6 + MAX_CHANNEL_PER_BLOCK);

                regVal = INREG32(&pDevice->pMcbspRegs->XCERG);
                regVal |= (1<<channelToEnable);
                OUTREG32(&pDevice->pMcbspRegs->XCERG, regVal);
                break;

            case kMcBSP_Block7:
                channelToEnable = pDevice->requestedTxChannels[nCount] %
                    (kMcBSP_Block7 + MAX_CHANNEL_PER_BLOCK);

                regVal = INREG32(&pDevice->pMcbspRegs->XCERH);
                regVal |= (1<<channelToEnable);
                OUTREG32(&pDevice->pMcbspRegs->XCERH, regVal);
                break;

            default:
                DEBUGMSG(ZONE_ERROR, (L"MCP: ERROR: mcbsp_ConfigTDMTxChannels: "
                    L"Invalid Channel Request\r\n"
                    ));
                break;
            }
        }
    DEBUGMSG(ZONE_FUNCTION, (L"MCP:-%S\r\n", __FUNCTION__));
}

//------------------------------------------------------------------------------
//
// mcbsp_ConfigTDMRxChannels
//
void
mcbsp_ConfigTDMRxChannels(
    McBSPDevice_t* pDevice
    )
{
    DWORD channelToEnable = 0;
    UINT  nCount = 0;
    DWORD regVal = 0;

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:+%S(0x%08x)\r\n", __FUNCTION__, pDevice));

    // Channel selection among the available 128 channels
    // which is distributed equally among 8 blocks.
    //
    for (nCount= 0; nCount < pDevice->numOfRxChannels; nCount++)
        {
        switch ((pDevice->requestedRxChannels[nCount]) / MAX_CHANNEL_PER_BLOCK)
            {
            case kMcBSP_Block0:
                channelToEnable = pDevice->requestedRxChannels[nCount] %
                    (kMcBSP_Block0 + MAX_CHANNEL_PER_BLOCK);

                regVal = INREG32(&pDevice->pMcbspRegs->RCERA);
                regVal |= (1<<channelToEnable);
                OUTREG32(&pDevice->pMcbspRegs->RCERA, regVal);
                break;

            case kMcBSP_Block1:
                channelToEnable = pDevice->requestedRxChannels[nCount] %
                    (kMcBSP_Block1 + MAX_CHANNEL_PER_BLOCK);

                regVal = INREG32(&pDevice->pMcbspRegs->RCERB);
                regVal |= (1<<channelToEnable);
                OUTREG32(&pDevice->pMcbspRegs->RCERB, regVal);
                break;

            case kMcBSP_Block2:
                channelToEnable = pDevice->requestedRxChannels[nCount] %
                    (kMcBSP_Block2 + MAX_CHANNEL_PER_BLOCK);

                regVal = INREG32(&pDevice->pMcbspRegs->RCERC);
                regVal |= (1<<channelToEnable);
                OUTREG32(&pDevice->pMcbspRegs->RCERC, regVal);
                break;

            case kMcBSP_Block3:
                channelToEnable = pDevice->requestedRxChannels[nCount] %
                    (kMcBSP_Block3 + MAX_CHANNEL_PER_BLOCK);

                regVal = INREG32(&pDevice->pMcbspRegs->RCERD);
                regVal |= (1<<channelToEnable);
                OUTREG32(&pDevice->pMcbspRegs->RCERD, regVal);
                break;

            case kMcBSP_Block4:
                channelToEnable = pDevice->requestedRxChannels[nCount] %
                    (kMcBSP_Block4 + MAX_CHANNEL_PER_BLOCK);

                regVal = INREG32(&pDevice->pMcbspRegs->RCERE);
                regVal |= (1<<channelToEnable);
                OUTREG32(&pDevice->pMcbspRegs->RCERE, regVal);
                break;

            case kMcBSP_Block5:
                channelToEnable = pDevice->requestedRxChannels[nCount] %
                    (kMcBSP_Block5 + MAX_CHANNEL_PER_BLOCK);

                regVal = INREG32(&pDevice->pMcbspRegs->RCERF);
                regVal |= (1<<channelToEnable);
                OUTREG32(&pDevice->pMcbspRegs->RCERF, regVal);
                break;

            case kMcBSP_Block6:
                channelToEnable = pDevice->requestedRxChannels[nCount] %
                    (kMcBSP_Block6 + MAX_CHANNEL_PER_BLOCK);

                regVal = INREG32(&pDevice->pMcbspRegs->RCERG);
                regVal |= (1<<channelToEnable);
                OUTREG32(&pDevice->pMcbspRegs->RCERG, regVal);
                break;

            case kMcBSP_Block7:
                channelToEnable = pDevice->requestedRxChannels[nCount] %
                    (kMcBSP_Block7 + MAX_CHANNEL_PER_BLOCK);

                regVal = INREG32(&pDevice->pMcbspRegs->RCERH);
                regVal |= (1<<channelToEnable);
                OUTREG32(&pDevice->pMcbspRegs->RCERH, regVal);
                break;

            default:
                DEBUGMSG(ZONE_ERROR, (L"MCP: ERROR:mcbsp_ConfigTDMRxChannels: "
                    L"Invalid Channel Request\r\n"
                    ));
                break;
            }
        }
    DEBUGMSG(ZONE_FUNCTION, (L"MCP:-%S\r\n", __FUNCTION__));
}


//------------------------------------------------------------------------------
//
// mcbsp_SideToneInit
//
void
mcbsp_SideToneInit(
    McBSPDevice_t* pDevice
    )
{
    DWORD regVal = 0;
    PHYSICAL_ADDRESS pa;
    OMAP35XX_MCBSP_REGS_ST_t *pSideToneRegs = NULL;

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:+%S(0x%08x)\r\n", __FUNCTION__, pDevice));

    // McBSP1,4,5 do not have sidetone support, so the physical address will be NULL
	pDevice->pPhysAddrSidetone = (OMAP35XX_MCBSP_REGS_t*)SOCGetMCBSPSidetoneAddress(pDevice->deviceID);
    if (pDevice->pPhysAddrSidetone == NULL)
    {
        goto cleanUp;
    }

    // Map BSP controller
    //
    pa.QuadPart = (LONGLONG)pDevice->pPhysAddrSidetone;
    pSideToneRegs = (OMAP35XX_MCBSP_REGS_ST_t*)
        MmMapIoSpace(pa, sizeof(OMAP35XX_MCBSP_REGS_ST_t), FALSE);
    if (pSideToneRegs == NULL)
        {
        DEBUGMSG(ZONE_ERROR, (L"MCP: ERROR: mcbsp_SideToneInit: "
            L"Failed to map Side Tone controller registers\r\n"
            ));
        goto cleanUp;
        }

    pDevice->pSideToneRegs = pSideToneRegs;

    // Enable Side tone
    //
    mcbsp_SideToneEnable(pDevice);

    // Start writing FIR coeff.
    //
    mcbsp_SideToneWriteFIRCoeff(pDevice);

    // Apply desired gain value for chan 0 and chan 1
    //
    regVal = (ST_SGAINCR_CH0GAIN(pDevice->sideToneGain) |
              ST_SGAINCR_CH1GAIN(pDevice->sideToneGain));

    OUTREG32(&pDevice->pSideToneRegs->SGAINCR, regVal);

    // Enable auto idle for sidetone.
    //
    mcbsp_SideToneAutoIdle(pDevice);

cleanUp:
    DEBUGMSG(ZONE_FUNCTION, (L"MCP:-%S\r\n", __FUNCTION__));
}

//------------------------------------------------------------------------------
//
// mcbsp_SideToneEnable
//
void
mcbsp_SideToneEnable(
    McBSPDevice_t* pDevice
    )
{
    DWORD regVal = 0;

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:+%S(0x%08x)\r\n", __FUNCTION__, pDevice));

    // Mapping the McBSP channels
    //
    pDevice->sideToneTxMapChannel0 = INREG32(&pDevice->pMcbspRegs->XCERA);
    pDevice->sideToneRxMapChannel0 = INREG32(&pDevice->pMcbspRegs->RCERA);

    pDevice->sideToneTxMapChannel1 = 0;
    pDevice->sideToneRxMapChannel1 = 0;

    // Enable side tone from mcbsp register, map mcbsp input and output
    // channels
    //
    regVal = (MCBSP_SSELCR_SIDETONEEN |
        MCBSP_SSELCR_OCH0ASSIGN(pDevice->sideToneTxMapChannel0) |
        MCBSP_SSELCR_OCH1ASSIGN(pDevice->sideToneTxMapChannel1) |
        MCBSP_SSELCR_ICH0ASSIGN(pDevice->sideToneRxMapChannel0) |
        MCBSP_SSELCR_ICH1ASSIGN(pDevice->sideToneRxMapChannel1));

    OUTREG32(&pDevice->pMcbspRegs->SSELCR, regVal);

    // Enable side tone in side tone register.
    //
    regVal = INREG32(&pDevice->pSideToneRegs->SSELCR);
    regVal |= ST_SSELCR_SIDETONEEN;
    OUTREG32(&pDevice->pSideToneRegs->SSELCR, regVal);

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:-%S\r\n", __FUNCTION__));
}

//------------------------------------------------------------------------------
//
// mcbsp_SideToneDisable
//
void
mcbsp_SideToneDisable(
    McBSPDevice_t* pDevice
    )
{
    DWORD regVal = 0;

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:+%S(0x%08x)\r\n", __FUNCTION__, pDevice));

    // API to disable side tone
    //
    OUTREG32(&pDevice->pMcbspRegs->SSELCR, regVal);
    regVal = 0;
    OUTREG32(&pDevice->pSideToneRegs->SSELCR, regVal);

    // Mute
    //
    regVal = 0;
    OUTREG32(&pDevice->pSideToneRegs->SGAINCR, regVal);

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:-%S\r\n", __FUNCTION__));
}

//------------------------------------------------------------------------------
//
// mcbsp_SideToneWriteFIRCoeff
//
void
mcbsp_SideToneWriteFIRCoeff(
    McBSPDevice_t* pDevice
    )
{
    DWORD nCount = 0;

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:+%S(0x%08x)\r\n", __FUNCTION__, pDevice));

    // Disable side tone before writing FIR coefficients reason refer TRM
    //
    mcbsp_SideToneDisable(pDevice);

    // write reset before FIR coeff is written
    //
    mcbsp_SideToneWriteReset(pDevice);

    // Write FIR coefficients
    //
    while ((INREG32(&pDevice->pSideToneRegs->SSELCR) &
        ST_SSELCR_COEFFWRDONE) == 0)
        {
        OUTREG32(&pDevice->pSideToneRegs->SFIRCR,
            ST_SFIRCR_FIRCOEFF(pDevice->sideToneFIRCoeffWrite[++nCount]));
        };

    // After FIR coeff write enable the side tone.
    //
    mcbsp_SideToneEnable(pDevice);

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:-%S\r\n", __FUNCTION__));
}

//------------------------------------------------------------------------------
//
// mcbsp_SideToneReadFIRCoeff
//
void
mcbsp_SideToneReadFIRCoeff(
    McBSPDevice_t* pDevice
    )
{
    DWORD nCount = 0;

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:+%S(0x%08x)\r\n", __FUNCTION__, pDevice));

    // Disable side tone before reading FIR coefficients reason refer TRM
    //
    mcbsp_SideToneDisable(pDevice);

    // Read reset
    //
    mcbsp_SideToneReadReset(pDevice);

    // Read FIR coefficients
    //
    while ((INREG32(&pDevice->pSideToneRegs->SSELCR) &
        ST_SSELCR_COEFFWRDONE) == 0)
        {
        pDevice->sideToneFIRCoeffRead[++nCount] =
            INREG32(&pDevice->pSideToneRegs->SFIRCR);
        };

    // After FIR coeff write enable the side tone.
    //
    mcbsp_SideToneEnable(pDevice);

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:-%S\r\n", __FUNCTION__));
}

//------------------------------------------------------------------------------
//
// mcbsp_SideToneWriteReset
//
void
mcbsp_SideToneWriteReset(
    McBSPDevice_t* pDevice
    )
{
    DWORD regVal = 0;

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:+%S(0x%08x)\r\n", __FUNCTION__, pDevice));

    // Reset Coeff write done bit which was set for previous write
    //
    regVal = INREG32(&pDevice->pSideToneRegs->SSELCR);
    regVal &= ~(ST_SSELCR_COEFFWRDONE);
    OUTREG32(&pDevice->pSideToneRegs->SSELCR, regVal);

    // API to Reset Coeff register for write process
    //
    regVal = INREG32(&pDevice->pSideToneRegs->SSELCR);
    regVal &= ~(ST_SSELCR_COEFFWREN);
    OUTREG32(&pDevice->pSideToneRegs->SSELCR, regVal);

    regVal = INREG32(&pDevice->pSideToneRegs->SSELCR);
    regVal |= ST_SSELCR_COEFFWREN;
    OUTREG32(&pDevice->pSideToneRegs->SSELCR, regVal);

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:-%S\r\n", __FUNCTION__));
}

//------------------------------------------------------------------------------
//
// mcbsp_SideToneReadReset
//
void
mcbsp_SideToneReadReset(
    McBSPDevice_t* pDevice
    )
{
    DWORD regVal = 0;

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:+%S(0x%08x)\r\n", __FUNCTION__, pDevice));

    // Reset Coeff write done bit which was set for previous read
    //
    regVal = INREG32(&pDevice->pSideToneRegs->SSELCR);
    regVal &= ~(ST_SSELCR_COEFFWRDONE);
    OUTREG32(&pDevice->pSideToneRegs->SSELCR, regVal);

    // API to Reset Coeff register for read process
    //
    regVal = INREG32(&pDevice->pSideToneRegs->SSELCR);
    regVal |= ST_SSELCR_COEFFWREN;
    OUTREG32(&pDevice->pSideToneRegs->SSELCR, regVal);

    regVal = INREG32(&pDevice->pSideToneRegs->SSELCR);
    regVal &= ~(ST_SSELCR_COEFFWREN);
    OUTREG32(&pDevice->pSideToneRegs->SSELCR, regVal);

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:-%S\r\n", __FUNCTION__));
}

//------------------------------------------------------------------------------
//
// mcbsp_SideToneAutoIdle
//
void
mcbsp_SideToneAutoIdle(
    McBSPDevice_t* pDevice
    )
{
    DEBUGMSG(ZONE_FUNCTION, (L"MCP:+%S(0x%08x)\r\n", __FUNCTION__, pDevice));

    // API for sidetone to enable/disable auto idle mode
    //
    OUTREG32(&pDevice->pSideToneRegs->SYSCONFIG, ST_SYSCONFIG_AUTOIDLE);

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:-%S\r\n", __FUNCTION__));
}

//------------------------------------------------------------------------------
//
// mcbsp_SideToneInterruptEnable
//
void
mcbsp_SideToneInterruptEnable(
    McBSPDevice_t* pDevice
    )
{
    DEBUGMSG(ZONE_FUNCTION, (L"MCP:+%S(0x%08x)\r\n", __FUNCTION__, pDevice));

    // API to enable sidetone interrupt
    //
    OUTREG32(&pDevice->pSideToneRegs->IRQENABLE, ST_IRQENABLE_OVRRERROREN);

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:-%S\r\n", __FUNCTION__));
}

//------------------------------------------------------------------------------
//
// mcbsp_SideToneInterruptStatus
//
void
mcbsp_SideToneInterruptStatus(
    McBSPDevice_t* pDevice
    )
{
    DEBUGMSG(ZONE_FUNCTION, (L"MCP:+%S(0x%08x)\r\n", __FUNCTION__, pDevice));

    // API to read interrupt status.
    //
    INREG32(&pDevice->pSideToneRegs->IRQSTATUS);

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:-%S\r\n", __FUNCTION__));
}

//------------------------------------------------------------------------------
//
// mcbsp_SideToneResetInterrupt
//
void
mcbsp_SideToneResetInterrupt(
    McBSPDevice_t* pDevice
    )
{
    DEBUGMSG(ZONE_FUNCTION, (L"MCP:+%S(0x%08x)\r\n", __FUNCTION__, pDevice));

    // API to reset IRQ Status register
    //
    OUTREG32(&pDevice->pSideToneRegs->IRQSTATUS, ST_IRQSTATUS_OVRRERROR);

    DEBUGMSG(ZONE_FUNCTION, (L"MCP:-%S\r\n", __FUNCTION__));
}


#ifndef SHIP_BUILD
//------------------------------------------------------------------------------
//
//
void
mcbsp_DumpReg(
    McBSPDevice_t* pDevice,
    WCHAR const* szMsg
    )
{
    OMAP35XX_MCBSP_REGS_t *pMcbspRegs = pDevice->pMcbspRegs;

    RETAILMSG(1,(szMsg));
    RETAILMSG(1,(TEXT("SPCR2    : %08x\r\n"), INREG32(&pMcbspRegs->SPCR2)));
    RETAILMSG(1,(TEXT("SPCR1    : %08x\r\n"), INREG32(&pMcbspRegs->SPCR1)));
    RETAILMSG(1,(TEXT("RCR2     : %08x\r\n"), INREG32(&pMcbspRegs->RCR2)));
    RETAILMSG(1,(TEXT("RCR1     : %08x\r\n"), INREG32(&pMcbspRegs->RCR1)));
    RETAILMSG(1,(TEXT("XCR2     : %08x\r\n"), INREG32(&pMcbspRegs->XCR2)));
    RETAILMSG(1,(TEXT("XCR1     : %08x\r\n"), INREG32(&pMcbspRegs->XCR1)));
    RETAILMSG(1,(TEXT("SRGR2    : %08x\r\n"), INREG32(&pMcbspRegs->SRGR2)));
    RETAILMSG(1,(TEXT("SRGR1    : %08x\r\n"), INREG32(&pMcbspRegs->SRGR1)));
    RETAILMSG(1,(TEXT("MCR2     : %08x\r\n"), INREG32(&pMcbspRegs->MCR2)));
    RETAILMSG(1,(TEXT("MCR1     : %08x\r\n"), INREG32(&pMcbspRegs->MCR1)));
    RETAILMSG(1,(TEXT("PCR      : %08x\r\n"), INREG32(&pMcbspRegs->PCR)));
    RETAILMSG(1,(TEXT("THRSH1   : %08x\r\n"), INREG32(&pMcbspRegs->THRSH1)));
    RETAILMSG(1,(TEXT("THRSH2   : %08x\r\n"), INREG32(&pMcbspRegs->THRSH2)));
    RETAILMSG(1,(TEXT("WAKEUPEN : %08x\r\n"), INREG32(&pMcbspRegs->WAKEUPEN)));
    RETAILMSG(1,(TEXT("SYSCONFIG : %08x\r\n"), INREG32(&pMcbspRegs->SYSCONFIG)));

}

#endif //!SHIP_BUILD

