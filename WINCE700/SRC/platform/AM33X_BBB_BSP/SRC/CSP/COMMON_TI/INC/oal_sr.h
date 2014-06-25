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
//  File:  oal_sr.h
//
#ifndef _OAL_SR_H
#define _OAL_SR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "omap_sr_regs.h"

#pragma warning (push)
#pragma warning (disable:4201)


//-----------------------------------------------------------------------------
typedef enum {
    kSmartReflex_Channel1 = 0,
    kSmartReflex_Channel2,
    kSmartReflex_ChannelCount
}
SmartReflexChannel_e;

//-----------------------------------------------------------------------------
typedef struct {
            UINT        rnsenn:8;
            UINT        rnsenp:8;
            UINT        senngain:4;
            UINT        senpgain:4;
            UINT        zzzReserved4:8;
}Efuse_SensorData_field_t;

//-----------------------------------------------------------------------------
typedef struct {
    union {
        struct {
            UINT        Efuse_Vdd1_Opp_1;
            UINT        Efuse_Vdd1_Opp_2;
            UINT        Efuse_Vdd1_Opp_3;
            UINT        Efuse_Vdd1_Opp_4;
            UINT        Efuse_Vdd1_Opp_5;
            UINT        Efuse_Vdd2_Opp_1;
            UINT        Efuse_Vdd2_Opp_2;
            UINT        Efuse_Vdd2_Opp_3;
        };
	 struct {
            UINT Efuse_Vdd1_Opp[5];
            UINT Efuse_Vdd2_Opp[3];
	 };  
	 struct {
            Efuse_SensorData_field_t Vdd1_sendata[5];
            Efuse_SensorData_field_t Vdd2_sendata[3];
	 };
    };
}
Efuse_SensorData_t;

//-----------------------------------------------------------------------------
typedef struct {
    union {
        struct {
            UINT        sr1_sennenable:2;
            UINT        sr1_senpenable:2;
            UINT        zzzReserved9:4;
            UINT        sr2_sennenable:2;
            UINT        sr2_senpenable:2;
            UINT        zzzReserved10:20;
            };
        UINT            Efuse_SR;
        };
}
SmartReflexSenorEnData_t;

//-----------------------------------------------------------------------------
typedef struct {
    SmartReflexChannel_e    channel;
    UINT                    srClkLengthT;
    UINT                    rnsenp;
    UINT                    rnsenn;
    UINT                    senpgain;
    UINT                    senngain;
}
SmartReflexSensorData_t;

typedef struct {
    union {
        struct {
            UINT   errMinLimit:8;
            UINT   errMaxLimit:8;
            UINT   errWeight:3;        
            UINT   zzzReserved1:13;
            };
        UINT   errConfig;
    };
}
SmartreflexConfiguration_t;

//-----------------------------------------------------------------------------
//  SmartReflex_Functions_t
//      
//  SmartReflex API Function table definition
// 
typedef struct {
	
void (*InitializeChannel)(
    SmartReflexChannel_e    channel,
    OMAP_SMARTREFLEX       *pSr
    );

void (*SetAccumulationSize)(
    SmartReflexChannel_e    channel,
    UINT                    accumData
    );	

void (*SetSensorMode)(
    SmartReflexChannel_e    channel,
    UINT                    senNMode,
    UINT                    senPMode
    );

void (*SetIdleMode)(
    SmartReflexChannel_e    channel,
    UINT                    idleMode
    );

void (*SetSensorReferenceData)(
    SmartReflexChannel_e     channel,
    SmartReflexSensorData_t *pSensorData    
    );

void (*SetErrorControl)(
    SmartReflexChannel_e    channel,
    UINT                    errorWeight,
    UINT                    errorMax,
    UINT                    errorMin
    );

void (*EnableInterrupts)(
    SmartReflexChannel_e    channel,
    UINT                    mask,
    BOOL                    bEnable    
    );

void (*EnableErrorGeneratorBlock)(
    SmartReflexChannel_e    channel,
    BOOL                    bEnable
    );

void (*EnableMinMaxAvgBlock)(
    SmartReflexChannel_e    channel,
    BOOL                    bEnable
    );

void (*EnableSensorBlock)(
    SmartReflexChannel_e    channel,
    BOOL                    bEnable
    );

void (*Enable)(
    SmartReflexChannel_e    channel,
    BOOL                    bEnable
    );

UINT32 (*ClearInterruptStatus)(
    SmartReflexChannel_e    channel,
    UINT                    mask
    );

UINT32 (*GetStatus)(
    SmartReflexChannel_e    channel
    );

void (*SetAvgWeight)(
    SmartReflexChannel_e    channel,
    UINT                    senNWeight,
    UINT                    senPWeight
    );

void  (*dump_register)( 
    SmartReflexChannel_e channel
    );

}SmartReflex_Functions_t;
SmartReflex_Functions_t* 
SmartReflex_Get_functions_hander(
    DWORD Version
    );

#pragma warning (pop)

#ifdef __cplusplus
}
#endif

#endif  // _OAL_SR_H

