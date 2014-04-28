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
//  File: omap35xx_hdq_1wire.h
//
#ifndef __OMAP35XX_HDQ_1WIRE_H
#define __OMAP35XX_HDQ_1WIRE_H

//------------------------------------------------------------------------------

typedef volatile struct {
    UINT32 REVISION;
    UINT32 TX_DATA;
    UINT32 RX_DATA;     //RX_RECEIVE_BUFFER_REG;   
    UINT32 CTRL_STATUS; //CNTL_STATUS_REG;
    UINT32 INT_STATUS;  //INTERRUPT_STATUS;
    UINT32 SYS_CONFIG;
    UINT32 SYS_STATUS;
} OMAP_HDQ_1WIRE_REGS;


//------------------------------------------------------------------------------

#define HDQ_CTRL_1_WIRE_SIGNLE_BIT              (1 << 7)
#define HDQ_CTRL_INTERRUPT_MASK                 (1 << 6)
#define HDQ_CTRL_CLOCK_ENABLE                   (1 << 5)
#define HDQ_CTRL_GO                             (1 << 4)
#define HDQ_CTRL_PRESENCE_DETECT                (1 << 3)
#define HDQ_CTRL_INITIALIZE                     (1 << 2)
#define HDQ_CTRL_WRITE                          (0 << 1)
#define HDQ_CTRL_READ                           (1 << 1)
#define HDQ_MODE_1WIRE                          (1 << 0)


#define HDQ_INT_TX_COMPLETE                     (1 << 2)
#define HDQ_INT_RX_COMPLETE                     (1 << 1)
#define HDQ_INT_TIMOUT                          (1 << 0)

#define HDQ_SYSCONFIG_SOFT_RESET                (1 << 1)
#define HDQ_SYSCONFIG_AUTOIDLE                  (1 << 0)

#define HDQ_SYSSTATUS_RESET_DONE                (1 << 0)

#define HDQ_WRITE_CMD                           (1 << 8)

//------------------------------------------------------------------------------
    
#endif // __OMAP35XX_HDQ_1WIRE_H