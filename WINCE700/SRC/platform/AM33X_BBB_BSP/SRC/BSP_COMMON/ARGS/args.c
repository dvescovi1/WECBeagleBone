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
//  File:  args.c
//
//  This file implements the OEMArgsQuery handler.
//

#include <bsp.h>
#include <bsp_def.h>

// TODO !!!!!!!!!!!!!!! update for ral board/device
//------------------------------------------------------------------------------
//  Static variables
//
//  These values must be generated uniquely for each WinMobile device.  On 35xx,
//  the eFuse key registers CONTROL_RPUB_KEY_H_x, CONTROL_RAND_KEY_x and 
//  CONTROL_CUST_KEY_x should be used to generate the UUID and HWEntropy values.
//  For EVM3530, these keys are all set to 0, so the Device ID set within the
//  EBOOT bootloader is used for unique identification of the EVM3530.
//  Another alternative is to use the one-time programmable area of NAND flash.
//
//  See the WinMobile 5.0 documentation on Hardware Device ID and Entropy

static UCHAR    s_deviceId[24];

//                         Manfacturer ID----  VarVer  Device ID------------------------------------- 
static GUID     s_uuid = { 0x00000000, 0x3024, 0x0801, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

static DWORD    s_dwHWEntropy[2] = { 0, 0 };

    // Note: Bit 1 (Locally Administered Address bit) in MAC address
    // must not be set. UsbFn driver uses returned address as MAC address
    // on desktop (XP, W2K) side of virtual RNDIS network. On device
    // side same address with set LAA bit is used.

static UCHAR    s_rndisMAC[6] = { 0x00, 0x24, 0x30, 0xAB, 0x12, 0x34 };


static  BOOL    s_bInitialized = FALSE;


//------------------------------------------------------------------------------
//
//  Function:  OEMArgsQuery
//
//  This function is called from other OAL modules to return boot arguments.
//  Boot arguments are typically placed in fixed memory location and they are
//  filled by boot loader. In case that boot arguments can't be located
//  the function should return NULL. The OAL module then must use
//  default values.
//
VOID* OALArgsQuery(UINT32 type)
{
    VOID *pData = NULL;
    BSP_ARGS *pArgs;

    OALMSG(OAL_ARGS&&OAL_FUNC, (L"+OALArgsQuery(%d)\r\n", type));

    // Get pointer to expected boot args location
    pArgs = OALPAtoCA(IMAGE_SHARE_ARGS_PA);

    // Check if there is expected signature
    if ((pArgs->header.signature != OAL_ARGS_SIGNATURE) ||
        (pArgs->header.oalVersion != OAL_ARGS_VERSION) ||
        (pArgs->header.bspVersion != BSP_ARGS_VERSION))
        goto cleanUp;


    //  Initialize settings
    if( s_bInitialized == FALSE )
    {
        int     i, count;
        UCHAR   val;

        // Copy prefix for DEVICEID    
        count = sizeof(BSP_DEVICE_PREFIX) - 1;
        if (count > sizeof(s_deviceId)-1) count = sizeof(s_deviceId) -1;
        memset(s_deviceId, 0, sizeof(s_deviceId));
        memcpy(s_deviceId, BSP_DEVICE_PREFIX, count);

        // Append DeviceID as hex string    
        for( i = 0; i < 8 && (count < sizeof(s_deviceId) - 1); i++, count++)
        {
            val = (UCHAR)((pArgs->deviceID >> (28 - i*4)) & 0xF);
            s_deviceId[count] = val < 10 ? '0' + val : 'A' + val - 10;
        }
        
        //  Use DeviceID as last part of UUID
        s_uuid.Data4[4] = (UCHAR)(pArgs->deviceID >> 24);
        s_uuid.Data4[5] = (UCHAR)(pArgs->deviceID >> 16);
        s_uuid.Data4[6] = (UCHAR)(pArgs->deviceID >> 8);
        s_uuid.Data4[7] = (UCHAR)(pArgs->deviceID);

        //  Use DeviceID for HWEntropy value
        s_dwHWEntropy[0] = pArgs->deviceID;
        s_dwHWEntropy[1] = pArgs->deviceID ^ 0xFFFFFFFF;

        //  Use DeviceID for ActiveSync RNDIS MAC
        s_rndisMAC[2] = (pArgs->deviceID >> 24);
        s_rndisMAC[3] = (pArgs->deviceID >> 16);
        s_rndisMAC[4] = (pArgs->deviceID >> 8);
        s_rndisMAC[5] = (pArgs->deviceID);

        s_bInitialized = TRUE;
    }


    // Depending on required args    
    switch (type) 
    {
        case OAL_ARGS_QUERY_UPDATEMODE:
            pData = &pArgs->updateMode;
            break;

        case OAL_ARGS_QUERY_KITL:
            pData = &pArgs->kitl;
            break;

        case OAL_ARGS_QUERY_COLDBOOT:
            pData = &pArgs->coldBoot;
            break;
    
        case OAL_ARGS_QUERY_DEVID:
            pData = s_deviceId;
            break;
        
        case OAL_ARGS_QUERY_UUID:
            pData = &s_uuid;
            break;

        case OAL_ARGS_QUERY_HWENTROPY:
            pData = s_dwHWEntropy;
            break;

        case OAL_ARGS_QUERY_RNDISMAC:
            pData = s_rndisMAC;
            break;

	    case OAL_ARGS_QUERY_ETHADDR_CPGMAC:
    		pData = pArgs->mac;
    		break;
            
    	case OAL_ARGS_QUERY_ETHADDR_CPGMAC1:
    		pData = pArgs->mac1;
    		break;
            
        case OAL_ARGS_QUERY_OALFLAGS:
            pData = &pArgs->oalFlags;
            break;

        case OAL_ARGS_QUERY_DISP_RES:
            pData = &pArgs->dispRes;
            break;

        case OAL_ARGS_QUERY_ECC_TYPE:
            pData = &pArgs->ECCtype;
            break;
       
        case OAL_ARGS_QUERY_OPP_MODE:
            pData = &pArgs->opp_mode;
            break;
            
        case OAL_ARGS_QUERY_BOOT_INFO:
            pData = &pArgs->bootInfo;
            break;
            
        case OAL_ARGS_QUERY_EBOOT_CFG:
            pData = &pArgs->ebootCfg;
            break;
                
        case OAL_ARGS_QUERY_EBOOT_CFGSZ:
            pData = &pArgs->cfgSize;
            break;
            
        default:
            pData = NULL;
            break;
    }

cleanUp:
    OALMSG(OAL_ARGS&&OAL_FUNC, (L"-OALArgsQuery(pData = 0x%08x)\r\n", pData));
    return pData;
}

//------------------------------------------------------------------------------
