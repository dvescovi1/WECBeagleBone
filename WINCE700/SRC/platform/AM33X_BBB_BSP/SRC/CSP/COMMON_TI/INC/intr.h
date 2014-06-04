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
//  File:  intr.h
//
#ifndef __INTR_H
#define __INTR_H

//------------------------------------------------------------------------------

#define OMAP_IRQ_MAXIMUM                320
#define OMAP_IRQ_PER_SYSINTR            5


//------------------------------------------------------------------------------
//
//  Software IRQs for kernel/driver interaction
//

#define IRQ_SW_RTC_QUERY                100         // Query to OAL RTC
#define IRQ_SW_CPUMONITOR               101
#define IRQ_SW_RESERVED_2               102
#define IRQ_SW_RESERVED_3               103
#define IRQ_SW_RESERVED_4               104
#define IRQ_SW_RESERVED_5               105
#define IRQ_SW_RESERVED_6               106
#define IRQ_SW_RESERVED_7               107
#define IRQ_SW_RESERVED_8               108
#define IRQ_SW_RESERVED_9               109

#define IRQ_SW_RESERVED_MAX             110

//------------------------------------------------------------------------------

#endif // __INTR_H

