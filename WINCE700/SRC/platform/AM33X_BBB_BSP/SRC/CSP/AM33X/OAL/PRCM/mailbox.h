
/*
 * \file     mailbox.h
 *
 * \brief    This file contains the function prototypes for 
			 communication between A8 and Mailbox.
 */

/* Copyright (C) 2010 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 */
 
#ifndef      __MAILBOX_H__
#define      __MAILBOX_H__
#ifdef __cplusplus
extern "C" {
#endif


void MBGenerateMailboxInt(volatile UINT32 baseAdd);
void MBClearMailboxMsg(volatile UINT32 baseAdd);
void MBInitializeMailbox();



#ifdef __cplusplus
}
#endif

#endif

