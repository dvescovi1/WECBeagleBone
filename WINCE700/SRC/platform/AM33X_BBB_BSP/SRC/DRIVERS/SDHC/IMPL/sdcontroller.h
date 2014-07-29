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
//  File: sdcontroller.h
//
//

#ifndef __SDCONTROLLER_H
#define __SDCONTROLLER_H

#include "sdhc.h"
#include "am33x_base_regs.h"
#include "am33x_config.h"
#include "bsp_cfg.h"

class CSDIOController : public CSDIOControllerBase
{
public:
    CSDIOController();
    ~CSDIOController();
       
protected:
    BOOL IsWriteProtected();
    BOOL SDCardDetect();
    DWORD SDHCCardDetectIstThreadImpl();
    virtual BOOL InitializeHardware();
    virtual void DeinitializeHardware();
    virtual VOID TurnCardPowerOn();
    virtual VOID TurnCardPowerOff();
    virtual VOID PreparePowerChange(CEDEVICE_POWER_STATE curPowerState, BOOL bInPowerHandler);
    virtual VOID PostPowerChange(CEDEVICE_POWER_STATE curPowerState, BOOL bInPowerHandler);

    VOID    EnableSDHCCDInterrupts();
    VOID    DisableSDHCCDInterrupts();
    inline  void  Write_MMC_CD_STAT( DWORD wVal );

    BOOL InitializeInterrupt(void);
    
    BOOL CDIntrEnabled;
};

#endif // __SDCONTROLLER_H

