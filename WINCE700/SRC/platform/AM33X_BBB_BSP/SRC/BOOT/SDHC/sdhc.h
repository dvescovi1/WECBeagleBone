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
//  File: SDHC.H
//
//  SDHC controller driver public API
//

#ifndef _SDHC_H
#define _SDHC_H

BOOL SdhcCardDetect();

VOID SendInitSequence();

VOID SdhcHandleInsertion();

VOID SdhcSetClockRate(PDWORD pdwRate);

VOID SdhcSetInterface(DWORD mode);

VOID SdhcSoftwareReset(DWORD dwResetBits);

SD_API_STATUS SdhcInitialize();

SD_API_STATUS SdhcBusRequestHandler(PSD_BUS_REQUEST pRequest);

SD_API_STATUS SdhcControllerIstThread(PSD_BUS_REQUEST pRequest);

#endif


