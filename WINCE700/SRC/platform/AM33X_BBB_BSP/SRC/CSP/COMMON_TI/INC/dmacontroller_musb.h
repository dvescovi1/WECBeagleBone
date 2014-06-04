// All rights reserved ADENEO EMBEDDED 2010
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
//------------------------------------------------------------------------------
//
//  File: dmacontroller_musb.h
//

#ifndef __DMACONTROLLER_MUSB_H
#define __DMACONTROLLER_MUSB_H

class OMAPMHSUSBOTG;

#pragma warning (push)
#pragma warning (disable: 4510 4512 4610 4245)
#include <csync.h>
#include <cmthread.h>
#pragma warning (pop)

class OMAPMHSUSBDMA : public CLockObject, public  CMiniThread {
public:
    OMAPMHSUSBDMA(OMAPMHSUSBOTG * pUsbOtg, DWORD dwIrq);
    ~OMAPMHSUSBDMA();
    virtual BOOL    Init() = 0;
    virtual DWORD   ThreadRun() = 0;
	virtual DWORD	ProcessInterrupts() = 0;
	virtual void	Start() = 0;
	virtual void	Stop() = 0;
	virtual BOOL	AllocateChannel(UCHAR endpoint, BOOL isTx, UCHAR* channel) = 0;
	virtual BOOL	ReleaseChannel(UCHAR channel, BOOL isTx) = 0;
	virtual BOOL	AbortChannel(UCHAR channel, BOOL isTx) = 0;
	virtual BOOL	ProgramChannel(UCHAR channel, 
								   BOOL isTx, 
								   DWORD dmaAddr, 
								   DWORD length, 
								   USHORT maxPacketSize,
								   BOOL isHost) = 0;
	virtual BOOL	UseDMAForRx() = 0;
	virtual BOOL	ProgramCSRFirst(BOOL isTx) = 0;
	virtual BOOL	AllowFullDuplex() = 0;
	virtual DWORD	GetChannelCopiedSize(UCHAR endpoint, BOOL isTx) = 0;
	virtual void	SetCriticalSection(LPCRITICAL_SECTION pCS) = 0;
protected:
    BOOL					m_fTerminated;
    OMAPMHSUSBOTG * const   m_pUsbOtg;
    HANDLE					m_hDMAIntrEvent;      // Interrupt Event due to DMA
    DWORD					m_dwDMASysIntr;       // SysInterrupt of DMA
    DWORD					m_dwDMAIrq;           // Interrupt for DMA
};

#endif // __DMACONTROLLER_MUSB_H

