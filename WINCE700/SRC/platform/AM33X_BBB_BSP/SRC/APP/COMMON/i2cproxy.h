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
//-----------------------------------------------------------------------------
//
//  File: i2cproxy.h
//
#ifndef __I2CPROXY_H__
#define __I2CPROXY_H__

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------

#define IOCTL_I2C_SET_SLAVE_ADDRESS     \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0200, METHOD_BUFFERED, FILE_ANY_ACCESS)

//------------------------------------------------------------------------------

#define IOCTL_I2C_SET_SUBADDRESS_MODE     \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0201, METHOD_BUFFERED, FILE_ANY_ACCESS)

//------------------------------------------------------------------------------

#define IOCTL_I2C_SET_BAUD_INDEX     \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0202, METHOD_BUFFERED, FILE_ANY_ACCESS)

#ifdef __cplusplus
}
#endif

#endif

