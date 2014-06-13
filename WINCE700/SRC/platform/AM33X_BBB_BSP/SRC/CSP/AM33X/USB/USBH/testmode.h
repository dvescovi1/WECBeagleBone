// All rights reserved ADENEO EMBEDDED 2010
//
// Copyright (c) MPC Data Limited 2009.  All rights reserved.
//
// Test modes for USB.org Embedded Hi-Speed Host Electrical Test
// Procedure, Revision 1.01.
//

#ifndef _TESTMODE_H_
#define _TESTMODE_H_

#ifdef __cplusplus
extern "C" {
#endif

void InitUsbTestMode(PUCHAR pUsbRegs);
void SetUsbTestMode(UCHAR mode);
void SetUsbTestModeSuspend(void);
void SetUsbTestModeResume(void);

#ifdef __cplusplus
}
#endif

#endif // _TESTMODE_H_
