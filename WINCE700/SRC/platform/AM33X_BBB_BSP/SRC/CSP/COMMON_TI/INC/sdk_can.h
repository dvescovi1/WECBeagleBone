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
//  File:  sdk_can.h
//
//
#ifndef __SDK_CAN_H
#define __SDK_CAN_H

#include "omap.h"


#ifdef __cplusplus
extern "C" {
#endif
typedef enum {
    STOP,
    START,
    RESET
} CAN_COMMAND;

typedef enum {    
    RXCRITICAL = 0,
    RXMEDIUM = 1,
    RXLOW = 2,    
    NB_RX_PRIORITIES
} RXCAN_PRIORITY;

typedef enum {    
    TXCRITICAL = 0,
    TXMEDIUM = 1,
    TXLOW = 2,
    NB_TX_PRIORITIES
} TXCAN_PRIORITY;

typedef enum BUS_STATE {
    ERROR_ACTIVE,
    ERROR_ACTIVE_WARNED,
    ERROR_PASSIVE,    
    BUS_OFF,
} BUS_STATE;

typedef enum{
    STOPPED,
    STARTED,    
} CTRL_STATE;

typedef enum {
    BAUDRATE_CFG,  
} CONFIG_TYPE;

typedef enum {
    CREATE_CLASS_FILTER_CFG,
    ENABLE_DISABLE_CLASS_FILTER_CFG,    
    DELETE_CLASS_FILTER_CFG,
    ADD_SUBCLASS_FILTER_CFG,
    REMOVE_SUBCLASS_FILTER_CFG,    
} FILTER_CONFIG_TYPE;


typedef union{
    struct {
        unsigned int id:29;
        unsigned int reserved:2;        
        unsigned int extended:1;
    } s_extended;
    struct {
        unsigned int reserved0:18;
        unsigned int id:11;
        unsigned int reserved1:2;        
        unsigned int extended:1;
    } s_standard;
    UINT32 u32;
} CAN_ID;

typedef struct{
    CAN_ID id;
    CAN_ID mask;
} CLASS_FILTER;

typedef struct{
    CLASS_FILTER classFilter;
    CAN_ID id;
    CAN_ID mask;    
} SUBCLASS_FILTER;

typedef struct {    
    LONG currentRxMsg;
    DWORD maxRxMsg;
    LONG currentTxMsg[NB_TX_PRIORITIES];
    DWORD maxTxMsg[NB_TX_PRIORITIES];    
    BUS_STATE BusState;
    CTRL_STATE CtrlState;
    LONG RxDiscarded;   
    LONG RxLost;
    LONG FilteredOut;
    LONG TotalReceived;
    LONG TotalSent;
    UINT32 CANTEC;
    UINT32 CANREC;
} CAN_STATUS;


typedef struct {    
    UINT32 MDL;    
    UINT32 MDH;
    BYTE length;
    CAN_ID id;
}CAN_MESSAGE;


//start, stop and reset the CAN ctrl
#define IOCTL_CAN_COMMAND   \
    CTL_CODE(FILE_DEVICE_SERIAL_PORT, 0x0100, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef CAN_COMMAND IOCTL_CAN_COMMAND_IN;

//configure some of the device properties like bit timings
#define IOCTL_CAN_CONFIG \
    CTL_CODE(FILE_DEVICE_SERIAL_PORT, 0x0101, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct {    
    CONFIG_TYPE cfgType;
    union {        
        DWORD BaudRate;
    };
} IOCTL_CAN_CONFIG_IN;


//get the CAN controller status
#define IOCTL_CAN_STATUS \
    CTL_CODE(FILE_DEVICE_SERIAL_PORT, 0x0102, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef CAN_STATUS IOCTL_CAN_STATUS_OUT;


// send a single message
#define IOCTL_CAN_SEND  \
    CTL_CODE(FILE_DEVICE_SERIAL_PORT, 0x0103, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct {    
    DWORD nbMsg;
    DWORD nbMsgSent;
    DWORD timeout;
    TXCAN_PRIORITY priority;
    CAN_MESSAGE *msgArray;
} IOCTL_CAN_SEND_IN;


// Get a single message
#define IOCTL_CAN_RECEIVE   \
    CTL_CODE(FILE_DEVICE_SERIAL_PORT, 0x0104, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct {    
    DWORD nbMaxMsg;
    DWORD nbMsgReceived;
    DWORD timeout;
    CAN_MESSAGE *msgArray;
} IOCTL_CAN_RECEIVE_OUT;


// configure automatic rtr responses
#define IOCTL_CAN_REMOTE_CONFIGURE_AUTO_ANSWER  \
    CTL_CODE(FILE_DEVICE_SERIAL_PORT, 0x0105, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef enum {
    SET_AUTO_ANSWER,
    DELETE_AUTO_ANSWER
}AUTO_ANSWER_CONFIG_TYPE;

typedef struct {
    AUTO_ANSWER_CONFIG_TYPE cfgType;
    CAN_MESSAGE msg;
} IOCTL_CAN_REMOTE_CONFIGURE_AUTO_ANSWER_IN;



//configure the filters
#define IOCTL_CAN_FILTER_CONFIG \
    CTL_CODE(FILE_DEVICE_SERIAL_PORT, 0x0106, METHOD_BUFFERED, FILE_ANY_ACCESS)


typedef struct {    
    FILTER_CONFIG_TYPE cfgType;
    CLASS_FILTER classFilter;
    RXCAN_PRIORITY rxPriority;
    BOOL fEnabled;    
} IOCTL_CAN_CLASS_FILTER_CONFIG_IN;

typedef struct {    
    FILTER_CONFIG_TYPE cfgType;
    SUBCLASS_FILTER subClassFilter;    
} IOCTL_CAN_SUBCLASS_FILTER_CONFIG_IN;



//remote frame equest
#define IOCTL_CAN_REMOTE_REQUEST \
    CTL_CODE(FILE_DEVICE_SERIAL_PORT, 0x0107, METHOD_BUFFERED, FILE_ANY_ACCESS)


typedef struct {    
    CAN_ID id;
} CAN_REMOTE_REQUEST;

typedef CAN_REMOTE_REQUEST IOCTL_CAN_REMOTE_REQUEST_IN;



#ifdef __cplusplus
}
#endif

#endif // __SDK_CAN_H
