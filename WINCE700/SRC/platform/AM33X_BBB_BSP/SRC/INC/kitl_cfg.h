//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this sample source code is subject to the terms of the Microsoft
// license agreement under which you licensed this sample source code. If
// you did not accept the terms of the license agreement, you are not
// authorized to use this sample source code. For the terms of the license,
// please see the license agreement between you and Microsoft or, if applicable,
// see the LICENSE.RTF on your install media or the root of your tools installation.
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
// Copyright (c) 2007, 2008 BSQUARE Corporation. All rights reserved.

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
//------------------------------------------------------------------------------
//
//  File:  kitl_cfg.h
//
#ifndef __KITL_CFG_H
#define __KITL_CFG_H

#include "am33x_base_regs.h"

BOOL   Cpsw3gInit(UINT8 *pAddress, UINT32 offset, UINT16 mac[3]);
UINT16 Cpsw3gSendFrame(UINT8 *pBuffer, UINT32 length);
UINT16 Cpsw3gGetFrame(UINT8 *pBuffer, UINT16 *pLength);
VOID   Cpsw3gEnableInts();
VOID   Cpsw3gDisableInts();

VOID Cpsw3gCurrentPacketFilter(UINT32 filter);
BOOL Cpsw3gMulticastList(UINT8 *pAddresses, UINT32 count);


#define BSP_ETHDRV_AM33X   { \
    Cpsw3gInit, NULL, NULL, Cpsw3gSendFrame, Cpsw3gGetFrame, \
    Cpsw3gEnableInts, Cpsw3gDisableInts, \
    NULL, NULL,  Cpsw3gCurrentPacketFilter, Cpsw3gMulticastList  \
}

//------------------------------------------------------------------------------

#if 0
/* oal_kitl.h */
typedef struct {
    OAL_KITLETH_INIT					pfnInit;
    OAL_KITLETH_INIT_DMABUFFER			pfnInitDmaBuffer;
    OAL_KITLETH_DEINIT					pfnDeinit;
    OAL_KITLETH_SEND_FRAME				pfnSendFrame;
    OAL_KITLETH_GET_FRAME				pfnGetFrame;
    OAL_KITLETH_ENABLE_INTS				pfnEnableInts;
    OAL_KITLETH_DISABLE_INTS			pfnDisableInts;
    OAL_KITLETH_POWER_OFF				pfnPowerOff;
    OAL_KITLETH_POWER_ON				pfnPowerOn;
    OAL_KITLETH_CURRENT_PACKET_FILTER	pfnCurrentPacketFilter;
    OAL_KITLETH_MULTICAST_LIST			pfnMulticastList;
} OAL_KITL_ETH_DRIVER;
#endif

OAL_KITL_ETH_DRIVER g_kitlEthLan_AM33X = BSP_ETHDRV_AM33X;

OAL_KITL_DEVICE g_kitlDevices[] = {
    { 
		L"Internal EMAC", Internal, AM33X_EMACSW_REGS_PA, 
        0, OAL_KITL_TYPE_ETH, &g_kitlEthLan_AM33X
    }, {
        NULL, 0, 0, 0, 0, NULL
    }
};    

#endif
