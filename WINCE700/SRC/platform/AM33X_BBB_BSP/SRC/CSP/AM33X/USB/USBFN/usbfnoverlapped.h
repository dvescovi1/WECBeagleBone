// All rights reserved ADENEO EMBEDDED 2010
//========================================================================
//   Copyright (c) Texas Instruments Incorporated 2008-2009
//
//   Use of this software is controlled by the terms and conditions found
//   in the license agreement under which this software has been supplied
//   or provided.
//========================================================================

//------------------------------------------------------------------------------
//
//  File:  usbfnoverlapped.h
//
#ifndef _USBFNOVERLAPPED_H_
#define _USBFNOVERLAPPED_H_

#ifdef __cplusplus
extern "C" {
#endif

// Overlapped IO flag.
// Flag to OR into STransfer.dwFlags parameter of UfnPdd_IssueTransfer()
#define USB_OVERLAPPED 0x01

// Overlapped IO data. 
// Pass in STransfer.pvPddTransferInfo parameter of UfnPdd_IssueTransfer()
typedef struct _USB_OVERLAPPED_INFO_tag
{
    DWORD dwBytesToIssueCallback; // callback is to be issued when this many bytes are completed
    PVOID lpUserData;             // this allows for the original userData value to be passed in
} USB_OVERLAPPED_INFO, *PUSB_OVERLAPPED_INFO;


#ifdef __cplusplus
}
#endif

#endif // _USBFNOVERLAPPED_H_
