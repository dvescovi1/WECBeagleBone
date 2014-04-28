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
//  mcbsp_profile.h


#ifndef __MCBSPPROFILE_H__
#define __MCBSPPROFILE_H__

#include "debug.h"
#include "mcbsptypes.h"


//------------------------------------------------------------------------------
//  McbspProfile_t class
//
class McbspProfile_t
{
protected:
    McBSPDevice_t               *m_pDevice;

public:
    McbspProfile_t(McBSPDevice_t *pDevice)
        {
        DEBUGMSG(ZONE_FUNCTION, (L"MCP:*%S\r\n", __FUNCTION__));
        m_pDevice = pDevice;
        };
    virtual ~McbspProfile_t() {};

    virtual void Initialize();
    virtual void ContextRestore();
    virtual void EnableSampleRateGenerator();
    virtual void EnableTransmitter();
    virtual void EnableReceiver();
    virtual void ResetSampleRateGenerator();
    virtual void ResetTransmitter();
    virtual void ResetReceiver();
    virtual void ClearIRQStatus();
    virtual McBSPProfile_e GetMode() = 0;
    virtual void SetTxChannelsRequested() = 0;
    virtual void SetRxChannelsRequested() = 0;

};


//------------------------------------------------------------------------------
//  I2SSlaveProfile_t class
//
class I2SSlaveProfile_t : public McbspProfile_t
{

public:
    I2SSlaveProfile_t(McBSPDevice_t *pDevice) :
        McbspProfile_t(pDevice)
        {
        DEBUGMSG(ZONE_FUNCTION, (L"MCP:*%S\r\n", __FUNCTION__));
        }
    virtual ~I2SSlaveProfile_t() {};

    void Initialize();
    void ContextRestore();

    McBSPProfile_e GetMode()
        {
        return kMcBSPProfile_I2S_Slave;
        }
    void SetTxChannelsRequested() {};
    void SetRxChannelsRequested() {};

};



//------------------------------------------------------------------------------
//  I2SMasterProfile_t class
//
class I2SMasterProfile_t : public McbspProfile_t
{

public:
    I2SMasterProfile_t(McBSPDevice_t *pDevice) :
        McbspProfile_t(pDevice)
        {
        DEBUGMSG(ZONE_FUNCTION, (L"MCP:*%S\r\n", __FUNCTION__));
        }
    virtual ~I2SMasterProfile_t() {};

    void Initialize();
    void ContextRestore();

    McBSPProfile_e GetMode()
        {
        return kMcBSPProfile_I2S_Master;
        }
    void SetTxChannelsRequested() {};
    void SetRxChannelsRequested() {};

};


//------------------------------------------------------------------------------
//  TDMProfile_t class
//
class TDMProfile_t : public McbspProfile_t
{

public:
    TDMProfile_t(McBSPDevice_t *pDevice) :
        McbspProfile_t(pDevice)
        {
        DEBUGMSG(ZONE_FUNCTION, (L"MCP:*%S\r\n", __FUNCTION__));
        }
    virtual ~TDMProfile_t() {};

    void Initialize();
    void ContextRestore();

    McBSPProfile_e GetMode()
        {
        return kMcBSPProfile_TDM;
        }
    void SetTxChannelsRequested();
    void SetRxChannelsRequested();

};

#endif
