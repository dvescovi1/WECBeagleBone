// Copyright (c) 2007, 2008 BSQUARE Corporation. All rights reserved.

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
//  File: SDHCFUNCS.H
//  SDHC controller driver public API

#ifndef _SDHCFUNCS_H_
#define _SDHCFUNCS_H_

#include "SDHC.h"
#include "SDHCRegs.h"

// Is the card present?
BOOL SdhcCardDetect();

// Initialize the card
VOID SdhcHandleInsertion();

VOID SdhcSetClockRate(PDWORD pdwRate);

VOID SdhcSetInterface(DWORD mode);

SD_API_STATUS SdhcControllerIstThread(PSD_BUS_REQUEST pRequest);

SD_API_STATUS SdhcInitialize();

SD_API_STATUS SdhcBusRequestHandler(PSD_BUS_REQUEST pRequest);

#endif // #ifndef _SDHCFUNCS_H_
